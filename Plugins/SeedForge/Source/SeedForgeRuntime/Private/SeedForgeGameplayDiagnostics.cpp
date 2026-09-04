#include "SeedForgeGameplayDiagnostics.h"

namespace SeedForge::GameplayDiagnostics::Private
{
    constexpr double PositionTolerance = 0.1;

    bool IsFinite(const FVector& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
    }

    bool IsInsideCell(const FVector& Position, const FIntPoint& Cell, double CellSize)
    {
        return FMath::Abs(Position.X - static_cast<double>(Cell.X) * CellSize) <= CellSize * 0.5 + PositionTolerance
            && FMath::Abs(Position.Y - static_cast<double>(Cell.Y) * CellSize) <= CellSize * 0.5 + PositionTolerance;
    }
}

bool FSeedForgeEnemyPathProof::ObserveAppliedPath(uint32 StableId, uint64 RunGeneration, uint64 SourceRequestId,
    const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result,
    const FSeedForgeEnemyPathSnapshot& Snapshot, const TArray<FIntPoint>& Walkable, float CellSize)
{
    using namespace SeedForge::GameplayDiagnostics::Private;
    if (Evidence.bComplete || RunGeneration == 0 || SourceRequestId == 0 || Snapshot.Revision == 0
        || !FMath::IsFinite(CellSize) || CellSize <= 0.0f
        || Result.Status != ESeedForgePathStatus::Success
        || Result.Path.Num() < 2 || Result.Path.Num() > MaxPathCells
        || Result.ExpandedNodes < Result.Path.Num() - 1
        || Snapshot.NextWaypointIndex != 0 || Snapshot.Waypoints.Num() != Result.Path.Num() - 1
        || Result.Path[0] != Start || Result.Path.Last() != Goal)
    {
        return false;
    }
    if (Evidence.PathRevision != 0 && (StableId != Evidence.StableId
        || RunGeneration != Evidence.RunGeneration || SourceRequestId != Evidence.SourceRequestId
        || Snapshot.Revision <= Evidence.PathRevision))
    {
        // Changing run/actor ownership requires the owner to explicitly Reset.
        return false;
    }
    const double Height = Snapshot.Waypoints[0].Z;
    if (!FMath::IsFinite(Height) || !FMath::IsFinite(static_cast<float>(Height))) { return false; }
    for (int32 Index = 0; Index < Result.Path.Num(); ++Index)
    {
        if (!Walkable.Contains(Result.Path[Index])) { return false; }
        if (Index == 0) { continue; }
        const FIntPoint Previous = Result.Path[Index - 1];
        const FIntPoint Cell = Result.Path[Index];
        const int64 Separation = FMath::Abs(static_cast<int64>(Cell.X) - Previous.X)
            + FMath::Abs(static_cast<int64>(Cell.Y) - Previous.Y);
        const FVector Expected = FSeedForgeGameplayMath::CellToWorld(Cell, CellSize, static_cast<float>(Height));
        if (Separation != 1 || !IsFinite(Snapshot.Waypoints[Index - 1])
            || !Snapshot.Waypoints[Index - 1].Equals(Expected, PositionTolerance))
        {
            return false;
        }
    }

    Reset();
    Evidence.StableId = StableId;
    Evidence.RunGeneration = RunGeneration;
    Evidence.SourceRequestId = SourceRequestId;
    Evidence.PathRevision = Snapshot.Revision;
    Evidence.Start = Start;
    Evidence.Goal = Goal;
    Evidence.Status = Result.Status;
    Evidence.ExpandedNodes = Result.ExpandedNodes;
    Evidence.Cells = Result.Path;
    Evidence.Waypoints = Snapshot.Waypoints;
    ProofCellSize = CellSize;
    return true;
}

bool FSeedForgeEnemyPathProof::ObserveMovement(uint32 StableId, uint64 RunGeneration, uint64 SourceRequestId,
    const FSeedForgeEnemyMoveObservation& Movement, float MaxSpeed)
{
    using namespace SeedForge::GameplayDiagnostics::Private;
    if (Evidence.bComplete || Evidence.PathRevision == 0 || StableId != Evidence.StableId
        || RunGeneration != Evidence.RunGeneration || SourceRequestId != Evidence.SourceRequestId
        || Movement.PathRevision != Evidence.PathRevision || Movement.Sequence == 0
        || Movement.Sequence <= LastSequence || Movement.Frame < LastFrame
        || !Evidence.Waypoints.IsValidIndex(Movement.WaypointIndex)
        || Movement.WaypointIndex < LastWaypointIndex || Movement.WaypointIndex > LastWaypointIndex + 1
        || !FMath::IsFinite(Movement.DeltaSeconds) || Movement.DeltaSeconds <= 0.0f
        || !FMath::IsFinite(MaxSpeed) || MaxSpeed <= 0.0f
        || !IsFinite(Movement.From) || !IsFinite(Movement.Target) || !IsFinite(Movement.To)
        || !Movement.Target.Equals(Evidence.Waypoints[Movement.WaypointIndex], PositionTolerance))
    {
        return false;
    }
    if (Evidence.ObservedMoveCount == 0)
    {
        if (Movement.WaypointIndex != 0 || !IsInsideCell(Movement.From, Evidence.Start, ProofCellSize)) { return false; }
    }
    else if (!Movement.From.Equals(LastPosition, PositionTolerance))
    {
        return false;
    }
    if (Movement.WaypointIndex > LastWaypointIndex && Evidence.ObservedMoveCount > 0
        && FVector::Dist2D(LastPosition, Evidence.Waypoints[LastWaypointIndex]) > 4.0 + PositionTolerance)
    {
        // Match the existing enemy's 4-unit waypoint-arrival rule.
        return false;
    }
    const double Distance = FVector::Distance(Movement.From, Movement.To);
    if (!FMath::IsFinite(Distance) || Distance <= UE_SMALL_NUMBER
        || Distance > static_cast<double>(MaxSpeed) * Movement.DeltaSeconds + PositionTolerance
        || FMath::Abs(Movement.From.Z - Movement.Target.Z) > PositionTolerance
        || FMath::Abs(Movement.To.Z - Movement.Target.Z) > PositionTolerance
        || FVector::DotProduct(Movement.To - Movement.From, Movement.Target - Movement.From) <= 0.0
        || !Movement.To.Equals(FMath::ClosestPointOnSegment(Movement.To, Movement.From, Movement.Target), PositionTolerance))
    {
        return false;
    }
    bool bFromOnRoute = false;
    bool bToOnRoute = false;
    for (const FIntPoint& Cell : Evidence.Cells)
    {
        bFromOnRoute |= IsInsideCell(Movement.From, Cell, ProofCellSize);
        bToOnRoute |= IsInsideCell(Movement.To, Cell, ProofCellSize);
    }
    if (!bFromOnRoute || !bToOnRoute
        || !FMath::IsFinite(Evidence.TotalDistance + Distance)
        || !FMath::IsFinite(Evidence.TotalDeltaSeconds + Movement.DeltaSeconds))
    {
        return false;
    }
    ++Evidence.ObservedMoveCount;
    Evidence.TotalDistance += Distance;
    Evidence.TotalDeltaSeconds += Movement.DeltaSeconds;
    if (Evidence.MovementSamples.Num() < MaxRetainedMovementSamples) { Evidence.MovementSamples.Add(Movement); }
    else { Evidence.MovementSamples.Last() = Movement; }
    LastSequence = Movement.Sequence;
    LastFrame = Movement.Frame;
    LastWaypointIndex = Movement.WaypointIndex;
    LastPosition = Movement.To;
    Evidence.bComplete = Evidence.ObservedMoveCount >= 2 && Evidence.TotalDistance >= 20.0;
    return true;
}

void FSeedForgeEnemyPathProof::Reset()
{
    Evidence = {};
    ProofCellSize = 0.0;
    LastSequence = LastFrame = 0;
    LastWaypointIndex = 0;
    LastPosition = FVector::ZeroVector;
}
bool FSeedForgeEnemyPathProof::IsComplete() const { return Evidence.bComplete; }
const FSeedForgeEnemyPathEvidence& FSeedForgeEnemyPathProof::GetEvidence() const { return Evidence; }

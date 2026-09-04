#include "SeedForgeGameplayTypes.h"

const TCHAR* LexToString(ESeedForgeRunFailureCode Code)
{
    switch (Code)
    {
    case ESeedForgeRunFailureCode::None: return TEXT("None");
    case ESeedForgeRunFailureCode::MissingWorldSubsystem: return TEXT("MissingWorldSubsystem");
    case ESeedForgeRunFailureCode::GenerationFailed: return TEXT("GenerationFailed");
    case ESeedForgeRunFailureCode::InvalidLayout: return TEXT("InvalidLayout");
    case ESeedForgeRunFailureCode::EncounterFailed: return TEXT("EncounterFailed");
    case ESeedForgeRunFailureCode::SpawnFailed: return TEXT("SpawnFailed");
    case ESeedForgeRunFailureCode::StartStateFailed: return TEXT("StartStateFailed");
    case ESeedForgeRunFailureCode::SmokeFailed: return TEXT("SmokeFailed");
    default: return TEXT("Unknown");
    }
}

FVector FSeedForgeGameplayMath::ResolveDashDirection(float ForwardAxis, float RightAxis, const FVector& Aim)
{
    if (FMath::IsFinite(ForwardAxis) && FMath::IsFinite(RightAxis))
    {
        FVector Movement(FMath::Clamp(ForwardAxis, -1.0f, 1.0f),
            FMath::Clamp(RightAxis, -1.0f, 1.0f), 0.0);
        if (Movement.Normalize())
        {
            return Movement;
        }
    }
    if (FMath::IsFinite(Aim.X) && FMath::IsFinite(Aim.Y) && FMath::IsFinite(Aim.Z))
    {
        FVector FlatAim(Aim.X, Aim.Y, 0.0);
        if (FlatAim.Normalize())
        {
            return FlatAim;
        }
    }
    return FVector::ForwardVector;
}

FVector FSeedForgeGameplayMath::CellToWorld(
    const FIntPoint& Cell,
    float CellSize,
    float Height)
{
    return FVector(
        static_cast<double>(Cell.X) * CellSize,
        static_cast<double>(Cell.Y) * CellSize,
        Height);
}

bool FSeedForgeGameplayMath::WorldToNearestWalkableCell(
    const FVector& WorldLocation,
    const TArray<FIntPoint>& WalkableCells,
    float CellSize,
    FIntPoint& OutCell)
{
    if (CellSize <= 0.0f || WalkableCells.IsEmpty())
    {
        return false;
    }

    double BestDistanceSquared = TNumericLimits<double>::Max();
    bool bFound = false;
    for (const FIntPoint& Cell : WalkableCells)
    {
        const FVector CellWorld = CellToWorld(Cell, CellSize, WorldLocation.Z);
        const double DistanceSquared = FVector::DistSquared2D(CellWorld, WorldLocation);
        const bool bCanonicalTieBreak = bFound
            && FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
            && (Cell.Y < OutCell.Y || (Cell.Y == OutCell.Y && Cell.X < OutCell.X));
        if (!bFound || DistanceSquared < BestDistanceSquared || bCanonicalTieBreak)
        {
            bFound = true;
            BestDistanceSquared = DistanceSquared;
            OutCell = Cell;
        }
    }
    return bFound;
}

int32 FSeedForgeGameplayMath::SelectAttackTarget(
    const FVector& Origin,
    const FVector& Forward,
    float Range,
    float MinForwardDot,
    const TArray<FSeedForgeAttackCandidate>& Candidates)
{
    FVector FlatForward(Forward.X, Forward.Y, 0.0);
    if (Range <= 0.0f
        || MinForwardDot < -1.0f
        || MinForwardDot > 1.0f
        || !FlatForward.Normalize())
    {
        return INDEX_NONE;
    }

    int32 BestIndex = INDEX_NONE;
    uint32 BestStableId = MAX_uint32;
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const FSeedForgeAttackCandidate& Candidate = Candidates[Index];
        if (!Candidate.bAlive)
        {
            continue;
        }
        FVector ToCandidate = Candidate.WorldLocation - Origin;
        ToCandidate.Z = 0.0;
        const double Distance = ToCandidate.Size();
        if (Distance > Range)
        {
            continue;
        }
        const double ForwardDot = Distance <= UE_DOUBLE_SMALL_NUMBER
            ? 1.0
            : FVector::DotProduct(FlatForward, ToCandidate / Distance);
        if (ForwardDot >= MinForwardDot && Candidate.StableId < BestStableId)
        {
            BestStableId = Candidate.StableId;
            BestIndex = Index;
        }
    }
    return BestIndex;
}

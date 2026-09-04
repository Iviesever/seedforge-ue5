#pragma once

#include "CoreMinimal.h"
#include "SeedForgeGameplayTypes.h"
#include "SeedForgeGridPathfinder.h"

struct SEEDFORGERUNTIME_API FSeedForgeEnemyMoveObservation
{
    uint64 Sequence = 0;
    uint64 Frame = 0;
    uint64 PathRevision = 0;
    int32 WaypointIndex = INDEX_NONE;
    FVector From = FVector::ZeroVector;
    FVector Target = FVector::ZeroVector;
    FVector To = FVector::ZeroVector;
    float DeltaSeconds = 0.0f;
};

struct SEEDFORGERUNTIME_API FSeedForgeEnemyPathSnapshot
{
    uint64 Revision = 0;
    int32 NextWaypointIndex = 0;
    TArray<FVector> Waypoints;
    FSeedForgeEnemyMoveObservation LastMove;
};

struct SEEDFORGERUNTIME_API FSeedForgeRunResourceSnapshot
{
    bool bInteractionTimerActive = false;
    bool bRepathTimerActive = false;
    double AttackCooldownRemaining = 0.0;
};

struct SEEDFORGERUNTIME_API FSeedForgeRunTransitionObservation
{
    ESeedForgeRunState From = ESeedForgeRunState::Generating;
    ESeedForgeRunState To = ESeedForgeRunState::Generating;
    FSeedForgeGameplaySnapshot Snapshot;
    uint64 Frame = 0;
    FDateTime AtUtc;
};

struct SEEDFORGERUNTIME_API FSeedForgeQueuedRunObservation
{
    ESeedForgeRunState StateBefore = ESeedForgeRunState::Generating;
    FSeedForgeGameplaySnapshot Snapshot;
    uint64 Frame = 0;
    FDateTime AtUtc;
};

struct SEEDFORGERUNTIME_API FSeedForgeEnemyPathEvidence
{
    uint32 StableId = 0;
    uint64 RunGeneration = 0;
    uint64 SourceRequestId = 0;
    uint64 PathRevision = 0;
    FIntPoint Start = FIntPoint::ZeroValue;
    FIntPoint Goal = FIntPoint::ZeroValue;
    ESeedForgePathStatus Status = ESeedForgePathStatus::InvalidInput;
    int32 ExpandedNodes = 0;
    TArray<FIntPoint> Cells;
    TArray<FVector> Waypoints;
    TArray<FSeedForgeEnemyMoveObservation> MovementSamples;
    uint64 ObservedMoveCount = 0;
    double TotalDistance = 0.0;
    double TotalDeltaSeconds = 0.0;
    bool bComplete = false;
};

class SEEDFORGERUNTIME_API FSeedForgeEnemyPathProof
{
public:
    static constexpr int32 MaxPathCells = 1025;
    static constexpr int32 MaxRetainedMovementSamples = 2;
    bool ObserveAppliedPath(uint32 StableId, uint64 RunGeneration, uint64 SourceRequestId,
        const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result,
        const FSeedForgeEnemyPathSnapshot& Snapshot, const TArray<FIntPoint>& Walkable, float CellSize);
    bool ObserveMovement(uint32 StableId, uint64 RunGeneration, uint64 SourceRequestId,
        const FSeedForgeEnemyMoveObservation& Movement, float MaxSpeed);
    void Reset();
    bool IsComplete() const;
    const FSeedForgeEnemyPathEvidence& GetEvidence() const;

private:
    FSeedForgeEnemyPathEvidence Evidence;
    double ProofCellSize = 0.0;
    uint64 LastSequence = 0;
    uint64 LastFrame = 0;
    int32 LastWaypointIndex = 0;
    FVector LastPosition = FVector::ZeroVector;
};

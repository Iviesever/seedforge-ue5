#include "SeedForgeGameplayTypes.h"

FVector FSeedForgeGameplayMath::CellToWorld(
    const FIntPoint& Cell,
    float CellSize,
    float Height)
{
    return FVector::ZeroVector;
}

bool FSeedForgeGameplayMath::WorldToNearestWalkableCell(
    const FVector& WorldLocation,
    const TArray<FIntPoint>& WalkableCells,
    float CellSize,
    FIntPoint& OutCell)
{
    return false;
}

int32 FSeedForgeGameplayMath::SelectAttackTarget(
    const FVector& Origin,
    const FVector& Forward,
    float Range,
    float MinForwardDot,
    const TArray<FSeedForgeAttackCandidate>& Candidates)
{
    return 0;
}

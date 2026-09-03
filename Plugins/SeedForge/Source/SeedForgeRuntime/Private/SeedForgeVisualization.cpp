#include "SeedForgeVisualization.h"

FSeedForgeVisualizationPlan FSeedForgeVisualizationPlanner::Build(
    const FSeedForgeLayout& Layout,
    float CellSize,
    float FloorThickness,
    float WallHeight,
    float WallThickness)
{
    check(CellSize > 0.0f);
    check(FloorThickness > 0.0f);
    check(WallHeight > 0.0f);
    check(WallThickness > 0.0f);

    FSeedForgeVisualizationPlan Plan;
    const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
    TSet<FIntPoint> WalkableSet;
    WalkableSet.Reserve(WalkableCells.Num());
    for (const FIntPoint& Cell : WalkableCells)
    {
        WalkableSet.Add(Cell);
    }

    const FVector FloorScale(CellSize / 100.0f, CellSize / 100.0f, FloorThickness / 100.0f);
    const FVector FloorExtent(CellSize * 0.5f, CellSize * 0.5f, FloorThickness * 0.5f);
    const FIntPoint Directions[] = {
        FIntPoint(0, -1),
        FIntPoint(1, 0),
        FIntPoint(0, 1),
        FIntPoint(-1, 0)};

    Plan.FloorTransforms.Reserve(WalkableCells.Num());
    for (const FIntPoint& Cell : WalkableCells)
    {
        const FVector FloorLocation(
            static_cast<double>(Cell.X) * CellSize,
            static_cast<double>(Cell.Y) * CellSize,
            -FloorThickness * 0.5f);
        Plan.FloorTransforms.Emplace(FRotator::ZeroRotator, FloorLocation, FloorScale);
        Plan.WorldBounds += FloorLocation - FloorExtent;
        Plan.WorldBounds += FloorLocation + FloorExtent;

        for (const FIntPoint& Direction : Directions)
        {
            if (WalkableSet.Contains(Cell + Direction))
            {
                continue;
            }

            const bool bVerticalEdge = Direction.X != 0;
            const FVector WallLocation(
                (static_cast<double>(Cell.X) + Direction.X * 0.5) * CellSize,
                (static_cast<double>(Cell.Y) + Direction.Y * 0.5) * CellSize,
                WallHeight * 0.5f);
            const FVector WallScale = bVerticalEdge
                ? FVector(WallThickness / 100.0f, CellSize / 100.0f, WallHeight / 100.0f)
                : FVector(CellSize / 100.0f, WallThickness / 100.0f, WallHeight / 100.0f);
            const FVector WallExtent = bVerticalEdge
                ? FVector(WallThickness * 0.5f, CellSize * 0.5f, WallHeight * 0.5f)
                : FVector(CellSize * 0.5f, WallThickness * 0.5f, WallHeight * 0.5f);
            Plan.WallTransforms.Emplace(FRotator::ZeroRotator, WallLocation, WallScale);
            Plan.WorldBounds += WallLocation - WallExtent;
            Plan.WorldBounds += WallLocation + WallExtent;
        }
    }

    return Plan;
}

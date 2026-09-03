#include "Misc/AutomationTest.h"
#include "SeedForgeVisualization.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::Tests
{
    FSeedForgeLayout MakeRectangularVisualizationLayout(int32 Width, int32 Height)
    {
        FSeedForgeLayout Layout;
        Layout.Rooms.Add({FIntPoint::ZeroValue, FIntPoint(Width, Height)});
        Layout.Entrance = FIntPoint::ZeroValue;
        Layout.Exit = FIntPoint(Width - 1, Height - 1);
        return Layout;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeSingleCellVisualizationTest,
    "SeedForge.Visualization.SingleCellCreatesFourBoundaryWalls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeSingleCellVisualizationTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout Layout = SeedForge::Tests::MakeRectangularVisualizationLayout(1, 1);
    const FSeedForgeVisualizationPlan Plan =
        FSeedForgeVisualizationPlanner::Build(Layout, 200.0f, 20.0f, 200.0f, 20.0f);

    TestEqual(TEXT("One walkable cell creates one floor instance"), Plan.FloorTransforms.Num(), 1);
    TestEqual(TEXT("One walkable cell creates four boundary walls"), Plan.WallTransforms.Num(), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAdjacentCellVisualizationTest,
    "SeedForge.Visualization.AdjacentCellsDoNotCreateInteriorWall",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAdjacentCellVisualizationTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout Layout = SeedForge::Tests::MakeRectangularVisualizationLayout(2, 1);
    const FSeedForgeVisualizationPlan Plan =
        FSeedForgeVisualizationPlanner::Build(Layout, 200.0f, 20.0f, 200.0f, 20.0f);

    TestEqual(TEXT("Two walkable cells create two floor instances"), Plan.FloorTransforms.Num(), 2);
    TestEqual(TEXT("Shared edge is omitted, leaving six exterior walls"), Plan.WallTransforms.Num(), 6);
    return true;
}

#endif

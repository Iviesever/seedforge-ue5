#include "Misc/AutomationTest.h"
#include "SeedForgeGridPathfinder.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::PathTests
{
    TArray<FIntPoint> MakeGrid(int32 Width, int32 Height)
    {
        TArray<FIntPoint> Cells;
        for (int32 Y = 0; Y < Height; ++Y)
        {
            for (int32 X = 0; X < Width; ++X)
            {
                Cells.Emplace(X, Y);
            }
        }
        return Cells;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgePathFindsStableShortestRouteTest,
    "SeedForge.Model.Path.StableShortestRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathFindsStableShortestRouteTest::RunTest(const FString& Parameters)
{
    FSeedForgePathRequest Request;
    Request.Start = FIntPoint(0, 0);
    Request.Goal = FIntPoint(2, 2);
    Request.WalkableCells = SeedForge::PathTests::MakeGrid(3, 3);
    const TArray<FIntPoint> Expected = {
        FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0),
        FIntPoint(2, 1), FIntPoint(2, 2)};

    const FSeedForgePathResult First = FSeedForgeGridPathfinder::FindPath(Request);
    const FSeedForgePathResult Second = FSeedForgeGridPathfinder::FindPath(Request);
    TestEqual(TEXT("Reachable route succeeds"), First.Status, ESeedForgePathStatus::Success);
    TestTrue(TEXT("Route is shortest and tie-broken exactly"), First.Path == Expected);
    TestTrue(TEXT("Repeated route is identical"), First.Path == Second.Path);
    TestEqual(TEXT("Repeated expansion count is identical"), First.ExpandedNodes, Second.ExpandedNodes);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgePathAlreadyAtGoalTest,
    "SeedForge.Model.Path.AlreadyAtGoal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathAlreadyAtGoalTest::RunTest(const FString& Parameters)
{
    FSeedForgePathRequest Request;
    Request.Start = FIntPoint(1, 1);
    Request.Goal = Request.Start;
    Request.WalkableCells = {Request.Start};
    const FSeedForgePathResult Result = FSeedForgeGridPathfinder::FindPath(Request);

    TestEqual(TEXT("Equal endpoints use dedicated status"), Result.Status, ESeedForgePathStatus::AlreadyAtGoal);
    TestEqual(TEXT("Already-at-goal path has one point"), Result.Path.Num(), 1);
    if (Result.Path.Num() == 1)
    {
        TestEqual(TEXT("Already-at-goal path contains start"), Result.Path[0], Request.Start);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgePathRejectsInvalidInputTest,
    "SeedForge.Model.Path.RejectsInvalidInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathRejectsInvalidInputTest::RunTest(const FString& Parameters)
{
    FSeedForgePathRequest Request;
    Request.Start = FIntPoint(0, 0);
    Request.Goal = FIntPoint(1, 0);
    Request.WalkableCells = {Request.Start};
    const FSeedForgePathResult MissingGoal = FSeedForgeGridPathfinder::FindPath(Request);
    TestEqual(TEXT("Missing goal is invalid"), MissingGoal.Status, ESeedForgePathStatus::InvalidInput);

    Request.WalkableCells.Add(Request.Goal);
    Request.MaxExpandedNodes = 0;
    const FSeedForgePathResult InvalidBudget = FSeedForgeGridPathfinder::FindPath(Request);
    TestEqual(TEXT("Non-positive budget is invalid"), InvalidBudget.Status, ESeedForgePathStatus::InvalidInput);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgePathReportsUnreachableTest,
    "SeedForge.Model.Path.ReportsUnreachable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathReportsUnreachableTest::RunTest(const FString& Parameters)
{
    FSeedForgePathRequest Request;
    Request.Start = FIntPoint(0, 0);
    Request.Goal = FIntPoint(5, 5);
    Request.WalkableCells = {Request.Start, Request.Goal};
    const FSeedForgePathResult Result = FSeedForgeGridPathfinder::FindPath(Request);

    TestEqual(TEXT("Disconnected cells are unreachable"), Result.Status, ESeedForgePathStatus::Unreachable);
    TestEqual(TEXT("Unreachable result has no partial path"), Result.Path.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgePathHonorsExpansionBudgetTest,
    "SeedForge.Model.Path.HonorsExpansionBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathHonorsExpansionBudgetTest::RunTest(const FString& Parameters)
{
    FSeedForgePathRequest Request;
    Request.Start = FIntPoint(0, 0);
    Request.Goal = FIntPoint(4, 4);
    Request.WalkableCells = SeedForge::PathTests::MakeGrid(5, 5);
    Request.MaxExpandedNodes = 1;
    const FSeedForgePathResult Result = FSeedForgeGridPathfinder::FindPath(Request);

    TestEqual(TEXT("Small budget reports budget exhaustion"), Result.Status, ESeedForgePathStatus::BudgetExceeded);
    TestTrue(TEXT("Expansion count never exceeds budget"), Result.ExpandedNodes <= Request.MaxExpandedNodes);
    TestEqual(TEXT("Budget failure has no partial path"), Result.Path.Num(), 0);
    return true;
}

#endif

#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "SeedForgeEncounter.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeGridPathfinder.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePathRejectsWrappedNeighborsTest,
    "SeedForge.Audit.CoordinateSafety.NoWrappedNeighbors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathRejectsWrappedNeighborsTest::RunTest(const FString& Parameters)
{
    const FIntPoint Endpoints[][2] = {
        {FIntPoint(MAX_int32, 0), FIntPoint(MIN_int32, 0)},
        {FIntPoint(MIN_int32, 0), FIntPoint(MAX_int32, 0)},
        {FIntPoint(0, MAX_int32), FIntPoint(0, MIN_int32)},
        {FIntPoint(0, MIN_int32), FIntPoint(0, MAX_int32)},
        {FIntPoint(MIN_int32, MIN_int32), FIntPoint(MAX_int32, MAX_int32)}};
    for (const auto& Pair : Endpoints)
    {
        FSeedForgePathRequest Request;
        Request.Start = Pair[0];
        Request.Goal = Pair[1];
        Request.WalkableCells = {Pair[1], Pair[0], Pair[1]};
        Request.MaxExpandedNodes = 1;
        const FSeedForgePathResult Result = FSeedForgeGridPathfinder::FindPath(Request);
        TestEqual(TEXT("Opposite extremes cannot become neighbors"), Result.Status, ESeedForgePathStatus::Unreachable);
        TestTrue(TEXT("Unreachable extremes have no partial path"), Result.Path.IsEmpty());
        TestEqual(TEXT("Only isolated start is expanded"), Result.ExpandedNodes, 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePathBudgetBoundaryTest,
    "SeedForge.Audit.CoordinateSafety.GoalAndExpansionBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathBudgetBoundaryTest::RunTest(const FString& Parameters)
{
    FSeedForgePathRequest Request;
    Request.Start = FIntPoint(MAX_int32 - 2, MAX_int32);
    Request.Goal = FIntPoint(MAX_int32 - 1, MAX_int32);
    Request.WalkableCells = {Request.Goal, Request.Start};
    Request.MaxExpandedNodes = 1;
    const FSeedForgePathResult Adjacent = FSeedForgeGridPathfinder::FindPath(Request);
    TestEqual(TEXT("Goal discovered by last allowed expansion succeeds"), Adjacent.Status, ESeedForgePathStatus::Success);
    TestEqual(TEXT("Goal selection is not another expansion"), Adjacent.ExpandedNodes, 1);
    TestEqual(TEXT("Adjacent route contains both endpoints"), Adjacent.Path.Num(), 2);
    Request.Goal = FIntPoint(MAX_int32, MAX_int32);
    Request.WalkableCells.Add(Request.Goal);
    const FSeedForgePathResult Limited = FSeedForgeGridPathfinder::FindPath(Request);
    TestEqual(TEXT("Undiscovered goal needs another expansion"), Limited.Status, ESeedForgePathStatus::BudgetExceeded);
    TestEqual(TEXT("Budget stops at exactly one expansion"), Limited.ExpandedNodes, 1);
    TestTrue(TEXT("Budget failure returns no partial path"), Limited.Path.IsEmpty());
    Request.MaxExpandedNodes = 2;
    const FSeedForgePathResult Enough = FSeedForgeGridPathfinder::FindPath(Request);
    TestEqual(TEXT("Two expansions discover two-edge goal"), Enough.Status, ESeedForgePathStatus::Success);
    TestEqual(TEXT("Selected goal does not increase count"), Enough.ExpandedNodes, 2);
    TestEqual(TEXT("Two-edge route has three cells"), Enough.Path.Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePathInputOrderSafetyTest,
    "SeedForge.Audit.CoordinateSafety.DuplicateAndShuffledInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathInputOrderSafetyTest::RunTest(const FString& Parameters)
{
    for (const int32 Origin : {0, MIN_int32, MAX_int32 - 2})
    {
        FSeedForgePathRequest Request;
        Request.Start = FIntPoint(Origin, Origin);
        Request.Goal = FIntPoint(Origin + 2, Origin + 2);
        for (int32 Y = 0; Y < 3; ++Y)
        {
            for (int32 X = 0; X < 3; ++X)
            {
                Request.WalkableCells.Emplace(Origin + X, Origin + Y);
            }
        }
        const TArray<FIntPoint> Expected = {Request.Start,
            FIntPoint(Origin + 1, Origin), FIntPoint(Origin + 2, Origin),
            FIntPoint(Origin + 2, Origin + 1), Request.Goal};
        const FSeedForgePathResult Original = FSeedForgeGridPathfinder::FindPath(Request);
        TestEqual(TEXT("Near-boundary connected grid succeeds"), Original.Status, ESeedForgePathStatus::Success);
        TestTrue(TEXT("Canonical shortest path is preserved"), Original.Path == Expected);
        TestEqual(TEXT("Canonical route expands four non-goal nodes"), Original.ExpandedNodes, 4);
        Algo::Reverse(Request.WalkableCells);
        Request.WalkableCells.Add(Request.Start);
        Request.WalkableCells.Add(Request.Goal);
        const FSeedForgePathResult Reordered = FSeedForgeGridPathfinder::FindPath(Request);
        TestEqual(TEXT("Shuffled duplicate input has same status"), Reordered.Status, Original.Status);
        TestTrue(TEXT("Shuffled duplicate input has same path"), Reordered.Path == Original.Path);
        TestEqual(TEXT("Shuffled duplicate input has same count"), Reordered.ExpandedNodes, Original.ExpandedNodes);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeExtremeEncounterGenerationTest,
    "SeedForge.Audit.CoordinateSafety.EncounterGenerateWideSeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeExtremeEncounterGenerationTest::RunTest(const FString& Parameters)
{
    FSeedForgeLayout Layout;
    Layout.Seed = 24301;
    Layout.Entrance = FIntPoint(MAX_int32, MAX_int32);
    Layout.Exit = FIntPoint(MAX_int32, MAX_int32 - 1);
    // Explicit sparse cells exercise the public distance contract, not a generated room domain.
    Layout.CorridorCells = {FIntPoint(MIN_int32, MIN_int32), FIntPoint(MIN_int32 + 1, MIN_int32),
        Layout.Exit, Layout.Entrance};
    Layout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(Layout);
    FSeedForgeEncounterConfig Config;
    Config.DataCoreCount = 1;
    Config.EnemyCount = 1;
    Config.MinPlayerEnemyDistance = 4096;
    const FSeedForgeEncounterResult Result = FSeedForgeEncounterPlanner::Generate(Layout, Config);
    TestTrue(TEXT("Full-domain separation remains larger than safety radius"), Result.IsSuccess());
    if (Result.IsSuccess())
    {
        TestEqual(TEXT("One enemy is generated"), Result.Plan.Enemies.Num(), 1);
        TestTrue(TEXT("Repeated wide-domain generation is deterministic"),
            FSeedForgeEncounterPlanner::Generate(Layout, Config).Plan == Result.Plan);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeExtremeEncounterValidationTest,
    "SeedForge.Audit.CoordinateSafety.EncounterValidateWideSubtraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeExtremeEncounterValidationTest::RunTest(const FString& Parameters)
{
    for (bool bFullTwoAxisSpan : {false, true})
    {
        FSeedForgeLayout Layout;
        Layout.Seed = 24301;
        Layout.Entrance = bFullTwoAxisSpan ? FIntPoint(MAX_int32, MAX_int32) : FIntPoint::ZeroValue;
        Layout.Exit = FIntPoint(1, 0);
        const FIntPoint Core(2, 0);
        const FIntPoint Enemy = bFullTwoAxisSpan ? FIntPoint(MIN_int32, MIN_int32) : FIntPoint(MIN_int32, 0);
        Layout.CorridorCells = {Layout.Entrance, Layout.Exit, Core, Enemy};
        Layout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(Layout);
        FSeedForgeEncounterConfig Config;
        Config.DataCoreCount = 1;
        Config.EnemyCount = 1;
        Config.MinPlayerEnemyDistance = 4096;
        FSeedForgeEncounterPlan Plan;
        Plan.Seed = Layout.Seed;
        Plan.SourceLayoutHash = Layout.CanonicalHash;
        Plan.Player = {ESeedForgeEncounterRole::Player, 0, Layout.Entrance};
        Plan.Exit = {ESeedForgeEncounterRole::Exit, 0, Layout.Exit};
        Plan.DataCores = {{ESeedForgeEncounterRole::DataCore, 0, Core}};
        Plan.Enemies = {{ESeedForgeEncounterRole::Enemy, 0, Enemy}};
        Plan.CanonicalHash = FSeedForgeEncounterPlanner::ComputeCanonicalHash(Plan, Config);
        TestTrue(TEXT("Wide coordinate subtraction is performed before absolute value"),
            FSeedForgeEncounterPlanner::Validate(Plan, Layout, Config).IsSuccess());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCoordinateGoldenPreservationTest,
    "SeedForge.Audit.CoordinateSafety.NormalIdentityPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCoordinateGoldenPreservationTest::RunTest(const FString& Parameters)
{
    const FSeedForgeResult Layout = FSeedForgeGenerator::Generate(24301, {});
    TestTrue(TEXT("Reference seed generates"), Layout.IsSuccess());
    TestEqual(TEXT("Existing layout golden is unchanged"), Layout.Layout.CanonicalHash, 7425849530159566348ULL);
    const FSeedForgeEncounterResult Encounter = FSeedForgeEncounterPlanner::Generate(Layout.Layout, {});
    TestTrue(TEXT("Reference encounter generates"), Encounter.IsSuccess());
    TestEqual(TEXT("Existing fixed-seed encounter identity is unchanged"), Encounter.Plan.CanonicalHash, 15303214708604970503ULL);
    return true;
}

#endif

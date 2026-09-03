#include "Misc/AutomationTest.h"
#include "SeedForgeEncounter.h"
#include "SeedForgeGenerator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::EncounterTests
{
    FSeedForgeLayout GenerateLayout(FAutomationTestBase& Test, uint64 Seed)
    {
        const FSeedForgeResult Generated = FSeedForgeGenerator::Generate(Seed, {});
        Test.TestTrue(FString::Printf(TEXT("Seed %llu generates"), Seed), Generated.IsSuccess());
        return Generated.Layout;
    }

    bool HasUniqueWalkableEntities(
        const FSeedForgeEncounterPlan& Plan,
        const FSeedForgeLayout& Layout)
    {
        const TArray<FIntPoint> Walkable = Layout.GetCanonicalWalkableCells();
        TSet<FIntPoint> Occupied;
        auto AddEntity = [&Walkable, &Occupied](const FSeedForgeEncounterEntity& Entity)
        {
            if (!Walkable.Contains(Entity.Cell) || Occupied.Contains(Entity.Cell))
            {
                return false;
            }
            Occupied.Add(Entity.Cell);
            return true;
        };

        if (!AddEntity(Plan.Player) || !AddEntity(Plan.Exit))
        {
            return false;
        }
        for (const FSeedForgeEncounterEntity& Core : Plan.DataCores)
        {
            if (!AddEntity(Core))
            {
                return false;
            }
        }
        for (const FSeedForgeEncounterEntity& Enemy : Plan.Enemies)
        {
            if (!AddEntity(Enemy))
            {
                return false;
            }
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeEncounterDeterministicPlanTest,
    "SeedForge.Model.Encounter.DeterministicPlan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEncounterDeterministicPlanTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout Layout = SeedForge::EncounterTests::GenerateLayout(*this, 24301);
    const FSeedForgeEncounterConfig Config;
    const FSeedForgeEncounterResult First = FSeedForgeEncounterPlanner::Generate(Layout, Config);
    const FSeedForgeEncounterResult Second = FSeedForgeEncounterPlanner::Generate(Layout, Config);

    TestTrue(TEXT("First plan succeeds"), First.IsSuccess());
    TestTrue(TEXT("Second plan succeeds"), Second.IsSuccess());
    if (First.IsSuccess() && Second.IsSuccess())
    {
        TestTrue(TEXT("Repeated plans are identical"), First.Plan == Second.Plan);
        TestEqual(TEXT("Default plan has three Data Cores"), First.Plan.DataCores.Num(), 3);
        TestEqual(TEXT("Default plan has five enemies"), First.Plan.Enemies.Num(), 5);
        TestEqual(TEXT("Player uses layout entrance"), First.Plan.Player.Cell, Layout.Entrance);
        TestEqual(TEXT("Exit uses layout exit"), First.Plan.Exit.Cell, Layout.Exit);
        TestTrue(TEXT("All entities are unique and walkable"), SeedForge::EncounterTests::HasUniqueWalkableEntities(First.Plan, Layout));
        TestNotEqual(TEXT("Encounter hash is non-zero"), First.Plan.CanonicalHash, 0ULL);
        TestNotEqual(TEXT("Encounter identity is independent of layout identity"), First.Plan.CanonicalHash, Layout.CanonicalHash);
        for (int32 Index = 0; Index < First.Plan.DataCores.Num(); ++Index)
        {
            TestEqual(TEXT("Core role is stable"), First.Plan.DataCores[Index].Role, ESeedForgeEncounterRole::DataCore);
            TestEqual(TEXT("Core ID follows canonical index"), First.Plan.DataCores[Index].StableId, static_cast<uint32>(Index));
        }
        for (int32 Index = 0; Index < First.Plan.Enemies.Num(); ++Index)
        {
            TestEqual(TEXT("Enemy role is stable"), First.Plan.Enemies[Index].Role, ESeedForgeEncounterRole::Enemy);
            TestEqual(TEXT("Enemy ID follows canonical index"), First.Plan.Enemies[Index].StableId, static_cast<uint32>(Index));
            const int32 Distance = FMath::Abs(First.Plan.Enemies[Index].Cell.X - First.Plan.Player.Cell.X)
                + FMath::Abs(First.Plan.Enemies[Index].Cell.Y - First.Plan.Player.Cell.Y);
            TestTrue(TEXT("Enemy respects player safety distance"), Distance >= Config.MinPlayerEnemyDistance);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeEncounterIdentityDivergenceTest,
    "SeedForge.Model.Encounter.IdentityDivergence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEncounterIdentityDivergenceTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout FirstLayout = SeedForge::EncounterTests::GenerateLayout(*this, 101);
    const FSeedForgeLayout SecondLayout = SeedForge::EncounterTests::GenerateLayout(*this, 202);
    const FSeedForgeEncounterResult First = FSeedForgeEncounterPlanner::Generate(FirstLayout, {});
    const FSeedForgeEncounterResult Second = FSeedForgeEncounterPlanner::Generate(SecondLayout, {});
    FSeedForgeEncounterConfig ChangedConfig;
    ChangedConfig.DataCoreCount = 2;
    const FSeedForgeEncounterResult Changed = FSeedForgeEncounterPlanner::Generate(FirstLayout, ChangedConfig);

    TestTrue(TEXT("First seed plan succeeds"), First.IsSuccess());
    TestTrue(TEXT("Second seed plan succeeds"), Second.IsSuccess());
    TestTrue(TEXT("Changed config plan succeeds"), Changed.IsSuccess());
    if (First.IsSuccess() && Second.IsSuccess() && Changed.IsSuccess())
    {
        TestNotEqual(TEXT("Different source seeds change encounter identity"), First.Plan.CanonicalHash, Second.Plan.CanonicalHash);
        TestNotEqual(TEXT("Different encounter config changes encounter identity"), First.Plan.CanonicalHash, Changed.Plan.CanonicalHash);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeEncounterRejectsInvalidConfigTest,
    "SeedForge.Model.Encounter.RejectsInvalidConfig",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEncounterRejectsInvalidConfigTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout Layout = SeedForge::EncounterTests::GenerateLayout(*this, 303);
    FSeedForgeEncounterConfig Config;
    Config.DataCoreCount = 0;
    const FSeedForgeEncounterResult Result = FSeedForgeEncounterPlanner::Generate(Layout, Config);

    TestFalse(TEXT("Zero Data Cores is rejected"), Result.IsSuccess());
    TestEqual(TEXT("Invalid config has a typed error"), Result.ErrorCode, ESeedForgeEncounterErrorCode::InvalidConfig);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeEncounterRejectsImpossibleSeparationTest,
    "SeedForge.Model.Encounter.RejectsImpossibleSeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEncounterRejectsImpossibleSeparationTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout Layout = SeedForge::EncounterTests::GenerateLayout(*this, 404);
    FSeedForgeEncounterConfig Config;
    Config.MinPlayerEnemyDistance = 10'000;
    const FSeedForgeEncounterResult Result = FSeedForgeEncounterPlanner::Generate(Layout, Config);

    TestFalse(TEXT("Impossible safety radius is rejected"), Result.IsSuccess());
    TestEqual(TEXT("Safety exhaustion has a typed error"), Result.ErrorCode, ESeedForgeEncounterErrorCode::EnemySafetyRadiusUnsatisfied);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeEncounterRejectsTamperingTest,
    "SeedForge.Model.Encounter.RejectsTampering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEncounterRejectsTamperingTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayout Layout = SeedForge::EncounterTests::GenerateLayout(*this, 505);
    const FSeedForgeEncounterConfig Config;
    const FSeedForgeEncounterResult Generated = FSeedForgeEncounterPlanner::Generate(Layout, Config);
    TestTrue(TEXT("Baseline plan succeeds"), Generated.IsSuccess());
    if (!Generated.IsSuccess())
    {
        return false;
    }

    FSeedForgeEncounterPlan Tampered = Generated.Plan;
    Tampered.DataCores[0].Cell = Tampered.Player.Cell;
    Tampered.CanonicalHash = FSeedForgeEncounterPlanner::ComputeCanonicalHash(Tampered, Config);
    const FSeedForgeEncounterResult Validation = FSeedForgeEncounterPlanner::Validate(Tampered, Layout, Config);
    TestFalse(TEXT("Overlapping tampered plan is rejected"), Validation.IsSuccess());
    TestEqual(TEXT("Tampered plan has a typed error"), Validation.ErrorCode, ESeedForgeEncounterErrorCode::InvalidPlan);
    return true;
}

#endif

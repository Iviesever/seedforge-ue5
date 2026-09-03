#include "Misc/AutomationTest.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeValidator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::Tests
{
    FSeedForgeLayout MakeTwoRoomLayout(
        const FSeedForgeRoom& First,
        const FSeedForgeRoom& Second,
        TArray<FIntPoint> CorridorCells)
    {
        FSeedForgeLayout Layout;
        Layout.Seed = 17;
        Layout.Rooms = {First, Second};
        Layout.CorridorCells = MoveTemp(CorridorCells);
        Layout.Entrance = First.Center();
        Layout.Exit = Second.Center();
        Layout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(Layout);
        return Layout;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRejectsOutOfBoundsRoomTest,
    "SeedForge.Validation.RejectsOutOfBoundsRoom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRejectsOutOfBoundsRoomTest::RunTest(const FString& Parameters)
{
    FSeedForgeConfig Config;
    Config.GridWidth = 16;
    Config.GridHeight = 16;
    Config.RoomCount = 2;

    const FSeedForgeLayout Layout = SeedForge::Tests::MakeTwoRoomLayout(
        {{-1, 1}, {4, 4}},
        {{8, 8}, {4, 4}},
        {});
    const FSeedForgeValidationResult Result = FSeedForgeValidator::Validate(Layout, Config);

    TestFalse(TEXT("Out-of-bounds room is rejected"), Result.IsValid());
    TestEqual(
        TEXT("Out-of-bounds room reports the specific error"),
        Result.ErrorCode,
        ESeedForgeErrorCode::LayoutOutOfBounds);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRejectsOverlappingRoomsTest,
    "SeedForge.Validation.RejectsOverlappingRooms",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRejectsOverlappingRoomsTest::RunTest(const FString& Parameters)
{
    FSeedForgeConfig Config;
    Config.GridWidth = 16;
    Config.GridHeight = 16;
    Config.RoomCount = 2;

    const FSeedForgeLayout Layout = SeedForge::Tests::MakeTwoRoomLayout(
        {{2, 2}, {5, 5}},
        {{5, 5}, {4, 4}},
        {});
    const FSeedForgeValidationResult Result = FSeedForgeValidator::Validate(Layout, Config);

    TestFalse(TEXT("Overlapping rooms are rejected"), Result.IsValid());
    TestEqual(
        TEXT("Overlap reports the specific error"),
        Result.ErrorCode,
        ESeedForgeErrorCode::RoomsOverlap);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRejectsDisconnectedLayoutTest,
    "SeedForge.Validation.RejectsDisconnectedLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRejectsDisconnectedLayoutTest::RunTest(const FString& Parameters)
{
    FSeedForgeConfig Config;
    Config.GridWidth = 20;
    Config.GridHeight = 20;
    Config.RoomCount = 2;

    const FSeedForgeLayout Layout = SeedForge::Tests::MakeTwoRoomLayout(
        {{1, 1}, {3, 3}},
        {{14, 14}, {3, 3}},
        {});
    const FSeedForgeValidationResult Result = FSeedForgeValidator::Validate(Layout, Config);

    TestFalse(TEXT("Disconnected walkable regions are rejected"), Result.IsValid());
    TestEqual(
        TEXT("Disconnected layout reports the specific error"),
        Result.ErrorCode,
        ESeedForgeErrorCode::DisconnectedLayout);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAcceptsGeneratedLayoutTest,
    "SeedForge.Validation.AcceptsGeneratedLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAcceptsGeneratedLayoutTest::RunTest(const FString& Parameters)
{
    const FSeedForgeConfig Config;
    const FSeedForgeResult Generation = FSeedForgeGenerator::Generate(0xC0FFEEULL, Config);
    TestTrue(TEXT("Representative generation succeeds"), Generation.IsSuccess());
    if (!Generation.IsSuccess())
    {
        return false;
    }

    const FSeedForgeValidationResult Result = FSeedForgeValidator::Validate(Generation.Layout, Config);
    TestTrue(TEXT("Generated layout satisfies every validator invariant"), Result.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeTenThousandSeedPropertyTest,
    "SeedForge.Validation.TenThousandSeedPropertySweep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeTenThousandSeedPropertyTest::RunTest(const FString& Parameters)
{
    const FSeedForgeConfig Config;
    for (uint64 Seed = 0; Seed < 10000; ++Seed)
    {
        const FSeedForgeResult Generation = FSeedForgeGenerator::Generate(Seed, Config);
        if (!Generation.IsSuccess())
        {
            AddError(FString::Printf(
                TEXT("Seed %llu failed generation: %s"),
                Seed,
                *Generation.ErrorMessage));
            return false;
        }

        const FSeedForgeValidationResult Validation = FSeedForgeValidator::Validate(Generation.Layout, Config);
        if (!Validation.IsValid())
        {
            AddError(FString::Printf(
                TEXT("Seed %llu violated invariant %d: %s"),
                Seed,
                static_cast<int32>(Validation.ErrorCode),
                *Validation.ErrorMessage));
            return false;
        }
    }

    return true;
}

#endif

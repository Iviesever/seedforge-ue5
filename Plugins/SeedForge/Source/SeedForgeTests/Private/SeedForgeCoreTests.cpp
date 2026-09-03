#include "Misc/AutomationTest.h"
#include "SeedForgeGenerator.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRejectsInvalidGridTest,
    "SeedForge.Core.RejectsInvalidGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRejectsInvalidGridTest::RunTest(const FString& Parameters)
{
    FSeedForgeConfig Config;
    Config.GridWidth = 0;

    const FSeedForgeResult Result = FSeedForgeGenerator::Generate(7, Config);

    TestFalse(TEXT("Invalid grid must not produce a layout"), Result.IsSuccess());
    TestEqual(
        TEXT("Invalid grid reports the specific error"),
        Result.ErrorCode,
        ESeedForgeErrorCode::InvalidGridSize);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRejectsInvalidRoomRangeTest,
    "SeedForge.Core.RejectsInvalidRoomRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRejectsInvalidRoomRangeTest::RunTest(const FString& Parameters)
{
    FSeedForgeConfig Config;
    Config.MinRoomWidth = 10;
    Config.MaxRoomWidth = 4;

    const FSeedForgeResult Result = FSeedForgeGenerator::Generate(11, Config);

    TestFalse(TEXT("Invalid room range must not produce a layout"), Result.IsSuccess());
    TestEqual(
        TEXT("Invalid room range reports the specific error"),
        Result.ErrorCode,
        ESeedForgeErrorCode::InvalidRoomSizeRange);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeSameSeedDeterministicTest,
    "SeedForge.Core.SameSeedIsDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeSameSeedDeterministicTest::RunTest(const FString& Parameters)
{
    const FSeedForgeConfig Config;
    const FSeedForgeResult First = FSeedForgeGenerator::Generate(0x5EEDULL, Config);
    const FSeedForgeResult Second = FSeedForgeGenerator::Generate(0x5EEDULL, Config);

    const bool bBothSucceeded = First.IsSuccess() && Second.IsSuccess();
    TestTrue(TEXT("Both deterministic generations succeed"), bBothSucceeded);
    if (bBothSucceeded)
    {
        TestTrue(TEXT("Layouts are byte-for-byte canonical equivalents"), First.Layout == Second.Layout);
        TestNotEqual(TEXT("Successful layout has a non-zero canonical hash"), First.Layout.CanonicalHash, 0ULL);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGoldenHashesStableTest,
    "SeedForge.Validation.GoldenHashesStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGoldenHashesStableTest::RunTest(const FString& Parameters)
{
    struct FGoldenCase
    {
        uint64 Seed;
        uint64 ExpectedHash;
    };

    const FSeedForgeConfig Config;
    const FGoldenCase Cases[] = {
        {0ULL, 3488165859926780287ULL},
        {1ULL, 8479853380352986717ULL},
        {0x5EEDULL, 7425849530159566348ULL},
        {0xC0FFEEULL, 7770407528328499089ULL},
        {MAX_uint64, 5108722159798011553ULL}};
    for (const FGoldenCase& Golden : Cases)
    {
        const FSeedForgeResult Result = FSeedForgeGenerator::Generate(Golden.Seed, Config);
        TestTrue(FString::Printf(TEXT("Seed %llu generates"), Golden.Seed), Result.IsSuccess());
        if (Result.IsSuccess())
        {
            TestEqual(
                FString::Printf(TEXT("Seed %llu retains its canonical hash"), Golden.Seed),
                Result.Layout.CanonicalHash,
                Golden.ExpectedHash);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeHundredSynchronousRepetitionsTest,
    "SeedForge.Validation.HundredSynchronousRepetitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeHundredSynchronousRepetitionsTest::RunTest(const FString& Parameters)
{
    const FSeedForgeConfig Config;
    const uint64 Seeds[] = {0ULL, 1ULL, 0x5EEDULL, 0xC0FFEEULL, MAX_uint64};
    for (const uint64 Seed : Seeds)
    {
        const FSeedForgeResult Baseline = FSeedForgeGenerator::Generate(Seed, Config);
        TestTrue(FString::Printf(TEXT("Baseline seed %llu generates"), Seed), Baseline.IsSuccess());
        if (!Baseline.IsSuccess())
        {
            continue;
        }

        for (int32 Repetition = 0; Repetition < 100; ++Repetition)
        {
            const FSeedForgeResult Current = FSeedForgeGenerator::Generate(Seed, Config);
            if (!Current.IsSuccess() || !(Current.Layout == Baseline.Layout))
            {
                AddError(FString::Printf(
                    TEXT("Seed %llu diverged at synchronous repetition %d."),
                    Seed,
                    Repetition));
                return false;
            }
        }
    }
    return true;
}

#endif

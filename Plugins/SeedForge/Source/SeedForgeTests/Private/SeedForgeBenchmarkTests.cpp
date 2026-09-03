#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SeedForgeBenchmark.h"
#include "SeedForgeLayoutCodec.h"
#include "SeedForgeReportCommandlet.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeKnownPercentilesTest,
    "SeedForge.Benchmark.KnownPercentiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeKnownPercentilesTest::RunTest(const FString& Parameters)
{
    const TArray<double> Samples = {100.0, 1.0, 4.0, 3.0, 2.0};
    TestEqual(
        TEXT("Nearest-rank median is exact"),
        FSeedForgeBenchmarkRunner::CalculateNearestRankPercentile(Samples, 0.50),
        3.0);
    TestEqual(
        TEXT("Nearest-rank P95 is exact"),
        FSeedForgeBenchmarkRunner::CalculateNearestRankPercentile(Samples, 0.95),
        100.0);
    TestEqual(
        TEXT("Minimum percentile is exact"),
        FSeedForgeBenchmarkRunner::CalculateNearestRankPercentile(Samples, 0.0),
        1.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeBoundedBenchmarkTest,
    "SeedForge.Benchmark.BoundedRun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeBoundedBenchmarkTest::RunTest(const FString& Parameters)
{
    FSeedForgeBenchmarkConfig Config;
    Config.SeedStart = 1000;
    Config.SeedCount = 32;
    Config.WarmupCount = 4;
    const FSeedForgeBenchmarkReport Report = FSeedForgeBenchmarkRunner::Run(Config);

    TestTrue(TEXT("Bounded benchmark succeeds"), Report.IsSuccess());
    TestEqual(TEXT("Requested samples are recorded"), Report.RequestedSampleCount, 32);
    TestEqual(TEXT("Every requested sample is attempted"), Report.AttemptedSampleCount, 32);
    TestEqual(TEXT("Every sample succeeds"), Report.SuccessCount, 32);
    TestEqual(TEXT("No sample fails"), Report.FailureCount, 0);
    TestNotEqual(TEXT("Aggregate hash is non-zero"), Report.AggregateHash, 0ULL);
    TestTrue(TEXT("Minimum is at most median"), Report.MinMilliseconds <= Report.MedianMilliseconds);
    TestTrue(TEXT("Median is at most P95"), Report.MedianMilliseconds <= Report.P95Milliseconds);
    TestTrue(TEXT("P95 is at most maximum"), Report.P95Milliseconds <= Report.MaxMilliseconds);
    TestTrue(TEXT("Total is at least maximum"), Report.TotalMilliseconds >= Report.MaxMilliseconds);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGenerateCommandletTest,
    "SeedForge.Benchmark.CommandletGenerate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGenerateCommandletTest::RunTest(const FString& Parameters)
{
    const FString OutputDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/Tests"));
    const FString OutputPath = FPaths::Combine(OutputDirectory, TEXT("commandlet-generate.json"));
    IFileManager::Get().MakeDirectory(*OutputDirectory, true);
    IFileManager::Get().Delete(*OutputPath, false, true);

    USeedForgeReportCommandlet* Commandlet = NewObject<USeedForgeReportCommandlet>();
    const int32 ExitCode = Commandlet->Main(FString::Printf(
        TEXT("-Mode=Generate -Seed=18446744073709551615 -Output=\"%s\""),
        *OutputPath));
    TestEqual(TEXT("Valid Generate command succeeds"), ExitCode, 0);

    FString Json;
    TestTrue(TEXT("Generate command writes output"), FFileHelper::LoadFileToString(Json, *OutputPath));
    if (!Json.IsEmpty())
    {
        const FSeedForgeDocumentResult Imported = FSeedForgeLayoutCodec::ImportCanonicalJson(Json);
        TestTrue(TEXT("Generated command output imports"), Imported.IsSuccess());
        if (Imported.IsSuccess())
        {
            TestEqual(TEXT("Generated command preserves maximum seed"), Imported.Document.Layout.Seed, MAX_uint64);
        }
    }
    IFileManager::Get().Delete(*OutputPath, false, true);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeInvalidCommandletArgumentsTest,
    "SeedForge.Benchmark.InvalidCommandletArguments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInvalidCommandletArgumentsTest::RunTest(const FString& Parameters)
{
    AddExpectedError(TEXT("Missing required Mode"), EAutomationExpectedErrorFlags::Contains, 1);
    USeedForgeReportCommandlet* Commandlet = NewObject<USeedForgeReportCommandlet>();
    const int32 ExitCode = Commandlet->Main(TEXT("-Seed=7"));
    TestNotEqual(TEXT("Missing Mode returns non-zero"), ExitCode, 0);
    return true;
}

#endif

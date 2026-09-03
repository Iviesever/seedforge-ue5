#include "SeedForgeReportCommandlet.h"

#include "GenericPlatform/GenericPlatformProperties.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "String/LexFromString.h"
#include "SeedForgeBenchmark.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeLayoutCodec.h"
#include "SeedForgeLayoutDiff.h"

DEFINE_LOG_CATEGORY_STATIC(LogSeedForgeReportCommandlet, Log, All);

namespace SeedForge::Report::Private
{
    constexpr int32 InvalidArgumentsExitCode = 2;
    constexpr int32 OperationFailedExitCode = 3;
    constexpr int32 WriteFailedExitCode = 4;

    int32 Fail(int32 ExitCode, const FString& Message)
    {
        UE_LOG(LogSeedForgeReportCommandlet, Error, TEXT("%s"), *Message);
        return ExitCode;
    }

    bool ReadArgument(const FString& Params, const TCHAR* Name, FString& Value)
    {
        const FString Match = FString(Name) + TEXT("=");
        return FParse::Value(*Params, *Match, Value) && !Value.IsEmpty();
    }

    bool ParseUInt64(const FString& Text, uint64& Value)
    {
        if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == TEXT('0')))
        {
            return false;
        }
        uint64 Parsed = 0;
        for (const TCHAR Character : Text)
        {
            if (Character < TEXT('0') || Character > TEXT('9'))
            {
                return false;
            }
            const uint64 Digit = static_cast<uint64>(Character - TEXT('0'));
            if (Parsed > (MAX_uint64 - Digit) / 10ULL)
            {
                return false;
            }
            Parsed = Parsed * 10ULL + Digit;
        }
        Value = Parsed;
        return true;
    }

    bool ParseInt32(const FString& Text, int32& Value)
    {
        return !Text.IsEmpty() && LexTryParseString(Value, *Text);
    }

    bool SaveText(const FString& OutputPath, const FString& Text)
    {
        const FString AbsolutePath = FPaths::ConvertRelativePathToFull(OutputPath);
        const FString Directory = FPaths::GetPath(AbsolutePath);
        if (Directory.IsEmpty() || !IFileManager::Get().MakeDirectory(*Directory, true))
        {
            return false;
        }
        return FFileHelper::SaveStringToFile(
            Text,
            *AbsolutePath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }

    FString EscapeReportJsonString(const FString& Text)
    {
        FString Escaped;
        Escaped.Reserve(Text.Len() + 16);
        for (const TCHAR Character : Text)
        {
            switch (Character)
            {
            case TEXT('"'):
                Escaped += TEXT("\\\"");
                break;
            case TEXT('\\'):
                Escaped += TEXT("\\\\");
                break;
            case TEXT('\b'):
                Escaped += TEXT("\\b");
                break;
            case TEXT('\f'):
                Escaped += TEXT("\\f");
                break;
            case TEXT('\n'):
                Escaped += TEXT("\\n");
                break;
            case TEXT('\r'):
                Escaped += TEXT("\\r");
                break;
            case TEXT('\t'):
                Escaped += TEXT("\\t");
                break;
            default:
                if (Character < 0x20)
                {
                    Escaped += FString::Printf(TEXT("\\u%04x"), static_cast<uint32>(Character));
                }
                else
                {
                    Escaped.AppendChar(Character);
                }
                break;
            }
        }
        return Escaped;
    }

    void AppendConfig(FString& Json, const FSeedForgeConfig& Config)
    {
        Json += FString::Printf(
            TEXT("{\"gridWidth\":%d,\"gridHeight\":%d,\"roomCount\":%d,")
            TEXT("\"minRoomWidth\":%d,\"maxRoomWidth\":%d,")
            TEXT("\"minRoomHeight\":%d,\"maxRoomHeight\":%d,")
            TEXT("\"roomPadding\":%d,\"maxPlacementAttempts\":%d}"),
            Config.GridWidth,
            Config.GridHeight,
            Config.RoomCount,
            Config.MinRoomWidth,
            Config.MaxRoomWidth,
            Config.MinRoomHeight,
            Config.MaxRoomHeight,
            Config.RoomPadding,
            Config.MaxPlacementAttempts);
    }

    FString ExportBenchmarkJson(
        const FSeedForgeBenchmarkConfig& Config,
        const FSeedForgeBenchmarkReport& Report)
    {
        const FString EngineVersion = EscapeReportJsonString(FEngineVersion::Current().ToString());
        const FString Platform = EscapeReportJsonString(FString(ANSI_TO_TCHAR(FPlatformProperties::PlatformName())));
        const FString Cpu = EscapeReportJsonString(FPlatformMisc::GetCPUBrand());
        const FString Timestamp = EscapeReportJsonString(FDateTime::UtcNow().ToIso8601());

        FString Json = FString::Printf(
            TEXT("{\"schema\":\"seedforge.benchmark\",\"schemaVersion\":1,")
            TEXT("\"tool\":{\"unrealEngine\":\"%s\",\"platform\":\"%s\",")
            TEXT("\"cpu\":\"%s\",\"generatedAtUtc\":\"%s\"},")
            TEXT("\"seedStart\":\"%llu\",\"warmupCount\":%d,")
            TEXT("\"requestedSampleCount\":%d,\"attemptedSampleCount\":%d,")
            TEXT("\"successCount\":%d,\"failureCount\":%d,")
            TEXT("\"aggregateHash\":\"%llu\",")
            TEXT("\"timingsMilliseconds\":{\"total\":%.9f,\"min\":%.9f,")
            TEXT("\"median\":%.9f,\"p95\":%.9f,\"max\":%.9f},\"config\":"),
            *EngineVersion,
            *Platform,
            *Cpu,
            *Timestamp,
            Report.SeedStart,
            Report.WarmupCount,
            Report.RequestedSampleCount,
            Report.AttemptedSampleCount,
            Report.SuccessCount,
            Report.FailureCount,
            Report.AggregateHash,
            Report.TotalMilliseconds,
            Report.MinMilliseconds,
            Report.MedianMilliseconds,
            Report.P95Milliseconds,
            Report.MaxMilliseconds);
        AppendConfig(Json, Config.GenerationConfig);
        Json += TEXT("}");
        return Json;
    }

    int32 RunGenerate(const FString& Params)
    {
        FString SeedText;
        FString OutputPath;
        if (!ReadArgument(Params, TEXT("Seed"), SeedText)
            || !ReadArgument(Params, TEXT("Output"), OutputPath))
        {
            return Fail(
                InvalidArgumentsExitCode,
                TEXT("Generate requires -Seed=<uint64> and -Output=<json>."));
        }

        uint64 Seed = 0;
        if (!ParseUInt64(SeedText, Seed))
        {
            return Fail(InvalidArgumentsExitCode, TEXT("Generate Seed must be an exact uint64 decimal value."));
        }

        FSeedForgeLayoutDocument Document;
        const FSeedForgeResult Generated = FSeedForgeGenerator::Generate(Seed, Document.Config);
        if (!Generated.IsSuccess())
        {
            return Fail(OperationFailedExitCode, TEXT("Generate failed: ") + Generated.ErrorMessage);
        }
        Document.Layout = Generated.Layout;
        if (!SaveText(OutputPath, FSeedForgeLayoutCodec::ExportCanonicalJson(Document)))
        {
            return Fail(WriteFailedExitCode, TEXT("Could not write Generate output to '") + OutputPath + TEXT("'."));
        }
        UE_LOG(LogSeedForgeReportCommandlet, Display, TEXT("Generated canonical layout '%s'."), *OutputPath);
        return 0;
    }

    int32 RunDiff(const FString& Params)
    {
        FString LeftPath;
        FString RightPath;
        FString OutputPath;
        if (!ReadArgument(Params, TEXT("Left"), LeftPath)
            || !ReadArgument(Params, TEXT("Right"), RightPath)
            || !ReadArgument(Params, TEXT("Output"), OutputPath))
        {
            return Fail(
                InvalidArgumentsExitCode,
                TEXT("Diff requires -Left=<json>, -Right=<json>, and -Output=<json>."));
        }

        FString LeftJson;
        FString RightJson;
        if (!FFileHelper::LoadFileToString(LeftJson, *LeftPath))
        {
            return Fail(OperationFailedExitCode, TEXT("Could not read Left document '") + LeftPath + TEXT("'."));
        }
        if (!FFileHelper::LoadFileToString(RightJson, *RightPath))
        {
            return Fail(OperationFailedExitCode, TEXT("Could not read Right document '") + RightPath + TEXT("'."));
        }

        const FSeedForgeDocumentResult Left = FSeedForgeLayoutCodec::ImportCanonicalJson(LeftJson);
        if (!Left.IsSuccess())
        {
            return Fail(OperationFailedExitCode, TEXT("Left document is invalid: ") + Left.ErrorMessage);
        }
        const FSeedForgeDocumentResult Right = FSeedForgeLayoutCodec::ImportCanonicalJson(RightJson);
        if (!Right.IsSuccess())
        {
            return Fail(OperationFailedExitCode, TEXT("Right document is invalid: ") + Right.ErrorMessage);
        }

        const FSeedForgeLayoutDifference Difference = FSeedForgeLayoutDiffer::Compare(
            Left.Document,
            Right.Document);
        if (!SaveText(OutputPath, FSeedForgeLayoutDiffer::ExportCanonicalJson(Difference)))
        {
            return Fail(WriteFailedExitCode, TEXT("Could not write Diff output to '") + OutputPath + TEXT("'."));
        }
        UE_LOG(LogSeedForgeReportCommandlet, Display, TEXT("%s"), *Difference.ToHumanSummary());
        UE_LOG(LogSeedForgeReportCommandlet, Display, TEXT("Wrote structural diff '%s'."), *OutputPath);
        return 0;
    }

    int32 RunBenchmark(const FString& Params)
    {
        FString SeedStartText;
        FString SeedCountText;
        FString WarmupText;
        FString OutputPath;
        if (!ReadArgument(Params, TEXT("SeedStart"), SeedStartText)
            || !ReadArgument(Params, TEXT("SeedCount"), SeedCountText)
            || !ReadArgument(Params, TEXT("Warmup"), WarmupText)
            || !ReadArgument(Params, TEXT("Output"), OutputPath))
        {
            return Fail(
                InvalidArgumentsExitCode,
                TEXT("Benchmark requires -SeedStart=<uint64>, -SeedCount=<int>, -Warmup=<int>, and -Output=<json>."));
        }

        FSeedForgeBenchmarkConfig Config;
        if (!ParseUInt64(SeedStartText, Config.SeedStart)
            || !ParseInt32(SeedCountText, Config.SeedCount)
            || !ParseInt32(WarmupText, Config.WarmupCount))
        {
            return Fail(InvalidArgumentsExitCode, TEXT("Benchmark numeric arguments are invalid."));
        }

        const FSeedForgeBenchmarkReport Report = FSeedForgeBenchmarkRunner::Run(Config);
        if (!Report.IsSuccess())
        {
            const FString Detail = Report.ErrorMessage.IsEmpty()
                ? FString::Printf(TEXT("%d sample generation(s) failed."), Report.FailureCount)
                : Report.ErrorMessage;
            return Fail(OperationFailedExitCode, TEXT("Benchmark failed: ") + Detail);
        }
        if (!SaveText(OutputPath, ExportBenchmarkJson(Config, Report)))
        {
            return Fail(WriteFailedExitCode, TEXT("Could not write Benchmark output to '") + OutputPath + TEXT("'."));
        }
        UE_LOG(
            LogSeedForgeReportCommandlet,
            Display,
            TEXT("Benchmark passed: samples=%d median=%.6fms p95=%.6fms aggregateHash=%llu output='%s'."),
            Report.SuccessCount,
            Report.MedianMilliseconds,
            Report.P95Milliseconds,
            Report.AggregateHash,
            *OutputPath);
        return 0;
    }
}

USeedForgeReportCommandlet::USeedForgeReportCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 USeedForgeReportCommandlet::Main(const FString& Params)
{
    FString Mode;
    if (!SeedForge::Report::Private::ReadArgument(Params, TEXT("Mode"), Mode))
    {
        return SeedForge::Report::Private::Fail(
            SeedForge::Report::Private::InvalidArgumentsExitCode,
            TEXT("Missing required Mode. Use -Mode=Generate, -Mode=Diff, or -Mode=Benchmark."));
    }
    if (Mode.Equals(TEXT("Generate"), ESearchCase::IgnoreCase))
    {
        return SeedForge::Report::Private::RunGenerate(Params);
    }
    if (Mode.Equals(TEXT("Diff"), ESearchCase::IgnoreCase))
    {
        return SeedForge::Report::Private::RunDiff(Params);
    }
    if (Mode.Equals(TEXT("Benchmark"), ESearchCase::IgnoreCase))
    {
        return SeedForge::Report::Private::RunBenchmark(Params);
    }
    return SeedForge::Report::Private::Fail(
        SeedForge::Report::Private::InvalidArgumentsExitCode,
        FString::Printf(TEXT("Unsupported Mode '%s'. Use Generate, Diff, or Benchmark."), *Mode));
}

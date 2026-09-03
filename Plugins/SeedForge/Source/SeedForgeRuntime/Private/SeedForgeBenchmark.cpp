#include "SeedForgeBenchmark.h"

#include "HAL/PlatformTime.h"
#include "SeedForgeGenerator.h"

namespace SeedForge::Benchmark::Private
{
    void HashByte(uint64& Hash, uint8 Byte)
    {
        Hash ^= Byte;
        Hash *= 1099511628211ULL;
    }

    void HashUInt64(uint64& Hash, uint64 Value)
    {
        for (uint32 Shift = 0; Shift < 64; Shift += 8)
        {
            HashByte(Hash, static_cast<uint8>((Value >> Shift) & 0xFFULL));
        }
    }

    bool WouldSeedRangeOverflow(uint64 SeedStart, int32 Count)
    {
        return Count > 0 && SeedStart > MAX_uint64 - static_cast<uint64>(Count - 1);
    }
}

double FSeedForgeBenchmarkRunner::CalculateNearestRankPercentile(
    TArray<double> Samples,
    double Percentile)
{
    if (Samples.IsEmpty() || !FMath::IsFinite(Percentile))
    {
        return 0.0;
    }

    Samples.Sort();
    const double Clamped = FMath::Clamp(Percentile, 0.0, 1.0);
    const int32 RankIndex = Clamped <= 0.0
        ? 0
        : FMath::CeilToInt(Clamped * Samples.Num()) - 1;
    return Samples[FMath::Clamp(RankIndex, 0, Samples.Num() - 1)];
}

FSeedForgeBenchmarkReport FSeedForgeBenchmarkRunner::Run(const FSeedForgeBenchmarkConfig& Config)
{
    FSeedForgeBenchmarkReport Report;
    Report.SeedStart = Config.SeedStart;
    Report.WarmupCount = Config.WarmupCount;
    Report.RequestedSampleCount = Config.SeedCount;

    if (Config.SeedCount < 1 || Config.SeedCount > MaxSampleCount)
    {
        Report.ErrorMessage = FString::Printf(
            TEXT("SeedCount must be in [1, %d]; received %d."),
            MaxSampleCount,
            Config.SeedCount);
        return Report;
    }
    if (Config.WarmupCount < 0 || Config.WarmupCount > MaxSampleCount)
    {
        Report.ErrorMessage = FString::Printf(
            TEXT("WarmupCount must be in [0, %d]; received %d."),
            MaxSampleCount,
            Config.WarmupCount);
        return Report;
    }
    if (SeedForge::Benchmark::Private::WouldSeedRangeOverflow(Config.SeedStart, Config.SeedCount)
        || SeedForge::Benchmark::Private::WouldSeedRangeOverflow(Config.SeedStart, Config.WarmupCount))
    {
        Report.ErrorMessage = TEXT("Seed range exceeds uint64 without wraparound.");
        return Report;
    }

    const FSeedForgeResult ConfigValidation = FSeedForgeGenerator::ValidateConfig(Config.GenerationConfig);
    if (!ConfigValidation.IsSuccess())
    {
        Report.ErrorMessage = TEXT("Invalid generation configuration: ") + ConfigValidation.ErrorMessage;
        return Report;
    }

    for (int32 Index = 0; Index < Config.WarmupCount; ++Index)
    {
        const FSeedForgeResult Warmup = FSeedForgeGenerator::Generate(
            Config.SeedStart + static_cast<uint64>(Index),
            Config.GenerationConfig);
        if (!Warmup.IsSuccess())
        {
            Report.ErrorMessage = FString::Printf(
                TEXT("Warm-up generation failed at seed %llu: %s"),
                Config.SeedStart + static_cast<uint64>(Index),
                *Warmup.ErrorMessage);
            return Report;
        }
    }

    TArray<double> Samples;
    Samples.Reserve(Config.SeedCount);
    uint64 AggregateHash = 14695981039346656037ULL;
    for (int32 Index = 0; Index < Config.SeedCount; ++Index)
    {
        const uint64 Seed = Config.SeedStart + static_cast<uint64>(Index);
        const double StartSeconds = FPlatformTime::Seconds();
        const FSeedForgeResult Result = FSeedForgeGenerator::Generate(Seed, Config.GenerationConfig);
        const double ElapsedMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

        ++Report.AttemptedSampleCount;
        Samples.Add(ElapsedMilliseconds);
        Report.TotalMilliseconds += ElapsedMilliseconds;
        if (Result.IsSuccess())
        {
            ++Report.SuccessCount;
            SeedForge::Benchmark::Private::HashUInt64(AggregateHash, Seed);
            SeedForge::Benchmark::Private::HashUInt64(AggregateHash, Result.Layout.CanonicalHash);
        }
        else
        {
            ++Report.FailureCount;
        }
    }

    Report.AggregateHash = AggregateHash;
    Samples.Sort();
    Report.MinMilliseconds = Samples[0];
    Report.MedianMilliseconds = CalculateNearestRankPercentile(Samples, 0.50);
    Report.P95Milliseconds = CalculateNearestRankPercentile(Samples, 0.95);
    Report.MaxMilliseconds = Samples.Last();
    return Report;
}

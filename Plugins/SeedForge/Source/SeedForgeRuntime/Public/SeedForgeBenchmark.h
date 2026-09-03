#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"

struct SEEDFORGERUNTIME_API FSeedForgeBenchmarkConfig
{
    uint64 SeedStart = 0;
    int32 SeedCount = 10'000;
    int32 WarmupCount = 100;
    FSeedForgeConfig GenerationConfig;
};

struct SEEDFORGERUNTIME_API FSeedForgeBenchmarkReport
{
    uint64 SeedStart = 0;
    int32 WarmupCount = 0;
    int32 RequestedSampleCount = 0;
    int32 AttemptedSampleCount = 0;
    int32 SuccessCount = 0;
    int32 FailureCount = 0;
    uint64 AggregateHash = 0;
    double TotalMilliseconds = 0.0;
    double MinMilliseconds = 0.0;
    double MedianMilliseconds = 0.0;
    double P95Milliseconds = 0.0;
    double MaxMilliseconds = 0.0;
    FString ErrorMessage;

    bool IsSuccess() const
    {
        return ErrorMessage.IsEmpty()
            && AttemptedSampleCount == RequestedSampleCount
            && SuccessCount == RequestedSampleCount
            && FailureCount == 0;
    }
};

class SEEDFORGERUNTIME_API FSeedForgeBenchmarkRunner
{
public:
    static constexpr int32 MaxSampleCount = 1'000'000;

    static double CalculateNearestRankPercentile(
        TArray<double> Samples,
        double Percentile);
    static FSeedForgeBenchmarkReport Run(const FSeedForgeBenchmarkConfig& Config);
};

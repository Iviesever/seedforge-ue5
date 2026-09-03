#include "SeedForgeBenchmark.h"

double FSeedForgeBenchmarkRunner::CalculateNearestRankPercentile(
    TArray<double> Samples,
    double Percentile)
{
    return 0.0;
}

FSeedForgeBenchmarkReport FSeedForgeBenchmarkRunner::Run(const FSeedForgeBenchmarkConfig& Config)
{
    FSeedForgeBenchmarkReport Report;
    Report.ErrorMessage = TEXT("PACT-23 benchmark is not implemented.");
    return Report;
}

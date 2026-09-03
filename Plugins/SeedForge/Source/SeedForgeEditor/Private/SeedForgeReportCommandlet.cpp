#include "SeedForgeReportCommandlet.h"

DEFINE_LOG_CATEGORY_STATIC(LogSeedForgeReportCommandlet, Log, All);

USeedForgeReportCommandlet::USeedForgeReportCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 USeedForgeReportCommandlet::Main(const FString& Params)
{
    UE_LOG(
        LogSeedForgeReportCommandlet,
        Error,
        TEXT("Missing required Mode because PACT-23 commandlet is not implemented."));
    return 1;
}

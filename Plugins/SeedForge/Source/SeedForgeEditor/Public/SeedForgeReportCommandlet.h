#pragma once

#include "Commandlets/Commandlet.h"
#include "SeedForgeReportCommandlet.generated.h"

UCLASS()
class SEEDFORGEEDITOR_API USeedForgeReportCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    USeedForgeReportCommandlet();
    virtual int32 Main(const FString& Params) override;
};

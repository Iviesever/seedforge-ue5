#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"

class SEEDFORGERUNTIME_API FSeedForgeValidator
{
public:
    static FSeedForgeValidationResult Validate(const FSeedForgeLayout& Layout, const FSeedForgeConfig& Config);
};


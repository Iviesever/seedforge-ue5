#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"

class SEEDFORGERUNTIME_API FSeedForgeGenerator
{
public:
    static FSeedForgeResult ValidateConfig(const FSeedForgeConfig& Config);
    static FSeedForgeResult Generate(uint64 Seed, const FSeedForgeConfig& Config);
    static uint64 ComputeCanonicalHash(const FSeedForgeLayout& Layout);
};

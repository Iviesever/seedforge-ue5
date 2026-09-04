#pragma once

#include "CoreMinimal.h"
#include "SeedForgeCaptureTypes.h"

struct SEEDFORGERUNTIME_API FSeedForgeGameplayActorCounts
{
    int32 Players = 0;
    int32 DataCores = 0;
    int32 Enemies = 0;
    int32 Exits = 0;
};

struct SEEDFORGERUNTIME_API FSeedForgeGameplaySmokeTrace
{
    FString GitSha;
    FString EngineVersion;
    uint64 Seed = 0;
    uint64 LayoutHash = 0;
    uint64 EncounterHash = 0;
    uint64 RunGeneration = 0;
    uint64 AppliedRequestId = 0;
    FSeedForgeGameplayActorCounts ActorCounts;
    TArray<FString> StateTransitions;
    TArray<FString> Actions;
    TArray<FString> ScreenshotPaths;
    TArray<FSeedForgeCaptureReceipt> Captures;
    bool bSuccess = false;
    FString FailureCode;
    FString FailureMessage;
};

class SEEDFORGERUNTIME_API FSeedForgeGameplaySmokeCodec
{
public:
    static FString ExportCanonicalJson(const FSeedForgeGameplaySmokeTrace& Trace);
};

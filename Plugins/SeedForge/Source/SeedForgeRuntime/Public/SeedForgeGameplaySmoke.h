#pragma once

#include "CoreMinimal.h"
#include "SeedForgeCaptureTypes.h"
#include "SeedForgeGameplayDiagnostics.h"
#include "Engine/EngineBaseTypes.h"

class ASeedForgeGameplayCoordinator;
class ASeedForgeEnemyPawn;
class UWorld;

struct SEEDFORGERUNTIME_API FSeedForgeGameplayActorCounts
{
    int32 Players = 0;
    int32 DataCores = 0;
    int32 Enemies = 0;
    int32 Exits = 0;
};

struct SEEDFORGERUNTIME_API FSeedForgeGameplaySmokeTrace
{
    static constexpr int32 MaxWalkableCells = 4096;
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
    FSeedForgeEnemyPathEvidence PathEvidence;
    TArray<FIntPoint> WalkableCells;
    int32 RemainingPathDelegateBindings = 0;
    bool bSuccess = false;
    FString FailureCode;
    FString FailureMessage;
};

class SEEDFORGERUNTIME_API FSeedForgeGameplaySmokePathObserver final
{
public:
    FSeedForgeGameplaySmokePathObserver() = default;
    ~FSeedForgeGameplaySmokePathObserver();
    FSeedForgeGameplaySmokePathObserver(const FSeedForgeGameplaySmokePathObserver&) = delete;
    FSeedForgeGameplaySmokePathObserver& operator=(const FSeedForgeGameplaySmokePathObserver&) = delete;
    bool Start(ASeedForgeGameplayCoordinator& Coordinator);
    void Cancel();
    bool IsComplete() const;
    const FSeedForgeEnemyPathEvidence& GetEvidence() const;
    int32 GetRemainingDelegateBindings() const;
private:
    bool MatchesCurrentRun() const;
    void Detach();
    void OnPath(ASeedForgeEnemyPawn* Enemy, uint64 Run, uint64 Request,
        const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result);
    void OnQueued(ESeedForgeRunState StateBefore, const FSeedForgeGameplaySnapshot& Snapshot);
    void OnWorld(UWorld* World, ELevelTick TickType, float DeltaSeconds);
    FSeedForgeEnemyPathProof Proof;
    TWeakObjectPtr<ASeedForgeGameplayCoordinator> Owner;
    TWeakObjectPtr<ASeedForgeEnemyPawn> Target;
    TWeakObjectPtr<UWorld> OwningWorld;
    TArray<FIntPoint> Walkable;
    FSeedForgeGameplayTuning Tuning;
    uint64 RunGeneration = 0;
    uint64 SourceRequestId = 0;
    FDelegateHandle PathHandle, QueueHandle, WorldHandle;
};

class SEEDFORGERUNTIME_API FSeedForgeGameplaySmokeCodec
{
public:
    static FString ExportCanonicalJson(const FSeedForgeGameplaySmokeTrace& Trace);
    static bool ValidatePathEvidence(const FSeedForgeGameplaySmokeTrace& Trace, FString& OutError);
};

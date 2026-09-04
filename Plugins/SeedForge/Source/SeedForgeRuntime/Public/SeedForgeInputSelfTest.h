#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SeedForgeGameplayDiagnostics.h"
#include "InputCoreTypes.h"
#include "SeedForgeInputSelfTest.generated.h"

enum class ESeedForgeInputSourceKind : uint8 { Unverified, CleanRevision, DiagnosticRevision };
enum class ESeedForgeInputSelfTestFailure : uint8
{
    None, MissingInputEvidence, InvalidSourceIdentity, ViewportUnavailable,
    ViewportNotReady, Cancelled, ObservationLimitExceeded, SelfTestTimeout,
    InvalidArguments, TraceWriteFailed
};

SEEDFORGERUNTIME_API const TCHAR* LexToString(ESeedForgeInputSelfTestFailure Code);

struct SEEDFORGERUNTIME_API FSeedForgeInputSelfTestOptions
{
    bool bEnabled = false;
    FString SourceIdentity;
    FString TracePath;
    uint64 Seed = 24301;
};

struct SEEDFORGERUNTIME_API FSeedForgeInputEffectObservation
{
    FString Action;
    FString Key;
    uint64 RunGeneration = 0;
    uint64 SourceRequestId = 0;
    uint64 InjectedFrame = 0;
    uint64 EffectFrame = 0;
    uint64 ReleaseFrame = 0;
    FDateTime InjectedAtUtc;
    FDateTime EffectAtUtc;
    FVector Before = FVector::ZeroVector;
    FVector After = FVector::ZeroVector;
    FVector LaunchVelocity = FVector::ZeroVector;
    FString ActorKey;
    double CooldownBefore = 0.0;
    double CooldownAfter = 0.0;
    int32 LiveEnemiesBefore = 0;
    int32 LiveEnemiesAfter = 0;
    bool bTargetAliveBefore = false;
    bool bTargetAliveAfter = false;
    bool bConfirmed = false;
};

struct SEEDFORGERUNTIME_API FSeedForgeInputActorObservation
{
    FString Key;
    FString Role;
    uint32 StableId = 0;
    FString OwnerKey;
    FIntPoint Cell = FIntPoint::ZeroValue;
};

struct SEEDFORGERUNTIME_API FSeedForgeInputRunObservation
{
    static constexpr int32 MaxActors = 16;
    FString Label;
    uint64 Frame = 0;
    FDateTime AtUtc;
    FSeedForgeGameplaySnapshot Snapshot;
    FSeedForgeRunResourceSnapshot Resources;
    double DashCooldownRemaining = 0.0;
    FString CoordinatorKey;
    FString ControllerKey;
    FString HudKey;
    int32 CoordinatorCount = 0;
    int32 ControllerCount = 0;
    int32 HudCount = 0;
    int32 HudOverlayCount = 0;
    bool bPreviousActorsDestroyed = false;
    TArray<FSeedForgeInputActorObservation> Actors;
};

struct SEEDFORGERUNTIME_API FSeedForgeInputSetupObservation
{
    FString Label;
    FString ActorKey;
    uint64 RunGeneration = 0;
    uint64 SourceRequestId = 0;
    uint64 Frame = 0;
    FDateTime AtUtc;
    FIntPoint Cell = FIntPoint::ZeroValue;
    FVector Position = FVector::ZeroVector;
};

struct SEEDFORGERUNTIME_API FSeedForgeInputSelfTestTrace
{
    static constexpr int32 MaxInputEvents = 96;
    static constexpr int32 MaxTransitions = 24;
    static constexpr int32 MaxQueuedRuns = 8;
    static constexpr int32 MaxRuns = 4;
    static constexpr int32 MaxSetups = 16;
    static constexpr int32 MaxWalkableCells = 4096;
    FString SourceIdentity;
    FString SourceRevision;
    ESeedForgeInputSourceKind SourceKind = ESeedForgeInputSourceKind::Unverified;
    FString EngineVersion;
    FString Mode = TEXT("ordinary");
    uint64 ExpectedInitialSeed = 0;
    bool bGameplaySmokeEnabled = false;
    FDateTime StartedAtUtc;
    FDateTime CompletedAtUtc;
    uint64 StartedFrame = 0;
    uint64 CompletedFrame = 0;
    FSeedForgeGameplaySnapshot Initial;
    FSeedForgeGameplaySnapshot Final;
    FSeedForgeEnemyPathEvidence PathEvidence;
    TArray<FIntPoint> WalkableCells;
    TArray<FSeedForgeInputRunObservation> Runs;
    TArray<FSeedForgeInputSetupObservation> Setups;
    TArray<FSeedForgeInputEffectObservation> InputEvents;
    TArray<FSeedForgeRunTransitionObservation> Transitions;
    TArray<FSeedForgeQueuedRunObservation> QueuedRuns;
    int32 CompletionCount = 0;
    int32 RemainingDelegateBindings = 0;
    int32 RemainingPressedKeys = 0;
    bool bSuccess = false;
    ESeedForgeInputSelfTestFailure FailureCode = ESeedForgeInputSelfTestFailure::MissingInputEvidence;
    FString FailureMessage;
};

class SEEDFORGERUNTIME_API FSeedForgeInputSelfTestCodec
{
public:
    static bool ParseSourceIdentity(const FString& Source, ESeedForgeInputSourceKind& OutKind, FString& OutRevision);
    static bool ParseOptions(const TCHAR* CommandLine, FSeedForgeInputSelfTestOptions& OutOptions, FString& OutError);
    static bool ValidateEvidence(const FSeedForgeInputSelfTestTrace& Trace, FString& OutError);
    static FString ExportCanonicalJson(const FSeedForgeInputSelfTestTrace& Trace);
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSeedForgeInputSelfTestFinished, const FSeedForgeInputSelfTestTrace&);

UCLASS()
class SEEDFORGERUNTIME_API USeedForgeInputSelfTestComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    USeedForgeInputSelfTestComponent();
    bool Start(const FString& SourceIdentity, uint64 ExpectedInitialSeed);
    void BeforeInput(float DeltaSeconds, bool bGamePaused);
    void AfterInput(float DeltaSeconds, bool bGamePaused);
    void Cancel();
    bool IsRunning() const;
    const FSeedForgeInputSelfTestTrace& GetTrace() const;
    FSeedForgeInputSelfTestFinished& OnFinished();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    enum class EPhase : uint8 { AwaitInitial, AwaitPath, Ready, AwaitEffect, AwaitRun, Complete };
    struct FHeldInput { FKey Key; int32 EventIndex = INDEX_NONE; uint64 Frame = 0; bool bReleaseNextFrame = false; };
    void ObserveWorld(UWorld* World, ELevelTick TickType, float DeltaSeconds);
    void ObserveTransition(ESeedForgeRunState From, ESeedForgeRunState To, const FSeedForgeGameplaySnapshot& Snapshot);
    void ObserveQueued(ESeedForgeRunState StateBefore, const FSeedForgeGameplaySnapshot& Snapshot);
    void ObservePath(class ASeedForgeEnemyPawn* Enemy, uint64 Run, uint64 Request,
        const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result);
    bool ResolveCoordinator();
    bool CaptureRun(const FString& Label);
    bool PreparePath();
    bool PlaceActor(AActor* Actor, const FIntPoint& Cell, const FString& Label, float Height);
    void PrepareEffect();
    void BeginEffect();
    void CompleteEffect(int32 Index);
    void InjectKey(const FKey& Key, int32 EventIndex, bool bReleaseNextFrame);
    bool InjectPointer(const FVector& Target);
    void ReleaseInputs(bool bAll);
    void Cleanup();
    void FinishSuccess();
    bool CheckBudget();
    void FinishFailure(ESeedForgeInputSelfTestFailure Code, const FString& Message);
    FSeedForgeInputSelfTestTrace Trace;
    FSeedForgeInputSelfTestFinished Finished;
    bool bStarted = false;
    bool bFinished = false;
    EPhase Phase = EPhase::AwaitInitial;
    int32 EffectIndex = 0;
    int32 PendingEvent = INDEX_NONE;
    bool bPrepared = false;
    bool bLossRequested = false;
    bool bHeldPointerIntent = false;
    FVector HeldPointerDirection = FVector::ZeroVector;
    uint64 SetupFrame = 0;
    double StartedSeconds = 0.0;
    FVector LastPhysicalPosition = FVector::ZeroVector;
    FIntPoint PadCell = FIntPoint::ZeroValue;
    TWeakObjectPtr<class ASeedForgePlayerController> Controller;
    TWeakObjectPtr<class ASeedForgeGameplayCoordinator> Coordinator;
    TWeakObjectPtr<class ASeedForgeEnemyPawn> TargetEnemy;
    TArray<TWeakObjectPtr<AActor>> PreviousActors;
    TArray<FHeldInput> HeldInputs;
    FSeedForgeEnemyPathProof PathProof;
    FDelegateHandle WorldHandle, TransitionHandle, QueueHandle, PathHandle;
};

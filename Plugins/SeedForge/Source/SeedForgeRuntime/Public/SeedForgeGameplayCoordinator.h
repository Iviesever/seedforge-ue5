#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeedForgeAsync.h"
#include "SeedForgeEncounter.h"
#include "SeedForgeGameplayTypes.h"
#include "SeedForgeGameplayDiagnostics.h"
#include "SeedForgeGameplaySmoke.h"
#include "SeedForgeRunState.h"
#include "SeedForgeGameplayCoordinator.generated.h"

class ASeedForgeCorePickup;
class ASeedForgeEnemyPawn;
class ASeedForgeExitActor;
class ASeedForgePlayerCharacter;
class ASeedForgePreviewActor;
class USeedForgeGameplayCaptureComponent;

#if WITH_DEV_AUTOMATION_TESTS
namespace SeedForge::GameplaySmokeTests { struct FWatchdogAccess; }
#endif

DECLARE_MULTICAST_DELEGATE_ThreeParams(FSeedForgeRunStateChanged,
    ESeedForgeRunState, ESeedForgeRunState, const FSeedForgeGameplaySnapshot&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSeedForgeRunQueued,
    ESeedForgeRunState, const FSeedForgeGameplaySnapshot&);
DECLARE_MULTICAST_DELEGATE_SixParams(FSeedForgeEnemyPathApplied,
    ASeedForgeEnemyPawn*, uint64, uint64, const FIntPoint&, const FIntPoint&, const FSeedForgePathResult&);

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgeGameplayCoordinator : public AActor
{
    GENERATED_BODY()

public:
    ASeedForgeGameplayCoordinator();

    uint64 StartRun(uint64 InSeed);
    bool ApplyGeneratedLayout(const FSeedForgeLayout& InLayout);
    void RestartSameSeed();
    void StartNewSeed();
    bool TryPlayerAttack(const FVector& Origin, const FVector& Forward);
    bool ApplyPlayerDamage(float Damage);
    FSeedForgeGameplaySnapshot GetSnapshot() const;
    FSeedForgeRunResourceSnapshot GetRunResourceSnapshot() const;
    FSeedForgeRunStateChanged& OnRunStateChanged() { return RunStateChanged; }
    FSeedForgeRunQueued& OnRunQueued() { return RunQueued; }
    FSeedForgeEnemyPathApplied& OnEnemyPathApplied() { return EnemyPathApplied; }
    const FSeedForgeGameplayTuning& GetTuning() const;
    const FSeedForgeLayout& GetLayout() const;
    const FSeedForgeEncounterPlan& GetEncounterPlan() const;
    int32 GetLiveCoreActorCount() const;
    int32 GetLiveEnemyActorCount() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend struct SeedForge::GameplaySmokeTests::FWatchdogAccess;
#endif
    void NotifyRunStateChanged(ESeedForgeRunState PreviousState);
    bool ApplyGeneratedLayout(const FSeedForgeLayout& InLayout, uint64 SourceRequestId);
    void HandleGenerationApplied(const FSeedForgeAsyncCompletion& Completion);
    void ClearRunObjects(bool bPreserveSmokeWatchdog = false);
    void TickInteractions();
    void ReplanEnemies();
    void EnterTerminalState();
    void EnterRunFailure(ESeedForgeRunFailureCode Code, const FString& Message,
        const TCHAR* SmokeCode = nullptr);
    void InitializeGameplaySmokeTrace();
    void CaptureScreenshot();
    void HandleCaptureCompleted(const FSeedForgeCaptureReceipt& Receipt);
    void HandleCaptureFailed(const FString& Error);
    void StartGameplaySmoke();
    bool IsGameplaySmokePathObservationInvalidated() const;
    void AdvanceGameplaySmoke();
    void TeleportSmokePlayer(const FIntPoint& Cell);
    void GameplaySmokeWatchdog();
    void RequestSmokeScreenshot(const TCHAR* Label);
    void CompleteGameplaySmoke();
    void FailGameplaySmoke(const TCHAR* FailureCode, const FString& FailureMessage);
    bool WriteGameplaySmokeTrace();
    ASeedForgePlayerCharacter* ResolvePlayer();

    enum class EGameplaySmokeStage : uint8
    {
        Disabled,
        AwaitPathProof,
        WaitingStartCapture,
        PrepareCombat,
        Attack,
        WaitingCombatCapture,
        FinishCombat,
        CollectCores,
        ReachExit,
        WaitingWinCapture,
        Complete,
        Failed
    };

    FSeedForgeGameplayTuning Tuning;
    FSeedForgeConfig GenerationConfig;
    FSeedForgeEncounterConfig EncounterConfig;
    FSeedForgeLayout Layout;
    FSeedForgeEncounterPlan EncounterPlan;
    FSeedForgeRunStateMachine RunState;
    FSeedForgeRunStateChanged RunStateChanged;
    FSeedForgeRunQueued RunQueued;
    FSeedForgeEnemyPathApplied EnemyPathApplied;
    uint64 Seed = 24301;
    uint64 ActiveRequestId = 0;
    uint64 AppliedRequestId = 0;
    uint64 RunGeneration = 0;
    float PlayerHealth = 0.0f;
    double NextAttackTime = 0.0;
    double NextContactDamageTime = 0.0;
    ESeedForgeRunFailureCode RunFailureCode = ESeedForgeRunFailureCode::None;
    FString RunFailureMessage;
    FDelegateHandle GenerationAppliedHandle;
    FTimerHandle InteractionTimer;
    FTimerHandle RepathTimer;
    FString CapturePath;
    FString CaptureRoot;
    bool bGameplaySmokeMode = false;
    bool bSmokeExitRequested = false;
    EGameplaySmokeStage GameplaySmokeStage = EGameplaySmokeStage::Disabled;
    int32 GameplaySmokeCoreIndex = 0;
    double GameplaySmokeStageDeadline = 0.0;
    FString GameplaySmokeTracePath;
    FString GameplaySmokeCaptureDirectory;
    FString GameplaySmokeGitSha;
    FSeedForgeGameplaySmokeTrace GameplaySmokeTrace;
    FSeedForgeGameplaySmokePathObserver GameplaySmokePathObserver;
    FTimerHandle GameplaySmokeTimer;
    FTimerHandle GameplaySmokeWatchdogTimer;

    UPROPERTY()
    TObjectPtr<USeedForgeGameplayCaptureComponent> CaptureComponent;

    UPROPERTY()
    TObjectPtr<ASeedForgePreviewActor> Visualization;

    UPROPERTY()
    TObjectPtr<ASeedForgePlayerCharacter> Player;

    UPROPERTY()
    TArray<TObjectPtr<ASeedForgeCorePickup>> CoreActors;

    UPROPERTY()
    TArray<TObjectPtr<ASeedForgeEnemyPawn>> EnemyActors;

    UPROPERTY()
    TObjectPtr<ASeedForgeExitActor> ExitActor;

    TMap<uint32, float> EnemyHealth;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeedForgeAsync.h"
#include "SeedForgeEncounter.h"
#include "SeedForgeGameplayTypes.h"
#include "SeedForgeGameplaySmoke.h"
#include "SeedForgeRunState.h"
#include "SeedForgeGameplayCoordinator.generated.h"

class ASeedForgeCorePickup;
class ASeedForgeEnemyPawn;
class ASeedForgeExitActor;
class ASeedForgePlayerCharacter;
class ASeedForgePreviewActor;

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgeGameplayCoordinator : public AActor
{
    GENERATED_BODY()

public:
    ASeedForgeGameplayCoordinator();

    void StartRun(uint64 InSeed);
    bool ApplyGeneratedLayout(const FSeedForgeLayout& InLayout);
    void RestartSameSeed();
    void StartNewSeed();
    bool TryPlayerAttack(const FVector& Origin, const FVector& Forward);
    bool ApplyPlayerDamage(float Damage);
    FSeedForgeGameplaySnapshot GetSnapshot() const;
    const FSeedForgeGameplayTuning& GetTuning() const;
    const FSeedForgeLayout& GetLayout() const;
    const FSeedForgeEncounterPlan& GetEncounterPlan() const;
    int32 GetLiveCoreActorCount() const;
    int32 GetLiveEnemyActorCount() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandleGenerationApplied(const FSeedForgeAsyncCompletion& Completion);
    void ClearRunObjects();
    void TickInteractions();
    void ReplanEnemies();
    void EnterTerminalState();
    void CaptureScreenshot();
    void ExitAfterCapture();
    void StartGameplaySmoke();
    void AdvanceGameplaySmoke();
    void GameplaySmokeWatchdog();
    void RequestSmokeScreenshot(const TCHAR* Label);
    bool IsPendingSmokeScreenshotReady() const;
    void CompleteGameplaySmoke();
    void FailGameplaySmoke(const TCHAR* FailureCode, const FString& FailureMessage);
    bool WriteGameplaySmokeTrace();
    ASeedForgePlayerCharacter* ResolvePlayer();

    enum class EGameplaySmokeStage : uint8
    {
        Disabled,
        Warmup,
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
    uint64 Seed = 24301;
    uint64 ActiveRequestId = 0;
    uint64 RunGeneration = 0;
    float PlayerHealth = 0.0f;
    double NextAttackTime = 0.0;
    double NextContactDamageTime = 0.0;
    FDelegateHandle GenerationAppliedHandle;
    FTimerHandle InteractionTimer;
    FTimerHandle RepathTimer;
    FTimerHandle CaptureTimer;
    FTimerHandle CaptureExitTimer;
    FString CapturePath;
    bool bGameplaySmokeMode = false;
    EGameplaySmokeStage GameplaySmokeStage = EGameplaySmokeStage::Disabled;
    int32 GameplaySmokeCoreIndex = 0;
    double GameplaySmokeStageDeadline = 0.0;
    FString GameplaySmokeTracePath;
    FString GameplaySmokeCaptureDirectory;
    FString GameplaySmokeGitSha;
    FString PendingSmokeScreenshotPath;
    FSeedForgeGameplaySmokeTrace GameplaySmokeTrace;
    FTimerHandle GameplaySmokeTimer;
    FTimerHandle GameplaySmokeWatchdogTimer;

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

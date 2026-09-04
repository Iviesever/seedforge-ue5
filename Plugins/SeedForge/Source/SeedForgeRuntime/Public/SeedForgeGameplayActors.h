#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SeedForgeGameplayDiagnostics.h"
#include "SeedForgeGameplayActors.generated.h"

class ASeedForgeGameplayCoordinator;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UGameViewportClient;
class USeedForgeInputSelfTestComponent;
struct FSeedForgeInputSelfTestTrace;
class SWidget;
class STextBlock;

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgePlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASeedForgePlayerCharacter();

    void SetGameplayCoordinator(ASeedForgeGameplayCoordinator* InCoordinator);
    void SetAimWorldPoint(const FVector& WorldPoint);
    FVector GetAimDirection() const;
    bool HasTopDownCamera() const;
    float GetAttackCooldownSeconds() const;
    float GetDashCooldownSeconds() const;
    double GetDashCooldownRemaining() const;
    void ShowAttackPulse();
    void ResetMovementIntent();
    virtual void PawnClientRestart() override;
    virtual void UnPossessed() override;

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Attack();
    void Dash();
    void HideAttackPulse();

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UStaticMeshComponent> AttackPulse;

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<USpringArmComponent> CameraArm;

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UCameraComponent> Camera;

    TWeakObjectPtr<ASeedForgeGameplayCoordinator> Coordinator;
    FVector AimDirection = FVector::ForwardVector;
    float CurrentForwardAxis = 0.0f;
    float CurrentRightAxis = 0.0f;
    double NextDashTime = 0.0;
    FTimerHandle AttackPulseTimer;
};

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASeedForgePlayerController();
    virtual void FlushPressedKeys() override;

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void PreProcessInput(float DeltaTime, bool bGamePaused) override;
    virtual void PostProcessInput(float DeltaTime, bool bGamePaused) override;
    virtual void SetupInputComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RestartSameSeed();
    void BeginInputSelfTest();
    void HandleInputSelfTestFinished(const FSeedForgeInputSelfTestTrace& Trace);
    void StartNewSeed();
    ASeedForgeGameplayCoordinator* ResolveGameplayCoordinator();
    TWeakObjectPtr<ASeedForgeGameplayCoordinator> GameplayCoordinator;
    UPROPERTY()
    TObjectPtr<USeedForgeInputSelfTestComponent> InputSelfTestComponent;
    FString InputSelfTestTracePath;
    FDelegateHandle InputSelfTestFinishedHandle;
    bool bInputSelfTestExitRequested = false;
};

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgeEnemyPawn : public APawn
{
    GENERATED_BODY()

public:
    ASeedForgeEnemyPawn();
    virtual void Tick(float DeltaSeconds) override;

    void Configure(uint32 InStableId, const FIntPoint& InCell);
    void SetPath(TArray<FVector> InWorldPath);
    void ClearPath();
    uint32 GetStableId() const;
    FIntPoint GetSpawnCell() const;
    FSeedForgeEnemyPathSnapshot GetPathSnapshot() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UStaticMeshComponent> Visual;

    uint32 StableId = 0;
    FIntPoint SpawnCell = FIntPoint::ZeroValue;
    TArray<FVector> WorldPath;
    int32 PathIndex = 0;
    uint64 PathRevision = 0;
    uint64 MovementSequence = 0;
    FSeedForgeEnemyMoveObservation LastMove;
};

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgeCorePickup : public AActor
{
    GENERATED_BODY()

public:
    ASeedForgeCorePickup();
    void Configure(uint32 InStableId, const FIntPoint& InCell);
    uint32 GetStableId() const;
    FIntPoint GetSpawnCell() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UStaticMeshComponent> Visual;

    uint32 StableId = 0;
    FIntPoint SpawnCell = FIntPoint::ZeroValue;
};

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgeExitActor : public AActor
{
    GENERATED_BODY()

public:
    ASeedForgeExitActor();
    void Configure(const FIntPoint& InCell);
    void SetUnlocked(bool bInUnlocked);
    bool IsUnlocked() const;
    FIntPoint GetSpawnCell() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UStaticMeshComponent> Visual;

    FIntPoint SpawnCell = FIntPoint::ZeroValue;
    bool bUnlocked = false;
};

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgeHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void PostRender() override;
    virtual void DrawHUD() override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RemoveOverlay();
    TSharedPtr<SWidget> Overlay;
    TSharedPtr<STextBlock> StatusText;
    TWeakObjectPtr<UGameViewportClient> OverlayViewport;
};

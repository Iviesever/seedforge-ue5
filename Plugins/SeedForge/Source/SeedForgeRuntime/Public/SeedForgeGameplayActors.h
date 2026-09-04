#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SeedForgeGameplayActors.generated.h"

class ASeedForgeGameplayCoordinator;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

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
    void ShowAttackPulse();

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
    FVector LastMoveDirection = FVector::ForwardVector;
    double NextDashTime = 0.0;
    FTimerHandle AttackPulseTimer;
};

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASeedForgePlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RestartSameSeed();
    void StartNewSeed();
    ASeedForgeGameplayCoordinator* ResolveGameplayCoordinator();
    TWeakObjectPtr<ASeedForgeGameplayCoordinator> GameplayCoordinator;
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

private:
    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UStaticMeshComponent> Visual;

    uint32 StableId = 0;
    FIntPoint SpawnCell = FIntPoint::ZeroValue;
    TArray<FVector> WorldPath;
    int32 PathIndex = 0;
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
    virtual void DrawHUD() override;
};

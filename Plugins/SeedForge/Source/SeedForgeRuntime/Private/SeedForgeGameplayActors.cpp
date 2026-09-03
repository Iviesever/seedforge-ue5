#include "SeedForgeGameplayActors.h"

ASeedForgePlayerCharacter::ASeedForgePlayerCharacter()
{
}

void ASeedForgePlayerCharacter::SetGameplayCoordinator(ASeedForgeGameplayCoordinator* InCoordinator)
{
}

void ASeedForgePlayerCharacter::SetAimWorldPoint(const FVector& WorldPoint)
{
}

FVector ASeedForgePlayerCharacter::GetAimDirection() const
{
    return FVector::ZeroVector;
}

bool ASeedForgePlayerCharacter::HasTopDownCamera() const
{
    return false;
}

float ASeedForgePlayerCharacter::GetAttackCooldownSeconds() const
{
    return 0.0f;
}

float ASeedForgePlayerCharacter::GetDashCooldownSeconds() const
{
    return 0.0f;
}

void ASeedForgePlayerCharacter::ShowAttackPulse()
{
}

void ASeedForgePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASeedForgePlayerCharacter::MoveForward(float Value)
{
}

void ASeedForgePlayerCharacter::MoveRight(float Value)
{
}

void ASeedForgePlayerCharacter::Attack()
{
}

void ASeedForgePlayerCharacter::Dash()
{
}

void ASeedForgePlayerCharacter::RestartSameSeed()
{
}

void ASeedForgePlayerCharacter::StartNewSeed()
{
}

void ASeedForgePlayerCharacter::HideAttackPulse()
{
}

ASeedForgePlayerController::ASeedForgePlayerController()
{
}

void ASeedForgePlayerController::BeginPlay()
{
    Super::BeginPlay();
}

void ASeedForgePlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
}

ASeedForgeEnemyPawn::ASeedForgeEnemyPawn()
{
}

void ASeedForgeEnemyPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void ASeedForgeEnemyPawn::Configure(uint32 InStableId, const FIntPoint& InCell)
{
}

void ASeedForgeEnemyPawn::SetPath(TArray<FVector> InWorldPath)
{
}

void ASeedForgeEnemyPawn::ClearPath()
{
}

uint32 ASeedForgeEnemyPawn::GetStableId() const
{
    return 0;
}

FIntPoint ASeedForgeEnemyPawn::GetSpawnCell() const
{
    return FIntPoint::ZeroValue;
}

ASeedForgeCorePickup::ASeedForgeCorePickup()
{
}

void ASeedForgeCorePickup::Configure(uint32 InStableId, const FIntPoint& InCell)
{
}

uint32 ASeedForgeCorePickup::GetStableId() const
{
    return 0;
}

FIntPoint ASeedForgeCorePickup::GetSpawnCell() const
{
    return FIntPoint::ZeroValue;
}

ASeedForgeExitActor::ASeedForgeExitActor()
{
}

void ASeedForgeExitActor::Configure(const FIntPoint& InCell)
{
}

void ASeedForgeExitActor::SetUnlocked(bool bInUnlocked)
{
}

bool ASeedForgeExitActor::IsUnlocked() const
{
    return false;
}

FIntPoint ASeedForgeExitActor::GetSpawnCell() const
{
    return FIntPoint::ZeroValue;
}

void ASeedForgeHUD::DrawHUD()
{
    Super::DrawHUD();
}

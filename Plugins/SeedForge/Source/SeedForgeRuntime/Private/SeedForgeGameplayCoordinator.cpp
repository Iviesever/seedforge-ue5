#include "SeedForgeGameplayCoordinator.h"

ASeedForgeGameplayCoordinator::ASeedForgeGameplayCoordinator()
{
}

void ASeedForgeGameplayCoordinator::StartRun(uint64 InSeed)
{
}

bool ASeedForgeGameplayCoordinator::ApplyGeneratedLayout(const FSeedForgeLayout& InLayout)
{
    return false;
}

void ASeedForgeGameplayCoordinator::RestartSameSeed()
{
}

void ASeedForgeGameplayCoordinator::StartNewSeed()
{
}

bool ASeedForgeGameplayCoordinator::TryPlayerAttack(const FVector& Origin, const FVector& Forward)
{
    return false;
}

bool ASeedForgeGameplayCoordinator::ApplyPlayerDamage(float Damage)
{
    return false;
}

FSeedForgeGameplaySnapshot ASeedForgeGameplayCoordinator::GetSnapshot() const
{
    return {};
}

const FSeedForgeGameplayTuning& ASeedForgeGameplayCoordinator::GetTuning() const
{
    return Tuning;
}

const FSeedForgeLayout& ASeedForgeGameplayCoordinator::GetLayout() const
{
    return Layout;
}

const FSeedForgeEncounterPlan& ASeedForgeGameplayCoordinator::GetEncounterPlan() const
{
    return EncounterPlan;
}

int32 ASeedForgeGameplayCoordinator::GetLiveCoreActorCount() const
{
    return 0;
}

int32 ASeedForgeGameplayCoordinator::GetLiveEnemyActorCount() const
{
    return 0;
}

void ASeedForgeGameplayCoordinator::BeginPlay()
{
    Super::BeginPlay();
}

void ASeedForgeGameplayCoordinator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ASeedForgeGameplayCoordinator::HandleGenerationApplied(const FSeedForgeAsyncCompletion& Completion)
{
}

void ASeedForgeGameplayCoordinator::ClearRunObjects()
{
}

void ASeedForgeGameplayCoordinator::TickInteractions()
{
}

void ASeedForgeGameplayCoordinator::ReplanEnemies()
{
}

void ASeedForgeGameplayCoordinator::EnterTerminalState()
{
}

ASeedForgePlayerCharacter* ASeedForgeGameplayCoordinator::ResolvePlayer()
{
    return nullptr;
}

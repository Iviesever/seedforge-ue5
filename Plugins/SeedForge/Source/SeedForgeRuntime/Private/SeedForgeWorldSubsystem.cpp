#include "SeedForgeWorldSubsystem.h"

#include "SeedForgeGenerator.h"

uint64 USeedForgeWorldSubsystem::RequestGeneration(
    uint64 Seed,
    const FSeedForgeConfig& Config)
{
    check(IsInGameThread());
    if (!Coordinator)
    {
        Coordinator = MakeUnique<FSeedForgeAsyncCoordinator>();
    }

    const TWeakObjectPtr<USeedForgeWorldSubsystem> WeakThis(this);
    return Coordinator->Start(
        [Seed, Config]()
        {
            return FSeedForgeGenerator::Generate(Seed, Config);
        },
        [WeakThis](FSeedForgeAsyncCompletion&& Completion)
        {
            if (USeedForgeWorldSubsystem* Subsystem = WeakThis.Get())
            {
                Subsystem->GenerationApplied.Broadcast(Completion);
            }
        });
}

void USeedForgeWorldSubsystem::CancelGeneration()
{
    check(IsInGameThread());
    if (Coordinator)
    {
        Coordinator->CancelActive();
    }
}

void USeedForgeWorldSubsystem::Deinitialize()
{
    if (Coordinator)
    {
        Coordinator->Shutdown();
    }
    GenerationApplied.Clear();
    Coordinator.Reset();
    Super::Deinitialize();
}

#pragma once

#include "CoreMinimal.h"
#include "SeedForgeAsync.h"
#include "Subsystems/WorldSubsystem.h"
#include "SeedForgeWorldSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(
    FSeedForgeGenerationApplied,
    const FSeedForgeAsyncCompletion&);

UCLASS()
class SEEDFORGERUNTIME_API USeedForgeWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    uint64 RequestGeneration(uint64 Seed, const FSeedForgeConfig& Config);
    void CancelGeneration();

    FSeedForgeGenerationApplied& OnGenerationApplied()
    {
        return GenerationApplied;
    }

    virtual void Deinitialize() override;

private:
    TUniquePtr<FSeedForgeAsyncCoordinator> Coordinator;
    FSeedForgeGenerationApplied GenerationApplied;
};

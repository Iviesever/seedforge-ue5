#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

struct SEEDFORGERUNTIME_API FSeedForgeAsyncCompletion
{
    uint64 RequestId = 0;
    uint32 WorkerThreadId = 0;
    FSeedForgeResult Result;
};

using FSeedForgeAsyncWork = TUniqueFunction<FSeedForgeResult()>;
using FSeedForgeAsyncApply = TUniqueFunction<void(FSeedForgeAsyncCompletion&&)>;

class SEEDFORGERUNTIME_API FSeedForgeAsyncCoordinator
{
public:
    FSeedForgeAsyncCoordinator();
    ~FSeedForgeAsyncCoordinator();

    FSeedForgeAsyncCoordinator(const FSeedForgeAsyncCoordinator&) = delete;
    FSeedForgeAsyncCoordinator& operator=(const FSeedForgeAsyncCoordinator&) = delete;

    uint64 Start(FSeedForgeAsyncWork&& Work, FSeedForgeAsyncApply&& Apply);
    void CancelActive();
    void Shutdown();

private:
    struct FState;

    uint64 NextRequestId = 0;
    TSharedRef<FState, ESPMode::ThreadSafe> State;
};

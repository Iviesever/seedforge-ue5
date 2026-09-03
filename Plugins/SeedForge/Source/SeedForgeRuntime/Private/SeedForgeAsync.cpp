#include "SeedForgeAsync.h"

#include "Async/Async.h"
#include "HAL/PlatformTLS.h"
#include "Tasks/Task.h"

struct FSeedForgeAsyncCoordinator::FState
{
    TAtomic<bool> bAlive{true};
    TAtomic<uint64> ActiveRequestId{0};
    TSharedPtr<TAtomic<bool>, ESPMode::ThreadSafe> ActiveCancellation;
};

FSeedForgeAsyncCoordinator::FSeedForgeAsyncCoordinator()
    : State(MakeShared<FState, ESPMode::ThreadSafe>())
{
}

FSeedForgeAsyncCoordinator::~FSeedForgeAsyncCoordinator()
{
    Shutdown();
}

uint64 FSeedForgeAsyncCoordinator::Start(
    FSeedForgeAsyncWork&& Work,
    FSeedForgeAsyncApply&& Apply)
{
    check(IsInGameThread());
    check(State->bAlive.Load());
    check(Work);
    check(Apply);

    if (State->ActiveCancellation)
    {
        State->ActiveCancellation->Store(true);
    }

    const uint64 RequestId = ++NextRequestId;
    State->ActiveRequestId.Store(RequestId);
    const TSharedRef<TAtomic<bool>, ESPMode::ThreadSafe> Cancellation =
        MakeShared<TAtomic<bool>, ESPMode::ThreadSafe>(false);
    State->ActiveCancellation = Cancellation;
    const TWeakPtr<FState, ESPMode::ThreadSafe> WeakState(State);

    UE::Tasks::Launch(
        UE_SOURCE_LOCATION,
        [WeakState,
         Cancellation,
         RequestId,
         Work = MoveTemp(Work),
         Apply = MoveTemp(Apply)]() mutable
        {
            if (Cancellation->Load())
            {
                return;
            }

            const uint32 WorkerThreadId = FPlatformTLS::GetCurrentThreadId();
            FSeedForgeResult Result = Work();
            if (Cancellation->Load())
            {
                return;
            }

            AsyncTask(
                ENamedThreads::GameThread,
                [WeakState,
                 Cancellation,
                 RequestId,
                 WorkerThreadId,
                 Result = MoveTemp(Result),
                 Apply = MoveTemp(Apply)]() mutable
                {
                    const TSharedPtr<FState, ESPMode::ThreadSafe> PinnedState = WeakState.Pin();
                    if (!PinnedState
                        || !PinnedState->bAlive.Load()
                        || Cancellation->Load()
                        || PinnedState->ActiveRequestId.Load() != RequestId)
                    {
                        return;
                    }

                    PinnedState->ActiveRequestId.Store(0);
                    PinnedState->ActiveCancellation.Reset();

                    FSeedForgeAsyncCompletion Completion;
                    Completion.RequestId = RequestId;
                    Completion.WorkerThreadId = WorkerThreadId;
                    Completion.Result = MoveTemp(Result);
                    Apply(MoveTemp(Completion));
                });
        });

    return RequestId;
}

void FSeedForgeAsyncCoordinator::CancelActive()
{
    if (State->ActiveCancellation)
    {
        State->ActiveCancellation->Store(true);
        State->ActiveCancellation.Reset();
    }
    State->ActiveRequestId.Store(0);
}

void FSeedForgeAsyncCoordinator::Shutdown()
{
    if (!State->bAlive.Exchange(false))
    {
        return;
    }
    CancelActive();
}

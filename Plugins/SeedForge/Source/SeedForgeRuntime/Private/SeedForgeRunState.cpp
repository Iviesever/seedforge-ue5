#include "SeedForgeRunState.h"

namespace
{
    FSeedForgeRunTransitionResult RedFailure()
    {
        return FSeedForgeRunTransitionResult::Failure(
            ESeedForgeRunTransitionError::InvalidTransition,
            TEXT("PACT-31 RED run-state stub."));
    }
}

ESeedForgeRunState FSeedForgeRunStateMachine::GetState() const
{
    return State;
}

int32 FSeedForgeRunStateMachine::GetCollectedCoreCount() const
{
    return CollectedCoreIds.Num();
}

int32 FSeedForgeRunStateMachine::GetRequiredCoreCount() const
{
    return RequiredCoreIds.Num();
}

bool FSeedForgeRunStateMachine::IsExitUnlocked() const
{
    return false;
}

const TArray<uint32>& FSeedForgeRunStateMachine::GetCollectedCoreIds() const
{
    return CollectedCoreIds;
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::StartPlaying(
    TArray<uint32> ExpectedCoreIds)
{
    return RedFailure();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::CollectCore(uint32 StableId)
{
    return RedFailure();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::ReachExit()
{
    return RedFailure();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::PlayerDied()
{
    return RedFailure();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::RequestRestart()
{
    return RedFailure();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::BeginGenerating()
{
    return RedFailure();
}

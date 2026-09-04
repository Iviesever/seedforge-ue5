#include "SeedForgeRunState.h"

namespace SeedForge::RunState::Private
{
    FSeedForgeRunTransitionResult Failure(
        ESeedForgeRunTransitionError ErrorCode,
        const TCHAR* Message)
    {
        return FSeedForgeRunTransitionResult::Failure(ErrorCode, Message);
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
    return !RequiredCoreIds.IsEmpty()
        && CollectedCoreIds.Num() == RequiredCoreIds.Num();
}

const TArray<uint32>& FSeedForgeRunStateMachine::GetCollectedCoreIds() const
{
    return CollectedCoreIds;
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::StartPlaying(
    TArray<uint32> ExpectedCoreIds)
{
    using namespace SeedForge::RunState::Private;

    if (State != ESeedForgeRunState::Generating)
    {
        return Failure(ESeedForgeRunTransitionError::InvalidTransition, TEXT("Only Generating may enter Playing."));
    }
    ExpectedCoreIds.Sort();
    if (ExpectedCoreIds.IsEmpty())
    {
        return Failure(ESeedForgeRunTransitionError::InvalidCoreSet, TEXT("At least one Data Core is required."));
    }
    for (int32 Index = 1; Index < ExpectedCoreIds.Num(); ++Index)
    {
        if (ExpectedCoreIds[Index - 1] == ExpectedCoreIds[Index])
        {
            return Failure(ESeedForgeRunTransitionError::InvalidCoreSet, TEXT("Required Data Core IDs must be unique."));
        }
    }

    RequiredCoreIds = MoveTemp(ExpectedCoreIds);
    CollectedCoreIds.Reset();
    State = ESeedForgeRunState::Playing;
    return FSeedForgeRunTransitionResult::Success();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::CollectCore(uint32 StableId)
{
    using namespace SeedForge::RunState::Private;

    if (State != ESeedForgeRunState::Playing)
    {
        return Failure(ESeedForgeRunTransitionError::InvalidTransition, TEXT("Data Cores can be collected only while Playing."));
    }
    if (!RequiredCoreIds.Contains(StableId))
    {
        return Failure(ESeedForgeRunTransitionError::UnknownCore, TEXT("Data Core ID is not part of this run."));
    }
    if (CollectedCoreIds.Contains(StableId))
    {
        return Failure(ESeedForgeRunTransitionError::DuplicateCore, TEXT("Data Core was already collected."));
    }

    CollectedCoreIds.Add(StableId);
    CollectedCoreIds.Sort();
    return FSeedForgeRunTransitionResult::Success();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::ReachExit()
{
    using namespace SeedForge::RunState::Private;

    if (State != ESeedForgeRunState::Playing)
    {
        return Failure(ESeedForgeRunTransitionError::InvalidTransition, TEXT("Exit can be reached only while Playing."));
    }
    if (!IsExitUnlocked())
    {
        return Failure(ESeedForgeRunTransitionError::ExitLocked, TEXT("Collect every Data Core before extraction."));
    }

    State = ESeedForgeRunState::Won;
    return FSeedForgeRunTransitionResult::Success();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::PlayerDied()
{
    using namespace SeedForge::RunState::Private;

    if (State != ESeedForgeRunState::Playing)
    {
        return Failure(ESeedForgeRunTransitionError::InvalidTransition, TEXT("Player death is valid only while Playing."));
    }
    State = ESeedForgeRunState::Lost;
    return FSeedForgeRunTransitionResult::Success();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::RequestRestart()
{
    using namespace SeedForge::RunState::Private;

    if (State != ESeedForgeRunState::Playing
        && State != ESeedForgeRunState::Won
        && State != ESeedForgeRunState::Lost
        && State != ESeedForgeRunState::Failed)
    {
        return Failure(ESeedForgeRunTransitionError::InvalidTransition, TEXT("Only an active or terminal run can restart."));
    }
    State = ESeedForgeRunState::Restarting;
    return FSeedForgeRunTransitionResult::Success();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::BeginGenerating()
{
    using namespace SeedForge::RunState::Private;

    if (State != ESeedForgeRunState::Restarting)
    {
        return Failure(ESeedForgeRunTransitionError::InvalidTransition, TEXT("Only Restarting may begin generation."));
    }
    RequiredCoreIds.Reset();
    CollectedCoreIds.Reset();
    State = ESeedForgeRunState::Generating;
    return FSeedForgeRunTransitionResult::Success();
}

FSeedForgeRunTransitionResult FSeedForgeRunStateMachine::FailRun()
{
    RequiredCoreIds.Reset();
    CollectedCoreIds.Reset();
    State = ESeedForgeRunState::Failed;
    return FSeedForgeRunTransitionResult::Success();
}

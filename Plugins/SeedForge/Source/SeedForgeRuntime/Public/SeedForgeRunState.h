#pragma once

#include "CoreMinimal.h"

enum class ESeedForgeRunState : uint8
{
    Generating,
    Playing,
    Won,
    Lost,
    Restarting,
    Failed
};

enum class ESeedForgeRunTransitionError : uint8
{
    None,
    InvalidTransition,
    InvalidCoreSet,
    UnknownCore,
    DuplicateCore,
    ExitLocked
};

struct SEEDFORGERUNTIME_API FSeedForgeRunTransitionResult
{
    ESeedForgeRunTransitionError ErrorCode = ESeedForgeRunTransitionError::None;
    FString ErrorMessage;

    bool IsSuccess() const
    {
        return ErrorCode == ESeedForgeRunTransitionError::None;
    }

    static FSeedForgeRunTransitionResult Success()
    {
        return {};
    }

    static FSeedForgeRunTransitionResult Failure(
        ESeedForgeRunTransitionError InCode,
        FString InMessage)
    {
        FSeedForgeRunTransitionResult Result;
        Result.ErrorCode = InCode;
        Result.ErrorMessage = MoveTemp(InMessage);
        return Result;
    }
};

class SEEDFORGERUNTIME_API FSeedForgeRunStateMachine
{
public:
    ESeedForgeRunState GetState() const;
    int32 GetCollectedCoreCount() const;
    int32 GetRequiredCoreCount() const;
    bool IsExitUnlocked() const;
    const TArray<uint32>& GetCollectedCoreIds() const;

    FSeedForgeRunTransitionResult StartPlaying(TArray<uint32> ExpectedCoreIds);
    FSeedForgeRunTransitionResult CollectCore(uint32 StableId);
    FSeedForgeRunTransitionResult ReachExit();
    FSeedForgeRunTransitionResult PlayerDied();
    FSeedForgeRunTransitionResult RequestRestart();
    FSeedForgeRunTransitionResult BeginGenerating();
    FSeedForgeRunTransitionResult FailRun();

private:
    ESeedForgeRunState State = ESeedForgeRunState::Generating;
    TArray<uint32> RequiredCoreIds;
    TArray<uint32> CollectedCoreIds;
};

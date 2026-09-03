#pragma once

#include "CoreMinimal.h"

enum class ESeedForgePathStatus : uint8
{
    Success,
    AlreadyAtGoal,
    Unreachable,
    InvalidInput,
    BudgetExceeded
};

struct SEEDFORGERUNTIME_API FSeedForgePathRequest
{
    FIntPoint Start = FIntPoint::ZeroValue;
    FIntPoint Goal = FIntPoint::ZeroValue;
    TArray<FIntPoint> WalkableCells;
    int32 MaxExpandedNodes = 2048;
};

struct SEEDFORGERUNTIME_API FSeedForgePathResult
{
    ESeedForgePathStatus Status = ESeedForgePathStatus::InvalidInput;
    TArray<FIntPoint> Path;
    int32 ExpandedNodes = 0;

    bool IsSuccess() const
    {
        return Status == ESeedForgePathStatus::Success
            || Status == ESeedForgePathStatus::AlreadyAtGoal;
    }
};

class SEEDFORGERUNTIME_API FSeedForgeGridPathfinder
{
public:
    static FSeedForgePathResult FindPath(const FSeedForgePathRequest& Request);
};

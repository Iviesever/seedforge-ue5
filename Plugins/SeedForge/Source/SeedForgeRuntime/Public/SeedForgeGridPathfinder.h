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
    // Limits non-goal nodes whose neighbors are enumerated. A discovered goal may
    // still be selected successfully after the last allowed expansion.
    int32 MaxExpandedNodes = 2048;
};

struct SEEDFORGERUNTIME_API FSeedForgePathResult
{
    ESeedForgePathStatus Status = ESeedForgePathStatus::InvalidInput;
    TArray<FIntPoint> Path;
    // Selecting the goal does not count as an expansion; failure has no partial path.
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

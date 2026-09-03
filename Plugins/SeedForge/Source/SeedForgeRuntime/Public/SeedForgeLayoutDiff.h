#pragma once

#include "CoreMinimal.h"
#include "SeedForgeLayoutCodec.h"

struct SEEDFORGERUNTIME_API FSeedForgeLayoutDifference
{
    uint64 LeftSeed = 0;
    uint64 RightSeed = 0;
    uint64 LeftHash = 0;
    uint64 RightHash = 0;
    bool bSeedChanged = false;
    bool bHashChanged = false;
    bool bConfigChanged = false;
    bool bEntranceChanged = false;
    bool bExitChanged = false;
    FIntPoint LeftEntrance = FIntPoint::ZeroValue;
    FIntPoint RightEntrance = FIntPoint::ZeroValue;
    FIntPoint LeftExit = FIntPoint::ZeroValue;
    FIntPoint RightExit = FIntPoint::ZeroValue;
    TArray<FSeedForgeRoom> AddedRooms;
    TArray<FSeedForgeRoom> RemovedRooms;
    TArray<FIntPoint> AddedWalkableCells;
    TArray<FIntPoint> RemovedWalkableCells;

    bool IsEmpty() const;
    FString ToHumanSummary() const;
};

class SEEDFORGERUNTIME_API FSeedForgeLayoutDiffer
{
public:
    static FSeedForgeLayoutDifference Compare(
        const FSeedForgeLayoutDocument& Left,
        const FSeedForgeLayoutDocument& Right);
    static FString ExportCanonicalJson(const FSeedForgeLayoutDifference& Difference);
};

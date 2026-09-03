#pragma once

#include "CoreMinimal.h"

enum class ESeedForgeErrorCode : uint8
{
    None,
    NotImplemented,
    InvalidGridSize,
    InvalidRoomCount,
    InvalidRoomSizeRange,
    InvalidPadding,
    InvalidAttemptBudget,
    PlacementExhausted,
    LayoutOutOfBounds,
    RoomsOverlap,
    InvalidEntrance,
    InvalidExit,
    DisconnectedLayout
};

struct SEEDFORGERUNTIME_API FSeedForgeConfig
{
    int32 GridWidth = 48;
    int32 GridHeight = 48;
    int32 RoomCount = 10;
    int32 MinRoomWidth = 4;
    int32 MaxRoomWidth = 9;
    int32 MinRoomHeight = 4;
    int32 MaxRoomHeight = 9;
    int32 RoomPadding = 1;
    int32 MaxPlacementAttempts = 512;

    bool operator==(const FSeedForgeConfig& Other) const = default;
};

struct SEEDFORGERUNTIME_API FSeedForgeRoom
{
    FIntPoint Min = FIntPoint::ZeroValue;
    FIntPoint Size = FIntPoint::ZeroValue;

    FIntPoint MaxExclusive() const
    {
        return Min + Size;
    }

    FIntPoint Center() const
    {
        return FIntPoint(Min.X + Size.X / 2, Min.Y + Size.Y / 2);
    }

    bool Contains(const FIntPoint& Cell) const
    {
        const FIntPoint Max = MaxExclusive();
        return Cell.X >= Min.X && Cell.Y >= Min.Y && Cell.X < Max.X && Cell.Y < Max.Y;
    }

    bool operator==(const FSeedForgeRoom& Other) const = default;
};

struct SEEDFORGERUNTIME_API FSeedForgeLayout
{
    uint64 Seed = 0;
    TArray<FSeedForgeRoom> Rooms;
    TArray<FIntPoint> CorridorCells;
    FIntPoint Entrance = FIntPoint::ZeroValue;
    FIntPoint Exit = FIntPoint::ZeroValue;
    uint64 CanonicalHash = 0;

    TArray<FIntPoint> GetCanonicalWalkableCells() const;

    bool operator==(const FSeedForgeLayout& Other) const
    {
        return Seed == Other.Seed
            && Rooms == Other.Rooms
            && CorridorCells == Other.CorridorCells
            && Entrance == Other.Entrance
            && Exit == Other.Exit
            && CanonicalHash == Other.CanonicalHash;
    }
};

struct SEEDFORGERUNTIME_API FSeedForgeResult
{
    ESeedForgeErrorCode ErrorCode = ESeedForgeErrorCode::None;
    FString ErrorMessage;
    FSeedForgeLayout Layout;

    bool IsSuccess() const
    {
        return ErrorCode == ESeedForgeErrorCode::None;
    }

    static FSeedForgeResult Success(FSeedForgeLayout&& InLayout)
    {
        FSeedForgeResult Result;
        Result.Layout = MoveTemp(InLayout);
        return Result;
    }

    static FSeedForgeResult Failure(ESeedForgeErrorCode InCode, FString InMessage)
    {
        FSeedForgeResult Result;
        Result.ErrorCode = InCode;
        Result.ErrorMessage = MoveTemp(InMessage);
        return Result;
    }
};

struct SEEDFORGERUNTIME_API FSeedForgeValidationResult
{
    ESeedForgeErrorCode ErrorCode = ESeedForgeErrorCode::None;
    FString ErrorMessage;

    bool IsValid() const
    {
        return ErrorCode == ESeedForgeErrorCode::None;
    }

    static FSeedForgeValidationResult Valid()
    {
        return {};
    }

    static FSeedForgeValidationResult Invalid(ESeedForgeErrorCode InCode, FString InMessage)
    {
        FSeedForgeValidationResult Result;
        Result.ErrorCode = InCode;
        Result.ErrorMessage = MoveTemp(InMessage);
        return Result;
    }
};


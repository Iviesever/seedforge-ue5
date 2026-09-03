#include "SeedForgeValidator.h"

namespace SeedForge::Private
{
    bool IsPointInBounds(const FIntPoint& Point, const FSeedForgeConfig& Config)
    {
        return Point.X >= 0
            && Point.Y >= 0
            && Point.X < Config.GridWidth
            && Point.Y < Config.GridHeight;
    }

    bool RoomsOverlap(const FSeedForgeRoom& Left, const FSeedForgeRoom& Right)
    {
        const FIntPoint LeftMax = Left.MaxExclusive();
        const FIntPoint RightMax = Right.MaxExclusive();
        return Left.Min.X < RightMax.X
            && LeftMax.X > Right.Min.X
            && Left.Min.Y < RightMax.Y
            && LeftMax.Y > Right.Min.Y;
    }
}

FSeedForgeValidationResult FSeedForgeValidator::Validate(
    const FSeedForgeLayout& Layout,
    const FSeedForgeConfig& Config)
{
    if (Layout.Rooms.Num() != Config.RoomCount)
    {
        return FSeedForgeValidationResult::Invalid(
            ESeedForgeErrorCode::InvalidRoomCount,
            FString::Printf(
                TEXT("Layout contains %d rooms but the configuration requires %d."),
                Layout.Rooms.Num(),
                Config.RoomCount));
    }

    for (int32 RoomIndex = 0; RoomIndex < Layout.Rooms.Num(); ++RoomIndex)
    {
        const FSeedForgeRoom& Room = Layout.Rooms[RoomIndex];
        const FIntPoint Max = Room.MaxExclusive();
        if (Room.Size.X <= 0
            || Room.Size.Y <= 0
            || Room.Min.X < 0
            || Room.Min.Y < 0
            || Max.X > Config.GridWidth
            || Max.Y > Config.GridHeight)
        {
            return FSeedForgeValidationResult::Invalid(
                ESeedForgeErrorCode::LayoutOutOfBounds,
                FString::Printf(
                    TEXT("Room %d is outside the %dx%d grid: min=(%d,%d), size=(%d,%d)."),
                    RoomIndex,
                    Config.GridWidth,
                    Config.GridHeight,
                    Room.Min.X,
                    Room.Min.Y,
                    Room.Size.X,
                    Room.Size.Y));
        }
    }

    for (int32 LeftIndex = 0; LeftIndex < Layout.Rooms.Num(); ++LeftIndex)
    {
        for (int32 RightIndex = LeftIndex + 1; RightIndex < Layout.Rooms.Num(); ++RightIndex)
        {
            if (SeedForge::Private::RoomsOverlap(Layout.Rooms[LeftIndex], Layout.Rooms[RightIndex]))
            {
                return FSeedForgeValidationResult::Invalid(
                    ESeedForgeErrorCode::RoomsOverlap,
                    FString::Printf(
                        TEXT("Rooms %d and %d overlap."),
                        LeftIndex,
                        RightIndex));
            }
        }
    }

    for (int32 CorridorIndex = 0; CorridorIndex < Layout.CorridorCells.Num(); ++CorridorIndex)
    {
        if (!SeedForge::Private::IsPointInBounds(Layout.CorridorCells[CorridorIndex], Config))
        {
            const FIntPoint Cell = Layout.CorridorCells[CorridorIndex];
            return FSeedForgeValidationResult::Invalid(
                ESeedForgeErrorCode::LayoutOutOfBounds,
                FString::Printf(
                    TEXT("Corridor cell %d is outside the grid: (%d,%d)."),
                    CorridorIndex,
                    Cell.X,
                    Cell.Y));
        }
    }

    const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
    if (!WalkableCells.Contains(Layout.Entrance))
    {
        return FSeedForgeValidationResult::Invalid(
            ESeedForgeErrorCode::InvalidEntrance,
            FString::Printf(
                TEXT("Entrance (%d,%d) is not walkable."),
                Layout.Entrance.X,
                Layout.Entrance.Y));
    }
    if (!WalkableCells.Contains(Layout.Exit))
    {
        return FSeedForgeValidationResult::Invalid(
            ESeedForgeErrorCode::InvalidExit,
            FString::Printf(
                TEXT("Exit (%d,%d) is not walkable."),
                Layout.Exit.X,
                Layout.Exit.Y));
    }

    TSet<FIntPoint> WalkableSet;
    WalkableSet.Reserve(WalkableCells.Num());
    for (const FIntPoint& Cell : WalkableCells)
    {
        WalkableSet.Add(Cell);
    }

    TSet<FIntPoint> Visited;
    Visited.Reserve(WalkableCells.Num());
    TArray<FIntPoint> Queue;
    Queue.Reserve(WalkableCells.Num());
    Queue.Add(Layout.Entrance);
    Visited.Add(Layout.Entrance);

    const FIntPoint Directions[] = {
        FIntPoint(1, 0),
        FIntPoint(-1, 0),
        FIntPoint(0, 1),
        FIntPoint(0, -1)};
    for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
    {
        const FIntPoint Current = Queue[QueueIndex];
        for (const FIntPoint& Direction : Directions)
        {
            const FIntPoint Neighbor = Current + Direction;
            if (WalkableSet.Contains(Neighbor) && !Visited.Contains(Neighbor))
            {
                Visited.Add(Neighbor);
                Queue.Add(Neighbor);
            }
        }
    }

    if (Visited.Num() != WalkableSet.Num())
    {
        return FSeedForgeValidationResult::Invalid(
            ESeedForgeErrorCode::DisconnectedLayout,
            FString::Printf(
                TEXT("Only %d of %d walkable cells are connected to the entrance."),
                Visited.Num(),
                WalkableSet.Num()));
    }

    return FSeedForgeValidationResult::Valid();
}

#include "SeedForgeGenerator.h"

namespace SeedForge::Private
{
    constexpr int32 MinGridDimension = 4;
    constexpr int32 MaxGridDimension = 2048;
    constexpr int32 MaxSupportedRoomCount = 4096;
    constexpr int32 MaxSupportedPadding = 64;
    constexpr int32 MaxSupportedAttemptBudget = 10'000'000;

    class FDeterministicRandom
    {
    public:
        explicit FDeterministicRandom(uint64 Seed)
            : State(Seed)
        {
        }

        uint64 Next()
        {
            State += 0x9E3779B97F4A7C15ULL;
            uint64 Value = State;
            Value = (Value ^ (Value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
            Value = (Value ^ (Value >> 27U)) * 0x94D049BB133111EBULL;
            return Value ^ (Value >> 31U);
        }

        int32 RangeInclusive(int32 Min, int32 Max)
        {
            check(Min <= Max);
            const uint64 Span = static_cast<uint64>(static_cast<int64>(Max) - Min + 1);
            return Min + static_cast<int32>(Next() % Span);
        }

        bool NextBool()
        {
            return (Next() & 1ULL) != 0;
        }

    private:
        uint64 State;
    };

    bool IsCanonicalPointLess(const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y != Right.Y ? Left.Y < Right.Y : Left.X < Right.X;
    }

    bool IsCanonicalRoomLess(const FSeedForgeRoom& Left, const FSeedForgeRoom& Right)
    {
        if (Left.Min != Right.Min)
        {
            return IsCanonicalPointLess(Left.Min, Right.Min);
        }
        if (Left.Size.Y != Right.Size.Y)
        {
            return Left.Size.Y < Right.Size.Y;
        }
        return Left.Size.X < Right.Size.X;
    }

    bool IntersectsWithPadding(
        const FSeedForgeRoom& Left,
        const FSeedForgeRoom& Right,
        int32 Padding)
    {
        const FIntPoint LeftMax = Left.MaxExclusive();
        const FIntPoint RightMax = Right.MaxExclusive();
        return Left.Min.X - Padding < RightMax.X
            && LeftMax.X + Padding > Right.Min.X
            && Left.Min.Y - Padding < RightMax.Y
            && LeftMax.Y + Padding > Right.Min.Y;
    }

    bool IsCellInsideAnyRoom(const FIntPoint& Cell, const TArray<FSeedForgeRoom>& Rooms)
    {
        for (const FSeedForgeRoom& Room : Rooms)
        {
            if (Room.Contains(Cell))
            {
                return true;
            }
        }
        return false;
    }

    void AddCorridorCell(
        TArray<FIntPoint>& CorridorCells,
        const TArray<FSeedForgeRoom>& Rooms,
        const FIntPoint& Cell)
    {
        if (!IsCellInsideAnyRoom(Cell, Rooms))
        {
            CorridorCells.AddUnique(Cell);
        }
    }

    void AppendHorizontalCorridor(
        TArray<FIntPoint>& CorridorCells,
        const TArray<FSeedForgeRoom>& Rooms,
        int32 StartX,
        int32 EndX,
        int32 Y)
    {
        const int32 Step = StartX <= EndX ? 1 : -1;
        for (int32 X = StartX;; X += Step)
        {
            AddCorridorCell(CorridorCells, Rooms, FIntPoint(X, Y));
            if (X == EndX)
            {
                break;
            }
        }
    }

    void AppendVerticalCorridor(
        TArray<FIntPoint>& CorridorCells,
        const TArray<FSeedForgeRoom>& Rooms,
        int32 X,
        int32 StartY,
        int32 EndY)
    {
        const int32 Step = StartY <= EndY ? 1 : -1;
        for (int32 Y = StartY;; Y += Step)
        {
            AddCorridorCell(CorridorCells, Rooms, FIntPoint(X, Y));
            if (Y == EndY)
            {
                break;
            }
        }
    }

    FSeedForgeResult ValidateGenerationConfig(const FSeedForgeConfig& Config)
    {
        if (Config.GridWidth < MinGridDimension
            || Config.GridHeight < MinGridDimension
            || Config.GridWidth > MaxGridDimension
            || Config.GridHeight > MaxGridDimension)
        {
            return FSeedForgeResult::Failure(
                ESeedForgeErrorCode::InvalidGridSize,
                FString::Printf(
                    TEXT("Grid dimensions must be in [%d, %d]; received %dx%d."),
                    MinGridDimension,
                    MaxGridDimension,
                    Config.GridWidth,
                    Config.GridHeight));
        }

        if (Config.RoomCount < 1 || Config.RoomCount > MaxSupportedRoomCount)
        {
            return FSeedForgeResult::Failure(
                ESeedForgeErrorCode::InvalidRoomCount,
                FString::Printf(
                    TEXT("RoomCount must be in [1, %d]; received %d."),
                    MaxSupportedRoomCount,
                    Config.RoomCount));
        }

        const bool bInvalidWidthRange = Config.MinRoomWidth < 2
            || Config.MaxRoomWidth < Config.MinRoomWidth
            || Config.MaxRoomWidth > Config.GridWidth;
        const bool bInvalidHeightRange = Config.MinRoomHeight < 2
            || Config.MaxRoomHeight < Config.MinRoomHeight
            || Config.MaxRoomHeight > Config.GridHeight;
        if (bInvalidWidthRange || bInvalidHeightRange)
        {
            return FSeedForgeResult::Failure(
                ESeedForgeErrorCode::InvalidRoomSizeRange,
                FString::Printf(
                    TEXT("Room ranges must be ordered, at least 2, and fit the grid; received width %d..%d and height %d..%d."),
                    Config.MinRoomWidth,
                    Config.MaxRoomWidth,
                    Config.MinRoomHeight,
                    Config.MaxRoomHeight));
        }

        if (Config.RoomPadding < 0 || Config.RoomPadding > MaxSupportedPadding)
        {
            return FSeedForgeResult::Failure(
                ESeedForgeErrorCode::InvalidPadding,
                FString::Printf(
                    TEXT("RoomPadding must be in [0, %d]; received %d."),
                    MaxSupportedPadding,
                    Config.RoomPadding));
        }

        if (Config.MaxPlacementAttempts < Config.RoomCount
            || Config.MaxPlacementAttempts > MaxSupportedAttemptBudget)
        {
            return FSeedForgeResult::Failure(
                ESeedForgeErrorCode::InvalidAttemptBudget,
                FString::Printf(
                    TEXT("MaxPlacementAttempts must be at least RoomCount and at most %d; received %d."),
                    MaxSupportedAttemptBudget,
                    Config.MaxPlacementAttempts));
        }

        return FSeedForgeResult::Success({});
    }

    void HashByte(uint64& Hash, uint8 Byte)
    {
        Hash ^= Byte;
        Hash *= 1099511628211ULL;
    }

    void HashUInt32(uint64& Hash, uint32 Value)
    {
        for (uint32 Shift = 0; Shift < 32; Shift += 8)
        {
            HashByte(Hash, static_cast<uint8>((Value >> Shift) & 0xFFU));
        }
    }

    void HashUInt64(uint64& Hash, uint64 Value)
    {
        for (uint32 Shift = 0; Shift < 64; Shift += 8)
        {
            HashByte(Hash, static_cast<uint8>((Value >> Shift) & 0xFFULL));
        }
    }

    void HashPoint(uint64& Hash, const FIntPoint& Point)
    {
        HashUInt32(Hash, static_cast<uint32>(Point.X));
        HashUInt32(Hash, static_cast<uint32>(Point.Y));
    }
}

FSeedForgeResult FSeedForgeGenerator::ValidateConfig(const FSeedForgeConfig& Config)
{
    return SeedForge::Private::ValidateGenerationConfig(Config);
}

TArray<FIntPoint> FSeedForgeLayout::GetCanonicalWalkableCells() const
{
    TArray<FIntPoint> Cells;
    for (const FSeedForgeRoom& Room : Rooms)
    {
        const FIntPoint Max = Room.MaxExclusive();
        for (int32 Y = Room.Min.Y; Y < Max.Y; ++Y)
        {
            for (int32 X = Room.Min.X; X < Max.X; ++X)
            {
                Cells.Emplace(X, Y);
            }
        }
    }
    Cells.Append(CorridorCells);
    Cells.Sort(SeedForge::Private::IsCanonicalPointLess);

    int32 WriteIndex = 0;
    for (const FIntPoint& Cell : Cells)
    {
        if (WriteIndex == 0 || Cells[WriteIndex - 1] != Cell)
        {
            Cells[WriteIndex++] = Cell;
        }
    }
    Cells.SetNum(WriteIndex, EAllowShrinking::No);
    return Cells;
}

FSeedForgeResult FSeedForgeGenerator::Generate(uint64 Seed, const FSeedForgeConfig& Config)
{
    const FSeedForgeResult ConfigResult = ValidateConfig(Config);
    if (!ConfigResult.IsSuccess())
    {
        return ConfigResult;
    }

    SeedForge::Private::FDeterministicRandom Random(Seed);
    FSeedForgeLayout Layout;
    Layout.Seed = Seed;
    Layout.Rooms.Reserve(Config.RoomCount);

    for (int32 Attempt = 0;
         Attempt < Config.MaxPlacementAttempts && Layout.Rooms.Num() < Config.RoomCount;
         ++Attempt)
    {
        FSeedForgeRoom Candidate;
        Candidate.Size.X = Random.RangeInclusive(Config.MinRoomWidth, Config.MaxRoomWidth);
        Candidate.Size.Y = Random.RangeInclusive(Config.MinRoomHeight, Config.MaxRoomHeight);
        Candidate.Min.X = Random.RangeInclusive(0, Config.GridWidth - Candidate.Size.X);
        Candidate.Min.Y = Random.RangeInclusive(0, Config.GridHeight - Candidate.Size.Y);

        bool bOverlaps = false;
        for (const FSeedForgeRoom& Existing : Layout.Rooms)
        {
            if (SeedForge::Private::IntersectsWithPadding(Candidate, Existing, Config.RoomPadding))
            {
                bOverlaps = true;
                break;
            }
        }

        if (!bOverlaps)
        {
            Layout.Rooms.Add(Candidate);
        }
    }

    if (Layout.Rooms.Num() != Config.RoomCount)
    {
        return FSeedForgeResult::Failure(
            ESeedForgeErrorCode::PlacementExhausted,
            FString::Printf(
                TEXT("Placed %d of %d rooms within %d attempts for seed %llu."),
                Layout.Rooms.Num(),
                Config.RoomCount,
                Config.MaxPlacementAttempts,
                Seed));
    }

    Layout.Rooms.Sort(SeedForge::Private::IsCanonicalRoomLess);
    for (int32 Index = 1; Index < Layout.Rooms.Num(); ++Index)
    {
        const FIntPoint Start = Layout.Rooms[Index - 1].Center();
        const FIntPoint End = Layout.Rooms[Index].Center();
        if (Random.NextBool())
        {
            SeedForge::Private::AppendHorizontalCorridor(
                Layout.CorridorCells, Layout.Rooms, Start.X, End.X, Start.Y);
            SeedForge::Private::AppendVerticalCorridor(
                Layout.CorridorCells, Layout.Rooms, End.X, Start.Y, End.Y);
        }
        else
        {
            SeedForge::Private::AppendVerticalCorridor(
                Layout.CorridorCells, Layout.Rooms, Start.X, Start.Y, End.Y);
            SeedForge::Private::AppendHorizontalCorridor(
                Layout.CorridorCells, Layout.Rooms, Start.X, End.X, End.Y);
        }
    }
    Layout.CorridorCells.Sort(SeedForge::Private::IsCanonicalPointLess);

    Layout.Entrance = Layout.Rooms[0].Center();
    Layout.Exit = Layout.Entrance;
    int32 GreatestDistance = -1;
    for (const FSeedForgeRoom& Room : Layout.Rooms)
    {
        const FIntPoint Center = Room.Center();
        const int32 Distance = FMath::Abs(Center.X - Layout.Entrance.X)
            + FMath::Abs(Center.Y - Layout.Entrance.Y);
        if (Distance > GreatestDistance)
        {
            GreatestDistance = Distance;
            Layout.Exit = Center;
        }
    }

    Layout.CanonicalHash = ComputeCanonicalHash(Layout);
    return FSeedForgeResult::Success(MoveTemp(Layout));
}

uint64 FSeedForgeGenerator::ComputeCanonicalHash(const FSeedForgeLayout& Layout)
{
    uint64 Hash = 14695981039346656037ULL;
    SeedForge::Private::HashUInt32(Hash, 0x53464731U);
    SeedForge::Private::HashUInt64(Hash, Layout.Seed);
    SeedForge::Private::HashUInt32(Hash, static_cast<uint32>(Layout.Rooms.Num()));
    for (const FSeedForgeRoom& Room : Layout.Rooms)
    {
        SeedForge::Private::HashPoint(Hash, Room.Min);
        SeedForge::Private::HashPoint(Hash, Room.Size);
    }
    SeedForge::Private::HashUInt32(Hash, static_cast<uint32>(Layout.CorridorCells.Num()));
    for (const FIntPoint& Cell : Layout.CorridorCells)
    {
        SeedForge::Private::HashPoint(Hash, Cell);
    }
    SeedForge::Private::HashPoint(Hash, Layout.Entrance);
    SeedForge::Private::HashPoint(Hash, Layout.Exit);
    return Hash;
}

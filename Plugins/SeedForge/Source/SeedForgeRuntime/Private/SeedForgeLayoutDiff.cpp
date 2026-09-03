#include "SeedForgeLayoutDiff.h"

namespace SeedForge::Diff::Private
{
    bool IsPointLess(const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y != Right.Y ? Left.Y < Right.Y : Left.X < Right.X;
    }

    bool IsRoomLess(const FSeedForgeRoom& Left, const FSeedForgeRoom& Right)
    {
        if (Left.Min != Right.Min)
        {
            return IsPointLess(Left.Min, Right.Min);
        }
        if (Left.Size.Y != Right.Size.Y)
        {
            return Left.Size.Y < Right.Size.Y;
        }
        return Left.Size.X < Right.Size.X;
    }

    template<typename ItemType, typename LessType>
    void CompareCollections(
        TArray<ItemType> Left,
        TArray<ItemType> Right,
        TArray<ItemType>& Added,
        TArray<ItemType>& Removed,
        LessType Less)
    {
        Left.Sort(Less);
        Right.Sort(Less);
        int32 LeftIndex = 0;
        int32 RightIndex = 0;
        while (LeftIndex < Left.Num() && RightIndex < Right.Num())
        {
            if (Left[LeftIndex] == Right[RightIndex])
            {
                ++LeftIndex;
                ++RightIndex;
            }
            else if (Less(Left[LeftIndex], Right[RightIndex]))
            {
                Removed.Add(Left[LeftIndex++]);
            }
            else
            {
                Added.Add(Right[RightIndex++]);
            }
        }
        while (LeftIndex < Left.Num())
        {
            Removed.Add(Left[LeftIndex++]);
        }
        while (RightIndex < Right.Num())
        {
            Added.Add(Right[RightIndex++]);
        }
    }

    const TCHAR* BoolText(bool bValue)
    {
        return bValue ? TEXT("true") : TEXT("false");
    }

    void AppendPoint(FString& Json, const FIntPoint& Point)
    {
        Json += FString::Printf(TEXT("[%d,%d]"), Point.X, Point.Y);
    }

    void AppendRooms(FString& Json, const TArray<FSeedForgeRoom>& Rooms)
    {
        Json += TEXT("[");
        for (int32 Index = 0; Index < Rooms.Num(); ++Index)
        {
            if (Index > 0)
            {
                Json += TEXT(",");
            }
            Json += TEXT("{\"min\":");
            AppendPoint(Json, Rooms[Index].Min);
            Json += TEXT(",\"size\":");
            AppendPoint(Json, Rooms[Index].Size);
            Json += TEXT("}");
        }
        Json += TEXT("]");
    }

    void AppendPoints(FString& Json, const TArray<FIntPoint>& Points)
    {
        Json += TEXT("[");
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            if (Index > 0)
            {
                Json += TEXT(",");
            }
            AppendPoint(Json, Points[Index]);
        }
        Json += TEXT("]");
    }
}

bool FSeedForgeLayoutDifference::IsEmpty() const
{
    return !bSeedChanged
        && !bHashChanged
        && !bConfigChanged
        && !bEntranceChanged
        && !bExitChanged
        && AddedRooms.IsEmpty()
        && RemovedRooms.IsEmpty()
        && AddedWalkableCells.IsEmpty()
        && RemovedWalkableCells.IsEmpty();
}

FString FSeedForgeLayoutDifference::ToHumanSummary() const
{
    return FString::Printf(
        TEXT("SeedForge diff: seeds %llu -> %llu; hashes %llu -> %llu; ")
        TEXT("configChanged=%s; entranceChanged=%s; exitChanged=%s; ")
        TEXT("rooms +%d/-%d; walkableCells +%d/-%d"),
        LeftSeed,
        RightSeed,
        LeftHash,
        RightHash,
        SeedForge::Diff::Private::BoolText(bConfigChanged),
        SeedForge::Diff::Private::BoolText(bEntranceChanged),
        SeedForge::Diff::Private::BoolText(bExitChanged),
        AddedRooms.Num(),
        RemovedRooms.Num(),
        AddedWalkableCells.Num(),
        RemovedWalkableCells.Num());
}

FSeedForgeLayoutDifference FSeedForgeLayoutDiffer::Compare(
    const FSeedForgeLayoutDocument& Left,
    const FSeedForgeLayoutDocument& Right)
{
    FSeedForgeLayoutDifference Difference;
    Difference.LeftSeed = Left.Layout.Seed;
    Difference.RightSeed = Right.Layout.Seed;
    Difference.LeftHash = Left.Layout.CanonicalHash;
    Difference.RightHash = Right.Layout.CanonicalHash;
    Difference.bSeedChanged = Difference.LeftSeed != Difference.RightSeed;
    Difference.bHashChanged = Difference.LeftHash != Difference.RightHash;
    Difference.bConfigChanged = !(Left.Config == Right.Config);
    Difference.LeftEntrance = Left.Layout.Entrance;
    Difference.RightEntrance = Right.Layout.Entrance;
    Difference.LeftExit = Left.Layout.Exit;
    Difference.RightExit = Right.Layout.Exit;
    Difference.bEntranceChanged = Difference.LeftEntrance != Difference.RightEntrance;
    Difference.bExitChanged = Difference.LeftExit != Difference.RightExit;

    SeedForge::Diff::Private::CompareCollections(
        Left.Layout.Rooms,
        Right.Layout.Rooms,
        Difference.AddedRooms,
        Difference.RemovedRooms,
        SeedForge::Diff::Private::IsRoomLess);
    SeedForge::Diff::Private::CompareCollections(
        Left.Layout.GetCanonicalWalkableCells(),
        Right.Layout.GetCanonicalWalkableCells(),
        Difference.AddedWalkableCells,
        Difference.RemovedWalkableCells,
        SeedForge::Diff::Private::IsPointLess);
    return Difference;
}

FString FSeedForgeLayoutDiffer::ExportCanonicalJson(const FSeedForgeLayoutDifference& Difference)
{
    using namespace SeedForge::Diff::Private;

    FString Json;
    Json.Reserve(
        768
        + (Difference.AddedRooms.Num() + Difference.RemovedRooms.Num()) * 48
        + (Difference.AddedWalkableCells.Num() + Difference.RemovedWalkableCells.Num()) * 16);
    Json += FString::Printf(
        TEXT("{\"schema\":\"seedforge.layout-diff\",\"schemaVersion\":1,")
        TEXT("\"leftSeed\":\"%llu\",\"rightSeed\":\"%llu\",")
        TEXT("\"leftHash\":\"%llu\",\"rightHash\":\"%llu\",")
        TEXT("\"seedChanged\":%s,\"hashChanged\":%s,\"configChanged\":%s,")
        TEXT("\"entranceChanged\":%s,\"exitChanged\":%s,"),
        Difference.LeftSeed,
        Difference.RightSeed,
        Difference.LeftHash,
        Difference.RightHash,
        BoolText(Difference.bSeedChanged),
        BoolText(Difference.bHashChanged),
        BoolText(Difference.bConfigChanged),
        BoolText(Difference.bEntranceChanged),
        BoolText(Difference.bExitChanged));
    Json += TEXT("\"leftEntrance\":");
    AppendPoint(Json, Difference.LeftEntrance);
    Json += TEXT(",\"rightEntrance\":");
    AppendPoint(Json, Difference.RightEntrance);
    Json += TEXT(",\"leftExit\":");
    AppendPoint(Json, Difference.LeftExit);
    Json += TEXT(",\"rightExit\":");
    AppendPoint(Json, Difference.RightExit);
    Json += FString::Printf(
        TEXT(",\"addedRoomCount\":%d,\"removedRoomCount\":%d,")
        TEXT("\"addedWalkableCellCount\":%d,\"removedWalkableCellCount\":%d,"),
        Difference.AddedRooms.Num(),
        Difference.RemovedRooms.Num(),
        Difference.AddedWalkableCells.Num(),
        Difference.RemovedWalkableCells.Num());
    Json += TEXT("\"addedRooms\":");
    AppendRooms(Json, Difference.AddedRooms);
    Json += TEXT(",\"removedRooms\":");
    AppendRooms(Json, Difference.RemovedRooms);
    Json += TEXT(",\"addedWalkableCells\":");
    AppendPoints(Json, Difference.AddedWalkableCells);
    Json += TEXT(",\"removedWalkableCells\":");
    AppendPoints(Json, Difference.RemovedWalkableCells);
    Json += TEXT("}");
    return Json;
}

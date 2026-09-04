#include "SeedForgeGridPathfinder.h"

#include "Algo/Reverse.h"

namespace SeedForge::Path::Private
{
    struct FSearchNode
    {
        FIntPoint Cell = FIntPoint::ZeroValue;
        int64 CostFromStart = MAX_int64;
        int64 Heuristic = MAX_int64;
        int32 ParentIndex = INDEX_NONE;
        bool bClosed = false;
        bool bOpen = false;
    };

    int64 ManhattanDistance(const FIntPoint& Left, const FIntPoint& Right)
    {
        return FMath::Abs(static_cast<int64>(Left.X) - Right.X)
            + FMath::Abs(static_cast<int64>(Left.Y) - Right.Y);
    }

    bool IsPreferred(const FSearchNode& Left, const FSearchNode& Right)
    {
        const int64 LeftScore = Left.CostFromStart + Left.Heuristic;
        const int64 RightScore = Right.CostFromStart + Right.Heuristic;
        if (LeftScore != RightScore)
        {
            return LeftScore < RightScore;
        }
        if (Left.Heuristic != Right.Heuristic)
        {
            return Left.Heuristic < Right.Heuristic;
        }
        if (Left.Cell.Y != Right.Cell.Y)
        {
            return Left.Cell.Y < Right.Cell.Y;
        }
        return Left.Cell.X < Right.Cell.X;
    }
}

FSeedForgePathResult FSeedForgeGridPathfinder::FindPath(const FSeedForgePathRequest& Request)
{
    using namespace SeedForge::Path::Private;

    FSeedForgePathResult Result;
    Result.Status = ESeedForgePathStatus::InvalidInput;
    if (Request.MaxExpandedNodes <= 0 || Request.WalkableCells.IsEmpty())
    {
        return Result;
    }

    TSet<FIntPoint> Walkable;
    Walkable.Reserve(Request.WalkableCells.Num());
    for (const FIntPoint& Cell : Request.WalkableCells)
    {
        Walkable.Add(Cell);
    }
    if (!Walkable.Contains(Request.Start) || !Walkable.Contains(Request.Goal))
    {
        return Result;
    }
    if (Request.Start == Request.Goal)
    {
        Result.Status = ESeedForgePathStatus::AlreadyAtGoal;
        Result.Path = {Request.Start};
        return Result;
    }

    TArray<FSearchNode> Nodes;
    Nodes.Reserve(FMath::Min(Walkable.Num(), Request.MaxExpandedNodes));
    TMap<FIntPoint, int32> NodeIndices;
    NodeIndices.Reserve(Walkable.Num());
    TArray<int32> OpenIndices;
    OpenIndices.Reserve(FMath::Min(Walkable.Num(), Request.MaxExpandedNodes));

    FSearchNode StartNode;
    StartNode.Cell = Request.Start;
    StartNode.CostFromStart = 0;
    StartNode.Heuristic = ManhattanDistance(Request.Start, Request.Goal);
    StartNode.bOpen = true;
    Nodes.Add(StartNode);
    NodeIndices.Add(Request.Start, 0);
    OpenIndices.Add(0);

    const FIntPoint Directions[] = {
        FIntPoint(1, 0),
        FIntPoint(0, 1),
        FIntPoint(-1, 0),
        FIntPoint(0, -1)};

    while (!OpenIndices.IsEmpty())
    {
        int32 BestOpenPosition = 0;
        for (int32 Position = 1; Position < OpenIndices.Num(); ++Position)
        {
            if (IsPreferred(Nodes[OpenIndices[Position]], Nodes[OpenIndices[BestOpenPosition]]))
            {
                BestOpenPosition = Position;
            }
        }
        const int32 CurrentIndex = OpenIndices[BestOpenPosition];
        OpenIndices.RemoveAt(BestOpenPosition, 1, EAllowShrinking::No);
        FSearchNode& Current = Nodes[CurrentIndex];
        Current.bOpen = false;

        if (Current.Cell == Request.Goal)
        {
            Result.Status = ESeedForgePathStatus::Success;
            for (int32 PathIndex = CurrentIndex;
                 PathIndex != INDEX_NONE;
                 PathIndex = Nodes[PathIndex].ParentIndex)
            {
                Result.Path.Add(Nodes[PathIndex].Cell);
            }
            Algo::Reverse(Result.Path);
            return Result;
        }
        if (Result.ExpandedNodes >= Request.MaxExpandedNodes)
        {
            Result.Status = ESeedForgePathStatus::BudgetExceeded;
            Result.Path.Reset();
            return Result;
        }

        Current.bClosed = true;
        ++Result.ExpandedNodes;
        const FIntPoint CurrentCell = Current.Cell;
        const int64 NextCost = Current.CostFromStart + 1;
        for (const FIntPoint& Direction : Directions)
        {
            const int64 NeighborX = static_cast<int64>(CurrentCell.X) + Direction.X;
            const int64 NeighborY = static_cast<int64>(CurrentCell.Y) + Direction.Y;
            if (NeighborX < MIN_int32 || NeighborX > MAX_int32
                || NeighborY < MIN_int32 || NeighborY > MAX_int32)
            {
                continue;
            }
            const FIntPoint NeighborCell(static_cast<int32>(NeighborX), static_cast<int32>(NeighborY));
            if (!Walkable.Contains(NeighborCell))
            {
                continue;
            }

            int32 NeighborIndex = INDEX_NONE;
            if (const int32* ExistingIndex = NodeIndices.Find(NeighborCell))
            {
                NeighborIndex = *ExistingIndex;
            }
            else
            {
                FSearchNode Neighbor;
                Neighbor.Cell = NeighborCell;
                Neighbor.Heuristic = ManhattanDistance(NeighborCell, Request.Goal);
                NeighborIndex = Nodes.Add(Neighbor);
                NodeIndices.Add(NeighborCell, NeighborIndex);
            }

            FSearchNode& Neighbor = Nodes[NeighborIndex];
            if (NextCost >= Neighbor.CostFromStart)
            {
                continue;
            }
            Neighbor.CostFromStart = NextCost;
            Neighbor.ParentIndex = CurrentIndex;
            Neighbor.bClosed = false;
            if (!Neighbor.bOpen)
            {
                Neighbor.bOpen = true;
                OpenIndices.Add(NeighborIndex);
            }
        }
    }

    Result.Status = ESeedForgePathStatus::Unreachable;
    Result.Path.Reset();
    return Result;
}

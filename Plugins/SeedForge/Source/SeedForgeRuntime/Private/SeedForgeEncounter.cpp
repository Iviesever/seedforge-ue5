#include "SeedForgeEncounter.h"

#include "SeedForgeGenerator.h"

namespace SeedForge::Encounter::Private
{
    constexpr uint32 CurrentEncounterVersion = 1;
    constexpr int32 MaxEntityCountPerRole = 1024;
    constexpr int32 MaxSafetyDistance = 4096;

    bool IsPointLess(const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y != Right.Y ? Left.Y < Right.Y : Left.X < Right.X;
    }

    uint64 Mix(uint64 Value)
    {
        Value += 0x9E3779B97F4A7C15ULL;
        Value = (Value ^ (Value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
        Value = (Value ^ (Value >> 27U)) * 0x94D049BB133111EBULL;
        return Value ^ (Value >> 31U);
    }

    uint64 ScoreCell(
        const FSeedForgeLayout& Layout,
        const FIntPoint& Cell,
        uint64 RoleSalt)
    {
        const uint64 PackedPoint = static_cast<uint64>(static_cast<uint32>(Cell.X))
            | (static_cast<uint64>(static_cast<uint32>(Cell.Y)) << 32U);
        return Mix(Layout.Seed ^ Mix(Layout.CanonicalHash) ^ RoleSalt ^ Mix(PackedPoint));
    }

    struct FCandidate
    {
        FIntPoint Cell = FIntPoint::ZeroValue;
        uint64 Score = 0;
    };

    TArray<FCandidate> RankCandidates(
        const FSeedForgeLayout& Layout,
        const TArray<FIntPoint>& WalkableCells,
        uint64 RoleSalt)
    {
        TArray<FCandidate> Candidates;
        Candidates.Reserve(WalkableCells.Num());
        for (const FIntPoint& Cell : WalkableCells)
        {
            if (Cell != Layout.Entrance && Cell != Layout.Exit)
            {
                Candidates.Add({Cell, ScoreCell(Layout, Cell, RoleSalt)});
            }
        }
        Candidates.Sort([](const FCandidate& Left, const FCandidate& Right)
        {
            if (Left.Score != Right.Score)
            {
                return Left.Score > Right.Score;
            }
            return IsPointLess(Left.Cell, Right.Cell);
        });
        return Candidates;
    }

    FSeedForgeEncounterResult ValidateInputs(
        const FSeedForgeLayout& Layout,
        const FSeedForgeEncounterConfig& Config)
    {
        if (Config.Version != CurrentEncounterVersion
            || Config.DataCoreCount < 1
            || Config.DataCoreCount > MaxEntityCountPerRole
            || Config.EnemyCount < 1
            || Config.EnemyCount > MaxEntityCountPerRole
            || Config.MinPlayerEnemyDistance < 0
            || Config.MinPlayerEnemyDistance > MaxSafetyDistance)
        {
            return FSeedForgeEncounterResult::Failure(
                ESeedForgeEncounterErrorCode::InvalidConfig,
                FString::Printf(
                    TEXT("Encounter config requires version %u, 1..%d Cores/enemies, and safety distance 0..%d."),
                    CurrentEncounterVersion,
                    MaxEntityCountPerRole,
                    MaxSafetyDistance));
        }

        const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
        if (Layout.CanonicalHash == 0
            || Layout.CanonicalHash != FSeedForgeGenerator::ComputeCanonicalHash(Layout)
            || WalkableCells.IsEmpty()
            || !WalkableCells.Contains(Layout.Entrance)
            || !WalkableCells.Contains(Layout.Exit)
            || Layout.Entrance == Layout.Exit)
        {
            return FSeedForgeEncounterResult::Failure(
                ESeedForgeEncounterErrorCode::InvalidLayout,
                TEXT("Encounter planning requires an intact layout hash and distinct walkable entrance/exit cells."));
        }

        const int64 RequiredCells = 2LL + Config.DataCoreCount + Config.EnemyCount;
        if (RequiredCells > WalkableCells.Num())
        {
            return FSeedForgeEncounterResult::Failure(
                ESeedForgeEncounterErrorCode::InsufficientWalkableCells,
                FString::Printf(
                    TEXT("Encounter requires %lld distinct cells but layout has %d."),
                    RequiredCells,
                    WalkableCells.Num()));
        }
        return FSeedForgeEncounterResult::Success({});
    }

    int64 ManhattanDistance(const FIntPoint& Left, const FIntPoint& Right)
    {
        return FMath::Abs(static_cast<int64>(Left.X) - Right.X)
            + FMath::Abs(static_cast<int64>(Left.Y) - Right.Y);
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

    void HashEntity(uint64& Hash, const FSeedForgeEncounterEntity& Entity)
    {
        HashByte(Hash, static_cast<uint8>(Entity.Role));
        HashUInt32(Hash, Entity.StableId);
        HashUInt32(Hash, static_cast<uint32>(Entity.Cell.X));
        HashUInt32(Hash, static_cast<uint32>(Entity.Cell.Y));
    }

    FSeedForgeEncounterResult InvalidPlan(FString Message)
    {
        return FSeedForgeEncounterResult::Failure(
            ESeedForgeEncounterErrorCode::InvalidPlan,
            MoveTemp(Message));
    }
}

FSeedForgeEncounterResult FSeedForgeEncounterPlanner::Generate(
    const FSeedForgeLayout& Layout,
    const FSeedForgeEncounterConfig& Config)
{
    using namespace SeedForge::Encounter::Private;

    const FSeedForgeEncounterResult InputResult = ValidateInputs(Layout, Config);
    if (!InputResult.IsSuccess())
    {
        return InputResult;
    }

    const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
    TSet<FIntPoint> Occupied;
    Occupied.Reserve(2 + Config.DataCoreCount + Config.EnemyCount);

    FSeedForgeEncounterPlan Plan;
    Plan.Version = Config.Version;
    Plan.Seed = Layout.Seed;
    Plan.SourceLayoutHash = Layout.CanonicalHash;
    Plan.Player = {ESeedForgeEncounterRole::Player, 0, Layout.Entrance};
    Plan.Exit = {ESeedForgeEncounterRole::Exit, 0, Layout.Exit};
    Occupied.Add(Plan.Player.Cell);
    Occupied.Add(Plan.Exit.Cell);

    const TArray<FCandidate> CoreCandidates = RankCandidates(
        Layout,
        WalkableCells,
        0x434F52455F5631ULL);
    for (const FCandidate& Candidate : CoreCandidates)
    {
        if (!Occupied.Contains(Candidate.Cell))
        {
            const uint32 StableId = static_cast<uint32>(Plan.DataCores.Num());
            Plan.DataCores.Add({ESeedForgeEncounterRole::DataCore, StableId, Candidate.Cell});
            Occupied.Add(Candidate.Cell);
            if (Plan.DataCores.Num() == Config.DataCoreCount)
            {
                break;
            }
        }
    }
    if (Plan.DataCores.Num() != Config.DataCoreCount)
    {
        return FSeedForgeEncounterResult::Failure(
            ESeedForgeEncounterErrorCode::InsufficientWalkableCells,
            TEXT("No deterministic non-overlapping Data Core placement exists."));
    }

    const TArray<FCandidate> EnemyCandidates = RankCandidates(
        Layout,
        WalkableCells,
        0x454E454D595F5631ULL);
    for (const FCandidate& Candidate : EnemyCandidates)
    {
        if (!Occupied.Contains(Candidate.Cell)
            && ManhattanDistance(Candidate.Cell, Plan.Player.Cell)
                >= Config.MinPlayerEnemyDistance)
        {
            const uint32 StableId = static_cast<uint32>(Plan.Enemies.Num());
            Plan.Enemies.Add({ESeedForgeEncounterRole::Enemy, StableId, Candidate.Cell});
            Occupied.Add(Candidate.Cell);
            if (Plan.Enemies.Num() == Config.EnemyCount)
            {
                break;
            }
        }
    }
    if (Plan.Enemies.Num() != Config.EnemyCount)
    {
        return FSeedForgeEncounterResult::Failure(
            ESeedForgeEncounterErrorCode::EnemySafetyRadiusUnsatisfied,
            FString::Printf(
                TEXT("Placed %d of %d enemies at Manhattan distance >= %d."),
                Plan.Enemies.Num(),
                Config.EnemyCount,
                Config.MinPlayerEnemyDistance));
    }

    Plan.CanonicalHash = ComputeCanonicalHash(Plan, Config);
    return Validate(Plan, Layout, Config);
}

FSeedForgeEncounterResult FSeedForgeEncounterPlanner::Validate(
    const FSeedForgeEncounterPlan& Plan,
    const FSeedForgeLayout& Layout,
    const FSeedForgeEncounterConfig& Config)
{
    using namespace SeedForge::Encounter::Private;

    const FSeedForgeEncounterResult InputResult = ValidateInputs(Layout, Config);
    if (!InputResult.IsSuccess())
    {
        return InputResult;
    }
    if (Plan.Version != Config.Version
        || Plan.Seed != Layout.Seed
        || Plan.SourceLayoutHash != Layout.CanonicalHash
        || Plan.DataCores.Num() != Config.DataCoreCount
        || Plan.Enemies.Num() != Config.EnemyCount)
    {
        return InvalidPlan(TEXT("Encounter plan identity or role counts do not match inputs."));
    }
    if (Plan.Player.Role != ESeedForgeEncounterRole::Player
        || Plan.Player.StableId != 0
        || Plan.Player.Cell != Layout.Entrance
        || Plan.Exit.Role != ESeedForgeEncounterRole::Exit
        || Plan.Exit.StableId != 0
        || Plan.Exit.Cell != Layout.Exit)
    {
        return InvalidPlan(TEXT("Player or exit identity does not match layout endpoints."));
    }

    const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
    TSet<FIntPoint> Occupied;
    Occupied.Reserve(2 + Plan.DataCores.Num() + Plan.Enemies.Num());
    auto AddUniqueWalkable = [&WalkableCells, &Occupied](const FIntPoint& Cell)
    {
        if (!WalkableCells.Contains(Cell) || Occupied.Contains(Cell))
        {
            return false;
        }
        Occupied.Add(Cell);
        return true;
    };
    if (!AddUniqueWalkable(Plan.Player.Cell) || !AddUniqueWalkable(Plan.Exit.Cell))
    {
        return InvalidPlan(TEXT("Player and exit must occupy distinct walkable cells."));
    }

    for (int32 Index = 0; Index < Plan.DataCores.Num(); ++Index)
    {
        const FSeedForgeEncounterEntity& Core = Plan.DataCores[Index];
        if (Core.Role != ESeedForgeEncounterRole::DataCore
            || Core.StableId != static_cast<uint32>(Index)
            || !AddUniqueWalkable(Core.Cell))
        {
            return InvalidPlan(FString::Printf(TEXT("Data Core %d is non-canonical, overlapping, or not walkable."), Index));
        }
    }
    for (int32 Index = 0; Index < Plan.Enemies.Num(); ++Index)
    {
        const FSeedForgeEncounterEntity& Enemy = Plan.Enemies[Index];
        if (Enemy.Role != ESeedForgeEncounterRole::Enemy
            || Enemy.StableId != static_cast<uint32>(Index)
            || !AddUniqueWalkable(Enemy.Cell)
            || ManhattanDistance(Enemy.Cell, Plan.Player.Cell)
                < Config.MinPlayerEnemyDistance)
        {
            return InvalidPlan(FString::Printf(TEXT("Enemy %d is non-canonical, overlapping, unsafe, or not walkable."), Index));
        }
    }
    if (Plan.CanonicalHash == 0
        || Plan.CanonicalHash != ComputeCanonicalHash(Plan, Config))
    {
        return InvalidPlan(TEXT("Encounter canonical hash does not match plan contents."));
    }

    FSeedForgeEncounterPlan ValidatedPlan = Plan;
    return FSeedForgeEncounterResult::Success(MoveTemp(ValidatedPlan));
}

uint64 FSeedForgeEncounterPlanner::ComputeCanonicalHash(
    const FSeedForgeEncounterPlan& Plan,
    const FSeedForgeEncounterConfig& Config)
{
    using namespace SeedForge::Encounter::Private;

    uint64 Hash = 14695981039346656037ULL;
    HashUInt32(Hash, 0x53464531U);
    HashUInt32(Hash, Plan.Version);
    HashUInt64(Hash, Plan.Seed);
    HashUInt64(Hash, Plan.SourceLayoutHash);
    HashUInt32(Hash, Config.Version);
    HashUInt32(Hash, static_cast<uint32>(Config.DataCoreCount));
    HashUInt32(Hash, static_cast<uint32>(Config.EnemyCount));
    HashUInt32(Hash, static_cast<uint32>(Config.MinPlayerEnemyDistance));
    HashEntity(Hash, Plan.Player);
    HashEntity(Hash, Plan.Exit);
    HashUInt32(Hash, static_cast<uint32>(Plan.DataCores.Num()));
    for (const FSeedForgeEncounterEntity& Core : Plan.DataCores)
    {
        HashEntity(Hash, Core);
    }
    HashUInt32(Hash, static_cast<uint32>(Plan.Enemies.Num()));
    for (const FSeedForgeEncounterEntity& Enemy : Plan.Enemies)
    {
        HashEntity(Hash, Enemy);
    }
    return Hash;
}

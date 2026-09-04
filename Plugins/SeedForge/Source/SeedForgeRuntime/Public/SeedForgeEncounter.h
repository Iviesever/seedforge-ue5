#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"

enum class ESeedForgeEncounterErrorCode : uint8
{
    None,
    InvalidLayout,
    InvalidConfig,
    InsufficientWalkableCells,
    EnemySafetyRadiusUnsatisfied,
    InvalidPlan
};

enum class ESeedForgeEncounterRole : uint8
{
    Player,
    Exit,
    DataCore,
    Enemy
};

struct SEEDFORGERUNTIME_API FSeedForgeEncounterConfig
{
    uint32 Version = 1;
    int32 DataCoreCount = 3;
    int32 EnemyCount = 5;
    int32 MinPlayerEnemyDistance = 8;

    bool operator==(const FSeedForgeEncounterConfig& Other) const = default;
};

struct SEEDFORGERUNTIME_API FSeedForgeEncounterEntity
{
    ESeedForgeEncounterRole Role = ESeedForgeEncounterRole::Player;
    uint32 StableId = 0;
    FIntPoint Cell = FIntPoint::ZeroValue;

    bool operator==(const FSeedForgeEncounterEntity& Other) const = default;
};

struct SEEDFORGERUNTIME_API FSeedForgeEncounterPlan
{
    uint32 Version = 1;
    uint64 Seed = 0;
    uint64 SourceLayoutHash = 0;
    FSeedForgeEncounterEntity Player;
    FSeedForgeEncounterEntity Exit;
    TArray<FSeedForgeEncounterEntity> DataCores;
    TArray<FSeedForgeEncounterEntity> Enemies;
    uint64 CanonicalHash = 0;

    bool operator==(const FSeedForgeEncounterPlan& Other) const = default;
};

struct SEEDFORGERUNTIME_API FSeedForgeEncounterResult
{
    ESeedForgeEncounterErrorCode ErrorCode = ESeedForgeEncounterErrorCode::None;
    FString ErrorMessage;
    FSeedForgeEncounterPlan Plan;

    bool IsSuccess() const
    {
        return ErrorCode == ESeedForgeEncounterErrorCode::None;
    }

    static FSeedForgeEncounterResult Success(FSeedForgeEncounterPlan&& InPlan)
    {
        FSeedForgeEncounterResult Result;
        Result.Plan = MoveTemp(InPlan);
        return Result;
    }

    static FSeedForgeEncounterResult Failure(
        ESeedForgeEncounterErrorCode InCode,
        FString InMessage)
    {
        FSeedForgeEncounterResult Result;
        Result.ErrorCode = InCode;
        Result.ErrorMessage = MoveTemp(InMessage);
        return Result;
    }
};

class SEEDFORGERUNTIME_API FSeedForgeEncounterPlanner
{
public:
    static FSeedForgeEncounterResult Generate(
        const FSeedForgeLayout& Layout,
        const FSeedForgeEncounterConfig& Config);
    static FSeedForgeEncounterResult Validate(
        const FSeedForgeEncounterPlan& Plan,
        const FSeedForgeLayout& Layout,
        const FSeedForgeEncounterConfig& Config);
    static uint64 ComputeCanonicalHash(
        const FSeedForgeEncounterPlan& Plan,
        const FSeedForgeEncounterConfig& Config);
};

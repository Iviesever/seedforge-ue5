#pragma once

#include "CoreMinimal.h"
#include "SeedForgeRunState.h"

struct SEEDFORGERUNTIME_API FSeedForgeGameplayTuning
{
    float CellSize = 200.0f;
    float PlayerMaxHealth = 100.0f;
    float PlayerMoveSpeed = 650.0f;
    float AttackRange = 360.0f;
    float AttackMinForwardDot = 0.25f;
    float AttackCooldownSeconds = 0.45f;
    float DashImpulse = 1200.0f;
    float DashCooldownSeconds = 1.25f;
    float EnemyMoveSpeed = 260.0f;
    float EnemyRepathSeconds = 0.5f;
    int32 EnemyPathExpansionBudget = 1024;
    float EnemyContactDistance = 115.0f;
    float EnemyContactDamage = 20.0f;
    float EnemyContactCooldownSeconds = 1.0f;
    float PickupDistance = 100.0f;
    float ExitDistance = 120.0f;
};

struct SEEDFORGERUNTIME_API FSeedForgeAttackCandidate
{
    uint32 StableId = 0;
    FVector WorldLocation = FVector::ZeroVector;
    bool bAlive = true;
};

enum class ESeedForgeRunFailureCode : uint8
{
    None,
    MissingWorldSubsystem,
    GenerationFailed,
    InvalidLayout,
    EncounterFailed,
    SpawnFailed,
    StartStateFailed,
    SmokeFailed
};

SEEDFORGERUNTIME_API const TCHAR* LexToString(ESeedForgeRunFailureCode Code);

struct SEEDFORGERUNTIME_API FSeedForgeGameplaySnapshot
{
    uint64 Seed = 0;
    uint64 LayoutHash = 0;
    uint64 EncounterHash = 0;
    uint64 RunGeneration = 0;
    uint64 PendingRequestId = 0;
    uint64 AppliedRequestId = 0;
    ESeedForgeRunState RunState = ESeedForgeRunState::Generating;
    float PlayerHealth = 0.0f;
    float PlayerMaxHealth = 0.0f;
    int32 CollectedCoreCount = 0;
    int32 RequiredCoreCount = 0;
    bool bExitUnlocked = false;
    ESeedForgeRunFailureCode FailureCode = ESeedForgeRunFailureCode::None;
    FString FailureMessage;
};

class SEEDFORGERUNTIME_API FSeedForgeGameplayPresentation
{
public:
    static FString BuildHudText(const FSeedForgeGameplaySnapshot& Snapshot);
};

class SEEDFORGERUNTIME_API FSeedForgeGameplayMath
{
public:
    static FVector ResolveDashDirection(float ForwardAxis, float RightAxis, const FVector& Aim);
    static FVector CellToWorld(
        const FIntPoint& Cell,
        float CellSize,
        float Height);
    static bool WorldToNearestWalkableCell(
        const FVector& WorldLocation,
        const TArray<FIntPoint>& WalkableCells,
        float CellSize,
        FIntPoint& OutCell);
    static int32 SelectAttackTarget(
        const FVector& Origin,
        const FVector& Forward,
        float Range,
        float MinForwardDot,
        const TArray<FSeedForgeAttackCandidate>& Candidates);
};

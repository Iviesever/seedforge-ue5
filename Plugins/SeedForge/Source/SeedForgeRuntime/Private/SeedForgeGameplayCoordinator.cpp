#include "SeedForgeGameplayCoordinator.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGridPathfinder.h"
#include "SeedForgePreviewActor.h"
#include "SeedForgeRuntime.h"
#include "SeedForgeValidator.h"
#include "SeedForgeWorldSubsystem.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace SeedForge::GameplayCoordinator::Private
{
    constexpr float PlayerHeight = 96.0f;
    constexpr float EnemyHeight = 58.0f;
    constexpr float CoreHeight = 52.0f;
    constexpr float ExitHeight = 92.0f;
    constexpr float EnemyMaxHealth = 100.0f;
    constexpr float PlayerAttackDamage = 50.0f;

    FActorSpawnParameters SpawnParameters(AActor* Owner)
    {
        FActorSpawnParameters Parameters;
        Parameters.Owner = Owner;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        return Parameters;
    }
}

ASeedForgeGameplayCoordinator::ASeedForgeGameplayCoordinator()
{
    PrimaryActorTick.bCanEverTick = false;
    PlayerHealth = Tuning.PlayerMaxHealth;
}

uint64 ASeedForgeGameplayCoordinator::StartRun(uint64 InSeed)
{
    check(IsInGameThread());
    ++RunGeneration;
    Seed = InSeed;
    ActiveRequestId = 0;
    AppliedRequestId = 0;
    Layout = {};
    EncounterPlan = {};
    RunFailureCode = ESeedForgeRunFailureCode::None;
    RunFailureMessage.Reset();
    bSmokeExitRequested = false;

    if (RunState.GetState() == ESeedForgeRunState::Restarting)
    {
        RunState.BeginGenerating();
    }
    else if (RunState.GetState() != ESeedForgeRunState::Generating)
    {
        if (!RunState.RequestRestart().IsSuccess()
            || !RunState.BeginGenerating().IsSuccess())
        {
            EnterRunFailure(ESeedForgeRunFailureCode::StartStateFailed,
                TEXT("Run state rejected restart before generation."));
            return 0;
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (USeedForgeWorldSubsystem* Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>())
        {
            Subsystem->CancelGeneration();
        }
    }
    ClearRunObjects();
    PlayerHealth = Tuning.PlayerMaxHealth;
    NextAttackTime = 0.0;
    NextContactDamageTime = 0.0;

    if (bGameplaySmokeMode)
    {
        InitializeGameplaySmokeTrace();
        if (GetWorld())
        {
            GetWorldTimerManager().SetTimer(GameplaySmokeWatchdogTimer, this,
                &ASeedForgeGameplayCoordinator::GameplaySmokeWatchdog, 30.0f, false);
        }
    }

    USeedForgeWorldSubsystem* Subsystem = GetWorld()
        ? GetWorld()->GetSubsystem<USeedForgeWorldSubsystem>()
        : nullptr;
    if (!Subsystem)
    {
        EnterRunFailure(ESeedForgeRunFailureCode::MissingWorldSubsystem,
            TEXT("Gameplay run cannot find SeedForge world subsystem."));
        return 0;
    }
    if (!GenerationAppliedHandle.IsValid())
    {
        GenerationAppliedHandle = Subsystem->OnGenerationApplied().AddUObject(
            this, &ASeedForgeGameplayCoordinator::HandleGenerationApplied);
    }
    ActiveRequestId = Subsystem->RequestGeneration(Seed, GenerationConfig);
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Gameplay queued run=%llu request=%llu seed=%llu."),
        RunGeneration,
        ActiveRequestId,
        Seed);
    return ActiveRequestId;
}

bool ASeedForgeGameplayCoordinator::ApplyGeneratedLayout(const FSeedForgeLayout& InLayout)
{
    if (RunState.GetState() != ESeedForgeRunState::Generating)
    {
        return false;
    }
    if (UWorld* World = GetWorld())
    {
        if (USeedForgeWorldSubsystem* Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>())
        {
            Subsystem->CancelGeneration();
        }
    }
    ActiveRequestId = 0;
    // Direct value application is not a subsystem completion: request 0 is explicit.
    return ApplyGeneratedLayout(InLayout, 0);
}

bool ASeedForgeGameplayCoordinator::ApplyGeneratedLayout(
    const FSeedForgeLayout& InLayout, uint64 SourceRequestId)
{
    using namespace SeedForge::GameplayCoordinator::Private;

    if (!GetWorld())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::MissingWorldSubsystem,
            TEXT("Cannot apply a layout without a World."));
        return false;
    }
    if (RunState.GetState() != ESeedForgeRunState::Generating)
    {
        return false;
    }
    const FSeedForgeValidationResult LayoutValidation = FSeedForgeValidator::Validate(
        InLayout,
        GenerationConfig);
    if (!LayoutValidation.IsValid())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::InvalidLayout,
            FString::Printf(TEXT("Layout validation code=%d: %s"),
                static_cast<int32>(LayoutValidation.ErrorCode), *LayoutValidation.ErrorMessage));
        return false;
    }
    const FSeedForgeEncounterResult Planned = FSeedForgeEncounterPlanner::Generate(
        InLayout,
        EncounterConfig);
    if (!Planned.IsSuccess())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::EncounterFailed,
            FString::Printf(TEXT("Encounter planning code=%d: %s"),
                static_cast<int32>(Planned.ErrorCode), *Planned.ErrorMessage));
        return false;
    }

    ClearRunObjects();
    Layout = InLayout;
    EncounterPlan = Planned.Plan;
    Seed = Layout.Seed;

    const FTransform VisualizationTransform = FTransform::Identity;
    Visualization = GetWorld()->SpawnActorDeferred<ASeedForgePreviewActor>(
        ASeedForgePreviewActor::StaticClass(),
        VisualizationTransform,
        this,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!IsValid(Visualization) || Visualization->IsActorBeingDestroyed())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::SpawnFailed,
            TEXT("Gameplay failed to spawn layout visualization."));
        return false;
    }
    Visualization->SetAutoGenerateOnBeginPlay(false);
    Visualization->FinishSpawning(VisualizationTransform);
    if (!IsValid(Visualization) || Visualization->IsActorBeingDestroyed())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::SpawnFailed,
            TEXT("Layout visualization was destroyed during initialization."));
        return false;
    }
    Visualization->ApplyLayout(Layout);

    Player = ResolvePlayer();
    if (!IsValid(Player) || Player->IsActorBeingDestroyed())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::SpawnFailed,
            TEXT("Gameplay failed to resolve player Character."));
        return false;
    }
    Player->SetGameplayCoordinator(this);
    Player->SetActorLocation(FSeedForgeGameplayMath::CellToWorld(
        EncounterPlan.Player.Cell,
        Tuning.CellSize,
        PlayerHeight));

    const FActorSpawnParameters Parameters = SpawnParameters(this);
    CoreActors.Reserve(EncounterPlan.DataCores.Num());
    for (const FSeedForgeEncounterEntity& CoreEntity : EncounterPlan.DataCores)
    {
        ASeedForgeCorePickup* Core = GetWorld()->SpawnActor<ASeedForgeCorePickup>(
            FSeedForgeGameplayMath::CellToWorld(CoreEntity.Cell, Tuning.CellSize, CoreHeight),
            FRotator::ZeroRotator,
            Parameters);
        if (!IsValid(Core) || Core->IsActorBeingDestroyed())
        {
            EnterRunFailure(ESeedForgeRunFailureCode::SpawnFailed,
                FString::Printf(TEXT("Gameplay failed to spawn Core %u."), CoreEntity.StableId));
            return false;
        }
        Core->Configure(CoreEntity.StableId, CoreEntity.Cell);
        CoreActors.Add(Core);
    }

    EnemyActors.Reserve(EncounterPlan.Enemies.Num());
    for (const FSeedForgeEncounterEntity& EnemyEntity : EncounterPlan.Enemies)
    {
        ASeedForgeEnemyPawn* Enemy = GetWorld()->SpawnActor<ASeedForgeEnemyPawn>(
            FSeedForgeGameplayMath::CellToWorld(EnemyEntity.Cell, Tuning.CellSize, EnemyHeight),
            FRotator::ZeroRotator,
            Parameters);
        if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed())
        {
            EnterRunFailure(ESeedForgeRunFailureCode::SpawnFailed,
                FString::Printf(TEXT("Gameplay failed to spawn enemy %u."), EnemyEntity.StableId));
            return false;
        }
        Enemy->Configure(EnemyEntity.StableId, EnemyEntity.Cell);
        EnemyActors.Add(Enemy);
        EnemyHealth.Add(EnemyEntity.StableId, EnemyMaxHealth);
    }

    ExitActor = GetWorld()->SpawnActor<ASeedForgeExitActor>(
        FSeedForgeGameplayMath::CellToWorld(EncounterPlan.Exit.Cell, Tuning.CellSize, ExitHeight),
        FRotator::ZeroRotator,
        Parameters);
    if (!IsValid(ExitActor) || ExitActor->IsActorBeingDestroyed())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::SpawnFailed,
            TEXT("Gameplay failed to spawn exit."));
        return false;
    }
    ExitActor->Configure(EncounterPlan.Exit.Cell);

    TArray<uint32> CoreIds;
    CoreIds.Reserve(EncounterPlan.DataCores.Num());
    for (const FSeedForgeEncounterEntity& CoreEntity : EncounterPlan.DataCores)
    {
        CoreIds.Add(CoreEntity.StableId);
    }
    const FSeedForgeRunTransitionResult Started = RunState.StartPlaying(MoveTemp(CoreIds));
    if (!Started.IsSuccess())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::StartStateFailed,
            FString::Printf(TEXT("Gameplay state rejected Playing: %s"), *Started.ErrorMessage));
        return false;
    }

    AppliedRequestId = SourceRequestId;
    PlayerHealth = Tuning.PlayerMaxHealth;
    GetWorldTimerManager().SetTimer(
        InteractionTimer,
        this,
        &ASeedForgeGameplayCoordinator::TickInteractions,
        0.05f,
        true);
    GetWorldTimerManager().SetTimer(
        RepathTimer,
        this,
        &ASeedForgeGameplayCoordinator::ReplanEnemies,
        Tuning.EnemyRepathSeconds,
        true,
        0.05f);
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Gameplay ready run=%llu request=%llu seed=%llu layout_hash=%llu encounter_hash=%llu players=1 cores=%d enemies=%d exits=1."),
        RunGeneration,
        AppliedRequestId,
        Seed,
        Layout.CanonicalHash,
        EncounterPlan.CanonicalHash,
        CoreActors.Num(),
        EnemyActors.Num());
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Applied request=%llu run=%llu seed=%llu hash=%llu floors=%d walls=%d gameplay=true."),
        AppliedRequestId,
        RunGeneration,
        Seed,
        Layout.CanonicalHash,
        Visualization->GetFloorInstanceCount(),
        Visualization->GetWallInstanceCount());
    if (bGameplaySmokeMode)
    {
        StartGameplaySmoke();
    }
    else if (!CapturePath.IsEmpty())
    {
        GetWorldTimerManager().SetTimer(
            CaptureTimer,
            this,
            &ASeedForgeGameplayCoordinator::CaptureScreenshot,
            1.0f,
            false);
    }
    return true;
}

void ASeedForgeGameplayCoordinator::RestartSameSeed()
{
    StartRun(Seed);
}

void ASeedForgeGameplayCoordinator::StartNewSeed()
{
    const uint64 NextSeed = Seed * 6364136223846793005ULL + 1442695040888963407ULL;
    StartRun(NextSeed);
}

bool ASeedForgeGameplayCoordinator::TryPlayerAttack(
    const FVector& Origin,
    const FVector& Forward)
{
    using namespace SeedForge::GameplayCoordinator::Private;

    if (RunState.GetState() != ESeedForgeRunState::Playing || !GetWorld())
    {
        return false;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextAttackTime)
    {
        return false;
    }
    NextAttackTime = Now + Tuning.AttackCooldownSeconds;

    TArray<FSeedForgeAttackCandidate> Candidates;
    TArray<ASeedForgeEnemyPawn*> CandidateActors;
    for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
    {
        if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
        {
            const float* Health = EnemyHealth.Find(Enemy->GetStableId());
            Candidates.Add({Enemy->GetStableId(), Enemy->GetActorLocation(), Health && *Health > 0.0f});
            CandidateActors.Add(Enemy);
        }
    }
    const int32 SelectedIndex = FSeedForgeGameplayMath::SelectAttackTarget(
        Origin,
        Forward,
        Tuning.AttackRange,
        Tuning.AttackMinForwardDot,
        Candidates);
    if (CandidateActors.IsValidIndex(SelectedIndex))
    {
        ASeedForgeEnemyPawn* Enemy = CandidateActors[SelectedIndex];
        float& Health = EnemyHealth.FindOrAdd(Enemy->GetStableId());
        Health = FMath::Max(0.0f, Health - PlayerAttackDamage);
        UE_LOG(
            LogSeedForge,
            Display,
            TEXT("Gameplay attack enemy=%u remaining_hp=%.0f."),
            Enemy->GetStableId(),
            Health);
        if (Health <= 0.0f)
        {
            Enemy->Destroy();
            for (TObjectPtr<ASeedForgeEnemyPawn>& TrackedEnemy : EnemyActors)
            {
                if (TrackedEnemy == Enemy)
                {
                    TrackedEnemy = nullptr;
                    break;
                }
            }
        }
    }
    return true;
}

bool ASeedForgeGameplayCoordinator::ApplyPlayerDamage(float Damage)
{
    if (RunState.GetState() != ESeedForgeRunState::Playing || Damage <= 0.0f)
    {
        return false;
    }
    PlayerHealth = FMath::Max(0.0f, PlayerHealth - Damage);
    UE_LOG(LogSeedForge, Display, TEXT("Gameplay player damage=%.0f hp=%.0f."), Damage, PlayerHealth);
    if (PlayerHealth <= 0.0f)
    {
        const FSeedForgeRunTransitionResult Lost = RunState.PlayerDied();
        if (!Lost.IsSuccess())
        {
            UE_LOG(LogSeedForge, Error, TEXT("Gameplay state rejected player death: %s"), *Lost.ErrorMessage);
            return false;
        }
        UE_LOG(LogSeedForge, Display, TEXT("Gameplay state transition Playing->Lost."));
        EnterTerminalState();
    }
    return true;
}

FSeedForgeGameplaySnapshot ASeedForgeGameplayCoordinator::GetSnapshot() const
{
    FSeedForgeGameplaySnapshot Snapshot;
    Snapshot.Seed = Seed;
    Snapshot.LayoutHash = Layout.CanonicalHash;
    Snapshot.EncounterHash = EncounterPlan.CanonicalHash;
    Snapshot.RunGeneration = RunGeneration;
    Snapshot.PendingRequestId = ActiveRequestId;
    Snapshot.AppliedRequestId = AppliedRequestId;
    Snapshot.RunState = RunState.GetState();
    Snapshot.PlayerHealth = PlayerHealth;
    Snapshot.PlayerMaxHealth = Tuning.PlayerMaxHealth;
    Snapshot.CollectedCoreCount = RunState.GetCollectedCoreCount();
    Snapshot.RequiredCoreCount = RunState.GetRequiredCoreCount();
    Snapshot.bExitUnlocked = RunState.IsExitUnlocked();
    Snapshot.FailureCode = RunFailureCode;
    Snapshot.FailureMessage = RunFailureMessage;
    return Snapshot;
}

const FSeedForgeGameplayTuning& ASeedForgeGameplayCoordinator::GetTuning() const
{
    return Tuning;
}

const FSeedForgeLayout& ASeedForgeGameplayCoordinator::GetLayout() const
{
    return Layout;
}

const FSeedForgeEncounterPlan& ASeedForgeGameplayCoordinator::GetEncounterPlan() const
{
    return EncounterPlan;
}

int32 ASeedForgeGameplayCoordinator::GetLiveCoreActorCount() const
{
    int32 Count = 0;
    for (const ASeedForgeCorePickup* Core : CoreActors)
    {
        Count += IsValid(Core) ? 1 : 0;
    }
    return Count;
}

int32 ASeedForgeGameplayCoordinator::GetLiveEnemyActorCount() const
{
    int32 Count = 0;
    for (const ASeedForgeEnemyPawn* Enemy : EnemyActors)
    {
        Count += IsValid(Enemy) && !Enemy->IsActorBeingDestroyed() ? 1 : 0;
    }
    return Count;
}

void ASeedForgeGameplayCoordinator::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeSeed="), Seed);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridWidth="), GenerationConfig.GridWidth);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridHeight="), GenerationConfig.GridHeight);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeRoomCount="), GenerationConfig.RoomCount);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeCapturePath="), CapturePath);
    bGameplaySmokeMode = FParse::Param(FCommandLine::Get(), TEXT("SeedForgeGameplaySmoke"));
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGameplayTrace="), GameplaySmokeTracePath);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGameplayCaptureDir="), GameplaySmokeCaptureDirectory);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGitSha="), GameplaySmokeGitSha);
    if (bGameplaySmokeMode)
    {
        InitializeGameplaySmokeTrace();
    }
    if (bGameplaySmokeMode
        && (GameplaySmokeTracePath.IsEmpty()
            || GameplaySmokeCaptureDirectory.IsEmpty()
            || GameplaySmokeGitSha.IsEmpty()))
    {
        FailGameplaySmoke(
            TEXT("InvalidArguments"),
            TEXT("Smoke requires SeedForgeGameplayTrace, SeedForgeGameplayCaptureDir, and SeedForgeGitSha."));
        return;
    }
    StartRun(Seed);
}

void ASeedForgeGameplayCoordinator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (USeedForgeWorldSubsystem* Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>())
        {
            Subsystem->OnGenerationApplied().Remove(GenerationAppliedHandle);
            Subsystem->CancelGeneration();
        }
    }
    ActiveRequestId = 0;
    AppliedRequestId = 0;
    ClearRunObjects();
    Super::EndPlay(EndPlayReason);
}

void ASeedForgeGameplayCoordinator::HandleGenerationApplied(
    const FSeedForgeAsyncCompletion& Completion)
{
    if (ActiveRequestId == 0 || Completion.RequestId != ActiveRequestId)
    {
        return;
    }
    ActiveRequestId = 0;
    if (!Completion.Result.IsSuccess())
    {
        EnterRunFailure(ESeedForgeRunFailureCode::GenerationFailed,
            FString::Printf(TEXT("Generation code=%d: %s"),
                static_cast<int32>(Completion.Result.ErrorCode), *Completion.Result.ErrorMessage));
        return;
    }
    ApplyGeneratedLayout(Completion.Result.Layout, Completion.RequestId);
}

void ASeedForgeGameplayCoordinator::ClearRunObjects()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InteractionTimer);
        World->GetTimerManager().ClearTimer(RepathTimer);
        World->GetTimerManager().ClearTimer(CaptureTimer);
        World->GetTimerManager().ClearTimer(CaptureExitTimer);
        World->GetTimerManager().ClearTimer(GameplaySmokeTimer);
        World->GetTimerManager().ClearTimer(GameplaySmokeWatchdogTimer);
    }
    for (ASeedForgeCorePickup* Core : CoreActors)
    {
        if (IsValid(Core))
        {
            Core->Destroy();
        }
    }
    CoreActors.Reset();
    for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
    {
        if (IsValid(Enemy))
        {
            Enemy->Destroy();
        }
    }
    EnemyActors.Reset();
    EnemyHealth.Reset();
    if (IsValid(ExitActor))
    {
        ExitActor->Destroy();
    }
    ExitActor = nullptr;
    if (IsValid(Visualization))
    {
        Visualization->Destroy();
    }
    Visualization = nullptr;
    if (IsValid(Player))
    {
        if (AController* Controller = Player->GetController())
        {
            Controller->UnPossess();
        }
        Player->SetGameplayCoordinator(nullptr);
        Player->Destroy();
    }
    Player = nullptr;
    PendingSmokeScreenshotPath.Reset();
    GameplaySmokeCoreIndex = 0;
    GameplaySmokeStageDeadline = 0.0;
    GameplaySmokeStage = EGameplaySmokeStage::Disabled;
}

void ASeedForgeGameplayCoordinator::TickInteractions()
{
    if (RunState.GetState() != ESeedForgeRunState::Playing)
    {
        return;
    }
    ASeedForgePlayerCharacter* CurrentPlayer = ResolvePlayer();
    if (!CurrentPlayer)
    {
        return;
    }
    const FVector PlayerLocation = CurrentPlayer->GetActorLocation();

    for (int32 Index = 0; Index < CoreActors.Num(); ++Index)
    {
        ASeedForgeCorePickup* Core = CoreActors[Index];
        if (IsValid(Core)
            && FVector::DistSquared2D(PlayerLocation, Core->GetActorLocation())
                <= FMath::Square(Tuning.PickupDistance))
        {
            const FSeedForgeRunTransitionResult Collected = RunState.CollectCore(Core->GetStableId());
            if (Collected.IsSuccess())
            {
                UE_LOG(
                    LogSeedForge,
                    Display,
                    TEXT("Gameplay collected core=%u progress=%d/%d."),
                    Core->GetStableId(),
                    RunState.GetCollectedCoreCount(),
                    RunState.GetRequiredCoreCount());
                Core->Destroy();
                CoreActors[Index] = nullptr;
            }
        }
    }

    if (IsValid(ExitActor))
    {
        if (ExitActor->IsUnlocked() != RunState.IsExitUnlocked())
        {
            ExitActor->SetUnlocked(RunState.IsExitUnlocked());
        }
        if (RunState.IsExitUnlocked()
            && FVector::DistSquared2D(PlayerLocation, ExitActor->GetActorLocation())
                <= FMath::Square(Tuning.ExitDistance))
        {
            const FSeedForgeRunTransitionResult Won = RunState.ReachExit();
            if (Won.IsSuccess())
            {
                UE_LOG(LogSeedForge, Display, TEXT("Gameplay state transition Playing->Won."));
                EnterTerminalState();
                return;
            }
        }
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextContactDamageTime)
    {
        for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
        {
            if (IsValid(Enemy)
                && !Enemy->IsActorBeingDestroyed()
                && FVector::DistSquared2D(PlayerLocation, Enemy->GetActorLocation())
                    <= FMath::Square(Tuning.EnemyContactDistance))
            {
                NextContactDamageTime = Now + Tuning.EnemyContactCooldownSeconds;
                ApplyPlayerDamage(Tuning.EnemyContactDamage);
                break;
            }
        }
    }
}

void ASeedForgeGameplayCoordinator::ReplanEnemies()
{
    if (RunState.GetState() != ESeedForgeRunState::Playing || !IsValid(Player))
    {
        return;
    }
    const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
    FIntPoint PlayerCell;
    if (!FSeedForgeGameplayMath::WorldToNearestWalkableCell(
        Player->GetActorLocation(),
        WalkableCells,
        Tuning.CellSize,
        PlayerCell))
    {
        return;
    }

    for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
    {
        if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed())
        {
            continue;
        }
        FIntPoint EnemyCell;
        if (!FSeedForgeGameplayMath::WorldToNearestWalkableCell(
            Enemy->GetActorLocation(),
            WalkableCells,
            Tuning.CellSize,
            EnemyCell))
        {
            Enemy->ClearPath();
            continue;
        }
        FSeedForgePathRequest Request;
        Request.Start = EnemyCell;
        Request.Goal = PlayerCell;
        Request.WalkableCells = WalkableCells;
        Request.MaxExpandedNodes = Tuning.EnemyPathExpansionBudget;
        const FSeedForgePathResult PathResult = FSeedForgeGridPathfinder::FindPath(Request);
        if (!PathResult.IsSuccess())
        {
            Enemy->ClearPath();
            UE_LOG(
                LogSeedForge,
                Verbose,
                TEXT("Enemy %u path status=%d expanded=%d."),
                Enemy->GetStableId(),
                static_cast<int32>(PathResult.Status),
                PathResult.ExpandedNodes);
            continue;
        }

        TArray<FVector> WorldPath;
        for (int32 PathIndex = 1; PathIndex < PathResult.Path.Num(); ++PathIndex)
        {
            WorldPath.Add(FSeedForgeGameplayMath::CellToWorld(
                PathResult.Path[PathIndex],
                Tuning.CellSize,
                SeedForge::GameplayCoordinator::Private::EnemyHeight));
        }
        Enemy->SetPath(MoveTemp(WorldPath));
    }
}

void ASeedForgeGameplayCoordinator::EnterTerminalState()
{
    GetWorldTimerManager().ClearTimer(RepathTimer);
    GetWorldTimerManager().ClearTimer(InteractionTimer);
    for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
    {
        if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
        {
            Enemy->ClearPath();
        }
    }
}

void ASeedForgeGameplayCoordinator::EnterRunFailure(
    ESeedForgeRunFailureCode Code, const FString& Message, const TCHAR* SmokeCode)
{
    if (RunFailureCode != ESeedForgeRunFailureCode::None
        && RunState.GetState() == ESeedForgeRunState::Failed)
    {
        return;
    }
    ActiveRequestId = 0;
    AppliedRequestId = 0;
    if (UWorld* World = GetWorld())
    {
        if (USeedForgeWorldSubsystem* Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>())
        {
            Subsystem->CancelGeneration();
            Subsystem->OnGenerationApplied().Remove(GenerationAppliedHandle);
        }
    }
    GenerationAppliedHandle.Reset();
    ClearRunObjects();
    Layout = {};
    EncounterPlan = {};
    PlayerHealth = 0.0f;
    NextAttackTime = 0.0;
    NextContactDamageTime = 0.0;
    RunState.FailRun();
    RunFailureCode = Code;
    RunFailureMessage = Message;
    UE_LOG(LogSeedForge, Error, TEXT("Gameplay run failed code=%s message=%s"),
        LexToString(Code), *Message);
    if (bGameplaySmokeMode)
    {
        FailGameplaySmoke(SmokeCode ? SmokeCode : LexToString(Code), Message);
    }
}

void ASeedForgeGameplayCoordinator::InitializeGameplaySmokeTrace()
{
    GameplaySmokeTrace = {};
    GameplaySmokeTrace.GitSha = GameplaySmokeGitSha;
    GameplaySmokeTrace.EngineVersion = FEngineVersion::Current().ToString();
    GameplaySmokeTrace.Seed = Seed;
    GameplaySmokeTrace.StateTransitions = {TEXT("Generating")};
}

void ASeedForgeGameplayCoordinator::CaptureScreenshot()
{
    CapturePath = FPaths::ConvertRelativePathToFull(CapturePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*FPaths::GetPath(CapturePath));
    FScreenshotRequest::RequestScreenshot(CapturePath, false, false);
    UE_LOG(LogSeedForge, Display, TEXT("Requested gameplay screenshot: %s"), *CapturePath);
    GetWorldTimerManager().SetTimer(
        CaptureExitTimer,
        this,
        &ASeedForgeGameplayCoordinator::ExitAfterCapture,
        2.0f,
        false);
}

void ASeedForgeGameplayCoordinator::ExitAfterCapture()
{
    UE_LOG(LogSeedForge, Display, TEXT("Gameplay capture complete; requesting clean exit."));
    FGenericPlatformMisc::RequestExit(false);
}

void ASeedForgeGameplayCoordinator::StartGameplaySmoke()
{
    GameplaySmokeTrace.LayoutHash = Layout.CanonicalHash;
    GameplaySmokeTrace.EncounterHash = EncounterPlan.CanonicalHash;
    GameplaySmokeTrace.ActorCounts = {
        IsValid(Player) ? 1 : 0,
        GetLiveCoreActorCount(),
        GetLiveEnemyActorCount(),
        IsValid(ExitActor) ? 1 : 0};
    GameplaySmokeTrace.StateTransitions.Add(TEXT("Playing"));

    const FSeedForgeEncounterResult Validation = FSeedForgeEncounterPlanner::Validate(
        EncounterPlan,
        Layout,
        EncounterConfig);
    if (!Validation.IsSuccess())
    {
        FailGameplaySmoke(TEXT("InvalidEncounter"), Validation.ErrorMessage);
        return;
    }
    if (GameplaySmokeTrace.ActorCounts.Players != 1
        || GameplaySmokeTrace.ActorCounts.DataCores != EncounterConfig.DataCoreCount
        || GameplaySmokeTrace.ActorCounts.Enemies != EncounterConfig.EnemyCount
        || GameplaySmokeTrace.ActorCounts.Exits != 1)
    {
        FailGameplaySmoke(
            TEXT("ActorCountMismatch"),
            FString::Printf(
                TEXT("Expected 1/%d/%d/1 player/core/enemy/exit actors, found %d/%d/%d/%d."),
                EncounterConfig.DataCoreCount,
                EncounterConfig.EnemyCount,
                GameplaySmokeTrace.ActorCounts.Players,
                GameplaySmokeTrace.ActorCounts.DataCores,
                GameplaySmokeTrace.ActorCounts.Enemies,
                GameplaySmokeTrace.ActorCounts.Exits));
        return;
    }

    FIntPoint PlayerCell;
    const TArray<FIntPoint> WalkableCells = Layout.GetCanonicalWalkableCells();
    if (!FSeedForgeGameplayMath::WorldToNearestWalkableCell(
            Player->GetActorLocation(), WalkableCells, Tuning.CellSize, PlayerCell)
        || PlayerCell != EncounterPlan.Player.Cell
        || ExitActor->GetSpawnCell() != EncounterPlan.Exit.Cell)
    {
        FailGameplaySmoke(TEXT("ActorPositionMismatch"), TEXT("Player or exit does not match the encounter plan."));
        return;
    }
    for (int32 Index = 0; Index < CoreActors.Num(); ++Index)
    {
        const ASeedForgeCorePickup* Core = CoreActors[Index];
        if (!IsValid(Core)
            || !EncounterPlan.DataCores.IsValidIndex(Index)
            || Core->GetStableId() != EncounterPlan.DataCores[Index].StableId
            || Core->GetSpawnCell() != EncounterPlan.DataCores[Index].Cell)
        {
            FailGameplaySmoke(TEXT("ActorPositionMismatch"), TEXT("A Data Core does not match the encounter plan."));
            return;
        }
    }
    for (int32 Index = 0; Index < EnemyActors.Num(); ++Index)
    {
        const ASeedForgeEnemyPawn* Enemy = EnemyActors[Index];
        if (!IsValid(Enemy)
            || !EncounterPlan.Enemies.IsValidIndex(Index)
            || Enemy->GetStableId() != EncounterPlan.Enemies[Index].StableId
            || Enemy->GetSpawnCell() != EncounterPlan.Enemies[Index].Cell)
        {
            FailGameplaySmoke(TEXT("ActorPositionMismatch"), TEXT("An enemy does not match the encounter plan."));
            return;
        }
    }

    GameplaySmokeCoreIndex = 0;
    GameplaySmokeStage = EGameplaySmokeStage::Warmup;
    GameplaySmokeStageDeadline = GetWorld()->GetTimeSeconds() + 1.0;
    GetWorldTimerManager().SetTimer(
        GameplaySmokeTimer,
        this,
        &ASeedForgeGameplayCoordinator::AdvanceGameplaySmoke,
        0.1f,
        true);
    GetWorldTimerManager().SetTimer(
        GameplaySmokeWatchdogTimer,
        this,
        &ASeedForgeGameplayCoordinator::GameplaySmokeWatchdog,
        30.0f,
        false);
}

void ASeedForgeGameplayCoordinator::AdvanceGameplaySmoke()
{
    const double Now = GetWorld()->GetTimeSeconds();
    auto AwaitScreenshot = [this, Now](EGameplaySmokeStage NextStage)
    {
        if (IsPendingSmokeScreenshotReady())
        {
            PendingSmokeScreenshotPath.Reset();
            GameplaySmokeStage = NextStage;
            return;
        }
        if (Now >= GameplaySmokeStageDeadline)
        {
            FailGameplaySmoke(TEXT("ScreenshotTimeout"), TEXT("A required gameplay screenshot was not materialized."));
        }
    };

    switch (GameplaySmokeStage)
    {
    case EGameplaySmokeStage::Warmup:
        if (Now >= GameplaySmokeStageDeadline)
        {
            GameplaySmokeStage = EGameplaySmokeStage::WaitingStartCapture;
            RequestSmokeScreenshot(TEXT("start"));
        }
        break;

    case EGameplaySmokeStage::WaitingStartCapture:
        AwaitScreenshot(EGameplaySmokeStage::PrepareCombat);
        break;

    case EGameplaySmokeStage::PrepareCombat:
    {
        ASeedForgeEnemyPawn* Target = nullptr;
        for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
        {
            if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
            {
                Target = Enemy;
                break;
            }
        }
        if (!Target || !IsValid(Player))
        {
            FailGameplaySmoke(TEXT("MissingCombatActor"), TEXT("Smoke could not prepare a live player and enemy."));
            break;
        }
        Player->SetActorLocation(FSeedForgeGameplayMath::CellToWorld(
            Target->GetSpawnCell(),
            Tuning.CellSize,
            SeedForge::GameplayCoordinator::Private::PlayerHeight), false);
        Player->SetAimWorldPoint(Target->GetActorLocation());
        GameplaySmokeStageDeadline = Now + 0.5;
        GameplaySmokeStage = EGameplaySmokeStage::Attack;
        break;
    }

    case EGameplaySmokeStage::Attack:
    {
        ASeedForgeEnemyPawn* Target = nullptr;
        for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
        {
            if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
            {
                Target = Enemy;
                break;
            }
        }
        if (!Target || !IsValid(Player))
        {
            FailGameplaySmoke(TEXT("MissingCombatActor"), TEXT("Smoke could not find a live player and enemy."));
            break;
        }
        if (Now < GameplaySmokeStageDeadline)
        {
            break;
        }
        const FVector TargetLocation = Target->GetActorLocation();
        Player->SetAimWorldPoint(TargetLocation);
        if (!TryPlayerAttack(Player->GetActorLocation(), Player->GetAimDirection()))
        {
            FailGameplaySmoke(TEXT("AttackRejected"), TEXT("Production attack boundary rejected the first smoke attack."));
            break;
        }
        Player->ShowAttackPulse();
        GameplaySmokeTrace.Actions.Add(FString::Printf(TEXT("Attack:Enemy:%u"), Target->GetStableId()));
        GameplaySmokeStage = EGameplaySmokeStage::WaitingCombatCapture;
        RequestSmokeScreenshot(TEXT("combat"));
        break;
    }

    case EGameplaySmokeStage::WaitingCombatCapture:
        if (IsPendingSmokeScreenshotReady())
        {
            PendingSmokeScreenshotPath.Reset();
            GameplaySmokeStage = EGameplaySmokeStage::FinishCombat;
            GameplaySmokeStageDeadline = Now + 3.0;
        }
        else if (Now >= GameplaySmokeStageDeadline)
        {
            FailGameplaySmoke(TEXT("ScreenshotTimeout"), TEXT("Combat screenshot was not materialized."));
        }
        break;

    case EGameplaySmokeStage::FinishCombat:
    {
        ASeedForgeEnemyPawn* Target = nullptr;
        for (ASeedForgeEnemyPawn* Enemy : EnemyActors)
        {
            if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
            {
                Target = Enemy;
                break;
            }
        }
        if (!Target || !IsValid(Player))
        {
            FailGameplaySmoke(TEXT("MissingCombatActor"), TEXT("Smoke lost the damaged enemy before the finishing attack."));
            break;
        }
        Player->SetAimWorldPoint(Target->GetActorLocation());
        if (!TryPlayerAttack(Player->GetActorLocation(), Player->GetAimDirection()))
        {
            if (Now >= GameplaySmokeStageDeadline)
            {
                FailGameplaySmoke(TEXT("AttackCooldownTimeout"), TEXT("Second production attack never left cooldown."));
            }
            break;
        }
        Player->ShowAttackPulse();
        GameplaySmokeTrace.Actions.Add(FString::Printf(TEXT("Kill:Enemy:%u"), Target->GetStableId()));
        if (GetLiveEnemyActorCount() != EncounterConfig.EnemyCount - 1)
        {
            FailGameplaySmoke(TEXT("EnemyDeathFailed"), TEXT("Second attack did not remove exactly one enemy."));
            break;
        }
        GameplaySmokeStage = EGameplaySmokeStage::CollectCores;
        break;
    }

    case EGameplaySmokeStage::CollectCores:
        if (GameplaySmokeCoreIndex < EncounterPlan.DataCores.Num())
        {
            const FSeedForgeEncounterEntity& CoreEntity = EncounterPlan.DataCores[GameplaySmokeCoreIndex];
            ASeedForgeCorePickup* Core = CoreActors.IsValidIndex(GameplaySmokeCoreIndex)
                ? CoreActors[GameplaySmokeCoreIndex]
                : nullptr;
            if (!IsValid(Core) || !IsValid(Player))
            {
                FailGameplaySmoke(TEXT("MissingCore"), TEXT("A required production Core actor is missing."));
                break;
            }
            Player->SetActorLocation(FSeedForgeGameplayMath::CellToWorld(
                CoreEntity.Cell,
                Tuning.CellSize,
                SeedForge::GameplayCoordinator::Private::PlayerHeight), false);
            TickInteractions();
            if (RunState.GetCollectedCoreCount() != GameplaySmokeCoreIndex + 1)
            {
                FailGameplaySmoke(TEXT("CoreCollectionFailed"), TEXT("Production proximity rule did not collect the expected Core."));
                break;
            }
            GameplaySmokeTrace.Actions.Add(FString::Printf(TEXT("Collect:Core:%u"), CoreEntity.StableId));
            ++GameplaySmokeCoreIndex;
            break;
        }
        if (!RunState.IsExitUnlocked() || !IsValid(ExitActor) || !ExitActor->IsUnlocked())
        {
            FailGameplaySmoke(TEXT("ExitUnlockFailed"), TEXT("Collecting every Core did not unlock the production exit."));
            break;
        }
        GameplaySmokeTrace.Actions.Add(TEXT("ExitUnlocked"));
        GameplaySmokeStage = EGameplaySmokeStage::ReachExit;
        break;

    case EGameplaySmokeStage::ReachExit:
        if (!IsValid(Player) || !IsValid(ExitActor))
        {
            FailGameplaySmoke(TEXT("MissingExit"), TEXT("Player or exit is missing before extraction."));
            break;
        }
        Player->SetActorLocation(FSeedForgeGameplayMath::CellToWorld(
            EncounterPlan.Exit.Cell,
            Tuning.CellSize,
            SeedForge::GameplayCoordinator::Private::PlayerHeight), false);
        TickInteractions();
        if (RunState.GetState() != ESeedForgeRunState::Won)
        {
            FailGameplaySmoke(TEXT("WinTransitionFailed"), TEXT("Production exit rule did not transition Playing to Won."));
            break;
        }
        GameplaySmokeTrace.Actions.Add(TEXT("ReachExit"));
        GameplaySmokeTrace.StateTransitions.Add(TEXT("Won"));
        GameplaySmokeStage = EGameplaySmokeStage::WaitingWinCapture;
        RequestSmokeScreenshot(TEXT("win"));
        break;

    case EGameplaySmokeStage::WaitingWinCapture:
        if (IsPendingSmokeScreenshotReady())
        {
            PendingSmokeScreenshotPath.Reset();
            CompleteGameplaySmoke();
        }
        else if (Now >= GameplaySmokeStageDeadline)
        {
            FailGameplaySmoke(TEXT("ScreenshotTimeout"), TEXT("Win screenshot was not materialized."));
        }
        break;

    case EGameplaySmokeStage::Disabled:
    case EGameplaySmokeStage::Complete:
    case EGameplaySmokeStage::Failed:
    default:
        break;
    }
}

void ASeedForgeGameplayCoordinator::GameplaySmokeWatchdog()
{
    FailGameplaySmoke(TEXT("SmokeTimeout"), TEXT("Gameplay smoke exceeded its 30 second runtime budget."));
}

void ASeedForgeGameplayCoordinator::RequestSmokeScreenshot(const TCHAR* Label)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*GameplaySmokeCaptureDirectory);
    PendingSmokeScreenshotPath = FPaths::Combine(
        GameplaySmokeCaptureDirectory,
        FString::Printf(TEXT("SeedForge-Gameplay-%s-%llu.png"), Label, Seed));
    PendingSmokeScreenshotPath = FPaths::ConvertRelativePathToFull(PendingSmokeScreenshotPath);
    FPaths::MakeStandardFilename(PendingSmokeScreenshotPath);
    if (PlatformFile.FileExists(*PendingSmokeScreenshotPath))
    {
        PlatformFile.DeleteFile(*PendingSmokeScreenshotPath);
    }
    GameplaySmokeTrace.ScreenshotPaths.Add(PendingSmokeScreenshotPath);
    GameplaySmokeStageDeadline = GetWorld()->GetTimeSeconds() + 8.0;
    FScreenshotRequest::RequestScreenshot(PendingSmokeScreenshotPath, false, false);
    UE_LOG(LogSeedForge, Display, TEXT("Gameplay smoke requested screenshot: %s"), *PendingSmokeScreenshotPath);
}

bool ASeedForgeGameplayCoordinator::IsPendingSmokeScreenshotReady() const
{
    if (PendingSmokeScreenshotPath.IsEmpty())
    {
        return false;
    }
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.FileSize(*PendingSmokeScreenshotPath) >= 10 * 1024;
}

void ASeedForgeGameplayCoordinator::CompleteGameplaySmoke()
{
    if (bSmokeExitRequested)
    {
        return;
    }
    GameplaySmokeStage = EGameplaySmokeStage::Complete;
    GameplaySmokeTrace.bSuccess = true;
    GameplaySmokeTrace.FailureCode.Reset();
    GameplaySmokeTrace.FailureMessage.Reset();
    GetWorldTimerManager().ClearTimer(GameplaySmokeTimer);
    GetWorldTimerManager().ClearTimer(GameplaySmokeWatchdogTimer);
    if (!WriteGameplaySmokeTrace())
    {
        FailGameplaySmoke(TEXT("TraceWriteFailed"), TEXT("Gameplay smoke could not write its JSON trace."));
        return;
    }
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS seed=%llu layout_hash=%llu encounter_hash=%llu trace=%s"),
        Seed,
        Layout.CanonicalHash,
        EncounterPlan.CanonicalHash,
        *GameplaySmokeTracePath);
    bSmokeExitRequested = true;
    FPlatformMisc::RequestExitWithStatus(false, 0);
}

void ASeedForgeGameplayCoordinator::FailGameplaySmoke(
    const TCHAR* FailureCode,
    const FString& FailureMessage)
{
    if (bSmokeExitRequested || GameplaySmokeStage == EGameplaySmokeStage::Failed)
    {
        return;
    }
    if (RunState.GetState() != ESeedForgeRunState::Failed)
    {
        EnterRunFailure(ESeedForgeRunFailureCode::SmokeFailed, FailureMessage, FailureCode);
        return;
    }
    GameplaySmokeStage = EGameplaySmokeStage::Failed;
    GameplaySmokeTrace.GitSha = GameplaySmokeGitSha;
    GameplaySmokeTrace.EngineVersion = FEngineVersion::Current().ToString();
    GameplaySmokeTrace.Seed = Seed;
    GameplaySmokeTrace.LayoutHash = Layout.CanonicalHash;
    GameplaySmokeTrace.EncounterHash = EncounterPlan.CanonicalHash;
    GameplaySmokeTrace.bSuccess = false;
    GameplaySmokeTrace.FailureCode = FailureCode;
    GameplaySmokeTrace.FailureMessage = FailureMessage;
    GameplaySmokeTrace.ActorCounts = {};
    if (GameplaySmokeTrace.StateTransitions.IsEmpty())
    {
        GameplaySmokeTrace.StateTransitions.Add(TEXT("Generating"));
    }
    GameplaySmokeTrace.StateTransitions.Add(TEXT("Failed"));
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(GameplaySmokeTimer);
        GetWorldTimerManager().ClearTimer(GameplaySmokeWatchdogTimer);
    }
    WriteGameplaySmokeTrace();
    UE_LOG(
        LogSeedForge,
        Error,
        TEXT("SEEDFORGE_GAMEPLAY_SMOKE_FAILURE code=%s message=%s"),
        FailureCode,
        *FailureMessage);
    bSmokeExitRequested = true;
    // A pre-first-frame Editor exit can discard PostQuitMessage's status. This
    // dedicated failing smoke process has synchronously cleaned its run and
    // saved its trace; force the requested failure code (and platform log flush).
    FPlatformMisc::RequestExitWithStatus(true, 2);
}

bool ASeedForgeGameplayCoordinator::WriteGameplaySmokeTrace()
{
    if (GameplaySmokeTracePath.IsEmpty())
    {
        return false;
    }
    GameplaySmokeTracePath = FPaths::ConvertRelativePathToFull(GameplaySmokeTracePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.CreateDirectoryTree(*FPaths::GetPath(GameplaySmokeTracePath)))
    {
        return false;
    }
    const FString Json = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(GameplaySmokeTrace)
        + LINE_TERMINATOR;
    return FFileHelper::SaveStringToFile(
        Json,
        *GameplaySmokeTracePath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

ASeedForgePlayerCharacter* ASeedForgeGameplayCoordinator::ResolvePlayer()
{
    using namespace SeedForge::GameplayCoordinator::Private;

    if (IsValid(Player))
    {
        return Player;
    }
    APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PlayerController)
    {
        Player = Cast<ASeedForgePlayerCharacter>(PlayerController->GetPawn());
    }
    if (!Player && GetWorld())
    {
        Player = GetWorld()->SpawnActor<ASeedForgePlayerCharacter>(
            FVector(0.0, 0.0, PlayerHeight),
            FRotator::ZeroRotator,
            SpawnParameters(this));
        if (PlayerController && Player)
        {
            PlayerController->Possess(Player);
        }
    }
    return Player;
}

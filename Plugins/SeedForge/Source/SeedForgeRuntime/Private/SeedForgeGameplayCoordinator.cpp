#include "SeedForgeGameplayCoordinator.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
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
    constexpr float EnemyMaxHealth = 50.0f;
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

void ASeedForgeGameplayCoordinator::StartRun(uint64 InSeed)
{
    check(IsInGameThread());
    ++RunGeneration;

    if (RunState.GetState() == ESeedForgeRunState::Restarting)
    {
        RunState.BeginGenerating();
    }
    else if (RunState.GetState() != ESeedForgeRunState::Generating)
    {
        if (!RunState.RequestRestart().IsSuccess()
            || !RunState.BeginGenerating().IsSuccess())
        {
            UE_LOG(LogSeedForge, Error, TEXT("Run state rejected restart before generation."));
            return;
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
    Seed = InSeed;
    PlayerHealth = Tuning.PlayerMaxHealth;
    NextAttackTime = 0.0;
    NextContactDamageTime = 0.0;

    USeedForgeWorldSubsystem* Subsystem = GetWorld()
        ? GetWorld()->GetSubsystem<USeedForgeWorldSubsystem>()
        : nullptr;
    if (!Subsystem)
    {
        UE_LOG(LogSeedForge, Error, TEXT("Gameplay run cannot find SeedForge world subsystem."));
        return;
    }
    ActiveRequestId = Subsystem->RequestGeneration(Seed, GenerationConfig);
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Gameplay queued run=%llu request=%llu seed=%llu."),
        RunGeneration,
        ActiveRequestId,
        Seed);
}

bool ASeedForgeGameplayCoordinator::ApplyGeneratedLayout(const FSeedForgeLayout& InLayout)
{
    using namespace SeedForge::GameplayCoordinator::Private;

    if (!GetWorld() || RunState.GetState() != ESeedForgeRunState::Generating)
    {
        return false;
    }
    const FSeedForgeValidationResult LayoutValidation = FSeedForgeValidator::Validate(
        InLayout,
        GenerationConfig);
    if (!LayoutValidation.IsValid())
    {
        UE_LOG(
            LogSeedForge,
            Error,
            TEXT("Gameplay rejected layout code=%d: %s"),
            static_cast<int32>(LayoutValidation.ErrorCode),
            *LayoutValidation.ErrorMessage);
        return false;
    }
    const FSeedForgeEncounterResult Planned = FSeedForgeEncounterPlanner::Generate(
        InLayout,
        EncounterConfig);
    if (!Planned.IsSuccess())
    {
        UE_LOG(
            LogSeedForge,
            Error,
            TEXT("Gameplay encounter failed code=%d: %s"),
            static_cast<int32>(Planned.ErrorCode),
            *Planned.ErrorMessage);
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
    if (!Visualization)
    {
        UE_LOG(LogSeedForge, Error, TEXT("Gameplay failed to spawn layout visualization."));
        return false;
    }
    Visualization->SetAutoGenerateOnBeginPlay(false);
    Visualization->FinishSpawning(VisualizationTransform);
    Visualization->ApplyLayout(Layout);

    Player = ResolvePlayer();
    if (!Player)
    {
        UE_LOG(LogSeedForge, Error, TEXT("Gameplay failed to resolve player Character."));
        ClearRunObjects();
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
        if (!Core)
        {
            UE_LOG(LogSeedForge, Error, TEXT("Gameplay failed to spawn Core %u."), CoreEntity.StableId);
            ClearRunObjects();
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
        if (!Enemy)
        {
            UE_LOG(LogSeedForge, Error, TEXT("Gameplay failed to spawn enemy %u."), EnemyEntity.StableId);
            ClearRunObjects();
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
    if (!ExitActor)
    {
        UE_LOG(LogSeedForge, Error, TEXT("Gameplay failed to spawn exit."));
        ClearRunObjects();
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
        UE_LOG(LogSeedForge, Error, TEXT("Gameplay state rejected Playing: %s"), *Started.ErrorMessage);
        ClearRunObjects();
        return false;
    }

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
        TEXT("Gameplay ready run=%llu seed=%llu layout_hash=%llu encounter_hash=%llu players=1 cores=%d enemies=%d exits=1."),
        RunGeneration,
        Seed,
        Layout.CanonicalHash,
        EncounterPlan.CanonicalHash,
        CoreActors.Num(),
        EnemyActors.Num());
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Applied request=%llu seed=%llu hash=%llu floors=%d walls=%d gameplay=true."),
        RunGeneration,
        Seed,
        Layout.CanonicalHash,
        Visualization->GetFloorInstanceCount(),
        Visualization->GetWallInstanceCount());
    if (!CapturePath.IsEmpty())
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
        if (IsValid(Enemy))
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
    Snapshot.RunState = RunState.GetState();
    Snapshot.PlayerHealth = PlayerHealth;
    Snapshot.PlayerMaxHealth = Tuning.PlayerMaxHealth;
    Snapshot.CollectedCoreCount = RunState.GetCollectedCoreCount();
    Snapshot.RequiredCoreCount = RunState.GetRequiredCoreCount();
    Snapshot.bExitUnlocked = RunState.IsExitUnlocked();
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
        Count += IsValid(Enemy) ? 1 : 0;
    }
    return Count;
}

void ASeedForgeGameplayCoordinator::BeginPlay()
{
    Super::BeginPlay();
    USeedForgeWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<USeedForgeWorldSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LogSeedForge, Error, TEXT("Gameplay startup cannot find SeedForge world subsystem."));
        return;
    }
    GenerationAppliedHandle = Subsystem->OnGenerationApplied().AddUObject(
        this,
        &ASeedForgeGameplayCoordinator::HandleGenerationApplied);

    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeSeed="), Seed);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridWidth="), GenerationConfig.GridWidth);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridHeight="), GenerationConfig.GridHeight);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeRoomCount="), GenerationConfig.RoomCount);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeCapturePath="), CapturePath);
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
    ClearRunObjects();
    Super::EndPlay(EndPlayReason);
}

void ASeedForgeGameplayCoordinator::HandleGenerationApplied(
    const FSeedForgeAsyncCompletion& Completion)
{
    if (Completion.RequestId != ActiveRequestId)
    {
        return;
    }
    ActiveRequestId = 0;
    if (!Completion.Result.IsSuccess())
    {
        UE_LOG(
            LogSeedForge,
            Error,
            TEXT("Gameplay generation failed code=%d: %s"),
            static_cast<int32>(Completion.Result.ErrorCode),
            *Completion.Result.ErrorMessage);
        return;
    }
    ApplyGeneratedLayout(Completion.Result.Layout);
}

void ASeedForgeGameplayCoordinator::ClearRunObjects()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InteractionTimer);
        World->GetTimerManager().ClearTimer(RepathTimer);
        World->GetTimerManager().ClearTimer(CaptureTimer);
        World->GetTimerManager().ClearTimer(CaptureExitTimer);
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
        Player->SetGameplayCoordinator(nullptr);
        Player->Destroy();
    }
    Player = nullptr;
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
        if (!IsValid(Enemy))
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
        if (IsValid(Enemy))
        {
            Enemy->ClearPath();
        }
    }
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

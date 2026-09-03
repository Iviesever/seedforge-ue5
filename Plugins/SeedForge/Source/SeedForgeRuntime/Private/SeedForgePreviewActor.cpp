#include "SeedForgePreviewActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "SeedForgeRuntime.h"
#include "SeedForgeValidator.h"
#include "SeedForgeVisualization.h"
#include "SeedForgeWorldSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UnrealClient.h"

ASeedForgePreviewActor::ASeedForgePreviewActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    FloorInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Floors"));
    FloorInstances->SetupAttachment(SceneRoot);
    FloorInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorInstances->SetMobility(EComponentMobility::Movable);

    WallInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Walls"));
    WallInstances->SetupAttachment(SceneRoot);
    WallInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WallInstances->SetMobility(EComponentMobility::Movable);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        FloorInstances->SetStaticMesh(CubeMesh.Object);
        WallInstances->SetStaticMesh(CubeMesh.Object);
    }
}

void ASeedForgePreviewActor::BeginPlay()
{
    Super::BeginPlay();

    uint64 Seed = 0x5EEDULL;
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeSeed="), Seed);

    FSeedForgeConfig Config;
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridWidth="), Config.GridWidth);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridHeight="), Config.GridHeight);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeRoomCount="), Config.RoomCount);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeCapturePath="), CapturePath);

    USeedForgeWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<USeedForgeWorldSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LogSeedForge, Error, TEXT("SeedForge world subsystem is unavailable."));
        return;
    }

    GenerationAppliedHandle = Subsystem->OnGenerationApplied().AddUObject(
        this,
        &ASeedForgePreviewActor::HandleGenerationApplied);
    RequestStartSeconds = FPlatformTime::Seconds();
    const uint64 RequestId = Subsystem->RequestGeneration(Seed, Config);
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Queued request=%llu seed=%llu grid=%dx%d rooms=%d."),
        RequestId,
        Seed,
        Config.GridWidth,
        Config.GridHeight,
        Config.RoomCount);
}

void ASeedForgePreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (USeedForgeWorldSubsystem* Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>())
        {
            Subsystem->OnGenerationApplied().Remove(GenerationAppliedHandle);
            Subsystem->CancelGeneration();
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ASeedForgePreviewActor::HandleGenerationApplied(const FSeedForgeAsyncCompletion& Completion)
{
    const double ElapsedMilliseconds = (FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0;
    if (!Completion.Result.IsSuccess())
    {
        UE_LOG(
            LogSeedForge,
            Error,
            TEXT("Request=%llu failed code=%d: %s"),
            Completion.RequestId,
            static_cast<int32>(Completion.Result.ErrorCode),
            *Completion.Result.ErrorMessage);
        return;
    }

    FSeedForgeConfig Config;
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridWidth="), Config.GridWidth);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeGridHeight="), Config.GridHeight);
    FParse::Value(FCommandLine::Get(), TEXT("SeedForgeRoomCount="), Config.RoomCount);
    const FSeedForgeValidationResult Validation =
        FSeedForgeValidator::Validate(Completion.Result.Layout, Config);
    if (!Validation.IsValid())
    {
        UE_LOG(
            LogSeedForge,
            Error,
            TEXT("Request=%llu produced invalid layout code=%d: %s"),
            Completion.RequestId,
            static_cast<int32>(Validation.ErrorCode),
            *Validation.ErrorMessage);
        return;
    }

    ApplyLayout(Completion.Result.Layout);
    UE_LOG(
        LogSeedForge,
        Display,
        TEXT("Applied request=%llu seed=%llu hash=%llu rooms=%d walkable=%d floors=%d walls=%d elapsed_ms=%.3f worker_thread=%u."),
        Completion.RequestId,
        Completion.Result.Layout.Seed,
        Completion.Result.Layout.CanonicalHash,
        Completion.Result.Layout.Rooms.Num(),
        Completion.Result.Layout.GetCanonicalWalkableCells().Num(),
        GetFloorInstanceCount(),
        GetWallInstanceCount(),
        ElapsedMilliseconds,
        Completion.WorkerThreadId);

    if (!CapturePath.IsEmpty())
    {
        GetWorldTimerManager().SetTimer(
            CaptureTimer,
            this,
            &ASeedForgePreviewActor::CaptureScreenshot,
            1.0f,
            false);
    }
}

void ASeedForgePreviewActor::ApplyLayout(const FSeedForgeLayout& Layout)
{
    FloorInstances->ClearInstances();
    WallInstances->ClearInstances();

    const FSeedForgeVisualizationPlan Plan = FSeedForgeVisualizationPlanner::Build(
        Layout,
        CellSize,
        FloorThickness,
        WallHeight,
        WallThickness);
    for (const FTransform& Transform : Plan.FloorTransforms)
    {
        FloorInstances->AddInstance(Transform);
    }
    for (const FTransform& Transform : Plan.WallTransforms)
    {
        WallInstances->AddInstance(Transform);
    }
}

int32 ASeedForgePreviewActor::GetFloorInstanceCount() const
{
    return FloorInstances->GetInstanceCount();
}

int32 ASeedForgePreviewActor::GetWallInstanceCount() const
{
    return WallInstances->GetInstanceCount();
}

void ASeedForgePreviewActor::CaptureScreenshot()
{
    CapturePath = FPaths::ConvertRelativePathToFull(CapturePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*FPaths::GetPath(CapturePath));
    FScreenshotRequest::RequestScreenshot(CapturePath, false, false);
    UE_LOG(LogSeedForge, Display, TEXT("Requested screenshot: %s"), *CapturePath);

    GetWorldTimerManager().SetTimer(
        ExitTimer,
        this,
        &ASeedForgePreviewActor::ExitAfterCapture,
        2.0f,
        false);
}

void ASeedForgePreviewActor::ExitAfterCapture()
{
    UE_LOG(LogSeedForge, Display, TEXT("Capture window complete; requesting clean exit."));
    FGenericPlatformMisc::RequestExit(false);
}


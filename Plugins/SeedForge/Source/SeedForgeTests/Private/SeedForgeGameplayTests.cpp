#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgeGameplayTypes.h"
#include "SeedForgePreviewActor.h"
#include "SeedForgeInputTestWorld.h"
#include "Engine/Canvas.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Slate/SceneViewport.h"
#include "CanvasTypes.h"
#include "Camera/CameraComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePlayerCameraReadabilityTest,
    "SeedForge.Gameplay.PlayerCameraReadability", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePlayerCameraReadabilityTest::RunTest(const FString& Parameters)
{
    const UCameraComponent* Camera = GetDefault<ASeedForgePlayerCharacter>()->FindComponentByClass<UCameraComponent>();
    TestNotNull(TEXT("Production player owns camera"), Camera);
    if (!Camera) { return false; }
    TestTrue(TEXT("Top-down camera explicitly overrides motion blur"), Camera->PostProcessSettings.bOverride_MotionBlurAmount);
    TestEqual(TEXT("Rotating player remains readable without motion streaks"), Camera->PostProcessSettings.MotionBlurAmount, 0.0f);
    TestTrue(TEXT("Camera postprocess settings are active"), Camera->PostProcessBlendWeight > 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeHudOwnershipTest,
    "SeedForge.Gameplay.Hud.OverlayOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeHudOwnershipTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(Fixture.World);
    UGameViewportClient* Previous = Context.GameViewport;
    UGameViewportClient* Client = NewObject<UGameViewportClient>(GEngine);
    Client->AddToRoot();
    Context.GameViewport = Client;
    const TSharedRef<SViewport> ViewportWidget = SNew(SViewport);
    TSharedPtr<FSceneViewport> Viewport = FSceneViewport::Create(TStrongPtrVariant<FViewportClient>(Client), ViewportWidget);
    TFunction<int32(const TSharedRef<SWidget>&)> CountHud = [&CountHud, this](const TSharedRef<SWidget>& Widget)
    {
        int32 Count = Widget->GetTag() == TEXT("SeedForgeHUD") ? 1 : 0;
        if (Count == 1)
        {
            TestEqual(TEXT("HUD cannot consume input"), Widget->GetVisibility(), EVisibility::HitTestInvisible);
        }
        FChildren* Children = Widget->GetChildren();
        for (int32 Index = 0; Index < Children->Num(); ++Index)
        {
            Count += CountHud(Children->GetChildAt(Index));
        }
        return Count;
    };
    ASeedForgeHUD* Hud = Fixture.World->SpawnActor<ASeedForgeHUD>();
    Hud->DispatchBeginPlay();
    UCanvas* TestCanvas = NewObject<UCanvas>(Hud);
    FCanvas DrawCanvas(nullptr, nullptr, Fixture.World, Fixture.World->GetFeatureLevel());
    TestCanvas->Canvas = &DrawCanvas;
    Hud->SetCanvas(TestCanvas, TestCanvas);
    Hud->DrawHUD();
    TestEqual(TEXT("Real viewport overlay receives exactly one HUD"), CountHud(ViewportWidget), 1);
    Hud->DrawHUD();
    TestEqual(TEXT("Repeated draw does not duplicate overlay"), CountHud(ViewportWidget), 1);
    Fixture.Coordinator->RestartSameSeed();
    Hud->DrawHUD();
    TestEqual(TEXT("Run restart keeps one HUD"), CountHud(ViewportWidget), 1);
    Hud->bShowHUD = false;
    Hud->PostRender();
    TestEqual(TEXT("Standard hidden HUD removes persistent overlay"), CountHud(ViewportWidget), 0);
    Hud->bShowHUD = true;
    Hud->DrawHUD();
    TestEqual(TEXT("HUD can show again exactly once"), CountHud(ViewportWidget), 1);
    Hud->SetCanvas(TestCanvas, nullptr);
    Hud->bShowDebugInfo = true;
    Hud->PostRender();
    TestEqual(TEXT("Debug-only mode removes gameplay overlay"), CountHud(ViewportWidget), 0);
    Hud->bShowDebugInfo = false;
    Hud->DrawHUD();
    TestEqual(TEXT("HUD returns once after debug mode"), CountHud(ViewportWidget), 1);
    Hud->Destroy(true);
    TestEqual(TEXT("EndPlay removes owned overlay"), CountHud(ViewportWidget), 0);
    TestCanvas->Canvas = nullptr;
    Viewport.Reset();
    Context.GameViewport = Previous;
    Client->RemoveFromRoot();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeHudContentTest,
    "SeedForge.Gameplay.Hud.Content", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeHudContentTest::RunTest(const FString& Parameters)
{
    FSeedForgeGameplaySnapshot Snapshot;
    Snapshot.Seed = MAX_uint64;
    Snapshot.PlayerHealth = 80.0f;
    Snapshot.PlayerMaxHealth = 100.0f;
    Snapshot.CollectedCoreCount = 2;
    Snapshot.RequiredCoreCount = 3;
    Snapshot.RunState = ESeedForgeRunState::Playing;
    const FString Text = FSeedForgeGameplayPresentation::BuildHudText(Snapshot);
    TestTrue(TEXT("Title retained"), Text.Contains(TEXT("SEEDFORGE // DETERMINISTIC EXTRACTION")));
    TestTrue(TEXT("HP is from snapshot"), Text.Contains(TEXT("HP  80 / 100")));
    TestTrue(TEXT("Core progress is from snapshot"), Text.Contains(TEXT("DATA CORES  2 / 3")));
    TestTrue(TEXT("Full uint64 seed is preserved"), Text.Contains(TEXT("SEED  18446744073709551615")));
    TestTrue(TEXT("Lock state retained"), Text.Contains(TEXT("EXIT  LOCKED")));
    TestTrue(TEXT("Movement and combat guidance retained"), Text.Contains(TEXT("WASD Move   Mouse Aim   LMB Attack   Space Dash")));
    TestTrue(TEXT("Persistent restart controls retained"), Text.Contains(TEXT("R Restart Same Seed   N New Seed")));
    TestFalse(TEXT("No invented failure"), Text.Contains(TEXT("FAILURE")));
    Snapshot.bExitUnlocked = true;
    TestTrue(TEXT("Unlocked exit reflects snapshot"), FSeedForgeGameplayPresentation::BuildHudText(Snapshot).Contains(TEXT("EXIT  UNLOCKED")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeHudStatesTest,
    "SeedForge.Gameplay.Hud.States", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeHudStatesTest::RunTest(const FString& Parameters)
{
    const ESeedForgeRunState States[] = {ESeedForgeRunState::Generating, ESeedForgeRunState::Playing,
        ESeedForgeRunState::Won, ESeedForgeRunState::Lost, ESeedForgeRunState::Restarting, ESeedForgeRunState::Failed};
    const TCHAR* Labels[] = {TEXT("GENERATING"), TEXT("PLAYING"), TEXT("EXTRACTED - YOU WIN"),
        TEXT("RUN LOST"), TEXT("RESTARTING"), TEXT("RUN FAILED")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(States); ++Index)
    {
        FSeedForgeGameplaySnapshot Snapshot;
        Snapshot.RunState = States[Index];
        const FString Text = FSeedForgeGameplayPresentation::BuildHudText(Snapshot);
        TestTrue(Labels[Index], Text.Contains(FString(TEXT("RUN  ")) + Labels[Index]));
        const bool bTerminal = Index == 2 || Index == 3 || Index == 5;
        TestEqual(TEXT("Replay hint is terminal only"), Text.Contains(TEXT("Press R to replay this layout or N for a new run")), bTerminal);
    }
    FSeedForgeGameplaySnapshot Failure;
    Failure.RunState = ESeedForgeRunState::Failed;
    Failure.FailureCode = ESeedForgeRunFailureCode::EncounterFailed;
    Failure.FailureMessage = TEXT("Insufficient walkable cells");
    TestTrue(TEXT("Typed failure includes details"), FSeedForgeGameplayPresentation::BuildHudText(Failure).Contains(
        TEXT("FAILURE  EncounterFailed: Insufficient walkable cells")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplayCellMappingTest,
    "SeedForge.Gameplay.CellMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplayCellMappingTest::RunTest(const FString& Parameters)
{
    const FIntPoint Cell(7, 11);
    const FVector World = FSeedForgeGameplayMath::CellToWorld(Cell, 200.0f, 90.0f);
    TestEqual(TEXT("Cell X maps to world X"), World.X, 1400.0);
    TestEqual(TEXT("Cell Y maps to world Y"), World.Y, 2200.0);
    TestEqual(TEXT("Requested height maps to world Z"), World.Z, 90.0);

    FIntPoint Nearest = FIntPoint::ZeroValue;
    const TArray<FIntPoint> Walkable = {FIntPoint(0, 1), FIntPoint(1, 0), Cell};
    TestTrue(TEXT("Nearest walkable lookup succeeds"), FSeedForgeGameplayMath::WorldToNearestWalkableCell(
        FVector(100.0, 100.0, 500.0), Walkable, 200.0f, Nearest));
    TestEqual(TEXT("Equal-distance tie uses canonical Y/X"), Nearest, FIntPoint(1, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplayStableAttackSelectionTest,
    "SeedForge.Gameplay.StableAttackSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplayStableAttackSelectionTest::RunTest(const FString& Parameters)
{
    const TArray<FSeedForgeAttackCandidate> Candidates = {
        {9, FVector(200.0, 0.0, 0.0), true},
        {3, FVector(200.0, 10.0, 0.0), true},
        {1, FVector(100.0, 0.0, 0.0), false},
        {0, FVector(-50.0, 0.0, 0.0), true}};
    const int32 Selected = FSeedForgeGameplayMath::SelectAttackTarget(
        FVector::ZeroVector,
        FVector::ForwardVector,
        300.0f,
        0.5f,
        Candidates);

    TestEqual(TEXT("First live in-arc stable ID wins"), Selected, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplayRejectsInvalidAttackTest,
    "SeedForge.Gameplay.RejectsInvalidAttack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplayRejectsInvalidAttackTest::RunTest(const FString& Parameters)
{
    const TArray<FSeedForgeAttackCandidate> Candidates = {
        {0, FVector(100.0, 0.0, 0.0), true}};
    TestEqual(TEXT("Zero forward vector rejects attack"), FSeedForgeGameplayMath::SelectAttackTarget(
        FVector::ZeroVector, FVector::ZeroVector, 300.0f, 0.5f, Candidates), INDEX_NONE);
    TestEqual(TEXT("Non-positive range rejects attack"), FSeedForgeGameplayMath::SelectAttackTarget(
        FVector::ZeroVector, FVector::ForwardVector, 0.0f, 0.5f, Candidates), INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplayCodeNativeFrameworkTest,
    "SeedForge.Gameplay.CodeNativeFramework",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplayCodeNativeFrameworkTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Player is a native Character"), ASeedForgePlayerCharacter::StaticClass()->IsChildOf(ACharacter::StaticClass()));
    TestTrue(TEXT("Controller is code native"), ASeedForgePlayerController::StaticClass()->IsChildOf(APlayerController::StaticClass()));
    TestTrue(TEXT("Enemy is a native Pawn"), ASeedForgeEnemyPawn::StaticClass()->IsChildOf(APawn::StaticClass()));
    TestTrue(TEXT("HUD is code native"), ASeedForgeHUD::StaticClass()->IsChildOf(AHUD::StaticClass()));

    const ASeedForgePlayerCharacter* PlayerDefaults = GetDefault<ASeedForgePlayerCharacter>();
    TestTrue(TEXT("Player owns a top-down camera"), PlayerDefaults->HasTopDownCamera());
    TestTrue(TEXT("Attack has a visible cooldown boundary"), PlayerDefaults->GetAttackCooldownSeconds() > 0.0f);
    TestTrue(TEXT("Dash has a cooldown boundary"), PlayerDefaults->GetDashCooldownSeconds() > 0.0f);

    ASeedForgePreviewActor* PreviewDefaults = GetMutableDefault<ASeedForgePreviewActor>();
    PreviewDefaults->SetAutoGenerateOnBeginPlay(false);
    TestFalse(TEXT("Preview supports apply-only mode"), PreviewDefaults->IsAutoGenerateOnBeginPlay());
    PreviewDefaults->SetAutoGenerateOnBeginPlay(true);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplayCoordinatorWorldTest,
    "SeedForge.Gameplay.CoordinatorWorld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplayCoordinatorWorldTest::RunTest(const FString& Parameters)
{
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    UWorld* World = UWorld::CreateWorld(
        EWorldType::Game,
        false,
        MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None, EUniqueObjectNameOptions::GloballyUnique),
        GetTransientPackage());
    TestNotNull(TEXT("Transient gameplay World is created"), World);
    if (!World)
    {
        GEngine->DestroyWorldContext(World);
        return false;
    }
    World->AddToRoot();
    WorldContext.SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());

    ASeedForgeGameplayCoordinator* Coordinator = World->SpawnActor<ASeedForgeGameplayCoordinator>();
    const FSeedForgeResult Generated = FSeedForgeGenerator::Generate(24301, {});
    TestNotNull(TEXT("Coordinator spawns in real World"), Coordinator);
    TestTrue(TEXT("Deterministic layout generates"), Generated.IsSuccess());
    if (Coordinator && Generated.IsSuccess())
    {
        TestTrue(TEXT("Coordinator accepts generated layout"), Coordinator->ApplyGeneratedLayout(Generated.Layout));
        const FSeedForgeGameplaySnapshot Playing = Coordinator->GetSnapshot();
        TestEqual(TEXT("Coordinator enters Playing"), Playing.RunState, ESeedForgeRunState::Playing);
        TestEqual(TEXT("Coordinator spawns three Core actors"), Coordinator->GetLiveCoreActorCount(), 3);
        TestEqual(TEXT("Coordinator spawns five Enemy actors"), Coordinator->GetLiveEnemyActorCount(), 5);
        TestEqual(TEXT("Player starts at full HP"), Playing.PlayerHealth, Playing.PlayerMaxHealth);
        TestTrue(TEXT("Lethal damage is accepted"), Coordinator->ApplyPlayerDamage(Playing.PlayerMaxHealth));
        TestEqual(TEXT("Lethal damage enters Lost"), Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Lost);
    }

    if (Coordinator)
    {
        Coordinator->Destroy(true);
    }
    GEngine->ShutdownWorldNetDriver(World);
    World->DestroyWorld(true);
    World->SetPhysicsScene(nullptr);
    GEngine->DestroyWorldContext(World);
    World->RemoveFromRoot();
    return true;
}

#endif

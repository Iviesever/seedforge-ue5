#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgeGameplayTypes.h"
#include "SeedForgePreviewActor.h"

#if WITH_DEV_AUTOMATION_TESTS

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

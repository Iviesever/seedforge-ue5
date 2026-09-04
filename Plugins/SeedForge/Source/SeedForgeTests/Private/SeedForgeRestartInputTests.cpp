#include "Misc/AutomationTest.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerState.h"
#include "InputKeyEventArgs.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeWorldSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::RestartInputTests
{
    int32 CountAction(const UInputComponent* Input, const FName Name)
    {
        int32 Count = 0;
        if (Input)
        {
            for (int32 Index = 0; Index < Input->GetNumActionBindings(); ++Index)
            {
                Count += Input->GetActionBinding(Index).GetActionName() == Name ? 1 : 0;
            }
        }
        return Count;
    }

    struct FWorldFixture
    {
        UWorld* World;
        ASeedForgePlayerController* Controller;
        ASeedForgeGameplayCoordinator* Coordinator;
        USeedForgeWorldSubsystem* Subsystem;

        FWorldFixture()
        {
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None,
                    EUniqueObjectNameOptions::GloballyUnique), GetTransientPackage());
            World->AddToRoot();
            Context.SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            Controller = World->SpawnActor<ASeedForgePlayerController>();
            Controller->SetAsLocalPlayerController();
            Controller->SetPlayerState(World->SpawnActor<APlayerState>());
            Controller->InitInputSystem();
            Coordinator = World->SpawnActor<ASeedForgeGameplayCoordinator>();
            Coordinator->DispatchBeginPlay();
            Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>();
        }

        ~FWorldFixture()
        {
            Coordinator->Destroy(true);
            Controller->Destroy(true);
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(true);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
            World->RemoveFromRoot();
        }

        void Dispatch(const FKey& Key, EInputEvent Event)
        {
            if (!IsValid(Controller) || Controller->IsActorBeingDestroyed()
                || !Controller->PlayerInput || !Controller->InputComponent)
            {
                return;
            }
            TArray<UInputComponent*> Stack;
            if (APawn* Pawn = Controller->GetPawn())
            {
                if (Pawn->InputComponent)
                {
                    Stack.Add(Pawn->InputComponent);
                }
            }
            Stack.Add(Controller->InputComponent);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                Key, Event, Event == IE_Released ? 0.0f : 1.0f));
            Controller->PlayerInput->ProcessInputStack(Stack, 1.0f / 60.0f, false);
        }

        void Press(const FKey& Key)
        {
            Dispatch(Key, IE_Pressed);
            Dispatch(Key, IE_Released);
        }

        void Apply(uint64 Request, uint64 Seed)
        {
            FSeedForgeAsyncCompletion Completion;
            Completion.RequestId = Request;
            Completion.Result = FSeedForgeGenerator::Generate(Seed, {});
            Subsystem->OnGenerationApplied().Broadcast(Completion);
        }

        void ApplyCurrent()
        {
            const auto Snapshot = Coordinator->GetSnapshot();
            Apply(Snapshot.PendingRequestId, Snapshot.Seed);
            if (APawn* Pawn = Controller->GetPawn())
            {
                if (!Pawn->InputComponent)
                {
                    Pawn->PawnClientRestart();
                }
            }
        }

        int32 OwnedLiveActors() const
        {
            int32 Count = 0;
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                const bool bOwnedRunActor = It->GetOwner() == Coordinator
                    || (It->IsA<ASeedForgePlayerCharacter>() && It->GetOwner() == Controller);
                Count += bOwnedRunActor && !It->IsActorBeingDestroyed() ? 1 : 0;
            }
            return Count;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeRestartBindingOwnershipTest,
    "SeedForge.Audit.RestartInput.PersistentBindingsExactlyOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRestartBindingOwnershipTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::RestartInputTests;
    FWorldFixture Fixture;
    Fixture.Controller->InitInputSystem();
    TestEqual(TEXT("Controller owns one R binding"), CountAction(Fixture.Controller->InputComponent, TEXT("RestartSameSeed")), 1);
    TestEqual(TEXT("Controller owns one N binding"), CountAction(Fixture.Controller->InputComponent, TEXT("StartNewSeed")), 1);
    TestNull(TEXT("Generating controller has no pawn"), Fixture.Controller->GetPawn());
    const auto Before = Fixture.Coordinator->GetSnapshot();
    Fixture.Press(EKeys::N);
    const auto After = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("One N dispatch starts exactly one run"), After.RunGeneration, Before.RunGeneration + 1);
    TestEqual(TEXT("One N applies next-seed transform once"), After.Seed,
        Before.Seed * 6364136223846793005ULL + 1442695040888963407ULL);
    TestTrue(TEXT("N while unpossessed supersedes request"), After.PendingRequestId > Before.PendingRequestId);
    Fixture.ApplyCurrent();
    APawn* Pawn = Fixture.Controller->GetPawn();
    TestNotNull(TEXT("Successful current apply possesses a player"), Pawn);
    if (Pawn)
    {
        TestNotNull(TEXT("Player input stack is initialized"), Pawn->InputComponent.Get());
        TestEqual(TEXT("Pawn has no duplicate R binding"), CountAction(Pawn->InputComponent, TEXT("RestartSameSeed")), 0);
        TestEqual(TEXT("Pawn has no duplicate N binding"), CountAction(Pawn->InputComponent, TEXT("StartNewSeed")), 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeRapidRestartInputTest,
    "SeedForge.Audit.RestartInput.RapidKeysIgnoreStaleApplyAndPossessOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRapidRestartInputTest::RunTest(const FString& Parameters)
{
    SeedForge::RestartInputTests::FWorldFixture Fixture;
    Fixture.ApplyCurrent();
    TWeakObjectPtr<APawn> OldPawn = Fixture.Controller->GetPawn();
    const auto Before = Fixture.Coordinator->GetSnapshot();
    Fixture.Press(EKeys::N);
    const auto First = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("N starts exactly one request from a possessed pawn"), First.RunGeneration, Before.RunGeneration + 1);
    TestTrue(TEXT("PlayerController remains alive across pawn destruction"),
        IsValid(Fixture.Controller) && !Fixture.Controller->IsActorBeingDestroyed());
    TestNotNull(TEXT("Persistent Controller retains PlayerInput"), Fixture.Controller->PlayerInput.Get());
    TestNull(TEXT("Restart unpossesses destroyed player immediately"), Fixture.Controller->GetPawn());
    TestTrue(TEXT("Old player is destroyed"), !OldPawn.IsValid() || OldPawn->IsActorBeingDestroyed());
    TestEqual(TEXT("No old owned actors remain"), Fixture.OwnedLiveActors(), 0);
    Fixture.Press(EKeys::R);
    const auto Newest = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("R while Generating starts exactly one newer run"), Newest.RunGeneration, First.RunGeneration + 1);
    TestTrue(TEXT("Rapid R supersedes N request"), Newest.PendingRequestId > First.PendingRequestId);
    TestEqual(TEXT("R retains pending seed"), Newest.Seed, First.Seed);
    Fixture.Apply(First.PendingRequestId, First.Seed);
    TestEqual(TEXT("Stale success does not populate new run"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Generating);
    TestNull(TEXT("Stale success does not possess a player"), Fixture.Controller->GetPawn());
    Fixture.ApplyCurrent();
    TestEqual(TEXT("Newest request reaches Playing"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Playing);
    TestTrue(TEXT("Exactly one fresh pawn is possessed"), Fixture.Controller->GetPawn() && Fixture.Controller->GetPawn() != OldPawn.Get());
    TestEqual(TEXT("Exactly one complete run object set exists"), Fixture.OwnedLiveActors(), 11);
    TestEqual(TEXT("New run resets Core eligibility"), Fixture.Coordinator->GetSnapshot().CollectedCoreCount, 0);
    TestEqual(TEXT("New run resets HP"), Fixture.Coordinator->GetSnapshot().PlayerHealth, Fixture.Coordinator->GetTuning().PlayerMaxHealth);
    const uint64 StableRun = Fixture.Coordinator->GetSnapshot().RunGeneration;
    Fixture.Dispatch(EKeys::R, IE_Repeat);
    Fixture.Dispatch(EKeys::R, IE_Released);
    TestEqual(TEXT("Key repeat does not dispatch a second press action"), Fixture.Coordinator->GetSnapshot().RunGeneration, StableRun);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeFailedRunInputRecoveryTest,
    "SeedForge.Audit.RestartInput.FailedUnpossessedRunRecoversThroughR",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeFailedRunInputRecoveryTest::RunTest(const FString& Parameters)
{
    SeedForge::RestartInputTests::FWorldFixture Fixture;
    FSeedForgeAsyncCompletion Failure;
    Failure.RequestId = Fixture.Coordinator->GetSnapshot().PendingRequestId;
    Failure.Result = FSeedForgeResult::Failure(ESeedForgeErrorCode::PlacementExhausted, TEXT("audit recovery"));
    AddExpectedError(TEXT("Gameplay run failed code=GenerationFailed"), EAutomationExpectedErrorFlags::Contains, 1);
    Fixture.Subsystem->OnGenerationApplied().Broadcast(Failure);
    TestEqual(TEXT("Failure entered Failed"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Failed);
    TestNull(TEXT("Failed run has no pawn"), Fixture.Controller->GetPawn());
    Fixture.Press(EKeys::R);
    TestEqual(TEXT("Controller R recovers into Generating"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Generating);
    TestTrue(TEXT("Recovery has a live request"), Fixture.Coordinator->GetSnapshot().PendingRequestId != 0);
    Fixture.ApplyCurrent();
    TestEqual(TEXT("Recovered current apply is Playing"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Playing);
    TestNotNull(TEXT("Recovered controller possesses player"), Fixture.Controller->GetPawn().Get());
    return true;
}

#endif

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgeGenerator.h"
#include "SeedForgePreviewActor.h"
#include "SeedForgeWorldSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::RunFailureTests
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        ASeedForgeGameplayCoordinator* Coordinator = nullptr;

        FWorldFixture()
        {
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None,
                    EUniqueObjectNameOptions::GloballyUnique), GetTransientPackage());
            World->AddToRoot();
            Context.SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            Coordinator = World->SpawnActor<ASeedForgeGameplayCoordinator>();
        }

        ~FWorldFixture()
        {
            Coordinator->Destroy(true);
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(true);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
            World->RemoveFromRoot();
        }

        USeedForgeWorldSubsystem* Begin()
        {
            Coordinator->DispatchBeginPlay();
            return World->GetSubsystem<USeedForgeWorldSubsystem>();
        }

        int32 OwnedLiveActors() const
        {
            int32 Count = 0;
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                Count += It->GetOwner() == Coordinator && !It->IsActorBeingDestroyed() ? 1 : 0;
            }
            return Count;
        }
    };

    FSeedForgeAsyncCompletion FailedCompletion(uint64 RequestId)
    {
        FSeedForgeAsyncCompletion Completion;
        Completion.RequestId = RequestId;
        Completion.Result = FSeedForgeResult::Failure(
            ESeedForgeErrorCode::InvalidGridSize, TEXT("audit generation failure"));
        return Completion;
    }

    FSeedForgeAsyncCompletion SuccessCompletion(uint64 RequestId, uint64 Seed)
    {
        FSeedForgeAsyncCompletion Completion;
        Completion.RequestId = RequestId;
        Completion.Result = FSeedForgeGenerator::Generate(Seed, {});
        return Completion;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeFailureStateLifecycleTest,
    "SeedForge.Audit.RunFailure.StateLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeFailureStateLifecycleTest::RunTest(const FString& Parameters)
{
    FSeedForgeRunStateMachine Run;
    TestTrue(TEXT("Generating failure accepted"), Run.FailRun().IsSuccess());
    TestEqual(TEXT("Failure has explicit terminal state"), Run.GetState(), ESeedForgeRunState::Failed);
    TestTrue(TEXT("Repeated failure is idempotent"), Run.FailRun().IsSuccess());
    TestTrue(TEXT("Failed run may restart"), Run.RequestRestart().IsSuccess());
    TestTrue(TEXT("Restart returns to Generating"), Run.BeginGenerating().IsSuccess());
    TestTrue(TEXT("Recovered run starts"), Run.StartPlaying({0, 1}).IsSuccess());
    TestTrue(TEXT("Recovered run collects"), Run.CollectCore(0).IsSuccess());
    TestTrue(TEXT("Playing failure accepted"), Run.FailRun().IsSuccess());
    TestEqual(TEXT("Failure clears collected Core eligibility"), Run.GetCollectedCoreCount(), 0);
    TestEqual(TEXT("Failure clears required Core eligibility"), Run.GetRequiredCoreCount(), 0);
    TestFalse(TEXT("Failed exit remains locked"), Run.IsExitUnlocked());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeMatchingFailureTest,
    "SeedForge.Audit.RunFailure.MatchingGenerationClosesAndRecovers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeMatchingFailureTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::RunFailureTests;
    FWorldFixture Fixture;
    USeedForgeWorldSubsystem* Subsystem = Fixture.Begin();
    const uint64 Request = Fixture.Coordinator->StartRun(24301);
    TestNotEqual(TEXT("Request has a real subsystem identity"), Request, 0ULL);
    AddExpectedError(TEXT("Gameplay (generation failed|run failed)"),
        EAutomationExpectedErrorFlags::Contains, 1);
    Subsystem->OnGenerationApplied().Broadcast(FailedCompletion(Request));
    const FSeedForgeGameplaySnapshot Failed = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("Matching failure is not abandoned Generating"), Failed.RunState, ESeedForgeRunState::Failed);
    TestEqual(TEXT("Matching failure is typed"), Failed.FailureCode, ESeedForgeRunFailureCode::GenerationFailed);
    TestTrue(TEXT("Original failure detail is retained"), Failed.FailureMessage.Contains(TEXT("audit generation failure")));
    TestEqual(TEXT("Failure owns no live actors"), Fixture.OwnedLiveActors(), 0);
    TestEqual(TEXT("Failure has no live gameplay HP"), Failed.PlayerHealth, 0.0f);
    Subsystem->OnGenerationApplied().Broadcast(FailedCompletion(Request));
    const uint64 Recovery = Fixture.Coordinator->StartRun(24301);
    TestTrue(TEXT("Recovery assigns a newer request"), Recovery > Request);
    Subsystem->OnGenerationApplied().Broadcast(SuccessCompletion(Recovery, 24301));
    TestEqual(TEXT("Recovered apply is Playing"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Playing);
    TestEqual(TEXT("Recovery clears failure detail"), Fixture.Coordinator->GetSnapshot().FailureCode, ESeedForgeRunFailureCode::None);
    TestEqual(TEXT("Recovery has exactly three Cores"), Fixture.Coordinator->GetLiveCoreActorCount(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeInvalidLayoutFailureTest,
    "SeedForge.Audit.RunFailure.InvalidLayoutCloses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInvalidLayoutFailureTest::RunTest(const FString& Parameters)
{
    SeedForge::RunFailureTests::FWorldFixture Fixture;
    AddExpectedError(TEXT("Gameplay (rejected layout|run failed)"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Invalid layout apply rejected"), Fixture.Coordinator->ApplyGeneratedLayout({}));
    const FSeedForgeGameplaySnapshot Snapshot = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("Invalid layout closes run"), Snapshot.RunState, ESeedForgeRunState::Failed);
    TestEqual(TEXT("Invalid layout failure is typed"), Snapshot.FailureCode, ESeedForgeRunFailureCode::InvalidLayout);
    TestEqual(TEXT("Invalid apply leaves no owned actors"), Fixture.OwnedLiveActors(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeEncounterFailureTest,
    "SeedForge.Audit.RunFailure.EncounterFailureCloses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEncounterFailureTest::RunTest(const FString& Parameters)
{
    SeedForge::RunFailureTests::FWorldFixture Fixture;
    FSeedForgeLayout Layout = FSeedForgeGenerator::Generate(24301, {}).Layout;
    Layout.Exit = Layout.Entrance;
    Layout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(Layout);
    AddExpectedError(TEXT("Gameplay (encounter failed|run failed)"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Impossible encounter rejected"), Fixture.Coordinator->ApplyGeneratedLayout(Layout));
    TestEqual(TEXT("Encounter failure closes run"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Failed);
    TestEqual(TEXT("Encounter failure typed"), Fixture.Coordinator->GetSnapshot().FailureCode, ESeedForgeRunFailureCode::EncounterFailed);
    TestEqual(TEXT("Encounter failure leaves no owned actors"), Fixture.OwnedLiveActors(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSpawnRollbackFailureTest,
    "SeedForge.Audit.RunFailure.PartialSpawnRollsBack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeSpawnRollbackFailureTest::RunTest(const FString& Parameters)
{
    SeedForge::RunFailureTests::FWorldFixture Fixture;
    bool bRejectedCore = false;
    const FDelegateHandle Handle = Fixture.World->AddOnActorSpawnedHandler(
        FOnActorSpawned::FDelegate::CreateLambda([&bRejectedCore](AActor* Actor)
        {
            if (!bRejectedCore && Actor->IsA<ASeedForgeCorePickup>())
            {
                bRejectedCore = true;
                Actor->Destroy(true);
            }
        }));
    AddExpectedError(TEXT("Gameplay (failed to spawn Core|run failed)"), EAutomationExpectedErrorFlags::Contains, 1);
    const bool bApplied = Fixture.Coordinator->ApplyGeneratedLayout(FSeedForgeGenerator::Generate(24301, {}).Layout);
    Fixture.World->RemoveOnActorSpawnedHandler(Handle);
    TestTrue(TEXT("Real World spawn rejection executed"), bRejectedCore);
    TestFalse(TEXT("Destroyed-on-spawn required actor rejects apply"), bApplied);
    TestEqual(TEXT("Spawn failure closes run"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Failed);
    TestEqual(TEXT("Spawn failure is typed"), Fixture.Coordinator->GetSnapshot().FailureCode, ESeedForgeRunFailureCode::SpawnFailed);
    TestEqual(TEXT("Partial actor set fully rolled back"), Fixture.OwnedLiveActors(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeStaleFailureTest,
    "SeedForge.Audit.RunFailure.StaleAndZeroCompletionsIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeStaleFailureTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::RunFailureTests;
    FWorldFixture Fixture;
    USeedForgeWorldSubsystem* Subsystem = Fixture.Begin();
    const uint64 Old = Fixture.Coordinator->StartRun(101);
    const uint64 Current = Fixture.Coordinator->StartRun(202);
    Subsystem->OnGenerationApplied().Broadcast(FailedCompletion(Old));
    TestEqual(TEXT("Stale failure preserves active generation"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Generating);
    TestEqual(TEXT("Stale failure preserves new seed"), Fixture.Coordinator->GetSnapshot().Seed, 202ULL);
    Subsystem->OnGenerationApplied().Broadcast(SuccessCompletion(Current, 202));
    Subsystem->OnGenerationApplied().Broadcast(FailedCompletion(0));
    TestEqual(TEXT("Zero request cannot fail an applied run"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Playing);
    TestEqual(TEXT("Newest run identity remains applied"), Fixture.Coordinator->GetLayout().Seed, 202ULL);
    return true;
}

#endif

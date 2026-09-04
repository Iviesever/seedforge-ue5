#include "SeedForgeInputTestWorld.h"
#include "SeedForgeGameplayDiagnostics.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeLossSameSeedObservationsTest,
    "SeedForge.Audit.RunIntegration.LostSameSeedFreshOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeLossSameSeedObservationsTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    Fixture.ApplyCurrent();
    const auto Before = Fixture.Coordinator->GetSnapshot();
    const auto OldActors = Fixture.OwnedRunActors();
    TArray<FSeedForgeRunTransitionObservation> Transitions;
    const auto Handle = Fixture.Coordinator->OnRunStateChanged().AddLambda(
        [&](ESeedForgeRunState From, ESeedForgeRunState To, const FSeedForgeGameplaySnapshot& Snapshot)
        {
            Transitions.Add({From, To, Snapshot});
        });
    const auto Resources = Fixture.Coordinator->GetRunResourceSnapshot();
    TestTrue(TEXT("Playing owns the normal interaction timer"), Resources.bInteractionTimerActive);
    TestTrue(TEXT("Playing owns the normal replanning timer"), Resources.bRepathTimerActive);
    Fixture.Press(EKeys::SpaceBar);
    auto* OldPlayer = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
    TestTrue(TEXT("Dispatched Dash establishes a real cooldown"), OldPlayer->GetDashCooldownRemaining() > 0);
    TestTrue(TEXT("Public lethal damage is accepted"), Fixture.Coordinator->ApplyPlayerDamage(Before.PlayerHealth));
    TestEqual(TEXT("Public damage enters Lost"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Lost);
    Fixture.Press(EKeys::R);
    const auto Pending = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("R preserves same seed"), Pending.Seed, Before.Seed);
    TestEqual(TEXT("Pending same-seed run clears applied hash"), Pending.LayoutHash, 0ULL);
    TestEqual(TEXT("Pending same-seed run clears request identity"), Pending.AppliedRequestId, 0ULL);
    TestNull(TEXT("Persistent controller is unpossessed during generation"), Fixture.Controller->GetPawn());
    TestEqual(TEXT("Restart leaves no live owned run objects"), Fixture.OwnedLiveActors(), 0);
    Fixture.ApplyCurrent(); // Controlled World fixture delivery; not packaged async proof.
    const auto After = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("Same seed restores layout hash"), After.LayoutHash, Before.LayoutHash);
    TestEqual(TEXT("Same seed restores encounter hash"), After.EncounterHash, Before.EncounterHash);
    TestEqual(TEXT("Same seed stores actual new request"), After.AppliedRequestId, Pending.PendingRequestId);
    TestEqual(TEXT("Fresh complete run has one owned actor set"), Fixture.OwnedLiveActors(), 11);
    TestEqual(TEXT("Fresh run resets HP"), After.PlayerHealth, After.PlayerMaxHealth);
    TestEqual(TEXT("Fresh run resets Core eligibility"), After.CollectedCoreCount, 0);
    TestFalse(TEXT("Fresh run exit is locked"), After.bExitUnlocked);
    TestEqual(TEXT("Fresh player has no old Dash cooldown"),
        CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn())->GetDashCooldownRemaining(), 0.0);
    for (const TWeakObjectPtr<AActor>& Old : OldActors)
    {
        TestTrue(TEXT("Every previous owned actor is destroyed"), !Old.IsValid() || Old->IsActorBeingDestroyed());
    }
    Fixture.Coordinator->OnRunStateChanged().Remove(Handle);
    TestEqual(TEXT("All four genuine transitions are observable"), Transitions.Num(), 4);
    if (Transitions.Num() == 4)
    {
        const ESeedForgeRunState Expected[] = {ESeedForgeRunState::Lost, ESeedForgeRunState::Restarting,
            ESeedForgeRunState::Generating, ESeedForgeRunState::Playing};
        for (int32 Index = 0; Index < 4; ++Index)
        {
            TestEqual(TEXT("Observed transition order is exact"), Transitions[Index].To, Expected[Index]);
            TestEqual(TEXT("Observed snapshot state is actual destination"), Transitions[Index].Snapshot.RunState, Expected[Index]);
        }
        TestEqual(TEXT("Playing transition includes applied request"), Transitions[3].Snapshot.AppliedRequestId, Pending.PendingRequestId);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeNewRapidRunObservationsTest,
    "SeedForge.Audit.RunIntegration.NewAndRapidQueuedIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeNewRapidRunObservationsTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    Fixture.ApplyCurrent();
    const auto Initial = Fixture.Coordinator->GetSnapshot();
    Fixture.Press(EKeys::N);
    const auto NewPending = Fixture.Coordinator->GetSnapshot();
    const uint64 ExpectedSeed = Initial.Seed * 6364136223846793005ULL + 1442695040888963407ULL;
    TestEqual(TEXT("N applies exact uint64 next-seed transform"), NewPending.Seed, ExpectedSeed);
    Fixture.ApplyCurrent();
    const auto ExpectedLayout = FSeedForgeGenerator::Generate(ExpectedSeed, {});
    const auto ExpectedEncounter = FSeedForgeEncounterPlanner::Generate(ExpectedLayout.Layout, {});
    TestEqual(TEXT("New seed has independently generated layout identity"), Fixture.Coordinator->GetSnapshot().LayoutHash, ExpectedLayout.Layout.CanonicalHash);
    TestEqual(TEXT("New seed has independently generated encounter identity"), Fixture.Coordinator->GetSnapshot().EncounterHash, ExpectedEncounter.Plan.CanonicalHash);
    TArray<FSeedForgeQueuedRunObservation> Queued;
    const auto Handle = Fixture.Coordinator->OnRunQueued().AddLambda(
        [&](ESeedForgeRunState StateBefore, const FSeedForgeGameplaySnapshot& Snapshot) { Queued.Add({StateBefore, Snapshot}); });
    const auto BeforeRapid = Fixture.Coordinator->GetSnapshot();
    Fixture.Queue(EKeys::N, IE_Pressed);
    Fixture.Queue(EKeys::R, IE_Pressed);
    Fixture.Process();
    const auto Latest = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("One batch performs two actual run requests"), Latest.RunGeneration, BeforeRapid.RunGeneration + 2);
    TestEqual(TEXT("N then R transforms seed once"), Latest.Seed,
        BeforeRapid.Seed * 6364136223846793005ULL + 1442695040888963407ULL);
    TestEqual(TEXT("Both queued identities are observable"), Queued.Num(), 2);
    if (Queued.Num() == 2)
    {
        TestEqual(TEXT("Second request entered while Generating"), Queued[1].StateBefore, ESeedForgeRunState::Generating);
        TestTrue(TEXT("Actual request IDs increase"), Queued[1].Snapshot.PendingRequestId > Queued[0].Snapshot.PendingRequestId);
        Fixture.Apply(Queued[0].Snapshot.PendingRequestId, Queued[0].Snapshot.Seed);
        TestEqual(TEXT("Stale fixture completion cannot apply"), Fixture.Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Generating);
    }
    Fixture.ApplyCurrent();
    TestEqual(TEXT("Only latest request owns the applied run"), Fixture.Coordinator->GetSnapshot().AppliedRequestId, Latest.PendingRequestId);
    TestEqual(TEXT("Rapid restart has exactly one owned actor set"), Fixture.OwnedLiveActors(), 11);
    Fixture.Coordinator->OnRunQueued().Remove(Handle);
    Fixture.Controller->FlushPressedKeys();
    return true;
}

#endif

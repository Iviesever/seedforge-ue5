#include "SeedForgeInputTestWorld.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::RunIdentityTests
{
    class FScopedRunLog final : public FOutputDevice
    {
    public:
        FScopedRunLog() { GLog->AddOutputDevice(this); Lines.Reset(); }
        ~FScopedRunLog() override { GLog->RemoveOutputDevice(this); }
        // Unbuffered delivery occurs on the emitting thread; only the game thread
        // touches Lines. Discarding other threads keeps this scoped sink race-free.
        bool CanBeUsedOnAnyThread() const override { return true; }
        bool CanBeUsedOnMultipleThreads() const override { return true; }
        void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
        {
            if (IsInGameThread() && Category == TEXT("LogSeedForge"))
            {
                Lines.Add(Text);
            }
        }
        bool Contains(const FString& Expected) const
        {
            return Lines.ContainsByPredicate([&Expected](const FString& Line) { return Line.Contains(Expected); });
        }
    private:
        TArray<FString> Lines;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeDistinctRunRequestIdentityTest,
    "SeedForge.Audit.RunIdentity.DistinctRequestAndRunLogs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDistinctRunRequestIdentityTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    Fixture.Subsystem->RequestGeneration(777, {});
    const uint64 Request = Fixture.Coordinator->StartRun(24301);
    const uint64 Run = Fixture.Coordinator->GetSnapshot().RunGeneration;
    TestNotEqual(TEXT("Fixture genuinely diverges request and run counters"), Request, Run);
    SeedForge::RunIdentityTests::FScopedRunLog Log;
    Fixture.ApplyCurrent();
    const auto Applied = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("Applied identity is actual subsystem completion"), Applied.AppliedRequestId, Request);
    TestEqual(TEXT("Run generation remains independent"), Applied.RunGeneration, Run);
    TestEqual(TEXT("Applied run has no pending request"), Applied.PendingRequestId, 0ULL);
    TestTrue(TEXT("Scoped sink actually observed ready output"), Log.Contains(TEXT("Gameplay ready run=")));
    TestTrue(TEXT("Scoped sink actually observed apply output"), Log.Contains(TEXT("Applied request=")));
    TestTrue(TEXT("Ready log carries both identities"), Log.Contains(FString::Printf(
        TEXT("Gameplay ready run=%llu request=%llu seed=24301"), Run, Request)));
    TestTrue(TEXT("Applied log is not mislabeled run generation"), Log.Contains(FString::Printf(
        TEXT("Applied request=%llu run=%llu seed=24301"), Request, Run)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePendingAndFailedIdentityTest,
    "SeedForge.Audit.RunIdentity.PendingAndFailedHaveNoStaleHashes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePendingAndFailedIdentityTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    const auto OldPending = Fixture.Coordinator->GetSnapshot();
    Fixture.ApplyCurrent();
    const auto OldApplied = Fixture.Coordinator->GetSnapshot();
    TestTrue(TEXT("Previous run has real nonzero hashes"), OldApplied.LayoutHash != 0 && OldApplied.EncounterHash != 0);
    Fixture.Coordinator->StartRun(202);
    const auto Pending = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("New pending seed is retained"), Pending.Seed, 202ULL);
    TestEqual(TEXT("Pending new seed has no old layout hash"), Pending.LayoutHash, 0ULL);
    TestEqual(TEXT("Pending new seed has no old encounter hash"), Pending.EncounterHash, 0ULL);
    TestEqual(TEXT("Pending run has no applied request"), Pending.AppliedRequestId, 0ULL);
    TestTrue(TEXT("Pending run has actual outstanding request"), Pending.PendingRequestId != 0);
    FSeedForgeAsyncCompletion Failure;
    Failure.RequestId = Pending.PendingRequestId;
    Failure.Result = FSeedForgeResult::Failure(ESeedForgeErrorCode::PlacementExhausted, TEXT("identity failure"));
    AddExpectedError(TEXT("Gameplay run failed code=GenerationFailed"), EAutomationExpectedErrorFlags::Contains, 1);
    Fixture.Subsystem->OnGenerationApplied().Broadcast(Failure);
    Fixture.Apply(OldPending.PendingRequestId, OldPending.Seed);
    const auto Failed = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("Failed run ignores stale apply"), Failed.RunState, ESeedForgeRunState::Failed);
    TestEqual(TEXT("Failure keeps requested seed"), Failed.Seed, 202ULL);
    TestEqual(TEXT("Failure clears layout hash"), Failed.LayoutHash, 0ULL);
    TestEqual(TEXT("Failure clears encounter hash"), Failed.EncounterHash, 0ULL);
    TestEqual(TEXT("Failure clears applied request"), Failed.AppliedRequestId, 0ULL);
    TestEqual(TEXT("Failure clears pending request"), Failed.PendingRequestId, 0ULL);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeDirectApplyIdentityTest,
    "SeedForge.Audit.RunIdentity.DirectApplyHasNoFictionalRequest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDirectApplyIdentityTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    const auto Pending = Fixture.Coordinator->GetSnapshot();
    const auto Generated = FSeedForgeGenerator::Generate(Pending.Seed, {});
    TestTrue(TEXT("Direct public apply accepts valid layout"), Fixture.Coordinator->ApplyGeneratedLayout(Generated.Layout));
    const auto Applied = Fixture.Coordinator->GetSnapshot();
    TestEqual(TEXT("Direct apply does not impersonate subsystem completion"), Applied.AppliedRequestId, 0ULL);
    TestEqual(TEXT("Direct apply cancels outstanding request"), Applied.PendingRequestId, 0ULL);
    TestEqual(TEXT("Direct apply reaches Playing"), Applied.RunState, ESeedForgeRunState::Playing);
    return true;
}

#endif

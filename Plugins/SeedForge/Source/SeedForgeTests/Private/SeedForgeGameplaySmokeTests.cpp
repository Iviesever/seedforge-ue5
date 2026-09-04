#include "Misc/AutomationTest.h"
#include "SeedForgeGameplaySmoke.h"
#include "SeedForgeInputTestWorld.h"
#include "TimerManager.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::GameplaySmokeTests
{
    // Test-only private lifecycle access. No command-line or live run-state mutation.
    struct FWatchdogAccess
    {
        static FTimerHandle Arm(ASeedForgeGameplayCoordinator& Coordinator)
        {
            Coordinator.GetWorldTimerManager().SetTimer(Coordinator.GameplaySmokeWatchdogTimer,
                FTimerDelegate::CreateLambda([] {}), 30.0f, false);
            return Coordinator.GameplaySmokeWatchdogTimer;
        }
        static void Cleanup(ASeedForgeGameplayCoordinator& Coordinator, bool bPreserve)
        { Coordinator.ClearRunObjects(bPreserve); }
        static void Fail(ASeedForgeGameplayCoordinator& Coordinator)
        { Coordinator.EnterRunFailure(ESeedForgeRunFailureCode::SmokeFailed, TEXT("Watchdog lifecycle fixture.")); }
        static bool StartOwnedObserver(ASeedForgeGameplayCoordinator& Coordinator)
        { return Coordinator.GameplaySmokePathObserver.Start(Coordinator); }
        static bool IsObservationInvalidated(const ASeedForgeGameplayCoordinator& Coordinator)
        { return Coordinator.IsGameplaySmokePathObservationInvalidated(); }
        static int32 OwnedBindings(const ASeedForgeGameplayCoordinator& Coordinator)
        { return Coordinator.GameplaySmokePathObserver.GetRemainingDelegateBindings(); }
    };

    // Pure/synthetic value fixture: no screenshot or runtime evidence file is written.
    FSeedForgeGameplaySmokeTrace MakeSyntheticCompleteTrace()
    {
        FSeedForgeGameplaySmokeTrace Trace;
        const auto Layout = FSeedForgeGenerator::Generate(24301, {});
        const auto Encounter = FSeedForgeEncounterPlanner::Generate(Layout.Layout, {});
        Trace.GitSha = TEXT("0123456789abcdef0123456789abcdef01234567");
        Trace.EngineVersion = TEXT("synthetic-value-fixture-UE-5.8");
        Trace.Seed = 24301; Trace.LayoutHash = Layout.Layout.CanonicalHash; Trace.EncounterHash = Encounter.Plan.CanonicalHash;
        Trace.RunGeneration = 7; Trace.AppliedRequestId = 11;
        Trace.ActorCounts = {1, 3, 5, 1};
        Trace.StateTransitions = {TEXT("Generating"), TEXT("Playing"), TEXT("Won")};
        Trace.Actions = {TEXT("Attack:Enemy:0"), TEXT("Collect:Core:0"), TEXT("ReachExit")};
        Trace.WalkableCells = Layout.Layout.GetCanonicalWalkableCells();
        FSeedForgePathRequest Request;
        Request.Start = Encounter.Plan.Enemies[0].Cell; Request.Goal = Layout.Layout.Entrance;
        Request.WalkableCells = Trace.WalkableCells; Request.MaxExpandedNodes = 1024;
        const auto Path = FSeedForgeGridPathfinder::FindPath(Request);
        FSeedForgeEnemyPathSnapshot Snapshot;
        Snapshot.Revision = 3;
        for (int32 I = 1; I < Path.Path.Num(); ++I)
        { Snapshot.Waypoints.Add(FSeedForgeGameplayMath::CellToWorld(Path.Path[I], 200, 58)); }
        FSeedForgeEnemyPathProof Proof;
        Proof.ObserveAppliedPath(0, Trace.RunGeneration, Trace.AppliedRequestId, Request.Start, Request.Goal,
            Path, Snapshot, Trace.WalkableCells, 200);
        if (!Snapshot.Waypoints.IsEmpty())
        {
            const FVector Origin = FSeedForgeGameplayMath::CellToWorld(Request.Start, 200, 58);
            const FVector Direction = (Snapshot.Waypoints[0] - Origin).GetSafeNormal();
            for (uint64 Sequence = 1; Sequence <= 2; ++Sequence)
            {
                FSeedForgeEnemyMoveObservation Move;
                Move.Sequence = Sequence; Move.Frame = 20 + Sequence; Move.PathRevision = Snapshot.Revision;
                Move.WaypointIndex = 0; Move.DeltaSeconds = 0.05f;
                Move.From = Origin + Direction * static_cast<double>((Sequence - 1) * 10);
                Move.To = Move.From + Direction * 10; Move.Target = Snapshot.Waypoints[0];
                Proof.ObserveMovement(0, Trace.RunGeneration, Trace.AppliedRequestId, Move, 260);
            }
        }
        Trace.PathEvidence = Proof.GetEvidence();
        const TCHAR* Labels[] = {TEXT("start"), TEXT("combat"), TEXT("win")};
        for (int32 I = 0; I < 3; ++I)
        {
            FSeedForgeCaptureReceipt Receipt;
            Receipt.Request.Token = FGuid(1, 2, 3, static_cast<uint32>(I + 1));
            Receipt.Request.Label = Labels[I];
            Receipt.Request.Path = FString::Printf(TEXT("D:/program/SeedForge/Artifacts/Reports/SmokeValueFixtures/%s-%s.png"), Labels[I],
                *Receipt.Request.Token.ToString(EGuidFormats::DigitsLower));
            Receipt.Request.RunGeneration = Trace.RunGeneration; Receipt.Request.SourceRequestId = Trace.AppliedRequestId;
            Receipt.Request.RequestedFrame = static_cast<uint64>(100 + I * 100);
            Receipt.RenderedFrame = Receipt.Request.RequestedFrame + 1;
            Receipt.CapturedFrame = Receipt.Request.RequestedFrame + 2;
            Receipt.CompletedFrame = Receipt.Request.RequestedFrame + 3;
            Receipt.Request.RequestedAtUtc = FDateTime(2026, 9, 4) + FTimespan::FromSeconds(I + 1);
            Receipt.CompletedAtUtc = Receipt.Request.RequestedAtUtc + FTimespan::FromMilliseconds(30);
            Receipt.FileBytes = 12345; Receipt.RemainingDelegateBindings = 0; Receipt.bSuccess = true;
            Trace.Captures.Add(Receipt); Trace.ScreenshotPaths.Add(Receipt.Request.Path);
        }
        Trace.bSuccess = true;
        return Trace;
    }

    class FObserveSmokePath final : public IAutomationLatentCommand
    {
    public:
        explicit FObserveSmokePath(FAutomationTestBase* InTest) : Test(InTest) {}
        ~FObserveSmokePath() override { Cleanup(); }
        bool Update() override
        {
            if (!Fixture)
            {
                Fixture = MakeUnique<SeedForge::InputTests::FWorldFixture>();
                Fixture->ApplyCurrent();
                for (TActorIterator<ASeedForgeEnemyPawn> It(Fixture->World); It; ++It)
                { if (It->GetOwner() == Fixture->Coordinator && It->GetStableId() == 0) { Target = *It; break; } }
                if (!Test->TestNotNull(TEXT("Real encounter contains the intended enemy"), Target.Get())) { Cleanup(); return true; }
                Identity = Fixture->Coordinator->GetSnapshot(); Before = Target->GetActorLocation();
                Test->TestTrue(TEXT("Passive observer starts on a real applied run"), Observer.Start(*Fixture->Coordinator));
                Test->TestEqual(TEXT("Observer owns exactly path, queued-run and World handles while observing"), Observer.GetRemainingDelegateBindings(), 3);
                Fixture->BeginWorldTicks();
                Fixture->World->GetTimerManager().SetTimer(TimerProbe, FTimerDelegate::CreateLambda([this] { ++TimerFirings; }), TickDelta, true);
                InitialFrame = LastFrame = GFrameCounter;
                return false;
            }
            if (LastFrame == GFrameCounter) { return false; }
            LastFrame = GFrameCounter;
            if (bAwaitInvalidation)
            {
                Fixture->TickWorld(TickDelta);
                Test->TestEqual(TEXT("Target destruction does not require a run-state change"), Fixture->Coordinator->GetSnapshot().RunState, ESeedForgeRunState::Playing);
                Test->TestTrue(TEXT("Production AwaitPathProof predicate detects cancelled observation without waiting for budget"),
                    FWatchdogAccess::IsObservationInvalidated(*Fixture->Coordinator));
                Test->TestEqual(TEXT("Destroyed target releases all Coordinator-owned path handles"), FWatchdogAccess::OwnedBindings(*Fixture->Coordinator), 0);
                Cleanup();
                return true;
            }
            if (!Target.IsValid() || Target->IsActorBeingDestroyed())
            { Test->AddError(TEXT("Real target vanished before the bounded path observation.")); Cleanup(); return true; }
            Fixture->TickWorld(TickDelta); ++Ticks;
            if (!Observer.IsComplete() && Ticks < 120) { return false; }
            const double Distance = FVector::Dist2D(Before, Target->GetActorLocation());
            Test->AddInfo(FString::Printf(TEXT("Smoke observer controls: ticks=%d timer_firings=%d frames=%llu..%llu displacement=%.6f."),
                Ticks, TimerFirings, InitialFrame, LastFrame, Distance));
            Test->TestTrue(TEXT("Independent timer advances on actual engine frames"), TimerFirings > 1);
            Test->TestTrue(TEXT("At most one normal World tick per engine frame"), LastFrame - InitialFrame >= static_cast<uint64>(Ticks));
            Test->TestTrue(TEXT("Real enemy movement is finite and within two-second speed bound"), FMath::IsFinite(Distance) && Distance > 1 && Distance <= 520.1);
            Test->TestTrue(TEXT("Passive observer requires actual replanning and movement before completion"), Observer.IsComplete());
            Test->TestTrue(TEXT("Completed observation has at least two real samples and 20 units"),
                Observer.GetEvidence().ObservedMoveCount >= 2 && Observer.GetEvidence().TotalDistance >= 20);
            Test->TestEqual(TEXT("Completed proof uses the actual run"), Observer.GetEvidence().RunGeneration, Identity.RunGeneration);
            Test->TestEqual(TEXT("Completed proof uses the actual applied request"), Observer.GetEvidence().SourceRequestId, Identity.AppliedRequestId);
            Test->TestEqual(TEXT("Completed proof releases all observer handles"), Observer.GetRemainingDelegateBindings(), 0);
            Test->TestTrue(TEXT("Movement storage is capped at two samples"), Observer.GetEvidence().MovementSamples.Num() <= 2);
            Observer.Cancel(); Observer.Cancel();
            Test->TestFalse(TEXT("Idempotent cancellation clears completed proof"), Observer.IsComplete());
            Test->TestEqual(TEXT("Cancellation clears path identity"), Observer.GetEvidence().RunGeneration, 0ULL);
            Test->TestEqual(TEXT("Cancellation leaves no handles"), Observer.GetRemainingDelegateBindings(), 0);
            Test->TestTrue(TEXT("Observer can begin another observation"), Observer.Start(*Fixture->Coordinator));
            Fixture->Coordinator->StartRun(24302);
            Test->TestFalse(TEXT("Queued restart cannot retain an old completed proof"), Observer.IsComplete());
            Test->TestEqual(TEXT("Queued restart removes observer handles"), Observer.GetRemainingDelegateBindings(), 0);
            Test->TestEqual(TEXT("Queued restart clears stale request identity"), Observer.GetEvidence().SourceRequestId, 0ULL);
            Fixture->ApplyCurrent();
            Test->TestTrue(TEXT("Observer can attach to the newly applied run"), Observer.Start(*Fixture->Coordinator));
            Observer.Cancel();
            Test->TestTrue(TEXT("Coordinator-owned observer starts for invalidation unit"), FWatchdogAccess::StartOwnedObserver(*Fixture->Coordinator));
            Test->TestFalse(TEXT("Active three-handle observation is not invalidated"), FWatchdogAccess::IsObservationInvalidated(*Fixture->Coordinator));
            ASeedForgeEnemyPawn* NewTarget = nullptr;
            for (TActorIterator<ASeedForgeEnemyPawn> It(Fixture->World); It; ++It)
            { if (It->GetOwner() == Fixture->Coordinator && It->GetStableId() == 0) { NewTarget = *It; break; } }
            if (!Test->TestNotNull(TEXT("New run contains the observer target"), NewTarget)) { Cleanup(); return true; }
            NewTarget->Destroy(); // Lifecycle unit, not attack or runtime proof.
            bAwaitInvalidation = true;
            return false;
        }
    private:
        void Cleanup()
        {
            Observer.Cancel();
            if (Fixture) { Fixture->World->GetTimerManager().ClearTimer(TimerProbe); Fixture.Reset(); }
        }
        static constexpr float TickDelta = 1.0f / 60.0f;
        FAutomationTestBase* Test;
        TUniquePtr<SeedForge::InputTests::FWorldFixture> Fixture;
        FSeedForgeGameplaySmokePathObserver Observer;
        TWeakObjectPtr<ASeedForgeEnemyPawn> Target;
        FSeedForgeGameplaySnapshot Identity;
        FVector Before = FVector::ZeroVector;
        FTimerHandle TimerProbe;
        uint64 InitialFrame = 0, LastFrame = 0;
        int32 Ticks = 0, TimerFirings = 0;
        bool bAwaitInvalidation = false;
    };

    class FObserveSmokeWatchdog final : public IAutomationLatentCommand
    {
    public:
        explicit FObserveSmokeWatchdog(FAutomationTestBase* InTest) : Test(InTest) {}
        bool Update() override
        {
            if (!Fixture)
            {
                Fixture = MakeUnique<SeedForge::InputTests::FWorldFixture>(); Fixture->ApplyCurrent();
                Handle = FWatchdogAccess::Arm(*Fixture->Coordinator);
                Fixture->BeginWorldTicks(); LastFrame = GFrameCounter;
                return false;
            }
            if (LastFrame == GFrameCounter) { return false; }
            LastFrame = GFrameCounter;
            Fixture->TickWorld(0.25f);
            if (++Ticks < 2) { return false; }
            FTimerManager& Timers = Fixture->World->GetTimerManager();
            const double Remaining = Timers.GetTimerRemaining(Handle);
            const double Elapsed = Timers.GetTimerElapsed(Handle);
            const double Deadline = Fixture->World->GetTimeSeconds() + Remaining;
            Test->TestTrue(TEXT("Real watchdog timer has elapsed before same-run cleanup"), Remaining > 0 && Remaining < 30 && Elapsed > 0);
            FWatchdogAccess::Cleanup(*Fixture->Coordinator, true);
            Test->TestTrue(TEXT("Same-run cleanup preserves the armed watchdog"), Timers.IsTimerActive(Handle));
            Test->TestTrue(TEXT("Same-run cleanup does not restart remaining budget"), FMath::IsNearlyEqual(static_cast<double>(Timers.GetTimerRemaining(Handle)), Remaining, 0.000001));
            Test->TestTrue(TEXT("Same-run cleanup preserves elapsed budget"), FMath::IsNearlyEqual(static_cast<double>(Timers.GetTimerElapsed(Handle)), Elapsed, 0.000001));
            Test->TestTrue(TEXT("Same-run cleanup preserves original deadline"), FMath::IsNearlyEqual(Fixture->World->GetTimeSeconds() + Timers.GetTimerRemaining(Handle), Deadline, 0.000001));
            FWatchdogAccess::Cleanup(*Fixture->Coordinator, false);
            Test->TestFalse(TEXT("Default cleanup clears watchdog"), Timers.TimerExists(Handle));
            Handle = FWatchdogAccess::Arm(*Fixture->Coordinator);
            Fixture->Coordinator->StartRun(24302);
            Test->TestFalse(TEXT("Restart clears the previous watchdog"), Timers.TimerExists(Handle));
            Handle = FWatchdogAccess::Arm(*Fixture->Coordinator);
            Test->AddExpectedError(TEXT("Gameplay run failed code=SmokeFailed message=Watchdog lifecycle fixture"), EAutomationExpectedErrorFlags::Contains, 1);
            FWatchdogAccess::Fail(*Fixture->Coordinator);
            Test->TestFalse(TEXT("Failure clears watchdog"), Timers.TimerExists(Handle));
            Handle = FWatchdogAccess::Arm(*Fixture->Coordinator);
            Fixture->Coordinator->Destroy();
            Test->TestFalse(TEXT("Actual actor EndPlay clears watchdog"), Timers.TimerExists(Handle));
            Test->AddInfo(TEXT("Lifecycle unit only: real World timer/private cleanup, not a live 30-second smoke process proof."));
            Fixture.Reset();
            return true;
        }
    private:
        FAutomationTestBase* Test;
        TUniquePtr<SeedForge::InputTests::FWorldFixture> Fixture;
        FTimerHandle Handle;
        uint64 LastFrame = 0;
        int32 Ticks = 0;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplaySmokeSuccessTraceTest,
    "SeedForge.GameplaySmoke.SuccessTrace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplaySmokeSuccessTraceTest::RunTest(const FString& Parameters)
{
    const auto Trace = SeedForge::GameplaySmokeTests::MakeSyntheticCompleteTrace();
    const FString Json = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Trace);
    auto Metadata = Trace; Metadata.Seed = MAX_uint64; Metadata.bSuccess = false;
    Metadata.FailureCode = TEXT("SyntheticMetadataOnly");
    const FString MetadataJson = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Metadata);
    TestTrue(TEXT("Trace declares schema and version"), Json.StartsWith(TEXT("{\"schema\":\"seedforge.gameplay-smoke\",\"schemaVersion\":1,")));
    TestTrue(TEXT("Maximum uint64 seed is an exact string"), MetadataJson.Contains(TEXT("\"seed\":\"18446744073709551615\"")));
    TestTrue(TEXT("Layout hash is an exact string"), Json.Contains(TEXT("\"layoutHash\":\"7425849530159566348\"")));
    TestTrue(TEXT("Encounter hash is an exact string"), Json.Contains(TEXT("\"encounterHash\":\"15303214708604970503\"")));
    TestTrue(TEXT("Actor counts are explicit"), Json.Contains(TEXT("\"actorCounts\":{\"players\":1,\"dataCores\":3,\"enemies\":5,\"exits\":1}")));
    TestTrue(TEXT("State order is preserved"), Json.Contains(TEXT("\"stateTransitions\":[\"Generating\",\"Playing\",\"Won\"]")));
    TestTrue(TEXT("Successful result is explicit"), Json.Contains(TEXT("\"result\":\"Passed\"")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplaySmokeFailureTraceTest,
    "SeedForge.GameplaySmoke.FailureTrace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplaySmokeFailureTraceTest::RunTest(const FString& Parameters)
{
    FSeedForgeGameplaySmokeTrace Trace;
    Trace.GitSha = TEXT("dirty\"sha");
    Trace.EngineVersion = TEXT("5.8.0\nWin64");
    Trace.FailureCode = TEXT("MissingActor");
    Trace.FailureMessage = TEXT("Expected 3 Cores\nfound 2");

    const FString Json = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Trace);
    TestTrue(TEXT("Failure result is explicit"), Json.Contains(TEXT("\"result\":\"Failed\"")));
    TestTrue(TEXT("Failure code is preserved"), Json.Contains(TEXT("\"failureCode\":\"MissingActor\"")));
    TestTrue(TEXT("Quotes and newlines are escaped"), Json.Contains(TEXT("dirty\\\"sha")) && Json.Contains(TEXT("Expected 3 Cores\\nfound 2")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSmokePathAcceptanceTest,
    "SeedForge.GameplaySmoke.PathEvidenceAcceptsCompleteSynthetic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeedForgeSmokePathAcceptanceTest::RunTest(const FString& Parameters)
{
    AddInfo(TEXT("SYNTHETIC VALUE FIXTURE ONLY: pure model/path plus synthetic moves and capture receipts."));
    const auto Trace = SeedForge::GameplaySmokeTests::MakeSyntheticCompleteTrace();
    TestTrue(TEXT("Existing path proof validates the synthetic model-derived positive control"), Trace.PathEvidence.bComplete);
    TestEqual(TEXT("Positive control has all three F-shaped receipts"), Trace.Captures.Num(), 3);
    FString Error;
    TestTrue(TEXT("Complete path and capture correlation is accepted"), FSeedForgeGameplaySmokeCodec::ValidatePathEvidence(Trace, Error));
    TestTrue(TEXT("Accepted evidence has no validation error"), Error.IsEmpty());
    auto NonAdjacent = Trace;
    auto& P = NonAdjacent.PathEvidence;
    const FVector Origin = P.MovementSamples[0].From;
    const FVector Direction = (P.MovementSamples[0].Target - Origin).GetSafeNormal();
    P.MovementSamples[0].To = Origin + Direction * 5;
    P.MovementSamples[1].From = Origin + Direction * 13;
    P.MovementSamples[1].To = Origin + Direction * 23;
    P.MovementSamples[1].Sequence = 3; P.MovementSamples[1].Frame = P.MovementSamples[0].Frame + 2;
    P.ObservedMoveCount = 3; P.TotalDistance = 23; P.TotalDeltaSeconds = 0.15;
    TestTrue(TEXT("Bounded first/latest evidence may omit a feasible middle move"), FSeedForgeGameplaySmokeCodec::ValidatePathEvidence(NonAdjacent, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSmokePathRejectionTest,
    "SeedForge.GameplaySmoke.PathEvidenceRejectsTampering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeedForgeSmokePathRejectionTest::RunTest(const FString& Parameters)
{
    const auto Complete = SeedForge::GameplaySmokeTests::MakeSyntheticCompleteTrace();
    if (!TestEqual(TEXT("Tamper baseline has two model-derived samples"), Complete.PathEvidence.MovementSamples.Num(), 2)) { return false; }
    const TCHAR* Cases[] = {TEXT("MissingPath"),TEXT("MissingSample"),TEXT("ShortMotion"),TEXT("StaleRun"),TEXT("StaleRequest"),
        TEXT("StaleRevision"),TEXT("NonfiniteSample"),TEXT("OffRoute"),TEXT("Overspeed"),TEXT("OversizedPath"),TEXT("OversizedWalkable"),
        TEXT("WrongReceiptIdentity"),TEXT("MovementAfterFirstRequest"),TEXT("RemainingBindings"),TEXT("ReorderedReceipts"),TEXT("MissingReceipt"),
        TEXT("WrongWaypoints"),TEXT("WrongExpandedNodes"),TEXT("NonfiniteAggregate"),TEXT("DuplicateWalkable"),TEXT("ZeroTopIdentity"),
        TEXT("AggregateCannotCoverHiddenGap"),TEXT("HiddenTimeCannotCoverGap")};
    FString Error;
    for (int32 Case = 0; Case < UE_ARRAY_COUNT(Cases); ++Case)
    {
        auto Invalid = Complete;
        switch (Case)
        {
        case 0: Invalid.PathEvidence = {}; break;
        case 1: Invalid.PathEvidence.MovementSamples.Pop(); break;
        case 2: Invalid.PathEvidence.TotalDistance = 19; break;
        case 3: ++Invalid.PathEvidence.RunGeneration; break;
        case 4: ++Invalid.PathEvidence.SourceRequestId; break;
        case 5: ++Invalid.PathEvidence.MovementSamples[0].PathRevision; break;
        case 6: Invalid.PathEvidence.MovementSamples[0].To.X = std::numeric_limits<double>::quiet_NaN(); break;
        case 7: Invalid.PathEvidence.MovementSamples[0].To += FVector(80,80,0); break;
        case 8: Invalid.PathEvidence.MovementSamples[0].DeltaSeconds = 0.0001f; break;
        case 9: Invalid.PathEvidence.Cells.SetNum(FSeedForgeEnemyPathProof::MaxPathCells + 1); break;
        case 10: Invalid.WalkableCells.SetNum(FSeedForgeGameplaySmokeTrace::MaxWalkableCells + 1); break;
        case 11: ++Invalid.Captures[0].Request.SourceRequestId; break;
        case 12: Invalid.PathEvidence.MovementSamples.Last().Frame = Invalid.Captures[0].Request.RequestedFrame + 1; break;
        case 13: Invalid.RemainingPathDelegateBindings = 1; break;
        case 14: Swap(Invalid.Captures[0], Invalid.Captures[1]); break;
        case 15: Invalid.Captures.Pop(); break;
        case 16: Invalid.PathEvidence.Waypoints[0].Z += 1; break;
        case 17: ++Invalid.PathEvidence.ExpandedNodes; break;
        case 18: Invalid.PathEvidence.TotalDeltaSeconds = std::numeric_limits<double>::infinity(); break;
        case 19: { const FIntPoint Duplicate = Invalid.WalkableCells[0]; Invalid.WalkableCells.Add(Duplicate); break; }
        case 20: Invalid.AppliedRequestId = 0; break;
        case 21:
        case 22:
        {
            auto& P = Invalid.PathEvidence;
            const FVector Origin = P.MovementSamples[0].From;
            const FVector Direction = (P.MovementSamples[0].Target - Origin).GetSafeNormal();
            P.MovementSamples[0].To = Origin + Direction * 5;
            P.MovementSamples[1].From = Origin + Direction * (Case == 21 ? 150 : 13);
            P.MovementSamples[1].To = Origin + Direction * (Case == 21 ? 160 : 23);
            P.MovementSamples[1].Sequence = 3; P.MovementSamples[1].Frame = P.MovementSamples[0].Frame + 2;
            P.ObservedMoveCount = 3; P.TotalDistance = Case == 21 ? 20 : 23; P.TotalDeltaSeconds = Case == 21 ? 0.15 : 0.101;
            break;
        }
        }
        TestFalse(Cases[Case], FSeedForgeGameplaySmokeCodec::ValidatePathEvidence(Invalid, Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSmokePathSchemaTest,
    "SeedForge.GameplaySmoke.PathEvidenceSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeedForgeSmokePathSchemaTest::RunTest(const FString& Parameters)
{
    auto Trace = SeedForge::GameplaySmokeTests::MakeSyntheticCompleteTrace();
    const FString Json = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Trace);
    for (const TCHAR* Field : {TEXT("pathEvidence"),TEXT("walkableCells"),TEXT("remainingPathDelegateBindings"),
        TEXT("movementSamples"),TEXT("observedMoveCount"),TEXT("waypoints")})
    { TestTrue(Field, Json.Contains(FString::Printf(TEXT("\"%s\":"), Field))); }
    TestTrue(TEXT("Path revision uses exact uint64 encoding"), Json.Contains(TEXT("\"pathRevision\":\"3\"")));
    TestTrue(TEXT("Movement sequence uses exact uint64 encoding"), Json.Contains(TEXT("\"sequence\":\"1\"")));
    Trace.PathEvidence = {};
    TestFalse(TEXT("Missing path proof cannot serialize Passed"), FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Trace).Contains(TEXT("\"result\":\"Passed\"")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSmokeObserverWorldTest,
    "SeedForge.GameplaySmoke.PassiveObserverUsesRealWorldAndCleansUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeedForgeSmokeObserverWorldTest::RunTest(const FString& Parameters)
{
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<SeedForge::GameplaySmokeTests::FObserveSmokePath>(this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSmokeWatchdogTest,
    "SeedForge.GameplaySmoke.WatchdogPreservesApplyDeadlineAndClearsLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSeedForgeSmokeWatchdogTest::RunTest(const FString& Parameters)
{
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<SeedForge::GameplaySmokeTests::FObserveSmokeWatchdog>(this));
    return true;
}

#endif

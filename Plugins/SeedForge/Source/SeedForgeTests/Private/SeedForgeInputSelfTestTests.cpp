#include "SeedForgeInputTestWorld.h"
#include "SeedForgeInputSelfTest.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/Paths.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::InputSelfTests
{
    const FString Revision = TEXT("2035e925ceb73fd2aa24de2646a0ad6197216dc8");

    FDateTime SyntheticTime(uint64 Frame)
    {
        return FDateTime(2026, 9, 4) + FTimespan::FromSeconds(static_cast<double>(Frame) * 0.1);
    }

    FSeedForgeInputRunObservation MakeSyntheticRun(const FString& Label, uint64 Seed, uint64 Run, uint64 Request, uint64 Frame)
    {
        const auto Layout = FSeedForgeGenerator::Generate(Seed, {});
        const auto Encounter = FSeedForgeEncounterPlanner::Generate(Layout.Layout, {});
        FSeedForgeInputRunObservation Result;
        Result.Label = Label;
        Result.Frame = Frame;
        Result.AtUtc = SyntheticTime(Frame);
        Result.Snapshot.Seed = Seed;
        Result.Snapshot.LayoutHash = Layout.Layout.CanonicalHash;
        Result.Snapshot.EncounterHash = Encounter.Plan.CanonicalHash;
        Result.Snapshot.RunGeneration = Run;
        Result.Snapshot.AppliedRequestId = Request;
        Result.Snapshot.RunState = ESeedForgeRunState::Playing;
        Result.Snapshot.PlayerHealth = Result.Snapshot.PlayerMaxHealth = 100;
        Result.Snapshot.RequiredCoreCount = 3;
        Result.Resources.bInteractionTimerActive = Result.Resources.bRepathTimerActive = true;
        Result.CoordinatorKey = TEXT("synthetic-coordinator");
        Result.ControllerKey = TEXT("synthetic-controller");
        Result.HudKey = TEXT("synthetic-hud");
        Result.CoordinatorCount = Result.ControllerCount = Result.HudCount = Result.HudOverlayCount = 1;
        Result.bPreviousActorsDestroyed = Label != TEXT("initial");
        auto AddActor = [&](const TCHAR* Role, uint32 StableId, const FIntPoint& Cell)
        {
            FSeedForgeInputActorObservation Actor;
            Actor.Role = Role;
            Actor.StableId = StableId;
            Actor.Key = FString::Printf(TEXT("synthetic-run-%llu-%s-%u"), Run, Role, StableId);
            Actor.OwnerKey = Actor.Role == TEXT("player") ? Result.ControllerKey : Result.CoordinatorKey;
            Actor.Cell = Cell;
            Result.Actors.Add(MoveTemp(Actor));
        };
        AddActor(TEXT("player"), 0, Layout.Layout.Entrance);
        AddActor(TEXT("visualization"), 0, FIntPoint::ZeroValue);
        for (const auto& Core : Encounter.Plan.DataCores) { AddActor(TEXT("core"), Core.StableId, Core.Cell); }
        for (const auto& Enemy : Encounter.Plan.Enemies) { AddActor(TEXT("enemy"), Enemy.StableId, Enemy.Cell); }
        AddActor(TEXT("exit"), 0, Layout.Layout.Exit);
        return Result;
    }

    // Synthetic value fixture only: these are not live actor/state mutations or
    // runtime/release evidence. Hashes/routes come from the real pure models.
    FSeedForgeInputSelfTestTrace MakeSyntheticSuccessEvidence(bool bDiagnostic)
    {
        FSeedForgeInputSelfTestTrace Trace;
        Trace.SourceIdentity = bDiagnostic ? TEXT("diagnostic-") + Revision : Revision;
        Trace.SourceRevision = Revision;
        Trace.SourceKind = bDiagnostic ? ESeedForgeInputSourceKind::DiagnosticRevision : ESeedForgeInputSourceKind::CleanRevision;
        Trace.EngineVersion = TEXT("synthetic-value-fixture-UE-5.8");
        Trace.ExpectedInitialSeed = 24301;
        Trace.StartedFrame = 1;
        Trace.CompletedFrame = 120;
        Trace.StartedAtUtc = SyntheticTime(1);
        Trace.CompletedAtUtc = SyntheticTime(120);
        Trace.CompletionCount = 1;
        Trace.bSuccess = true;
        Trace.FailureCode = ESeedForgeInputSelfTestFailure::None;
        const uint64 NewSeed = 24301ULL * 6364136223846793005ULL + 1442695040888963407ULL;
        const uint64 RapidSeed = NewSeed * 6364136223846793005ULL + 1442695040888963407ULL;
        Trace.Runs = {MakeSyntheticRun(TEXT("initial"), 24301, 7, 11, 10),
            MakeSyntheticRun(TEXT("same"), 24301, 8, 22, 70),
            MakeSyntheticRun(TEXT("new"), NewSeed, 9, 33, 90),
            MakeSyntheticRun(TEXT("rapid"), RapidSeed, 11, 55, 110)};
        Trace.Initial = Trace.Runs[0].Snapshot;
        Trace.Final = Trace.Runs.Last().Snapshot;
        const auto Layout = FSeedForgeGenerator::Generate(24301, {});
        const auto Encounter = FSeedForgeEncounterPlanner::Generate(Layout.Layout, {});
        Trace.WalkableCells = Layout.Layout.GetCanonicalWalkableCells();
        FSeedForgePathRequest Request;
        Request.Start = Encounter.Plan.Enemies[0].Cell;
        Request.Goal = Layout.Layout.Entrance;
        Request.WalkableCells = Trace.WalkableCells;
        Request.MaxExpandedNodes = 1024;
        const auto Path = FSeedForgeGridPathfinder::FindPath(Request);
        FSeedForgeEnemyPathSnapshot Snapshot;
        Snapshot.Revision = 1;
        for (int32 Index = 1; Index < Path.Path.Num(); ++Index)
        {
            Snapshot.Waypoints.Add(FSeedForgeGameplayMath::CellToWorld(Path.Path[Index], 200, 58));
        }
        FSeedForgeEnemyPathProof Proof;
        Proof.ObserveAppliedPath(0, 7, 11, Request.Start, Request.Goal, Path, Snapshot, Trace.WalkableCells, 200);
        if (!Snapshot.Waypoints.IsEmpty())
        {
            const FVector Origin = FSeedForgeGameplayMath::CellToWorld(Request.Start, 200, 58);
            const FVector Direction = (Snapshot.Waypoints[0] - Origin).GetSafeNormal();
            for (uint64 Sequence = 1; Sequence <= 2; ++Sequence)
            {
                FSeedForgeEnemyMoveObservation Move;
                Move.Sequence = Sequence;
                Move.Frame = 13 + Sequence;
                Move.PathRevision = 1;
                Move.WaypointIndex = 0;
                Move.From = Origin + Direction * static_cast<double>((Sequence - 1) * 10);
                Move.To = Move.From + Direction * 10;
                Move.Target = Snapshot.Waypoints[0];
                Move.DeltaSeconds = 0.05f;
                Proof.ObserveMovement(0, 7, 11, Move, 260);
            }
        }
        Trace.PathEvidence = Proof.GetEvidence();
        const TCHAR* Actions[] = {TEXT("AimX"), TEXT("AimY"), TEXT("AttackFirst"), TEXT("AttackKill"),
            TEXT("MoveW"), TEXT("MoveS"), TEXT("MoveA"), TEXT("MoveD"), TEXT("Dash"),
            TEXT("RestartSameSeed"), TEXT("StartNewSeed"), TEXT("RapidNewSeed"), TEXT("RapidRestartSameSeed")};
        const TCHAR* Keys[] = {TEXT("MouseMove"), TEXT("MouseMove"), TEXT("LeftMouseButton"), TEXT("LeftMouseButton"),
            TEXT("W"), TEXT("S"), TEXT("A"), TEXT("D"), TEXT("SpaceBar"), TEXT("R"), TEXT("N"), TEXT("N"), TEXT("R")};
        const uint64 Frames[] = {20, 22, 24, 30, 40, 44, 48, 52, 58, 62, 80, 100, 100};
        const FVector Pad = FSeedForgeGameplayMath::CellToWorld(Layout.Layout.Entrance, 200, 96);
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
        {
            FSeedForgeInputEffectObservation Event;
            Event.Action = Actions[Index]; Event.Key = Keys[Index];
            Event.RunGeneration = Index < 9 ? 7 : static_cast<uint64>(Index - 1);
            Event.SourceRequestId = Index < 9 ? 11 : static_cast<uint64>(Index - 7) * 11;
            Event.InjectedFrame = Frames[Index];
            Event.EffectFrame = Index == 9 ? 70 : Index == 10 ? 90 : Index == 11 ? 100 : Index == 12 ? 110 : Frames[Index] + 1;
            Event.ReleaseFrame = Index < 2 ? 0 : Frames[Index] + 1;
            Event.InjectedAtUtc = SyntheticTime(Event.InjectedFrame);
            Event.EffectAtUtc = SyntheticTime(Event.EffectFrame);
            Event.ActorKey = Index >= 9 ? TEXT("synthetic-controller") : TEXT("synthetic-run-7-player-0");
            Event.bConfirmed = true;
            if (Index == 0) { Event.Before = FVector(0, -1, 0); Event.After = FVector(1, 0, 0); }
            if (Index == 1) { Event.Before = FVector(1, 0, 0); Event.After = FVector(0, 1, 0); }
            if (Index == 2 || Index == 3)
            {
                Event.ActorKey = TEXT("synthetic-run-7-enemy-0");
                Event.LiveEnemiesBefore = 5; Event.LiveEnemiesAfter = Index == 2 ? 5 : 4;
                Event.bTargetAliveBefore = true; Event.bTargetAliveAfter = Index == 2;
                Event.CooldownAfter = 0.45;
            }
            if (Index >= 4 && Index <= 7)
            {
                const FVector Directions[] = {FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, -1, 0), FVector(0, 1, 0)};
                Event.Before = Event.After = Pad;
                Event.After += Directions[Index - 4] * 20;
            }
            if (Index == 8)
            {
                Event.Before = Event.After = Pad;
                Event.LaunchVelocity = FVector(1, 1, 0).GetSafeNormal() * 1200;
                Event.After += FVector(1, 1, 0).GetSafeNormal() * 20;
                Event.CooldownAfter = 1.25;
            }
            Trace.InputEvents.Add(MoveTemp(Event));
        }
        auto Pending = [](FSeedForgeGameplaySnapshot Value, uint64 Run, uint64 Request)
        {
            Value.RunGeneration = Run; Value.RunState = ESeedForgeRunState::Generating;
            Value.LayoutHash = Value.EncounterHash = Value.AppliedRequestId = 0;
            Value.PendingRequestId = Request; Value.RequiredCoreCount = 0;
            return Value;
        };
        Trace.QueuedRuns = {
            {ESeedForgeRunState::Lost, Pending(Trace.Runs[1].Snapshot, 8, 22), 62, SyntheticTime(62)},
            {ESeedForgeRunState::Playing, Pending(Trace.Runs[2].Snapshot, 9, 33), 80, SyntheticTime(80)},
            {ESeedForgeRunState::Playing, Pending(Trace.Runs[3].Snapshot, 10, 44), 100, SyntheticTime(100)},
            {ESeedForgeRunState::Generating, Pending(Trace.Runs[3].Snapshot, 11, 55), 100, SyntheticTime(100)}};
        auto Transition = [&](ESeedForgeRunState From, ESeedForgeRunState To, FSeedForgeGameplaySnapshot Value, uint64 Frame)
        {
            Value.RunState = To;
            Trace.Transitions.Add({From, To, Value, Frame, SyntheticTime(Frame)});
        };
        auto Lost = Trace.Initial; Lost.PlayerHealth = 0;
        Transition(ESeedForgeRunState::Playing, ESeedForgeRunState::Lost, Lost, 60);
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const auto& Queued = Trace.QueuedRuns[Index];
            auto InTransition = Queued.Snapshot; InTransition.PendingRequestId = 0;
            if (Index == 0) { InTransition.PlayerHealth = 0; }
            Transition(Index == 0 ? ESeedForgeRunState::Lost : ESeedForgeRunState::Playing, ESeedForgeRunState::Restarting, InTransition, Queued.Frame);
            Transition(ESeedForgeRunState::Restarting, ESeedForgeRunState::Generating, InTransition, Queued.Frame);
            Transition(ESeedForgeRunState::Generating, ESeedForgeRunState::Playing, Trace.Runs[Index + 1].Snapshot, Trace.Runs[Index + 1].Frame);
        }
        for (uint64 Frame : {39ULL, 43ULL, 47ULL, 51ULL})
        {
            Trace.Setups.Add({TEXT("movement-pad"), TEXT("synthetic-run-7-player-0"), 7, 11,
                Frame, SyntheticTime(Frame), Layout.Layout.Entrance, Pad});
        }
        const FIntPoint AttackCell(Layout.Layout.Entrance.X, Layout.Layout.Entrance.Y + 1);
        const FSeedForgeInputSetupObservation AttackSetup{TEXT("attack-target"), TEXT("synthetic-run-7-enemy-0"), 7, 11, 23,
            SyntheticTime(23), AttackCell, FSeedForgeGameplayMath::CellToWorld(AttackCell, 200, 58)};
        Trace.Setups.Insert(AttackSetup, 0);
        return Trace;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeInputSourceIdentityTest,
    "SeedForge.Audit.InputSelfTest.SourceIdentityClassification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInputSourceIdentityTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::InputSelfTests;
    ESeedForgeInputSourceKind Kind = ESeedForgeInputSourceKind::Unverified;
    FString ParsedRevision;
    TestTrue(TEXT("Native parser accepts an explicit clean-shaped revision"),
        FSeedForgeInputSelfTestCodec::ParseSourceIdentity(Revision, Kind, ParsedRevision));
    TestEqual(TEXT("Clean-shaped input is classified explicitly"), Kind, ESeedForgeInputSourceKind::CleanRevision);
    TestEqual(TEXT("All 40 revision digits are retained"), ParsedRevision, Revision);
    TestTrue(TEXT("Native diagnostic development identity is accepted"),
        FSeedForgeInputSelfTestCodec::ParseSourceIdentity(TEXT("diagnostic-") + Revision, Kind, ParsedRevision));
    TestEqual(TEXT("Diagnostic identity is never labeled clean"), Kind, ESeedForgeInputSourceKind::DiagnosticRevision);
    TestEqual(TEXT("Diagnostic base revision is retained"), ParsedRevision, Revision);
    for (const FString& Invalid : {FString(), FString(TEXT("HEAD")), FString(TEXT("diagnostic-abc")),
        FString(TEXT("diagnostic-diagnostic-")) + Revision})
    {
        TestFalse(TEXT("Malformed source identity is rejected"), FSeedForgeInputSelfTestCodec::ParseSourceIdentity(Invalid, Kind, ParsedRevision));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeDiagnosticTraceLabelTest,
    "SeedForge.Audit.InputSelfTest.DiagnosticTraceCannotClaimClean",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDiagnosticTraceLabelTest::RunTest(const FString& Parameters)
{
    FSeedForgeInputSelfTestTrace Trace;
    Trace.SourceIdentity = TEXT("diagnostic-") + SeedForge::InputSelfTests::Revision;
    Trace.SourceRevision = SeedForge::InputSelfTests::Revision;
    Trace.SourceKind = ESeedForgeInputSourceKind::DiagnosticRevision;
    Trace.FailureCode = ESeedForgeInputSelfTestFailure::ViewportUnavailable;
    const FString Json = FSeedForgeInputSelfTestCodec::ExportCanonicalJson(Trace);
    TestTrue(TEXT("Diagnostic source label is serialized"), Json.Contains(TEXT("\"sourceKind\":\"diagnostic\"")));
    TestTrue(TEXT("Diagnostic prefix remains in raw source identity"), Json.Contains(Trace.SourceIdentity));
    TestFalse(TEXT("A failed diagnostic trace cannot become Passed"), Json.Contains(TEXT("\"result\":\"Passed\"")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeInputUnavailableViewportTest,
    "SeedForge.Audit.InputSelfTest.MissingViewportFailsOnceAndCancels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInputUnavailableViewportTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    auto* Component = NewObject<USeedForgeInputSelfTestComponent>(Fixture.Controller);
    Fixture.Controller->AddInstanceComponent(Component);
    Component->RegisterComponent();
    int32 Finished = 0;
    const auto Handle = Component->OnFinished().AddLambda([&](const FSeedForgeInputSelfTestTrace&) { ++Finished; });
    TestFalse(TEXT("A transient fixture without a real scene viewport cannot start input proof"),
        Component->Start(SeedForge::InputSelfTests::Revision, 24301));
    TestEqual(TEXT("Missing real viewport has a typed failure"), Component->GetTrace().FailureCode, ESeedForgeInputSelfTestFailure::ViewportUnavailable);
    TestEqual(TEXT("Start failure reports completion once"), Finished, 1);
    Component->Cancel();
    Component->Cancel();
    Component->BeforeInput(1.0f / 60.0f, false);
    Component->AfterInput(1.0f / 60.0f, false);
    TestFalse(TEXT("Failed/cancelled runner is inactive"), Component->IsRunning());
    TestEqual(TEXT("Late input hooks cannot complete twice"), Finished, 1);
    TestEqual(TEXT("Failure owns no delegates"), Component->GetTrace().RemainingDelegateBindings, 0);
    TestTrue(TEXT("No synthetic aim/input fallback was recorded"), Component->GetTrace().InputEvents.IsEmpty());
    Component->OnFinished().Remove(Handle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeDisabledInputRunnerTest,
    "SeedForge.Audit.InputSelfTest.DisabledRunnerHasNoEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDisabledInputRunnerTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    const auto Before = Fixture.Coordinator->GetSnapshot();
    auto* Component = NewObject<USeedForgeInputSelfTestComponent>(Fixture.Controller);
    Fixture.Controller->AddInstanceComponent(Component);
    Component->RegisterComponent();
    for (int32 Index = 0; Index < 256; ++Index)
    {
        Component->BeforeInput(1.0f / 60.0f, false);
        Component->AfterInput(1.0f / 60.0f, false);
    }
    TestFalse(TEXT("Opt-in runner is inactive by default"), Component->IsRunning());
    TestEqual(TEXT("Disabled hooks cannot restart the live coordinator"), Fixture.Coordinator->GetSnapshot().RunGeneration, Before.RunGeneration);
    TestTrue(TEXT("Disabled hooks retain no per-frame observations"), Component->GetTrace().InputEvents.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeMissingInputBindingsTest,
    "SeedForge.Audit.InputSelfTest.RealBindingRemovalCannotProduceEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeMissingInputBindingsTest::RunTest(const FString& Parameters)
{
    {
        SeedForge::InputTests::FWorldFixture Fixture;
        Fixture.ApplyCurrent();
        auto* Player = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
        const int32 Removed = Player->InputComponent->AxisBindings.RemoveAll(
            [](const FInputAxisBinding& Binding) { return Binding.AxisName == TEXT("MoveForward"); });
        TestTrue(TEXT("Real forward axis binding was removed from this fixture only"), Removed > 0);
        Fixture.Dispatch(EKeys::W, IE_Pressed);
        TestTrue(TEXT("Unbound W produces no movement intent"), Player->GetPendingMovementInputVector().IsNearlyZero());
        Fixture.Dispatch(EKeys::D, IE_Pressed);
        TestTrue(TEXT("Unmodified D still reaches the real input handler"), Player->GetPendingMovementInputVector().Y > 0);
        Fixture.Controller->FlushPressedKeys();
    }
    const FName Actions[] = {TEXT("Attack"), TEXT("Dash"), TEXT("RestartSameSeed"), TEXT("StartNewSeed")};
    const FKey Keys[] = {EKeys::LeftMouseButton, EKeys::SpaceBar, EKeys::R, EKeys::N};
    for (int32 Case = 0; Case < UE_ARRAY_COUNT(Actions); ++Case)
    {
        SeedForge::InputTests::FWorldFixture Fixture;
        Fixture.ApplyCurrent();
        auto* Player = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
        UInputComponent* Input = Case < 2 ? Player->InputComponent.Get() : Fixture.Controller->InputComponent.Get();
        bool bRemoved = false;
        for (int32 Index = Input->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Input->GetActionBinding(Index).GetActionName() == Actions[Case]) { Input->RemoveActionBinding(Index); bRemoved = true; }
        }
        TestTrue(TEXT("Real action binding was removed from this fixture only"), bRemoved);
        const auto Before = Fixture.Coordinator->GetSnapshot();
        Fixture.Press(Keys[Case]);
        TestEqual(TEXT("An unbound action cannot restart a run"), Fixture.Coordinator->GetSnapshot().RunGeneration, Before.RunGeneration);
        if (Case == 0) { TestEqual(TEXT("Unbound LMB cannot consume attack cooldown"), Fixture.Coordinator->GetRunResourceSnapshot().AttackCooldownRemaining, 0.0); }
        if (Case == 1) { TestTrue(TEXT("Unbound Space cannot launch CharacterMovement"), Player->GetCharacterMovement()->PendingLaunchVelocity.IsNearlyZero()); }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeInputTraceFailClosedTest,
    "SeedForge.Audit.InputSelfTest.TraceRejectsMissingAndUnboundedEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInputTraceFailClosedTest::RunTest(const FString& Parameters)
{
    FSeedForgeInputSelfTestTrace Trace;
    Trace.SourceIdentity = SeedForge::InputSelfTests::Revision;
    Trace.SourceRevision = SeedForge::InputSelfTests::Revision;
    Trace.SourceKind = ESeedForgeInputSourceKind::CleanRevision;
    Trace.bSuccess = true; // Synthetic codec value only; no live gameplay mutation.
    FString Error;
    TestFalse(TEXT("An asserted success without real effects is rejected"), FSeedForgeInputSelfTestCodec::ValidateEvidence(Trace, Error));
    TestFalse(TEXT("Exporter cannot publish missing proof as Passed"), FSeedForgeInputSelfTestCodec::ExportCanonicalJson(Trace).Contains(TEXT("\"result\":\"Passed\"")));
    Trace.InputEvents.SetNum(FSeedForgeInputSelfTestTrace::MaxInputEvents + 1);
    TestFalse(TEXT("Oversized input evidence is rejected"), FSeedForgeInputSelfTestCodec::ValidateEvidence(Trace, Error));
    Trace.InputEvents.Reset();
    Trace.QueuedRuns.SetNum(FSeedForgeInputSelfTestTrace::MaxQueuedRuns + 1);
    TestFalse(TEXT("Oversized queued-run evidence is rejected"), FSeedForgeInputSelfTestCodec::ValidateEvidence(Trace, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCompleteInputEvidenceTest,
    "SeedForge.Audit.InputSelfTest.CompleteSyntheticEvidenceAccepted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCompleteInputEvidenceTest::RunTest(const FString& Parameters)
{
    AddInfo(TEXT("SYNTHETIC VALUE FIXTURES ONLY: no runtime or release proof is produced by this test."));
    for (bool bDiagnostic : {false, true})
    {
        const auto Trace = SeedForge::InputSelfTests::MakeSyntheticSuccessEvidence(bDiagnostic);
        TestTrue(TEXT("Synthetic fixture uses a valid real-model path proof"), Trace.PathEvidence.bComplete);
        FString Error;
        TestTrue(TEXT("Complete ordered evidence is structurally accepted"), FSeedForgeInputSelfTestCodec::ValidateEvidence(Trace, Error));
        TestTrue(TEXT("Accepted evidence has no validation error"), Error.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCompleteInputSchemaTest,
    "SeedForge.Audit.InputSelfTest.CompleteSyntheticSchemaSerialized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCompleteInputSchemaTest::RunTest(const FString& Parameters)
{
    AddInfo(TEXT("SYNTHETIC VALUE SERIALIZATION ONLY: the JSON string is not written as runtime evidence."));
    const FString Json = FSeedForgeInputSelfTestCodec::ExportCanonicalJson(SeedForge::InputSelfTests::MakeSyntheticSuccessEvidence(true));
    for (const TCHAR* Field : {TEXT("initial"), TEXT("final"), TEXT("pathEvidence"), TEXT("walkableCells"),
        TEXT("runs"), TEXT("setups"), TEXT("inputEvents"), TEXT("transitions"), TEXT("queuedRuns")})
    {
        TestTrue(Field, Json.Contains(FString::Printf(TEXT("\"%s\":"), Field)));
    }
    TestTrue(TEXT("Only complete validated evidence may serialize Passed"), Json.Contains(TEXT("\"result\":\"Passed\"")));
    TestTrue(TEXT("Passed envelope has no failure code or message"), Json.Contains(TEXT("\"result\":\"Passed\",\"failureCode\":\"\",\"failureMessage\":\"\"")));
    TestTrue(TEXT("Native success never certifies source cleanliness"), Json.Contains(TEXT("\"sourceVerified\":false")));
    TestTrue(TEXT("Synthetic diagnostic classification remains diagnostic"), Json.Contains(TEXT("\"sourceKind\":\"diagnostic\"")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCompleteInputNegativeTest,
    "SeedForge.Audit.InputSelfTest.CompleteSyntheticEvidenceRejectsTampering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCompleteInputNegativeTest::RunTest(const FString& Parameters)
{
    const auto Complete = SeedForge::InputSelfTests::MakeSyntheticSuccessEvidence(false);
    if (!TestEqual(TEXT("Synthetic negative baseline has two valid model-derived movement samples"), Complete.PathEvidence.MovementSamples.Num(), 2)) { return false; }
    FString Error;
    for (int32 Index = 0; Index < Complete.InputEvents.Num(); ++Index)
    {
        auto Missing = Complete; Missing.InputEvents.RemoveAt(Index);
        TestFalse(TEXT("Every required mapped effect is mandatory"), FSeedForgeInputSelfTestCodec::ValidateEvidence(Missing, Error));
    }
    for (int32 Case = 0; Case < 15; ++Case)
    {
        auto Invalid = Complete;
        switch (Case)
        {
        case 0: Invalid.InputEvents[4].After = Invalid.InputEvents[4].Before; break;
        case 1: Invalid.InputEvents[8].LaunchVelocity = FVector::ZeroVector; break;
        case 2: Invalid.InputEvents[3].bTargetAliveAfter = true; break;
        case 3: Invalid.InputEvents[0].After = FVector::ZeroVector; break;
        case 4: Invalid.Runs[1].Snapshot.LayoutHash ^= 1; break;
        case 5: Invalid.Runs[2].Snapshot.Seed = Invalid.Initial.Seed; break;
        case 6: Invalid.Runs[1].Actors[0].Key = Invalid.Runs[0].Actors[0].Key; break;
        case 7: Invalid.QueuedRuns.Last().Snapshot.PendingRequestId = 44; break;
        case 8: Invalid.Transitions.RemoveAt(1); break;
        case 9: Invalid.PathEvidence.MovementSamples[0].To.X = std::numeric_limits<double>::quiet_NaN(); break;
        case 10: Invalid.InputEvents[0].SourceRequestId = 99; break;
        case 11: Invalid.CompletionCount = 2; break;
        case 12: Invalid.RemainingDelegateBindings = 1; break;
        case 13: Invalid.RemainingPressedKeys = 1; break;
        case 14: Invalid.Runs[3].HudOverlayCount = 2; break;
        }
        TestFalse(TEXT("Tampered complete evidence cannot pass"), FSeedForgeInputSelfTestCodec::ValidateEvidence(Invalid, Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeInputCliOptionsTest,
    "SeedForge.Audit.InputSelfTest.CliOptionsSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInputCliOptionsTest::RunTest(const FString& Parameters)
{
    const FString TracePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/Reports/InputSelfTest/options-fixture.json")));
    const FString Good = FString::Printf(TEXT("-SeedForgeInputSelfTest -SeedForgeInputTrace=\"%s\" -SeedForgeGitSha=diagnostic-%s -SeedForgeSeed=24301"),
        *TracePath, *SeedForge::InputSelfTests::Revision);
    FSeedForgeInputSelfTestOptions Options;
    FString Error;
    TestTrue(TEXT("Explicit diagnostic CLI options are valid"), FSeedForgeInputSelfTestCodec::ParseOptions(*Good, Options, Error));
    TestTrue(TEXT("Opt-in is explicit"), Options.bEnabled);
    TestEqual(TEXT("Diagnostic prefix is preserved by options"), Options.SourceIdentity, TEXT("diagnostic-") + SeedForge::InputSelfTests::Revision);
    for (const FString& Bad : {Good + TEXT(" -SeedForgeGameplaySmoke"), Good + TEXT(" -SeedForgeCapturePath=bad.png"),
        FString(TEXT("-SeedForgeInputSelfTest")), Good.Replace(TEXT("24301"), TEXT("-1")),
        Good.Replace(*TracePath, TEXT("relative.json"))})
    {
        TestFalse(TEXT("Missing, malformed and conflicting options fail"), FSeedForgeInputSelfTestCodec::ParseOptions(*Bad, Options, Error));
    }
    TestTrue(TEXT("No opt-in leaves ordinary startup alone"), FSeedForgeInputSelfTestCodec::ParseOptions(TEXT("-game"), Options, Error));
    TestFalse(TEXT("Ordinary startup does not enable the self-test"), Options.bEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeInputDashBoundsTest,
    "SeedForge.Audit.InputSelfTest.DashRejectsUnboundedAndOffMapMotion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInputDashBoundsTest::RunTest(const FString& Parameters)
{
    AddInfo(TEXT("SYNTHETIC VALUE TAMPERING ONLY: finite input coordinates may still produce infinite derived distance."));
    const auto Complete = SeedForge::InputSelfTests::MakeSyntheticSuccessEvidence(false);
    FString Error;
    TestTrue(TEXT("Complete bounded diagonal Dash remains the positive control"),
        FSeedForgeInputSelfTestCodec::ValidateEvidence(Complete, Error));
    const TCHAR* Cases[] = {TEXT("TooShort"), TEXT("NegativeDiagonal"), TEXT("WrongDirection"),
        TEXT("OffMapBothEndpoints"), TEXT("DerivedDistanceOverflow"), TEXT("TooLong"),
        TEXT("OffMapStart"), TEXT("OffMapEnd")};
    for (int32 Case = 0; Case < UE_ARRAY_COUNT(Cases); ++Case)
    {
        auto Invalid = Complete;
        auto& Dash = Invalid.InputEvents[8];
        const FVector Diagonal = FVector(1,1,0).GetSafeNormal();
        switch (Case)
        {
        case 0: Dash.After = Dash.Before + Diagonal * 2; break;
        case 1: Dash.After = Dash.Before - Diagonal * 20; break;
        case 2: Dash.After = Dash.Before + FVector(-1,1,0).GetSafeNormal() * 20; break;
        case 3: Dash.Before = FVector(10000000,10000000,96); Dash.After = Dash.Before + Diagonal * 20; break;
        case 4:
            Dash.Before = FVector(std::numeric_limits<double>::max() / 2, std::numeric_limits<double>::max() / 2, 96);
            Dash.After = -Dash.Before; break;
        case 5: Dash.After = Dash.Before + Diagonal * 201; break;
        case 6: Dash.Before = FVector(-10000000,-10000000,96); break;
        case 7: Dash.After = FVector(10000000,10000000,96); break;
        }
        TestFalse(Cases[Case], FSeedForgeInputSelfTestCodec::ValidateEvidence(Invalid, Error));
    }
    return true;
}

#endif

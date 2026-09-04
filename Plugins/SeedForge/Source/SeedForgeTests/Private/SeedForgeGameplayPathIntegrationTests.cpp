#include "SeedForgeInputTestWorld.h"
#include "SeedForgeGameplayDiagnostics.h"
#include "SeedForgeGridPathfinder.h"
#include "TimerManager.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::GameplayPathTests
{
    FSeedForgePathResult MakePath()
    {
        FSeedForgePathRequest Request;
        Request.Start = FIntPoint(0, 0);
        Request.Goal = FIntPoint(2, 0);
        Request.WalkableCells = {Request.Start, FIntPoint(1, 0), Request.Goal};
        return FSeedForgeGridPathfinder::FindPath(Request);
    }

    FSeedForgeEnemyPathSnapshot MakeSnapshot()
    {
        FSeedForgeEnemyPathSnapshot Snapshot;
        Snapshot.Revision = 1;
        Snapshot.Waypoints = {FVector(200, 0, 58), FVector(400, 0, 58)};
        return Snapshot;
    }

    FSeedForgeEnemyMoveObservation MakeMove(uint64 Sequence, double FromX)
    {
        FSeedForgeEnemyMoveObservation Move;
        Move.Sequence = Sequence;
        Move.Frame = Sequence;
        Move.PathRevision = 1;
        Move.WaypointIndex = 0;
        Move.From = FVector(FromX, 0, 58);
        Move.Target = FVector(200, 0, 58);
        Move.To = FVector(FromX + 10, 0, 58);
        Move.DeltaSeconds = 0.05f;
        return Move;
    }

    class FObserveRealWorldPath final : public IAutomationLatentCommand
    {
    public:
        explicit FObserveRealWorldPath(FAutomationTestBase* InTest) : Test(InTest) {}
        ~FObserveRealWorldPath() override { Cleanup(); }

        bool Update() override
        {
            if (!Fixture)
            {
                Fixture = MakeUnique<SeedForge::InputTests::FWorldFixture>();
                Fixture->ApplyCurrent();
                for (TActorIterator<ASeedForgeEnemyPawn> It(Fixture->World); It; ++It)
                {
                    if (It->GetOwner() == Fixture->Coordinator &&
                        (!Target.IsValid() || It->GetStableId() < Target->GetStableId()))
                    {
                        Target = *It;
                    }
                }
                if (!Test->TestNotNull(TEXT("Genuine encounter has a live target"), Target.Get()))
                {
                    Cleanup();
                    return true;
                }
                Identity = Fixture->Coordinator->GetSnapshot();
                Walkable = Fixture->Coordinator->GetLayout().GetCanonicalWalkableCells();
                Tuning = Fixture->Coordinator->GetTuning();
                Before = Target->GetActorLocation();
                Handle = Fixture->Coordinator->OnEnemyPathApplied().AddLambda(
                    [this](ASeedForgeEnemyPawn* Enemy, uint64 Run, uint64 Request,
                        const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result)
                    {
                        if (Enemy != Target.Get()) { return; }
                        ++AppliedPaths;
                        Test->TestEqual(TEXT("Path observation uses actual applied run"), Run, Identity.RunGeneration);
                        Test->TestEqual(TEXT("Path observation uses actual applied request"), Request, Identity.AppliedRequestId);
                        Proof.ObserveAppliedPath(Enemy->GetStableId(), Run, Request, Start, Goal, Result,
                            Enemy->GetPathSnapshot(), Walkable, Tuning.CellSize);
                    });
                Fixture->BeginWorldTicks();
                Fixture->World->GetTimerManager().SetTimer(TimerProbe,
                    FTimerDelegate::CreateLambda([this] { ++TimerFirings; }), TickDelta, true);
                InitialEngineFrame = LastEngineFrame = GFrameCounter;
                return false;
            }

            // The engine owns GFrameCounter. TimerManager rejects duplicate ticks
            // in one engine frame, so never loop multiple World ticks in Update.
            if (GFrameCounter == LastEngineFrame) { return false; }
            LastEngineFrame = GFrameCounter;
            if (!Target.IsValid() || Target->IsActorBeingDestroyed())
            {
                Test->AddError(TEXT("Path target was destroyed before the bounded observation completed."));
                Cleanup();
                return true;
            }
            Fixture->TickWorld(TickDelta);
            ++Ticks;
            Proof.ObserveMovement(Target->GetStableId(), Identity.RunGeneration, Identity.AppliedRequestId,
                Target->GetPathSnapshot().LastMove, Tuning.EnemyMoveSpeed);
            if (!Proof.IsComplete() && Ticks < MaxTicks) { return false; }

            const double Distance = FVector::Dist2D(Before, Target->GetActorLocation());
            Test->AddInfo(FString::Printf(
                TEXT("Frame-driven fixture: ticks=%d timer_firings=%d engine_frames=%llu..%llu displacement=%.6f."),
                Ticks, TimerFirings, InitialEngineFrame, LastEngineFrame, Distance));
            Test->TestTrue(TEXT("Fixture timer advances across actual engine frames"), TimerFirings > 1);
            Test->TestTrue(TEXT("Each fixture tick used a distinct engine frame"), LastEngineFrame - InitialEngineFrame >= static_cast<uint64>(Ticks));
            Test->TestTrue(TEXT("Existing enemy movement remains finite"), FMath::IsFinite(Distance));
            Test->TestTrue(TEXT("Real enemy moved without directly ticking it"), Distance > 1.0);
            Test->TestTrue(TEXT("Actual displacement respects the two-second speed bound"), Distance <= Tuning.EnemyMoveSpeed * 2.0 + 0.1);
            Test->TestTrue(TEXT("Normal coordinator timer exposed a real A* application"), AppliedPaths > 0);
            Test->TestTrue(TEXT("Path and actual stored waypoints produced bounded movement proof"), Proof.IsComplete());
            Test->TestTrue(TEXT("Proof retains at most two movement samples"),
                Proof.GetEvidence().MovementSamples.Num() <= FSeedForgeEnemyPathProof::MaxRetainedMovementSamples);
            Cleanup();
            return true;
        }

    private:
        void Cleanup()
        {
            if (Fixture)
            {
                Fixture->Coordinator->OnEnemyPathApplied().Remove(Handle);
                Fixture->World->GetTimerManager().ClearTimer(TimerProbe);
                Fixture.Reset();
            }
        }

        static constexpr int32 MaxTicks = 120;
        static constexpr float TickDelta = 1.0f / 60.0f;
        FAutomationTestBase* Test;
        TUniquePtr<SeedForge::InputTests::FWorldFixture> Fixture;
        TWeakObjectPtr<ASeedForgeEnemyPawn> Target;
        FSeedForgeGameplaySnapshot Identity;
        FSeedForgeGameplayTuning Tuning;
        TArray<FIntPoint> Walkable;
        FVector Before = FVector::ZeroVector;
        FSeedForgeEnemyPathProof Proof;
        FDelegateHandle Handle;
        FTimerHandle TimerProbe;
        uint64 InitialEngineFrame = 0;
        uint64 LastEngineFrame = 0;
        int32 Ticks = 0;
        int32 TimerFirings = 0;
        int32 AppliedPaths = 0;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeWorldEnemyMovementProofTest,
    "SeedForge.Audit.GameplayPath.RealReplanAndMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeWorldEnemyMovementProofTest::RunTest(const FString& Parameters)
{
    FAutomationTestFramework::Get().EnqueueLatentCommand(
        MakeShared<SeedForge::GameplayPathTests::FObserveRealWorldPath>(this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePathProofPositiveTest,
    "SeedForge.Audit.GameplayPath.ProofRequiresActualBoundedSamples",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathProofPositiveTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::GameplayPathTests;
    const auto Path = MakePath();
    FSeedForgeEnemyPathProof Proof;
    TestTrue(TEXT("Real A* value is valid"), Path.IsSuccess());
    TestTrue(TEXT("Matching applied path is accepted"), Proof.ObserveAppliedPath(3, 7, 11,
        FIntPoint(0, 0), FIntPoint(2, 0), Path, MakeSnapshot(), Path.Path, 200));
    TestFalse(TEXT("Path receipt alone is not movement evidence"), Proof.IsComplete());
    TestTrue(TEXT("First finite real-shaped sample is accepted"), Proof.ObserveMovement(3, 7, 11, MakeMove(1, 0), 260));
    TestFalse(TEXT("One sample cannot complete proof"), Proof.IsComplete());
    TestTrue(TEXT("Second distinct bounded sample is accepted"), Proof.ObserveMovement(3, 7, 11, MakeMove(2, 10), 260));
    TestTrue(TEXT("Two samples and 20 units complete proof"), Proof.IsComplete());
    const int32 Retained = Proof.GetEvidence().MovementSamples.Num();
    for (int32 Index = 0; Index < 4096; ++Index)
    {
        Proof.ObserveMovement(3, 7, 11, MakeMove(3 + Index, 20), 260);
    }
    TestEqual(TEXT("Completed proof does not grow per frame"), Proof.GetEvidence().MovementSamples.Num(), Retained);
    TestTrue(TEXT("Movement storage is bounded"), Retained <= FSeedForgeEnemyPathProof::MaxRetainedMovementSamples);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgePathProofRejectionTest,
    "SeedForge.Audit.GameplayPath.RejectsInvalidAttributionAndBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgePathProofRejectionTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::GameplayPathTests;
    const auto Path = MakePath();
    for (int32 Case = 0; Case < 5; ++Case)
    {
        FSeedForgeEnemyPathProof Proof;
        Proof.ObserveAppliedPath(3, 7, 11, FIntPoint(0, 0), FIntPoint(2, 0), Path, MakeSnapshot(), Path.Path, 200);
        auto Move = MakeMove(1, 0);
        if (Case == 2) { Move.PathRevision = 99; }
        if (Case == 3) { Move.To.Y = 80; }
        if (Case == 4) { Move.To.X = std::numeric_limits<double>::quiet_NaN(); }
        TestFalse(TEXT("Wrong identity/revision or nonfinite/off-route motion is rejected"),
            Proof.ObserveMovement(3, Case == 0 ? 8 : 7, Case == 1 ? 12 : 11, Move, 260));
        TestFalse(TEXT("Rejected observations do not complete proof"), Proof.IsComplete());
    }
    FSeedForgeEnemyPathProof Oversized;
    auto TooLong = Path;
    TooLong.Path.SetNum(FSeedForgeEnemyPathProof::MaxPathCells + 1);
    TestFalse(TEXT("Oversized route cannot enter retained proof"), Oversized.ObserveAppliedPath(3, 7, 11,
        FIntPoint(0, 0), FIntPoint(2, 0), TooLong, MakeSnapshot(), Path.Path, 200));
    return true;
}

#endif

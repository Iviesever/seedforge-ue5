#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTLS.h"
#include "Misc/AutomationTest.h"
#include "SeedForgeAsync.h"
#include "SeedForgeGenerator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::Tests
{
    struct FAsyncObservation
    {
        TUniquePtr<FSeedForgeAsyncCoordinator> Coordinator = MakeUnique<FSeedForgeAsyncCoordinator>();
        TAtomic<bool> WorkerStarted{false};
        TAtomic<bool> ReleaseWorker{false};
        TAtomic<bool> WorkerFinished{false};
        TAtomic<uint32> ObservedWorkerThreadId{0};
        bool bApplied = false;
        bool bAppliedOnGameThread = false;
        uint32 ApplyThreadId = 0;
        uint64 ExpectedRequestId = 0;
        TArray<uint64> AppliedRequestIds;
        double DeadlineSeconds = FPlatformTime::Seconds() + 5.0;
        int32 SettlingFrames = 3;
        bool bActionTaken = false;

        ~FAsyncObservation()
        {
            ReleaseWorker.Store(true);
            if (Coordinator)
            {
                Coordinator->Shutdown();
            }
        }
    };

    using FAsyncObservationPtr = TSharedPtr<FAsyncObservation, ESPMode::ThreadSafe>;

    struct FAsyncRepetitionObservation
    {
        TUniquePtr<FSeedForgeAsyncCoordinator> Coordinator = MakeUnique<FSeedForgeAsyncCoordinator>();
        FSeedForgeLayout Baseline;
        FSeedForgeResult CurrentResult;
        bool bCompletionReady = false;
        int32 CompletedRepetitions = 0;
        double DeadlineSeconds = FPlatformTime::Seconds() + 15.0;

        ~FAsyncRepetitionObservation()
        {
            if (Coordinator)
            {
                Coordinator->Shutdown();
            }
        }
    };

    using FAsyncRepetitionObservationPtr = TSharedPtr<FAsyncRepetitionObservation, ESPMode::ThreadSafe>;

    void StartNextAsyncRepetition(const FAsyncRepetitionObservationPtr& Observation)
    {
        Observation->bCompletionReady = false;
        const TWeakPtr<FAsyncRepetitionObservation, ESPMode::ThreadSafe> WeakObservation(Observation);
        Observation->Coordinator->Start(
            []()
            {
                return FSeedForgeGenerator::Generate(0x5EEDULL, {});
            },
            [WeakObservation](FSeedForgeAsyncCompletion&& Completion)
            {
                const FAsyncRepetitionObservationPtr Pinned = WeakObservation.Pin();
                if (!Pinned)
                {
                    return;
                }
                Pinned->CurrentResult = MoveTemp(Completion.Result);
                Pinned->bCompletionReady = true;
            });
    }
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FWaitForSeedForgeApply,
    SeedForge::Tests::FAsyncObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FWaitForSeedForgeApply::Update()
{
    if (Observation->bApplied)
    {
        Test->TestTrue(TEXT("Apply callback runs on the game thread"), Observation->bAppliedOnGameThread);
        Test->TestEqual(
            TEXT("Apply callback reports the active request"),
            Observation->AppliedRequestIds.Last(),
            Observation->ExpectedRequestId);
        Test->TestNotEqual(
            TEXT("Work executes away from the game thread"),
            Observation->ObservedWorkerThreadId.Load(),
            Observation->ApplyThreadId);
        Observation->Coordinator.Reset();
        return true;
    }

    if (FPlatformTime::Seconds() >= Observation->DeadlineSeconds)
    {
        Test->AddError(TEXT("Timed out waiting for SeedForge async apply."));
        Observation->ReleaseWorker.Store(true);
        Observation->Coordinator.Reset();
        return true;
    }
    return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FCancelSeedForgeAfterWorkerStarts,
    SeedForge::Tests::FAsyncObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FCancelSeedForgeAfterWorkerStarts::Update()
{
    if (!Observation->bActionTaken && Observation->WorkerStarted.Load())
    {
        Observation->Coordinator->CancelActive();
        Observation->ReleaseWorker.Store(true);
        Observation->bActionTaken = true;
        return false;
    }

    if (Observation->bActionTaken && Observation->WorkerFinished.Load())
    {
        if (--Observation->SettlingFrames <= 0)
        {
            Test->TestFalse(TEXT("In-flight cancellation suppresses apply"), Observation->bApplied);
            Observation->Coordinator.Reset();
            return true;
        }
    }

    if (FPlatformTime::Seconds() >= Observation->DeadlineSeconds)
    {
        Test->AddError(TEXT("Timed out waiting to observe and cancel in-flight work."));
        Observation->ReleaseWorker.Store(true);
        Observation->Coordinator.Reset();
        return true;
    }
    return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FWaitForSeedForgeStaleSuppression,
    SeedForge::Tests::FAsyncObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FWaitForSeedForgeStaleSuppression::Update()
{
    if (Observation->bApplied)
    {
        if (--Observation->SettlingFrames <= 0)
        {
            Test->TestEqual(TEXT("Only one request is applied"), Observation->AppliedRequestIds.Num(), 1);
            if (Observation->AppliedRequestIds.Num() == 1)
            {
                Test->TestEqual(
                    TEXT("Only the newest request is applied"),
                    Observation->AppliedRequestIds[0],
                    Observation->ExpectedRequestId);
            }
            Observation->Coordinator.Reset();
            return true;
        }
    }

    if (FPlatformTime::Seconds() >= Observation->DeadlineSeconds)
    {
        Test->AddError(TEXT("Timed out waiting for newest-request apply."));
        Observation->Coordinator.Reset();
        return true;
    }
    return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FWaitFramesForSeedForgeSuppression,
    SeedForge::Tests::FAsyncObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FWaitFramesForSeedForgeSuppression::Update()
{
    if (--Observation->SettlingFrames <= 0)
    {
        Test->TestFalse(TEXT("Cancelled or shut-down work does not apply"), Observation->bApplied);
        Observation->Coordinator.Reset();
        return true;
    }
    return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FRunSeedForgeAsyncRepetitions,
    SeedForge::Tests::FAsyncRepetitionObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FRunSeedForgeAsyncRepetitions::Update()
{
    if (Observation->bCompletionReady)
    {
        if (!Observation->CurrentResult.IsSuccess()
            || !(Observation->CurrentResult.Layout == Observation->Baseline))
        {
            Test->AddError(FString::Printf(
                TEXT("Async repetition %d diverged from the synchronous baseline."),
                Observation->CompletedRepetitions));
            Observation->Coordinator.Reset();
            return true;
        }

        ++Observation->CompletedRepetitions;
        if (Observation->CompletedRepetitions == 100)
        {
            Test->TestEqual(
                TEXT("Exactly 100 asynchronous repetitions completed"),
                Observation->CompletedRepetitions,
                100);
            Observation->Coordinator.Reset();
            return true;
        }
        SeedForge::Tests::StartNextAsyncRepetition(Observation);
    }

    if (FPlatformTime::Seconds() >= Observation->DeadlineSeconds)
    {
        Test->AddError(FString::Printf(
            TEXT("Timed out after %d of 100 asynchronous repetitions."),
            Observation->CompletedRepetitions));
        Observation->Coordinator.Reset();
        return true;
    }
    return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAsyncRunsWorkerAndAppliesOnGameThreadTest,
    "SeedForge.Async.RunsWorkerAndAppliesOnGameThread",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAsyncRunsWorkerAndAppliesOnGameThreadTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FAsyncObservationPtr Observation = MakeShared<FAsyncObservation, ESPMode::ThreadSafe>();
    Observation->ExpectedRequestId = Observation->Coordinator->Start(
        [Observation]()
        {
            Observation->ObservedWorkerThreadId.Store(FPlatformTLS::GetCurrentThreadId());
            return FSeedForgeGenerator::Generate(0x5EEDULL, {});
        },
        [Observation](FSeedForgeAsyncCompletion&& Completion)
        {
            Observation->bAppliedOnGameThread = IsInGameThread();
            Observation->ApplyThreadId = FPlatformTLS::GetCurrentThreadId();
            Observation->AppliedRequestIds.Add(Completion.RequestId);
            Observation->bApplied = true;
        });

    ADD_LATENT_AUTOMATION_COMMAND(FWaitForSeedForgeApply(Observation, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAsyncCancelsInFlightWorkTest,
    "SeedForge.Async.CancelsInFlightWork",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAsyncCancelsInFlightWorkTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FAsyncObservationPtr Observation = MakeShared<FAsyncObservation, ESPMode::ThreadSafe>();
    Observation->Coordinator->Start(
        [Observation]()
        {
            Observation->WorkerStarted.Store(true);
            while (!Observation->ReleaseWorker.Load())
            {
                FPlatformProcess::SleepNoStats(0.001f);
            }
            Observation->WorkerFinished.Store(true);
            return FSeedForgeGenerator::Generate(91, {});
        },
        [Observation](FSeedForgeAsyncCompletion&& Completion)
        {
            Observation->bApplied = true;
            Observation->AppliedRequestIds.Add(Completion.RequestId);
        });

    ADD_LATENT_AUTOMATION_COMMAND(FCancelSeedForgeAfterWorkerStarts(Observation, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAsyncSuppressesStaleResultTest,
    "SeedForge.Async.SuppressStaleResult",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAsyncSuppressesStaleResultTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FAsyncObservationPtr Observation = MakeShared<FAsyncObservation, ESPMode::ThreadSafe>();
    Observation->Coordinator->Start(
        []()
        {
            return FSeedForgeGenerator::Generate(1, {});
        },
        [Observation](FSeedForgeAsyncCompletion&& Completion)
        {
            Observation->AppliedRequestIds.Add(Completion.RequestId);
            Observation->bApplied = true;
        });
    Observation->ExpectedRequestId = Observation->Coordinator->Start(
        []()
        {
            return FSeedForgeGenerator::Generate(2, {});
        },
        [Observation](FSeedForgeAsyncCompletion&& Completion)
        {
            Observation->AppliedRequestIds.Add(Completion.RequestId);
            Observation->bApplied = true;
        });

    ADD_LATENT_AUTOMATION_COMMAND(FWaitForSeedForgeStaleSuppression(Observation, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAsyncImmediateCancelSuppressesApplyTest,
    "SeedForge.Async.ImmediateCancelSuppressesApply",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAsyncImmediateCancelSuppressesApplyTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FAsyncObservationPtr Observation = MakeShared<FAsyncObservation, ESPMode::ThreadSafe>();
    Observation->Coordinator->Start(
        []()
        {
            return FSeedForgeGenerator::Generate(3, {});
        },
        [Observation](FSeedForgeAsyncCompletion&& Completion)
        {
            Observation->bApplied = true;
        });
    Observation->Coordinator->CancelActive();

    ADD_LATENT_AUTOMATION_COMMAND(FWaitFramesForSeedForgeSuppression(Observation, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeAsyncShutdownSuppressesApplyTest,
    "SeedForge.Async.ShutdownSuppressesApply",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeAsyncShutdownSuppressesApplyTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FAsyncObservationPtr Observation = MakeShared<FAsyncObservation, ESPMode::ThreadSafe>();
    Observation->Coordinator->Start(
        []()
        {
            return FSeedForgeGenerator::Generate(4, {});
        },
        [Observation](FSeedForgeAsyncCompletion&& Completion)
        {
            Observation->bApplied = true;
        });
    Observation->Coordinator->Shutdown();

    ADD_LATENT_AUTOMATION_COMMAND(FWaitFramesForSeedForgeSuppression(Observation, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeHundredAsyncRepetitionsTest,
    "SeedForge.Async.HundredSequentialRepetitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeHundredAsyncRepetitionsTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FSeedForgeResult Baseline = FSeedForgeGenerator::Generate(0x5EEDULL, {});
    TestTrue(TEXT("Synchronous baseline generation succeeds"), Baseline.IsSuccess());
    if (!Baseline.IsSuccess())
    {
        return false;
    }

    const FAsyncRepetitionObservationPtr Observation =
        MakeShared<FAsyncRepetitionObservation, ESPMode::ThreadSafe>();
    Observation->Baseline = Baseline.Layout;
    StartNextAsyncRepetition(Observation);

    ADD_LATENT_AUTOMATION_COMMAND(FRunSeedForgeAsyncRepetitions(Observation, this));
    return true;
}

#endif

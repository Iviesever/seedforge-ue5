#include "Misc/AutomationTest.h"
#include "SeedForgeWorldSubsystem.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::Tests
{
    struct FSubsystemObservation
    {
        FSubsystemObservation()
            : Subsystem(NewObject<USeedForgeWorldSubsystem>(GetTransientPackage()))
        {
        }

        TStrongObjectPtr<USeedForgeWorldSubsystem> Subsystem;
        bool bApplied = false;
        bool bAppliedOnGameThread = false;
        uint64 ExpectedRequestId = 0;
        uint64 AppliedRequestId = 0;
        FSeedForgeResult Result;
        double DeadlineSeconds = FPlatformTime::Seconds() + 5.0;
        int32 SettlingFrames = 3;
    };

    using FSubsystemObservationPtr = TSharedPtr<FSubsystemObservation, ESPMode::ThreadSafe>;

    void BindObservation(const FSubsystemObservationPtr& Observation)
    {
        const TWeakPtr<FSubsystemObservation, ESPMode::ThreadSafe> WeakObservation(Observation);
        Observation->Subsystem->OnGenerationApplied().AddLambda(
            [WeakObservation](const FSeedForgeAsyncCompletion& Completion)
            {
                const FSubsystemObservationPtr Pinned = WeakObservation.Pin();
                if (!Pinned)
                {
                    return;
                }
                Pinned->bApplied = true;
                Pinned->bAppliedOnGameThread = IsInGameThread();
                Pinned->AppliedRequestId = Completion.RequestId;
                Pinned->Result = Completion.Result;
            });
    }
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FWaitForSeedForgeSubsystemApply,
    SeedForge::Tests::FSubsystemObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FWaitForSeedForgeSubsystemApply::Update()
{
    if (Observation->bApplied)
    {
        Test->TestTrue(TEXT("Subsystem broadcasts on the game thread"), Observation->bAppliedOnGameThread);
        Test->TestEqual(
            TEXT("Subsystem broadcasts the returned request id"),
            Observation->AppliedRequestId,
            Observation->ExpectedRequestId);
        Test->TestTrue(TEXT("Subsystem broadcasts a successful generation"), Observation->Result.IsSuccess());
        Observation->Subsystem->Deinitialize();
        Observation->Subsystem.Reset();
        return true;
    }

    if (FPlatformTime::Seconds() >= Observation->DeadlineSeconds)
    {
        Test->AddError(TEXT("Timed out waiting for USeedForgeWorldSubsystem generation."));
        Observation->Subsystem->Deinitialize();
        Observation->Subsystem.Reset();
        return true;
    }
    return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FWaitForSeedForgeSubsystemDeinitializeSuppression,
    SeedForge::Tests::FSubsystemObservationPtr,
    Observation,
    FAutomationTestBase*,
    Test);

bool FWaitForSeedForgeSubsystemDeinitializeSuppression::Update()
{
    if (--Observation->SettlingFrames <= 0)
    {
        Test->TestFalse(TEXT("Deinitialized subsystem never broadcasts late work"), Observation->bApplied);
        Observation->Subsystem.Reset();
        return true;
    }
    return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeSubsystemAppliesGenerationTest,
    "SeedForge.Async.SubsystemAppliesGeneration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeSubsystemAppliesGenerationTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FSubsystemObservationPtr Observation = MakeShared<FSubsystemObservation, ESPMode::ThreadSafe>();
    BindObservation(Observation);
    Observation->ExpectedRequestId = Observation->Subsystem->RequestGeneration(0x5EEDULL, {});
    TestNotEqual(TEXT("Subsystem assigns a non-zero request id"), Observation->ExpectedRequestId, 0ULL);

    ADD_LATENT_AUTOMATION_COMMAND(FWaitForSeedForgeSubsystemApply(Observation, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeSubsystemDeinitializeSuppressesLateWorkTest,
    "SeedForge.Async.SubsystemDeinitializeSuppressesLateWork",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeSubsystemDeinitializeSuppressesLateWorkTest::RunTest(const FString& Parameters)
{
    using namespace SeedForge::Tests;
    const FSubsystemObservationPtr Observation = MakeShared<FSubsystemObservation, ESPMode::ThreadSafe>();
    BindObservation(Observation);
    Observation->Subsystem->RequestGeneration(0xC0FFEEULL, {});
    Observation->Subsystem->Deinitialize();

    ADD_LATENT_AUTOMATION_COMMAND(FWaitForSeedForgeSubsystemDeinitializeSuppression(Observation, this));
    return true;
}

#endif


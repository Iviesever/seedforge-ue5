#include "Misc/AutomationTest.h"
#include "SeedForgeRunState.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRunStateCompletesWinPathTest,
    "SeedForge.Model.RunState.CompletesWinPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRunStateCompletesWinPathTest::RunTest(const FString& Parameters)
{
    FSeedForgeRunStateMachine Run;
    TestEqual(TEXT("Run begins generating"), Run.GetState(), ESeedForgeRunState::Generating);
    TestTrue(TEXT("Generation can enter Playing"), Run.StartPlaying({30, 10, 20}).IsSuccess());
    TestEqual(TEXT("Run is Playing"), Run.GetState(), ESeedForgeRunState::Playing);
    TestEqual(TEXT("Three Core IDs are required"), Run.GetRequiredCoreCount(), 3);
    TestTrue(TEXT("First Core collects"), Run.CollectCore(20).IsSuccess());
    TestTrue(TEXT("Second Core collects"), Run.CollectCore(10).IsSuccess());
    TestFalse(TEXT("Exit remains locked before all Cores"), Run.IsExitUnlocked());
    TestTrue(TEXT("Final Core collects"), Run.CollectCore(30).IsSuccess());
    TestTrue(TEXT("All Cores unlock the exit"), Run.IsExitUnlocked());
    TestTrue(TEXT("Unlocked exit completes run"), Run.ReachExit().IsSuccess());
    TestEqual(TEXT("Run is Won"), Run.GetState(), ESeedForgeRunState::Won);
    const TArray<uint32> ExpectedCollected = {10, 20, 30};
    TestTrue(TEXT("Collected IDs stay canonical"), Run.GetCollectedCoreIds() == ExpectedCollected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRunStateLocksExitTest,
    "SeedForge.Model.RunState.LocksExit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRunStateLocksExitTest::RunTest(const FString& Parameters)
{
    FSeedForgeRunStateMachine Run;
    TestTrue(TEXT("Generation enters Playing"), Run.StartPlaying({0, 1, 2}).IsSuccess());
    const FSeedForgeRunTransitionResult Result = Run.ReachExit();
    TestFalse(TEXT("Locked exit rejects completion"), Result.IsSuccess());
    TestEqual(TEXT("Locked exit has typed failure"), Result.ErrorCode, ESeedForgeRunTransitionError::ExitLocked);
    TestEqual(TEXT("Locked exit failure keeps Playing"), Run.GetState(), ESeedForgeRunState::Playing);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRunStateRejectsCoreErrorsTest,
    "SeedForge.Model.RunState.RejectsCoreErrors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRunStateRejectsCoreErrorsTest::RunTest(const FString& Parameters)
{
    FSeedForgeRunStateMachine Run;
    const FSeedForgeRunTransitionResult BadSet = Run.StartPlaying({7, 7});
    TestFalse(TEXT("Duplicate required Core IDs are rejected"), BadSet.IsSuccess());
    TestEqual(TEXT("Duplicate required set has typed failure"), BadSet.ErrorCode, ESeedForgeRunTransitionError::InvalidCoreSet);
    TestEqual(TEXT("Invalid set keeps Generating"), Run.GetState(), ESeedForgeRunState::Generating);

    TestTrue(TEXT("Valid set starts Playing"), Run.StartPlaying({7, 8}).IsSuccess());
    const FSeedForgeRunTransitionResult Unknown = Run.CollectCore(99);
    TestEqual(TEXT("Unknown Core has typed failure"), Unknown.ErrorCode, ESeedForgeRunTransitionError::UnknownCore);
    TestTrue(TEXT("Known Core collects"), Run.CollectCore(7).IsSuccess());
    const FSeedForgeRunTransitionResult Duplicate = Run.CollectCore(7);
    TestEqual(TEXT("Duplicate collection has typed failure"), Duplicate.ErrorCode, ESeedForgeRunTransitionError::DuplicateCore);
    TestEqual(TEXT("Duplicate collection does not increment"), Run.GetCollectedCoreCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRunStateCompletesLossAndRestartTest,
    "SeedForge.Model.RunState.CompletesLossAndRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRunStateCompletesLossAndRestartTest::RunTest(const FString& Parameters)
{
    FSeedForgeRunStateMachine Run;
    TestTrue(TEXT("Generation enters Playing"), Run.StartPlaying({1}).IsSuccess());
    TestTrue(TEXT("Player death transitions"), Run.PlayerDied().IsSuccess());
    TestEqual(TEXT("Run is Lost"), Run.GetState(), ESeedForgeRunState::Lost);
    TestTrue(TEXT("Lost run requests restart"), Run.RequestRestart().IsSuccess());
    TestEqual(TEXT("Run is Restarting"), Run.GetState(), ESeedForgeRunState::Restarting);
    TestTrue(TEXT("Restart begins generation"), Run.BeginGenerating().IsSuccess());
    TestEqual(TEXT("Run returns to Generating"), Run.GetState(), ESeedForgeRunState::Generating);
    TestEqual(TEXT("Restart clears required Cores"), Run.GetRequiredCoreCount(), 0);
    TestEqual(TEXT("Restart clears collected Cores"), Run.GetCollectedCoreCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeRunStateFailsClosedTest,
    "SeedForge.Model.RunState.FailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeRunStateFailsClosedTest::RunTest(const FString& Parameters)
{
    FSeedForgeRunStateMachine Run;
    const FSeedForgeRunTransitionResult EarlyDeath = Run.PlayerDied();
    TestEqual(TEXT("Generating death is invalid"), EarlyDeath.ErrorCode, ESeedForgeRunTransitionError::InvalidTransition);
    TestEqual(TEXT("Invalid death keeps Generating"), Run.GetState(), ESeedForgeRunState::Generating);

    TestTrue(TEXT("Generation enters Playing"), Run.StartPlaying({1}).IsSuccess());
    const FSeedForgeRunTransitionResult DuplicateStart = Run.StartPlaying({1});
    TestEqual(TEXT("Second start is invalid"), DuplicateStart.ErrorCode, ESeedForgeRunTransitionError::InvalidTransition);
    TestEqual(TEXT("Second start keeps Playing"), Run.GetState(), ESeedForgeRunState::Playing);
    TestTrue(TEXT("Playing run can request restart"), Run.RequestRestart().IsSuccess());
    TestEqual(TEXT("Playing restart enters Restarting"), Run.GetState(), ESeedForgeRunState::Restarting);
    return true;
}

#endif

#include "Misc/AutomationTest.h"
#include "SeedForgeGameplaySmoke.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeGameplaySmokeSuccessTraceTest,
    "SeedForge.GameplaySmoke.SuccessTrace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeGameplaySmokeSuccessTraceTest::RunTest(const FString& Parameters)
{
    FSeedForgeGameplaySmokeTrace Trace;
    Trace.GitSha = TEXT("0123456789abcdef");
    Trace.EngineVersion = TEXT("5.8.0");
    Trace.Seed = MAX_uint64;
    Trace.LayoutHash = 7425849530159566348ULL;
    Trace.EncounterHash = 15303214708604970503ULL;
    Trace.ActorCounts = {1, 3, 5, 1};
    Trace.StateTransitions = {TEXT("Generating"), TEXT("Playing"), TEXT("Won")};
    Trace.Actions = {TEXT("Attack:Enemy:0"), TEXT("Collect:Core:0"), TEXT("ReachExit")};
    Trace.ScreenshotPaths = {TEXT("D:/SeedForge/start.png"), TEXT("D:/SeedForge/win.png")};
    Trace.bSuccess = true;

    const FString Json = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Trace);
    TestTrue(TEXT("Trace declares schema and version"), Json.StartsWith(TEXT("{\"schema\":\"seedforge.gameplay-smoke\",\"schemaVersion\":1,")));
    TestTrue(TEXT("Maximum uint64 seed is an exact string"), Json.Contains(TEXT("\"seed\":\"18446744073709551615\"")));
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

#endif

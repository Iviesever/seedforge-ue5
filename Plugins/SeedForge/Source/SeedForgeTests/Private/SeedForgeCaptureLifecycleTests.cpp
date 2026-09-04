#include "Misc/AutomationTest.h"
#include "SeedForgeCaptureTypes.h"
#include "SeedForgeGameplaySmoke.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::CaptureTests
{
    FSeedForgeCaptureRequest Request()
    {
        FSeedForgeCaptureRequest Value;
        Value.Token = FGuid(1, 2, 3, 4);
        Value.RunGeneration = 7;
        Value.SourceRequestId = 11;
        Value.Label = TEXT("start");
        Value.Path = TEXT("D:/program/SeedForge/Artifacts/start-token.png");
        Value.RequestedAtUtc = FDateTime(2026, 9, 4, 4, 0, 0);
        Value.RequestedFrame = 100;
        return Value;
    }

    bool Deliver(FSeedForgeCaptureLifecycle& Lifecycle, const FSeedForgeCaptureRequest& Request)
    {
        return Lifecycle.OnRendered(Request.Token, Request.RunGeneration, 101)
            && Lifecycle.OnPixels(Request.Token, Request.RunGeneration, Request.Path, Request.Size, 1280LL * 720, 101)
            && Lifecycle.OnProcessed(Request.Token, Request.RunGeneration, true,
                Request.RequestedAtUtc + FTimespan::FromSeconds(1), 101, 12000);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureOrderedCompletionTest,
    "SeedForge.Audit.Capture.OrderedExactlyOnceCompletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureOrderedCompletionTest::RunTest(const FString& Parameters)
{
    const auto Request = SeedForge::CaptureTests::Request();
    FSeedForgeCaptureLifecycle Lifecycle;
    TestTrue(TEXT("Valid capture begins"), Lifecycle.Begin(Request));
    TestEqual(TEXT("Request waits for a rendered frame"), Lifecycle.GetPhase(), ESeedForgeCapturePhase::AwaitRenderedFrame);
    TestFalse(TEXT("No second request can overwrite an active token"), Lifecycle.Begin(Request));
    TestFalse(TEXT("Pixels before rendered acknowledgement are rejected"), Lifecycle.OnPixels(
        Request.Token, Request.RunGeneration, Request.Path, Request.Size, 1280LL * 720, 101));
    TestTrue(TEXT("Ordered render/pixels/process callbacks complete"), SeedForge::CaptureTests::Deliver(Lifecycle, Request));
    TestEqual(TEXT("Receipt is complete"), Lifecycle.GetPhase(), ESeedForgeCapturePhase::Complete);
    TestTrue(TEXT("Receipt belongs to its token"), Lifecycle.GetReceipt().Request.Token == Request.Token);
    TestEqual(TEXT("Receipt preserves real source request"), Lifecycle.GetReceipt().Request.SourceRequestId, 11ULL);
    TestTrue(TEXT("Receipt is successful"), Lifecycle.GetReceipt().bSuccess);
    TestFalse(TEXT("Duplicate processed notification cannot complete twice"), Lifecycle.OnProcessed(
        Request.Token, Request.RunGeneration, true, Request.RequestedAtUtc + FTimespan::FromSeconds(2), 102, 12000));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureWrongIdentityTest,
    "SeedForge.Audit.Capture.RejectsWrongIdentityAndPixels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureWrongIdentityTest::RunTest(const FString& Parameters)
{
    const auto Request = SeedForge::CaptureTests::Request();
    FSeedForgeCaptureLifecycle Lifecycle;
    TestTrue(TEXT("Request begins"), Lifecycle.Begin(Request));
    TestFalse(TEXT("Foreign token cannot acknowledge render"), Lifecycle.OnRendered(FGuid(9, 9, 9, 9), Request.RunGeneration, 101));
    TestFalse(TEXT("Old run cannot acknowledge render"), Lifecycle.OnRendered(Request.Token, Request.RunGeneration - 1, 101));
    TestFalse(TEXT("Pre-request frame cannot acknowledge render"), Lifecycle.OnRendered(Request.Token, Request.RunGeneration, 99));
    TestTrue(TEXT("Own current rendered frame is accepted"), Lifecycle.OnRendered(Request.Token, Request.RunGeneration, 101));
    TestFalse(TEXT("Foreign path cannot provide pixels"), Lifecycle.OnPixels(Request.Token, Request.RunGeneration,
        TEXT("D:/foreign.png"), Request.Size, 1280LL * 720, 101));
    TestFalse(TEXT("Wrong dimensions cannot provide pixels"), Lifecycle.OnPixels(Request.Token, Request.RunGeneration,
        Request.Path, FIntPoint(640, 360), 640LL * 360, 101));
    TestFalse(TEXT("Incomplete pixel buffer is rejected"), Lifecycle.OnPixels(Request.Token, Request.RunGeneration,
        Request.Path, Request.Size, 1, 101));
    TestFalse(TEXT("Pixels before acknowledged frame are rejected"), Lifecycle.OnPixels(Request.Token, Request.RunGeneration,
        Request.Path, Request.Size, 1280LL * 720, 100));
    TestTrue(TEXT("Matching complete pixels are accepted"), Lifecycle.OnPixels(Request.Token, Request.RunGeneration,
        Request.Path, Request.Size, 1280LL * 720, 101));
    TestFalse(TEXT("Foreign processed token is ignored"), Lifecycle.OnProcessed(FGuid(9, 9, 9, 9), Request.RunGeneration,
        true, Request.RequestedAtUtc + FTimespan::FromSeconds(1), 101, 12000));
    TestEqual(TEXT("Wrong callback did not complete request"), Lifecycle.GetPhase(), ESeedForgeCapturePhase::AwaitProcessed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureCancelRestartTest,
    "SeedForge.Audit.Capture.CancelAndRestartRejectLateCallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureCancelRestartTest::RunTest(const FString& Parameters)
{
    auto Request = SeedForge::CaptureTests::Request();
    const auto Old = Request;
    FSeedForgeCaptureLifecycle Lifecycle;
    TestTrue(TEXT("Old request begins"), Lifecycle.Begin(Old));
    Lifecycle.Cancel();
    TestEqual(TEXT("Cancel removes pending phase"), Lifecycle.GetPhase(), ESeedForgeCapturePhase::Idle);
    TestFalse(TEXT("Cancelled token cannot advance"), Lifecycle.OnRendered(Old.Token, Old.RunGeneration, 101));
    Request.Token = FGuid(5, 6, 7, 8);
    ++Request.RunGeneration;
    TestTrue(TEXT("New run request begins"), Lifecycle.Begin(Request));
    TestFalse(TEXT("Old callback cannot advance new run"), Lifecycle.OnRendered(Old.Token, Old.RunGeneration, 101));
    TestTrue(TEXT("New token completes independently"), SeedForge::CaptureTests::Deliver(Lifecycle, Request));
    Lifecycle.Cancel();
    TestFalse(TEXT("Cancelled receipt no longer claims success"), Lifecycle.GetReceipt().bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureFailedSaveTest,
    "SeedForge.Audit.Capture.FailedWriteOrStaleCompletionFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureFailedSaveTest::RunTest(const FString& Parameters)
{
    const auto Request = SeedForge::CaptureTests::Request();
    for (int32 Case = 0; Case < 4; ++Case)
    {
        FSeedForgeCaptureLifecycle Lifecycle;
        TestTrue(TEXT("Request begins"), Lifecycle.Begin(Request));
        TestTrue(TEXT("Render accepted"), Lifecycle.OnRendered(Request.Token, Request.RunGeneration, 101));
        TestTrue(TEXT("Pixels accepted"), Lifecycle.OnPixels(Request.Token, Request.RunGeneration,
            Request.Path, Request.Size, 1280LL * 720, 101));
        TestFalse(TEXT("Invalid completion cannot succeed"), Lifecycle.OnProcessed(Request.Token, Request.RunGeneration,
            Case != 0, Request.RequestedAtUtc + FTimespan::FromSeconds(Case == 2 ? -1 : 1),
            Case == 3 ? 100 : 101, Case == 1 ? 100 : 12000));
        TestEqual(TEXT("Invalid owned completion is a terminal failure"), Lifecycle.GetPhase(), ESeedForgeCapturePhase::Failed);
        TestFalse(TEXT("Failed receipt is not success"), Lifecycle.GetReceipt().bSuccess);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureInvalidRequestTest,
    "SeedForge.Audit.Capture.InvalidRequestRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureInvalidRequestTest::RunTest(const FString& Parameters)
{
    for (int32 Case = 0; Case < 6; ++Case)
    {
        auto Request = SeedForge::CaptureTests::Request();
        if (Case == 0) { Request.Token.Invalidate(); }
        if (Case == 1) { Request.RunGeneration = 0; }
        if (Case == 2) { Request.Path.Reset(); }
        if (Case == 3) { Request.Label.Reset(); }
        if (Case == 4) { Request.Size.X = 0; }
        if (Case == 5) { Request.RequestedAtUtc = FDateTime(); }
        FSeedForgeCaptureLifecycle Lifecycle;
        TestFalse(TEXT("Invalid request is rejected"), Lifecycle.Begin(Request));
        TestEqual(TEXT("Invalid request does not become pending"), Lifecycle.GetPhase(), ESeedForgeCapturePhase::Idle);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureReceiptTraceTest,
    "SeedForge.Audit.Capture.TraceContainsCallbackReceipt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureReceiptTraceTest::RunTest(const FString& Parameters)
{
    FSeedForgeGameplaySmokeTrace Trace;
    Trace.RunGeneration = 7;
    Trace.AppliedRequestId = 11;
    FSeedForgeCaptureReceipt Receipt;
    Receipt.Request = SeedForge::CaptureTests::Request();
    Receipt.RenderedFrame = 101;
    Receipt.CapturedFrame = 101;
    Receipt.CompletedFrame = 101;
    Receipt.CompletedAtUtc = Receipt.Request.RequestedAtUtc + FTimespan::FromSeconds(1);
    Receipt.FileBytes = 12000;
    Receipt.bSuccess = true;
    Trace.Captures.Add(Receipt);
    const FString Json = FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(Trace);
    TestTrue(TEXT("Trace identifies the applied run independently of captures"), Json.Contains(TEXT("\"runGeneration\":\"7\",\"appliedRequestId\":\"11\"")));
    TestTrue(TEXT("Capture token is serialized"), Json.Contains(TEXT("\"token\":\"00000001000000020000000300000004\"")));
    TestTrue(TEXT("Capture run/request remain independent exact strings"), Json.Contains(TEXT("\"runGeneration\":\"7\",\"sourceRequestId\":\"11\"")));
    TestTrue(TEXT("Capture frames and pixel size are explicit"), Json.Contains(TEXT("\"requestedFrame\":\"100\",\"renderedFrame\":\"101\",\"capturedFrame\":\"101\",\"completedFrame\":\"101\"")));
    TestTrue(TEXT("Capture width is recorded"), Json.Contains(TEXT("\"width\":1280,\"height\":720")));
    TestTrue(TEXT("Capture success is recorded only in receipt"), Json.Contains(TEXT("\"fileBytes\":12000,\"success\":true")));
    return true;
}

#endif

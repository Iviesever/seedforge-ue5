#include "SeedForgeInputTestWorld.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"
#include "SeedForgeGameplayCapture.h"
#include "Slate/SceneViewport.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureMissingViewportTest,
    "SeedForge.Audit.Capture.ComponentRejectsMissingViewport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureMissingViewportTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    auto* Capture = Fixture.Coordinator->FindComponentByClass<USeedForgeGameplayCaptureComponent>();
    TestNotNull(TEXT("Coordinator owns its capture component"), Capture);
    if (!Capture) { return false; }
    TestFalse(TEXT("Headless World cannot invent render completion"), Capture->RequestCapture(
        TEXT("start"), FPaths::ProjectSavedDir() / TEXT("CaptureComponent/no-viewport.png"), 1, 1));
    TestEqual(TEXT("Rejected request has no native bindings"), Capture->GetNativeBindingCount(), 0);
    TestFalse(TEXT("Rejected request has an explicit diagnostic"), Capture->GetLastError().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeCaptureNativeCleanupTest,
    "SeedForge.Audit.Capture.ComponentCancelAndEndPlayUnbind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeCaptureNativeCleanupTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    auto* Capture = Fixture.Coordinator->FindComponentByClass<USeedForgeGameplayCaptureComponent>();
    TestNotNull(TEXT("Coordinator owns capture component"), Capture);
    if (!Capture) { return false; }
    FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(Fixture.World);
    UGameViewportClient* Previous = Context.GameViewport;
    UGameViewportClient* Client = NewObject<UGameViewportClient>(GEngine);
    Client->AddToRoot();
    Context.GameViewport = Client;
    TSharedPtr<FSceneViewport> Viewport = FSceneViewport::Create(TStrongPtrVariant<FViewportClient>(Client), nullptr);
    Client->Viewport = Viewport.Get();
    const bool ScreenMessagesBefore = GAreScreenMessagesEnabled;
    const FString Path = FPaths::ProjectSavedDir() / TEXT("CaptureComponent/cleanup.png");
    const int32 MovieBefore = GIsDumpingMovie;
    const bool HighResBefore = GIsHighResScreenshot;
    for (int32 Mode = 0; Mode < 2; ++Mode)
    {
        GIsDumpingMovie = Mode == 0 ? 1 : MovieBefore;
        GIsHighResScreenshot = Mode == 1 ? true : HighResBefore;
        TestFalse(TEXT("Foreign movie/highres capture blocks before binding"), Capture->RequestCapture(TEXT("start"), Path, 1, 1));
        TestEqual(TEXT("Busy capture mode receives no interceptor"), Capture->GetNativeBindingCount(), 0);
        TestEqual(TEXT("Request did not consume movie state"), GIsDumpingMovie, Mode == 0 ? 1 : MovieBefore);
        TestEqual(TEXT("Request did not consume highres state"), GIsHighResScreenshot, Mode == 1 ? true : HighResBefore);
        Capture->CancelCapture();
        GIsDumpingMovie = MovieBefore;
        GIsHighResScreenshot = HighResBefore;
    }
    TestFalse(TEXT("Explicit capture root confines output even inside a broader project"), Capture->RequestCapture(
        TEXT("start"), Path, 1, 1, FPaths::ProjectSavedDir() / TEXT("CaptureComponent/isolated")));
    Capture->CancelCapture();
    TestTrue(TEXT("Explicit host root accepts its own output"), Capture->RequestCapture(
        TEXT("start"), Path, 1, 1, FPaths::ProjectSavedDir() / TEXT("CaptureComponent")));
    Capture->CancelCapture();
    // Isolate component error notification; runtime exit behavior is a separate process gate.
    Capture->OnFailed.RemoveAll(Fixture.Coordinator);
    int32 FailureNotifications = 0;
    const FDelegateHandle FailureHandle = Capture->OnFailed.AddLambda([&FailureNotifications](const FString&) { ++FailureNotifications; });
    for (int32 Mode = 0; Mode < 2; ++Mode)
    {
        TestTrue(TEXT("Normal request starts before foreign mode changes"), Capture->RequestCapture(TEXT("start"), Path, 1, 1));
        GIsDumpingMovie = Mode == 0 ? 1 : MovieBefore;
        GIsHighResScreenshot = Mode == 1 ? true : HighResBefore;
        UGameViewportClient::OnViewportRendered().Broadcast(Viewport.Get());
        TestEqual(TEXT("Rendered-frame recheck reports one failure"), FailureNotifications, Mode + 1);
        TestEqual(TEXT("Rendered-frame recheck unbinds without interception"), Capture->GetNativeBindingCount(), 0);
        TestFalse(TEXT("Foreign mode did not receive an owned screenshot request"), FScreenshotRequest::IsScreenshotRequested());
        Capture->CancelCapture();
        FScreenshotRequest::Reset();
        GIsDumpingMovie = MovieBefore;
        GIsHighResScreenshot = HighResBefore;
        GAreScreenMessagesEnabled = ScreenMessagesBefore;
        GScreenMessagesRestoreState = ScreenMessagesBefore;
    }
    Capture->OnFailed.Remove(FailureHandle);
    TestTrue(TEXT("Viewport-backed request registers render listener"), Capture->RequestCapture(TEXT("start"), Path, 1, 1));
    TestEqual(TEXT("Only render acknowledgement is initially bound"), Capture->GetNativeBindingCount(), 1);
    // This is a delegate-lifecycle simulation with a real FSceneViewport, not GPU/render evidence.
    UGameViewportClient::OnViewportRendered().Broadcast(Viewport.Get());
    TestEqual(TEXT("Pixel and processed boundaries are bound after rendered event"), Capture->GetNativeBindingCount(), 2);
    TestTrue(TEXT("Owned native screenshot request is pending"), FScreenshotRequest::IsScreenshotRequested());
    Capture->CancelCapture();
    TestEqual(TEXT("Cancel unregisters actual native delegates"), Capture->GetNativeBindingCount(), 0);
    TestFalse(TEXT("Cancel removes its pending global screenshot"), FScreenshotRequest::IsScreenshotRequested());
    TestEqual(TEXT("Cancel restores prior screen message flag"), GAreScreenMessagesEnabled, ScreenMessagesBefore);
    TestTrue(TEXT("A fresh request is possible after cancellation"), Capture->RequestCapture(TEXT("start"), Path, 2, 3));
    UGameViewportClient::OnViewportRendered().Broadcast(Viewport.Get());
    Fixture.Coordinator->Destroy(true);
    TestEqual(TEXT("EndPlay removes native bindings"), Capture->GetNativeBindingCount(), 0);
    TestFalse(TEXT("EndPlay removes owned screenshot request"), FScreenshotRequest::IsScreenshotRequested());
    TestEqual(TEXT("EndPlay restores screen message flag"), GAreScreenMessagesEnabled, ScreenMessagesBefore);
    Capture->CancelCapture();
    Client->Viewport = nullptr;
    Context.GameViewport = Previous;
    Viewport.Reset();
    Client->RemoveFromRoot();
    return true;
}

#endif

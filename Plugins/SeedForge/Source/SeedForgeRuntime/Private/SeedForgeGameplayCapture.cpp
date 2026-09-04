#include "SeedForgeGameplayCapture.h"

#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/PlatformFileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "SeedForgeRuntime.h"
#include "UObject/UObjectIterator.h"
#include "UnrealClient.h"

USeedForgeGameplayCaptureComponent::USeedForgeGameplayCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool USeedForgeGameplayCaptureComponent::RequestCapture(
    const FString& Label, const FString& OutputPath, uint64 Run, uint64 SourceRequestId, const FString& AllowedRoot)
{
    check(IsInGameThread());
    LastError.Reset();
    UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    if (!Viewport || GetNativeBindingCount() != 0 || GIsDumpingMovie || GIsHighResScreenshot
        || FScreenshotRequest::IsScreenshotRequested()
        || UGameViewportClient::OnScreenshotCaptured().IsBound())
    {
        LastError = TEXT("Capture requires an available owning viewport and an idle screenshot boundary.");
        return false;
    }
    FSeedForgeCaptureRequest Request;
    Request.Token = FGuid::NewGuid();
    Request.RunGeneration = Run;
    Request.SourceRequestId = SourceRequestId;
    Request.Label = Label;
    Request.Path = FPaths::ConvertRelativePathToFull(OutputPath);
    if (Label != TEXT("single"))
    {
        Request.Path = FPaths::Combine(FPaths::GetPath(Request.Path),
            FPaths::GetBaseFilename(Request.Path) + TEXT("-")
            + Request.Token.ToString(EGuidFormats::DigitsLower) + TEXT(".png"));
    }
    FPaths::MakeStandardFilename(Request.Path);
    FString CaptureRoot = FPaths::ConvertRelativePathToFull(AllowedRoot.IsEmpty() ? FPaths::ProjectDir() : AllowedRoot);
    FPaths::MakeStandardFilename(CaptureRoot);
    Request.RequestedAtUtc = FDateTime::UtcNow();
    Request.RequestedFrame = GFrameCounter;
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    if (!FPaths::IsUnderDirectory(Request.Path, CaptureRoot)
        || !FPaths::GetExtension(Request.Path).Equals(TEXT("png"), ESearchCase::IgnoreCase)
        || Files.FileExists(*Request.Path)
        || !Files.CreateDirectoryTree(*FPaths::GetPath(Request.Path))
        || !Lifecycle.Begin(Request))
    {
        LastError = TEXT("Capture requires a valid fresh PNG path inside the explicit capture root and a valid run identity.");
        return false;
    }
    OwningViewport = Viewport;
    bSavedPixels = false;
    LastPendingPrimitives = INDEX_NONE;
    RenderHandle = UGameViewportClient::OnViewportRendered().AddUObject(
        this, &USeedForgeGameplayCaptureComponent::HandleRendered, Request.Token, Run);
    GetWorld()->GetTimerManager().SetTimer(Watchdog, this,
        &USeedForgeGameplayCaptureComponent::Timeout, 8.0f, false);
    return true;
}

void USeedForgeGameplayCaptureComponent::HandleRendered(FViewport* Viewport, FGuid Token, uint64 Run)
{
    const auto& Request = Lifecycle.GetReceipt().Request;
    if (Request.Token != Token || Request.RunGeneration != Run
        || Lifecycle.GetPhase() != ESeedForgeCapturePhase::AwaitRenderedFrame
        || !Viewport || !OwningViewport.IsValid() || OwningViewport->Viewport != Viewport)
    {
        return;
    }
    if (GIsDumpingMovie || GIsHighResScreenshot || FScreenshotRequest::IsScreenshotRequested()
        || UGameViewportClient::OnScreenshotCaptured().IsBound())
    {
        Fail(TEXT("Another screenshot owns the viewport at the requested rendered frame."));
        return;
    }
    // UE 5.8 deliberately hides the first three standalone presents while driver
    // caches initialize. A startup canvas callback is not yet a gameplay frame.
    constexpr uint64 StandaloneHiddenStartupFrames = 3;
    if ((!GIsEditor && GFrameCounter < StandaloneHiddenStartupFrames)
        || !FViewport::IsGameRenderingEnabled() || OwningViewport->bDisableWorldRendering) { return; }
    int32 PendingPrimitives = 0;
    int32 VisiblePrimitives = 0;
    for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
    {
        if (It->GetWorld() == GetWorld() && It->IsRegistered() && It->ShouldRender() && It->GetStaticMesh())
        {
            ++VisiblePrimitives;
            PendingPrimitives += It->IsPSOPrecaching() || !It->IsRenderStateCreated() || !It->GetSceneProxy() ? 1 : 0;
        }
    }
    if (PendingPrimitives != LastPendingPrimitives)
    {
        UE_LOG(LogSeedForge, Display, TEXT("Gameplay capture render readiness label=%s pending=%d visible=%d frame=%llu."),
            *Request.Label, PendingPrimitives, VisiblePrimitives, GFrameCounter);
        LastPendingPrimitives = PendingPrimitives;
    }
    if (PendingPrimitives != 0) { return; }
    if (!Lifecycle.OnRendered(Token, Run, GFrameCounter))
    {
        return;
    }
    UGameViewportClient::OnViewportRendered().Remove(RenderHandle);
    RenderHandle.Reset();
    PixelsHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(
        this, &USeedForgeGameplayCaptureComponent::HandlePixels, Token, Run);
    ProcessedHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddUObject(
        this, &USeedForgeGameplayCaptureComponent::HandleProcessed, Token, Run);
    FScreenshotRequest::RequestScreenshot(Lifecycle.GetReceipt().Request.Path, true, false, false, FIntRect(), true);
}

void USeedForgeGameplayCaptureComponent::HandlePixels(
    int32 Width, int32 Height, const TArray<FColor>& Pixels, FGuid Token, uint64 Run)
{
    const auto& Request = Lifecycle.GetReceipt().Request;
    if (Request.Token != Token || Request.RunGeneration != Run) { return; }
    FString ActualPath = FPaths::ConvertRelativePathToFull(FScreenshotRequest::GetFilename());
    FPaths::MakeStandardFilename(ActualPath);
    if (!Lifecycle.OnPixels(Token, Run, ActualPath, FIntPoint(Width, Height), Pixels.Num(), GFrameCounter))
    {
        Fail(TEXT("Screenshot callback does not match the owned path, dimensions, pixels, run, or frame."));
        return;
    }
    bSavedPixels = FImageUtils::SaveImageByExtension(*ActualPath, FImageView(Pixels.GetData(), Width, Height));
}

void USeedForgeGameplayCaptureComponent::HandleProcessed(FGuid Token, uint64 Run)
{
    const auto& Request = Lifecycle.GetReceipt().Request;
    if (Request.Token != Token || Request.RunGeneration != Run) { return; }
    const FString Path = Lifecycle.GetReceipt().Request.Path;
    const int64 Bytes = FPlatformFileManager::Get().GetPlatformFile().FileSize(*Path);
    if (!Lifecycle.OnProcessed(Token, Run, bSavedPixels, FDateTime::UtcNow(), GFrameCounter, Bytes))
    {
        Fail(TEXT("Screenshot processing completed without a matching fresh saved PNG."));
        return;
    }
    FSeedForgeCaptureReceipt Receipt = Lifecycle.GetReceipt();
    CancelCapture();
    Receipt.RemainingDelegateBindings = GetNativeBindingCount();
    OnCompleted.Broadcast(Receipt);
}

int32 USeedForgeGameplayCaptureComponent::GetNativeBindingCount() const
{
    return (UGameViewportClient::OnViewportRendered().IsBoundToObject(this) ? 1 : 0)
        + (UGameViewportClient::OnScreenshotCaptured().IsBoundToObject(this) ? 1 : 0)
        + (FScreenshotRequest::OnScreenshotRequestProcessed().IsBoundToObject(this) ? 1 : 0);
}

void USeedForgeGameplayCaptureComponent::CancelCapture()
{
    UGameViewportClient::OnViewportRendered().Remove(RenderHandle);
    UGameViewportClient::OnScreenshotCaptured().Remove(PixelsHandle);
    FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ProcessedHandle);
    RenderHandle.Reset();
    PixelsHandle.Reset();
    ProcessedHandle.Reset();
    const FString OwnedPath = Lifecycle.GetReceipt().Request.Path;
    if (!OwnedPath.IsEmpty() && FScreenshotRequest::IsScreenshotRequested())
    {
        FString ActualPath = FPaths::ConvertRelativePathToFull(FScreenshotRequest::GetFilename());
        FPaths::MakeStandardFilename(ActualPath);
        if (ActualPath == OwnedPath)
        {
            FScreenshotRequest::Reset();
            GAreScreenMessagesEnabled = GScreenMessagesRestoreState;
        }
    }
    if (UWorld* World = GetWorld()) { World->GetTimerManager().ClearTimer(Watchdog); }
    Lifecycle.Cancel();
    OwningViewport.Reset();
    bSavedPixels = false;
    LastPendingPrimitives = INDEX_NONE;
}

void USeedForgeGameplayCaptureComponent::Timeout()
{
    Fail(TEXT("Owned gameplay capture exceeded its eight-second callback budget."));
}

void USeedForgeGameplayCaptureComponent::Fail(const FString& Error)
{
    LastError = Error;
    const FString ReportedError = Error;
    CancelCapture();
    OnFailed.Broadcast(ReportedError);
}

void USeedForgeGameplayCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelCapture();
    Super::EndPlay(EndPlayReason);
}

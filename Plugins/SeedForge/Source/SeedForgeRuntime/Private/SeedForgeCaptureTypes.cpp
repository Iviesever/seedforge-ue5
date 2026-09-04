#include "SeedForgeCaptureTypes.h"

bool FSeedForgeCaptureLifecycle::Begin(const FSeedForgeCaptureRequest& Request)
{
    if (Phase == ESeedForgeCapturePhase::AwaitRenderedFrame
        || Phase == ESeedForgeCapturePhase::AwaitPixels
        || Phase == ESeedForgeCapturePhase::AwaitProcessed
        || !Request.Token.IsValid() || Request.RunGeneration == 0
        || Request.Path.IsEmpty() || Request.Label.IsEmpty()
        || Request.Size.X <= 0 || Request.Size.Y <= 0 || Request.RequestedAtUtc.GetTicks() <= 0)
    {
        return false;
    }
    Receipt = {};
    Receipt.Request = Request;
    Phase = ESeedForgeCapturePhase::AwaitRenderedFrame;
    return true;
}

bool FSeedForgeCaptureLifecycle::OnRendered(const FGuid& Token, uint64 Run, uint64 Frame)
{
    if (!Matches(Token, Run) || Phase != ESeedForgeCapturePhase::AwaitRenderedFrame
        || Frame < Receipt.Request.RequestedFrame)
    {
        return false;
    }
    Receipt.RenderedFrame = Frame;
    Phase = ESeedForgeCapturePhase::AwaitPixels;
    return true;
}

bool FSeedForgeCaptureLifecycle::OnPixels(const FGuid& Token, uint64 Run, const FString& Path,
    const FIntPoint& Size, int64 PixelCount, uint64 Frame)
{
    if (!Matches(Token, Run) || Phase != ESeedForgeCapturePhase::AwaitPixels
        || Path != Receipt.Request.Path || Size != Receipt.Request.Size
        || PixelCount != static_cast<int64>(Size.X) * Size.Y || Frame < Receipt.RenderedFrame)
    {
        return false;
    }
    Receipt.CapturedFrame = Frame;
    Phase = ESeedForgeCapturePhase::AwaitProcessed;
    return true;
}

bool FSeedForgeCaptureLifecycle::OnProcessed(const FGuid& Token, uint64 Run, bool bSaved,
    const FDateTime& CompletedUtc, uint64 Frame, int64 FileBytes)
{
    if (!Matches(Token, Run) || Phase != ESeedForgeCapturePhase::AwaitProcessed)
    {
        return false;
    }
    if (!bSaved || FileBytes < 10 * 1024 || CompletedUtc < Receipt.Request.RequestedAtUtc
        || Frame < Receipt.CapturedFrame)
    {
        Phase = ESeedForgeCapturePhase::Failed;
        return false;
    }
    Receipt.CompletedAtUtc = CompletedUtc;
    Receipt.CompletedFrame = Frame;
    Receipt.FileBytes = FileBytes;
    Receipt.bSuccess = true;
    Phase = ESeedForgeCapturePhase::Complete;
    return true;
}

void FSeedForgeCaptureLifecycle::Cancel()
{
    Phase = ESeedForgeCapturePhase::Idle;
    Receipt = {};
}

bool FSeedForgeCaptureLifecycle::Matches(const FGuid& Token, uint64 Run) const
{
    return Receipt.Request.Token.IsValid() && Receipt.Request.Token == Token
        && Receipt.Request.RunGeneration == Run;
}

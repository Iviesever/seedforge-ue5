#pragma once

#include "CoreMinimal.h"

enum class ESeedForgeCapturePhase : uint8
{
    Idle,
    AwaitRenderedFrame,
    AwaitPixels,
    AwaitProcessed,
    Complete,
    Failed
};

struct SEEDFORGERUNTIME_API FSeedForgeCaptureRequest
{
    FGuid Token;
    uint64 RunGeneration = 0;
    uint64 SourceRequestId = 0;
    FString Label;
    FString Path;
    FIntPoint Size = FIntPoint(1280, 720);
    FDateTime RequestedAtUtc;
    uint64 RequestedFrame = 0;
};

struct SEEDFORGERUNTIME_API FSeedForgeCaptureReceipt
{
    FSeedForgeCaptureRequest Request;
    uint64 RenderedFrame = 0;
    uint64 CapturedFrame = 0;
    uint64 CompletedFrame = 0;
    FDateTime CompletedAtUtc;
    int64 FileBytes = 0;
    int32 RemainingDelegateBindings = INDEX_NONE;
    bool bSuccess = false;
};

class SEEDFORGERUNTIME_API FSeedForgeCaptureLifecycle
{
public:
    bool Begin(const FSeedForgeCaptureRequest& Request);
    bool OnRendered(const FGuid& Token, uint64 Run, uint64 Frame);
    bool OnPixels(const FGuid& Token, uint64 Run, const FString& Path,
        const FIntPoint& Size, int64 PixelCount, uint64 Frame);
    bool OnProcessed(const FGuid& Token, uint64 Run, bool bSaved,
        const FDateTime& CompletedUtc, uint64 Frame, int64 FileBytes);
    void Cancel();
    ESeedForgeCapturePhase GetPhase() const { return Phase; }
    const FSeedForgeCaptureReceipt& GetReceipt() const { return Receipt; }

private:
    bool Matches(const FGuid& Token, uint64 Run) const;
    ESeedForgeCapturePhase Phase = ESeedForgeCapturePhase::Idle;
    FSeedForgeCaptureReceipt Receipt;
};

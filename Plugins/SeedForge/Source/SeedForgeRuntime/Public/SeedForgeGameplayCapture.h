#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SeedForgeCaptureTypes.h"
#include "SeedForgeGameplayCapture.generated.h"

class FViewport;
class UGameViewportClient;

DECLARE_MULTICAST_DELEGATE_OneParam(FSeedForgeCaptureCompleted, const FSeedForgeCaptureReceipt&);
DECLARE_MULTICAST_DELEGATE_OneParam(FSeedForgeCaptureFailed, const FString&);

UCLASS()
class SEEDFORGERUNTIME_API USeedForgeGameplayCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USeedForgeGameplayCaptureComponent();
    bool RequestCapture(const FString& Label, const FString& OutputPath, uint64 Run, uint64 SourceRequestId,
        const FString& AllowedRoot = FString());
    void CancelCapture();
    int32 GetNativeBindingCount() const;
    const FString& GetLastError() const { return LastError; }
    FSeedForgeCaptureCompleted OnCompleted;
    FSeedForgeCaptureFailed OnFailed;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandleRendered(FViewport* Viewport, FGuid Token, uint64 Run);
    void HandlePixels(int32 Width, int32 Height, const TArray<FColor>& Pixels, FGuid Token, uint64 Run);
    void HandleProcessed(FGuid Token, uint64 Run);
    void Timeout();
    void Fail(const FString& Error);
    FSeedForgeCaptureLifecycle Lifecycle;
    TWeakObjectPtr<UGameViewportClient> OwningViewport;
    FDelegateHandle RenderHandle;
    FDelegateHandle PixelsHandle;
    FDelegateHandle ProcessedHandle;
    FTimerHandle Watchdog;
    FString LastError;
    bool bSavedPixels = false;
    int32 LastPendingPrimitives = INDEX_NONE;
};

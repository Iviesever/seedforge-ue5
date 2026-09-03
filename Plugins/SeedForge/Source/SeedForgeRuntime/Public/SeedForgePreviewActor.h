#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeedForgeAsync.h"
#include "SeedForgePreviewActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;

UCLASS()
class SEEDFORGERUNTIME_API ASeedForgePreviewActor : public AActor
{
    GENERATED_BODY()

public:
    ASeedForgePreviewActor();

    void ApplyLayout(const FSeedForgeLayout& Layout);
    int32 GetFloorInstanceCount() const;
    int32 GetWallInstanceCount() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandleGenerationApplied(const FSeedForgeAsyncCompletion& Completion);
    void CaptureScreenshot();
    void ExitAfterCapture();

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FloorInstances;

    UPROPERTY(VisibleAnywhere, Category = "SeedForge")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WallInstances;

    UPROPERTY(EditAnywhere, Category = "SeedForge|Visualization")
    float CellSize = 200.0f;

    UPROPERTY(EditAnywhere, Category = "SeedForge|Visualization")
    float FloorThickness = 20.0f;

    UPROPERTY(EditAnywhere, Category = "SeedForge|Visualization")
    float WallHeight = 180.0f;

    UPROPERTY(EditAnywhere, Category = "SeedForge|Visualization")
    float WallThickness = 18.0f;

    FDelegateHandle GenerationAppliedHandle;
    double RequestStartSeconds = 0.0;
    FString CapturePath;
    FTimerHandle CaptureTimer;
    FTimerHandle ExitTimer;
};

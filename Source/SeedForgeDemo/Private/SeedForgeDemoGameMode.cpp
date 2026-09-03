#include "SeedForgeDemoGameMode.h"

#include "Camera/CameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "SeedForgePreviewActor.h"

ASeedForgeDemoGameMode::ASeedForgeDemoGameMode()
{
    DefaultPawnClass = ADefaultPawn::StaticClass();
    HUDClass = nullptr;
}

void ASeedForgeDemoGameMode::StartPlay()
{
    Super::StartPlay();

    GetWorld()->SpawnActor<ASeedForgePreviewActor>(FVector::ZeroVector, FRotator::ZeroRotator);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector(0.0, 0.0, 6000.0),
        FRotator(-55.0, -35.0, 0.0));
    if (Sun)
    {
        Sun->GetLightComponent()->SetIntensity(8.0f);
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(1.5f);
    }

    ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>(
        FVector(4700.0, 4700.0, 11000.0),
        FRotator(-90.0, 0.0, 0.0));
    if (Camera)
    {
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
        {
            FString CapturePath;
            const bool bCaptureMode = FParse::Value(
                FCommandLine::Get(),
                TEXT("SeedForgeCapturePath="),
                CapturePath);
            if (bCaptureMode)
            {
                if (APawn* Pawn = PlayerController->GetPawn())
                {
                    Pawn->SetActorHiddenInGame(true);
                    Pawn->SetActorEnableCollision(false);
                }
                PlayerController->SetViewTarget(Camera);
            }
            else if (APawn* Pawn = PlayerController->GetPawn())
            {
                Pawn->SetActorLocation(FVector(4700.0, 4700.0, 3500.0));
                PlayerController->SetControlRotation(FRotator(-55.0, -135.0, 0.0));
            }
        }
    }
}

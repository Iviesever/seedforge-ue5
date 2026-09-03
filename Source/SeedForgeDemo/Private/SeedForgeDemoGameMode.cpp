#include "SeedForgeDemoGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"

ASeedForgeDemoGameMode::ASeedForgeDemoGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = ASeedForgePlayerController::StaticClass();
    HUDClass = ASeedForgeHUD::StaticClass();
}

void ASeedForgeDemoGameMode::StartPlay()
{
    Super::StartPlay();

    GetWorld()->SpawnActor<ASeedForgeGameplayCoordinator>(
        FVector::ZeroVector,
        FRotator::ZeroRotator);

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

}

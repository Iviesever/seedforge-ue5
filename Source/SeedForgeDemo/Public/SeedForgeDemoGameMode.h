#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SeedForgeDemoGameMode.generated.h"

UCLASS()
class SEEDFORGEDEMO_API ASeedForgeDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASeedForgeDemoGameMode();
    virtual void StartPlay() override;
};


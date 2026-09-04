#pragma once

#include "Misc/AutomationTest.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerState.h"
#include "InputKeyEventArgs.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeWorldSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::InputTests
{
    inline int32 CountAction(const UInputComponent* Input, const FName Name)
    {
        int32 Count = 0;
        if (Input)
        {
            for (int32 Index = 0; Index < Input->GetNumActionBindings(); ++Index)
            {
                Count += Input->GetActionBinding(Index).GetActionName() == Name ? 1 : 0;
            }
        }
        return Count;
    }

    struct FWorldFixture
    {
        UWorld* World;
        ASeedForgePlayerController* Controller;
        ASeedForgeGameplayCoordinator* Coordinator;
        USeedForgeWorldSubsystem* Subsystem;

        FWorldFixture()
        {
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None,
                    EUniqueObjectNameOptions::GloballyUnique), GetTransientPackage());
            World->AddToRoot();
            Context.SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            Controller = World->SpawnActor<ASeedForgePlayerController>();
            Controller->SetAsLocalPlayerController();
            Controller->SetPlayerState(World->SpawnActor<APlayerState>());
            Controller->InitInputSystem();
            Coordinator = World->SpawnActor<ASeedForgeGameplayCoordinator>();
            Coordinator->DispatchBeginPlay();
            Subsystem = World->GetSubsystem<USeedForgeWorldSubsystem>();
        }

        ~FWorldFixture()
        {
            Coordinator->Destroy(true);
            Controller->Destroy(true);
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(true);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
            World->RemoveFromRoot();
        }

        void Queue(const FKey& Key, EInputEvent Event)
        {
            if (IsValid(Controller) && !Controller->IsActorBeingDestroyed() && Controller->PlayerInput)
            {
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    Key, Event, Event == IE_Released ? 0.0f : 1.0f));
            }
        }

        void Process()
        {
            if (!IsValid(Controller) || Controller->IsActorBeingDestroyed()
                || !Controller->PlayerInput || !Controller->InputComponent)
            {
                return;
            }
            TArray<UInputComponent*> Stack;
            if (APawn* Pawn = Controller->GetPawn())
            {
                if (Pawn->InputComponent)
                {
                    Stack.Add(Pawn->InputComponent);
                }
            }
            Stack.Add(Controller->InputComponent);
            Controller->PlayerInput->ProcessInputStack(Stack, 1.0f / 60.0f, false);
        }

        void Dispatch(const FKey& Key, EInputEvent Event)
        {
            Queue(Key, Event);
            Process();
        }

        void Press(const FKey& Key)
        {
            Dispatch(Key, IE_Pressed);
            Dispatch(Key, IE_Released);
        }

        void Apply(uint64 Request, uint64 Seed)
        {
            FSeedForgeAsyncCompletion Completion;
            Completion.RequestId = Request;
            Completion.Result = FSeedForgeGenerator::Generate(Seed, {});
            Subsystem->OnGenerationApplied().Broadcast(Completion);
        }

        void ApplyCurrent()
        {
            const auto Snapshot = Coordinator->GetSnapshot();
            Apply(Snapshot.PendingRequestId, Snapshot.Seed);
            if (APawn* Pawn = Controller->GetPawn())
            {
                if (!Pawn->InputComponent)
                {
                    Pawn->PawnClientRestart();
                }
            }
        }

        int32 OwnedLiveActors() const
        {
            int32 Count = 0;
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                const bool bOwnedRunActor = It->GetOwner() == Coordinator
                    || (It->IsA<ASeedForgePlayerCharacter>() && It->GetOwner() == Controller);
                Count += bOwnedRunActor && !It->IsActorBeingDestroyed() ? 1 : 0;
            }
            return Count;
        }
    };
}

#endif

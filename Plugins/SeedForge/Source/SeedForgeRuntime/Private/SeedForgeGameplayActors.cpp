#include "SeedForgeGameplayActors.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgeGameplayTypes.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace SeedForge::GameplayActors::Private
{
    UStaticMesh* FindCubeMesh()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
            TEXT("/Engine/BasicShapes/Cube.Cube"));
        return Mesh.Object;
    }

    UStaticMesh* FindSphereMesh()
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
            TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        return Mesh.Object;
    }

    void ApplyColor(UStaticMeshComponent* Component, UObject* Owner, const FLinearColor& Color)
    {
        if (!Component || !Owner)
        {
            return;
        }
        UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (BaseMaterial)
        {
            UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, Owner);
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Component->SetMaterial(0, Material);
        }
    }

    const TCHAR* RunStateText(ESeedForgeRunState State)
    {
        switch (State)
        {
        case ESeedForgeRunState::Generating: return TEXT("GENERATING");
        case ESeedForgeRunState::Playing: return TEXT("PLAYING");
        case ESeedForgeRunState::Won: return TEXT("EXTRACTED - YOU WIN");
        case ESeedForgeRunState::Lost: return TEXT("RUN LOST");
        case ESeedForgeRunState::Restarting: return TEXT("RESTARTING");
        case ESeedForgeRunState::Failed: return TEXT("RUN FAILED");
        default: return TEXT("UNKNOWN");
        }
    }
}

ASeedForgePlayerCharacter::ASeedForgePlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    bUseControllerRotationYaw = false;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 82.0f);
    GetCharacterMovement()->MaxWalkSpeed = FSeedForgeGameplayTuning().PlayerMoveSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = false;

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerVisual"));
    Visual->SetupAttachment(GetCapsuleComponent());
    Visual->SetStaticMesh(SeedForge::GameplayActors::Private::FindCubeMesh());
    Visual->SetRelativeLocation(FVector(0.0, 0.0, -12.0));
    Visual->SetRelativeScale3D(FVector(0.62, 0.62, 1.0));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    AttackPulse = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackPulse"));
    AttackPulse->SetupAttachment(GetCapsuleComponent());
    AttackPulse->SetStaticMesh(SeedForge::GameplayActors::Private::FindSphereMesh());
    AttackPulse->SetRelativeLocation(FVector(170.0, 0.0, -15.0));
    AttackPulse->SetRelativeScale3D(FVector(3.1, 1.4, 0.08));
    AttackPulse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttackPulse->SetVisibility(false);

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(GetCapsuleComponent());
    CameraArm->SetUsingAbsoluteRotation(true);
    CameraArm->TargetArmLength = 1250.0f;
    CameraArm->SetRelativeRotation(FRotator(-62.0, 0.0, 0.0));
    CameraArm->bDoCollisionTest = false;
    CameraArm->bInheritPitch = false;
    CameraArm->bInheritYaw = false;
    CameraArm->bInheritRoll = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void ASeedForgePlayerCharacter::SetGameplayCoordinator(
    ASeedForgeGameplayCoordinator* InCoordinator)
{
    Coordinator = InCoordinator;
    SeedForge::GameplayActors::Private::ApplyColor(
        Visual,
        this,
        FLinearColor(0.05f, 0.45f, 1.0f));
    SeedForge::GameplayActors::Private::ApplyColor(
        AttackPulse,
        this,
        FLinearColor(1.0f, 0.8f, 0.05f));
}

void ASeedForgePlayerCharacter::SetAimWorldPoint(const FVector& WorldPoint)
{
    FVector Direction = WorldPoint - GetActorLocation();
    Direction.Z = 0.0;
    if (Direction.Normalize())
    {
        AimDirection = Direction;
        SetActorRotation(Direction.Rotation());
    }
}

FVector ASeedForgePlayerCharacter::GetAimDirection() const
{
    return AimDirection;
}

bool ASeedForgePlayerCharacter::HasTopDownCamera() const
{
    return CameraArm != nullptr && Camera != nullptr;
}

float ASeedForgePlayerCharacter::GetAttackCooldownSeconds() const
{
    return FSeedForgeGameplayTuning().AttackCooldownSeconds;
}

float ASeedForgePlayerCharacter::GetDashCooldownSeconds() const
{
    return FSeedForgeGameplayTuning().DashCooldownSeconds;
}

void ASeedForgePlayerCharacter::ShowAttackPulse()
{
    AttackPulse->SetVisibility(true);
    GetWorldTimerManager().SetTimer(
        AttackPulseTimer,
        this,
        &ASeedForgePlayerCharacter::HideAttackPulse,
        0.12f,
        false);
}

void ASeedForgePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASeedForgePlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASeedForgePlayerCharacter::MoveRight);
    PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &ASeedForgePlayerCharacter::Attack);
    PlayerInputComponent->BindAction(TEXT("Dash"), IE_Pressed, this, &ASeedForgePlayerCharacter::Dash);
}

void ASeedForgePlayerCharacter::MoveForward(float Value)
{
    CurrentForwardAxis = FMath::IsFinite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
    if (!FMath::IsNearlyZero(CurrentForwardAxis))
    {
        AddMovementInput(FVector::ForwardVector, CurrentForwardAxis);
    }
}

void ASeedForgePlayerCharacter::MoveRight(float Value)
{
    CurrentRightAxis = FMath::IsFinite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
    if (!FMath::IsNearlyZero(CurrentRightAxis))
    {
        AddMovementInput(FVector::RightVector, CurrentRightAxis);
    }
}

void ASeedForgePlayerCharacter::ResetMovementIntent()
{
    CurrentForwardAxis = 0.0f;
    CurrentRightAxis = 0.0f;
}

void ASeedForgePlayerCharacter::PawnClientRestart()
{
    ResetMovementIntent();
    Super::PawnClientRestart();
}

void ASeedForgePlayerCharacter::UnPossessed()
{
    ResetMovementIntent();
    Super::UnPossessed();
}

void ASeedForgePlayerCharacter::Attack()
{
    if (ASeedForgeGameplayCoordinator* Run = Coordinator.Get())
    {
        if (Run->TryPlayerAttack(GetActorLocation(), AimDirection))
        {
            ShowAttackPulse();
        }
    }
}

void ASeedForgePlayerCharacter::Dash()
{
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (Now < NextDashTime)
    {
        return;
    }
    const FSeedForgeGameplayTuning Tuning;
    NextDashTime = Now + Tuning.DashCooldownSeconds;
    // UE sums current axes before actions, but dispatches axis callbacks after actions.
    // Sample the summed values so press/release plus Space in one frame is current.
    const float Forward = InputComponent ? InputComponent->GetAxisValue(TEXT("MoveForward")) : CurrentForwardAxis;
    const float Right = InputComponent ? InputComponent->GetAxisValue(TEXT("MoveRight")) : CurrentRightAxis;
    const FVector Direction = FSeedForgeGameplayMath::ResolveDashDirection(Forward, Right, AimDirection);
    LaunchCharacter(Direction * Tuning.DashImpulse, true, false);
}

void ASeedForgePlayerCharacter::HideAttackPulse()
{
    AttackPulse->SetVisibility(false);
}

ASeedForgePlayerController::ASeedForgePlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = false;
    PrimaryActorTick.bCanEverTick = true;
}

void ASeedForgePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    for (int32 Index = InputComponent->GetNumActionBindings() - 1; Index >= 0; --Index)
    {
        const FName Action = InputComponent->GetActionBinding(Index).GetActionName();
        if (Action == TEXT("RestartSameSeed") || Action == TEXT("StartNewSeed"))
        {
            InputComponent->RemoveActionBinding(Index);
        }
    }
    InputComponent->BindAction(TEXT("RestartSameSeed"), IE_Pressed,
        this, &ASeedForgePlayerController::RestartSameSeed);
    InputComponent->BindAction(TEXT("StartNewSeed"), IE_Pressed,
        this, &ASeedForgePlayerController::StartNewSeed);
}

void ASeedForgePlayerController::FlushPressedKeys()
{
    Super::FlushPressedKeys();
    if (ASeedForgePlayerCharacter* PlayerCharacter = Cast<ASeedForgePlayerCharacter>(GetPawn()))
    {
        PlayerCharacter->ResetMovementIntent();
    }
}

ASeedForgeGameplayCoordinator* ASeedForgePlayerController::ResolveGameplayCoordinator()
{
    if (ASeedForgeGameplayCoordinator* Existing = GameplayCoordinator.Get())
    {
        if (!Existing->IsActorBeingDestroyed())
        {
            return Existing;
        }
    }
    GameplayCoordinator.Reset();
    ASeedForgeGameplayCoordinator* Unique = nullptr;
    for (TActorIterator<ASeedForgeGameplayCoordinator> It(GetWorld()); It; ++It)
    {
        if (It->IsActorBeingDestroyed())
        {
            continue;
        }
        if (Unique)
        {
            // A host with multiple run owners is ambiguous; never pick by iteration order.
            return nullptr;
        }
        Unique = *It;
    }
    GameplayCoordinator = Unique;
    return Unique;
}

void ASeedForgePlayerController::RestartSameSeed()
{
    if (ASeedForgeGameplayCoordinator* Run = ResolveGameplayCoordinator())
    {
        Run->RestartSameSeed();
    }
}

void ASeedForgePlayerController::StartNewSeed()
{
    if (ASeedForgeGameplayCoordinator* Run = ResolveGameplayCoordinator())
    {
        Run->StartNewSeed();
    }
}

void ASeedForgePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GameplayCoordinator.Reset();
    Super::EndPlay(EndPlayReason);
}

void ASeedForgePlayerController::BeginPlay()
{
    Super::BeginPlay();
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void ASeedForgePlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    ASeedForgePlayerCharacter* PlayerCharacter = Cast<ASeedForgePlayerCharacter>(GetPawn());
    if (!PlayerCharacter)
    {
        return;
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection)
        || FMath::IsNearlyZero(RayDirection.Z))
    {
        return;
    }
    const double PlaneZ = PlayerCharacter->GetActorLocation().Z;
    const double Distance = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
    if (Distance > 0.0)
    {
        PlayerCharacter->SetAimWorldPoint(RayOrigin + RayDirection * Distance);
    }
}

ASeedForgeEnemyPawn::ASeedForgeEnemyPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyVisual"));
    SetRootComponent(Visual);
    Visual->SetStaticMesh(SeedForge::GameplayActors::Private::FindCubeMesh());
    Visual->SetRelativeScale3D(FVector(0.62, 0.62, 0.72));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASeedForgeEnemyPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!WorldPath.IsValidIndex(PathIndex))
    {
        return;
    }
    const FVector Target = WorldPath[PathIndex];
    const FVector Next = FMath::VInterpConstantTo(
        GetActorLocation(),
        Target,
        DeltaSeconds,
        FSeedForgeGameplayTuning().EnemyMoveSpeed);
    SetActorLocation(Next, false);
    FVector Facing = Target - GetActorLocation();
    Facing.Z = 0.0;
    if (!Facing.IsNearlyZero())
    {
        SetActorRotation(Facing.Rotation());
    }
    if (FVector::DistSquared2D(Next, Target) <= FMath::Square(4.0))
    {
        ++PathIndex;
    }
}

void ASeedForgeEnemyPawn::Configure(uint32 InStableId, const FIntPoint& InCell)
{
    StableId = InStableId;
    SpawnCell = InCell;
    SeedForge::GameplayActors::Private::ApplyColor(
        Visual,
        this,
        FLinearColor(0.95f, 0.08f, 0.06f));
}

void ASeedForgeEnemyPawn::SetPath(TArray<FVector> InWorldPath)
{
    WorldPath = MoveTemp(InWorldPath);
    PathIndex = 0;
}

void ASeedForgeEnemyPawn::ClearPath()
{
    WorldPath.Reset();
    PathIndex = 0;
}

uint32 ASeedForgeEnemyPawn::GetStableId() const
{
    return StableId;
}

FIntPoint ASeedForgeEnemyPawn::GetSpawnCell() const
{
    return SpawnCell;
}

ASeedForgeCorePickup::ASeedForgeCorePickup()
{
    PrimaryActorTick.bCanEverTick = false;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreVisual"));
    SetRootComponent(Visual);
    Visual->SetStaticMesh(SeedForge::GameplayActors::Private::FindSphereMesh());
    Visual->SetRelativeScale3D(FVector(0.42));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASeedForgeCorePickup::Configure(uint32 InStableId, const FIntPoint& InCell)
{
    StableId = InStableId;
    SpawnCell = InCell;
    SeedForge::GameplayActors::Private::ApplyColor(
        Visual,
        this,
        FLinearColor(0.0f, 0.95f, 1.0f));
}

uint32 ASeedForgeCorePickup::GetStableId() const
{
    return StableId;
}

FIntPoint ASeedForgeCorePickup::GetSpawnCell() const
{
    return SpawnCell;
}

ASeedForgeExitActor::ASeedForgeExitActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExitVisual"));
    SetRootComponent(Visual);
    Visual->SetStaticMesh(SeedForge::GameplayActors::Private::FindCubeMesh());
    Visual->SetRelativeScale3D(FVector(0.76, 0.76, 1.8));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASeedForgeExitActor::Configure(const FIntPoint& InCell)
{
    SpawnCell = InCell;
    SetUnlocked(false);
}

void ASeedForgeExitActor::SetUnlocked(bool bInUnlocked)
{
    bUnlocked = bInUnlocked;
    SeedForge::GameplayActors::Private::ApplyColor(
        Visual,
        this,
        bUnlocked
            ? FLinearColor(0.05f, 1.0f, 0.15f)
            : FLinearColor(1.0f, 0.22f, 0.02f));
}

bool ASeedForgeExitActor::IsUnlocked() const
{
    return bUnlocked;
}

FIntPoint ASeedForgeExitActor::GetSpawnCell() const
{
    return SpawnCell;
}

void ASeedForgeHUD::DrawHUD()
{
    Super::DrawHUD();
    ASeedForgeGameplayCoordinator* Coordinator = nullptr;
    for (TActorIterator<ASeedForgeGameplayCoordinator> It(GetWorld()); It; ++It)
    {
        Coordinator = *It;
        break;
    }
    if (!Coordinator || !Canvas)
    {
        return;
    }

    const FSeedForgeGameplaySnapshot Snapshot = Coordinator->GetSnapshot();
    UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
    const FLinearColor TextColor = FLinearColor::White;
    float Y = 32.0f;
    auto Line = [this, Font, TextColor, &Y](const FString& Text, float Scale = 1.0f)
    {
        DrawText(Text, TextColor, 32.0f, Y, Font, Scale, false);
        Y += 28.0f * Scale;
    };

    Line(TEXT("SEEDFORGE // DETERMINISTIC EXTRACTION"), 1.15f);
    Line(FString::Printf(TEXT("HP  %.0f / %.0f"), Snapshot.PlayerHealth, Snapshot.PlayerMaxHealth));
    Line(FString::Printf(TEXT("DATA CORES  %d / %d"), Snapshot.CollectedCoreCount, Snapshot.RequiredCoreCount));
    Line(FString::Printf(TEXT("SEED  %llu"), Snapshot.Seed));
    Line(FString::Printf(TEXT("RUN  %s"), SeedForge::GameplayActors::Private::RunStateText(Snapshot.RunState)));
    if (Snapshot.FailureCode != ESeedForgeRunFailureCode::None)
    {
        Line(FString::Printf(TEXT("FAILURE  %s: %s"),
            LexToString(Snapshot.FailureCode), *Snapshot.FailureMessage), 0.7f);
    }
    Line(Snapshot.bExitUnlocked ? TEXT("EXIT  UNLOCKED") : TEXT("EXIT  LOCKED"));
    Y += 12.0f;
    Line(TEXT("WASD Move   Mouse Aim   LMB Attack   Space Dash"), 0.82f);
    Line(TEXT("R Restart Same Seed   N New Seed"), 0.82f);
    if (Snapshot.RunState == ESeedForgeRunState::Won
        || Snapshot.RunState == ESeedForgeRunState::Lost
        || Snapshot.RunState == ESeedForgeRunState::Failed)
    {
        Y += 18.0f;
        Line(TEXT("Press R to replay this layout or N for a new run"), 1.0f);
    }
}

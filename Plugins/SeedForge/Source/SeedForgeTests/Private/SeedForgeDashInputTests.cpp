#include "SeedForgeInputTestWorld.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeDashDirectionContractTest,
    "SeedForge.Audit.DashInput.PureDirectionContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDashDirectionContractTest::RunTest(const FString& Parameters)
{
    const FVector Diagonal = FVector(1.0, 1.0, 0.0).GetSafeNormal();
    TestTrue(TEXT("Two live axes produce diagonal"), FSeedForgeGameplayMath::ResolveDashDirection(1, 1, FVector::ForwardVector).Equals(Diagonal));
    TestTrue(TEXT("Released right preserves forward"), FSeedForgeGameplayMath::ResolveDashDirection(-1, 0, FVector::RightVector).Equals(-FVector::ForwardVector));
    TestTrue(TEXT("Released forward preserves right"), FSeedForgeGameplayMath::ResolveDashDirection(0, -1, FVector::ForwardVector).Equals(-FVector::RightVector));
    TestTrue(TEXT("No live axes use current flat aim"), FSeedForgeGameplayMath::ResolveDashDirection(0, 0, FVector(0, -5, 7)).Equals(-FVector::RightVector));
    TestTrue(TEXT("Zero aim has safe fallback"), FSeedForgeGameplayMath::ResolveDashDirection(0, 0, FVector::ZeroVector).Equals(FVector::ForwardVector));
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    TestTrue(TEXT("Non-finite aim has safe fallback"), FSeedForgeGameplayMath::ResolveDashDirection(0, 0, FVector(NaN, 0, 0)).Equals(FVector::ForwardVector));
    TestTrue(TEXT("Non-finite axes fall back to aim"), FSeedForgeGameplayMath::ResolveDashDirection(static_cast<float>(NaN), 1, FVector::RightVector).Equals(FVector::RightVector));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeDiagonalDashInputTest,
    "SeedForge.Audit.DashInput.DiagonalSpaceAndCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDiagonalDashInputTest::RunTest(const FString& Parameters)
{
    SeedForge::InputTests::FWorldFixture Fixture;
    Fixture.ApplyCurrent();
    ASeedForgePlayerCharacter* Player = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
    Fixture.Dispatch(EKeys::W, IE_Pressed);
    Fixture.Dispatch(EKeys::D, IE_Pressed);
    Fixture.Press(EKeys::SpaceBar);
    const FVector Launched = Player->GetCharacterMovement()->PendingLaunchVelocity;
    const FVector Expected = FVector(1, 1, 0).GetSafeNormal() * Fixture.Coordinator->GetTuning().DashImpulse;
    TestTrue(TEXT("Space launches along combined W+D direction"), Launched.Equals(Expected, 0.01));
    Fixture.Dispatch(EKeys::W, IE_Released);
    Fixture.Dispatch(EKeys::D, IE_Released);
    Fixture.Controller->FlushPressedKeys();
    Player->SetAimWorldPoint(Player->GetActorLocation() - FVector(100, 0, 0));
    Fixture.Press(EKeys::SpaceBar);
    TestTrue(TEXT("Second Space within cooldown does not replace accepted velocity"), Player->GetCharacterMovement()->PendingLaunchVelocity.Equals(Launched, 0.01));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeReleasedDashInputTest,
    "SeedForge.Audit.DashInput.ReleaseAndOppositeKeysUseAim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeReleasedDashInputTest::RunTest(const FString& Parameters)
{
    for (bool bOppositeKeys : {false, true})
    {
        SeedForge::InputTests::FWorldFixture Fixture;
        Fixture.ApplyCurrent();
        ASeedForgePlayerCharacter* Player = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
        Player->SetAimWorldPoint(Player->GetActorLocation() - FVector(0, 100, 0));
        Fixture.Dispatch(EKeys::W, IE_Pressed);
        Fixture.Dispatch(bOppositeKeys ? EKeys::S : EKeys::W, bOppositeKeys ? IE_Pressed : IE_Released);
        Fixture.Press(EKeys::SpaceBar);
        TestTrue(bOppositeKeys ? TEXT("W+S cancellation uses aim") : TEXT("Released axes use aim"),
            Player->GetCharacterMovement()->PendingLaunchVelocity.Equals(
                -FVector::RightVector * Fixture.Coordinator->GetTuning().DashImpulse, 0.01));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeFlushedDashInputTest,
    "SeedForge.Audit.DashInput.FocusFlushAndPossessionReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeFlushedDashInputTest::RunTest(const FString& Parameters)
{
    for (bool bRepossess : {false, true})
    {
        SeedForge::InputTests::FWorldFixture Fixture;
        Fixture.ApplyCurrent();
        ASeedForgePlayerCharacter* Player = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
        Player->SetAimWorldPoint(Player->GetActorLocation() + FVector(0, 100, 0));
        Fixture.Dispatch(EKeys::W, IE_Pressed);
        if (bRepossess)
        {
            Fixture.Controller->UnPossess();
            Fixture.Dispatch(EKeys::W, IE_Released);
            Fixture.Controller->Possess(Player);
            Player->PawnClientRestart();
        }
        else
        {
            Fixture.Controller->FlushPressedKeys();
        }
        Fixture.Press(EKeys::SpaceBar);
        TestTrue(bRepossess ? TEXT("Re-possession cannot retain held movement") : TEXT("Focus flush clears phantom held axis"),
            Player->GetCharacterMovement()->PendingLaunchVelocity.Equals(
                FVector::RightVector * Fixture.Coordinator->GetTuning().DashImpulse, 0.01));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedForgeSameFrameDashInputTest,
    "SeedForge.Audit.DashInput.SameFrameAxesPrecedeDash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeSameFrameDashInputTest::RunTest(const FString& Parameters)
{
    for (bool bRelease : {false, true})
    {
        SeedForge::InputTests::FWorldFixture Fixture;
        Fixture.ApplyCurrent();
        ASeedForgePlayerCharacter* Player = CastChecked<ASeedForgePlayerCharacter>(Fixture.Controller->GetPawn());
        Player->SetAimWorldPoint(Player->GetActorLocation() - FVector(0, 100, 0));
        if (bRelease)
        {
            Fixture.Dispatch(EKeys::W, IE_Pressed);
            Fixture.Queue(EKeys::W, IE_Released);
        }
        else
        {
            Fixture.Queue(EKeys::W, IE_Pressed);
            Fixture.Queue(EKeys::D, IE_Pressed);
        }
        Fixture.Queue(EKeys::SpaceBar, IE_Pressed);
        Fixture.Process();
        const FVector Direction = bRelease ? -FVector::RightVector : FVector(1, 1, 0).GetSafeNormal();
        TestTrue(bRelease ? TEXT("Same-frame release is not previous-frame movement") : TEXT("Same-frame W+D+Space uses both axes"),
            Player->GetCharacterMovement()->PendingLaunchVelocity.Equals(
                Direction * Fixture.Coordinator->GetTuning().DashImpulse, 0.01));
    }
    return true;
}

#endif

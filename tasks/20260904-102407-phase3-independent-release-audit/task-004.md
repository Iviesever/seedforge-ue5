# SF-IRA-004 live Dash intent implementation plan

> **Execution:** `executing-tasks` inline with test-first changes; one root cause/commit.

**Goal:** Dash follows current combined movement, then current aim, then safe +X; release/focus flush/re-possession cannot retain a phantom axis.

**Architecture:** Add one pure `FSeedForgeGameplayMath::ResolveDashDirection(float ForwardAxis, float RightAxis, const FVector& Aim)` helper. Character retains both live axes including zero samples. A production `ResetMovementIntent()` boundary is called by Controller flush and Pawn possession lifecycle; it does not reset Dash cooldown.

**Engine-order refinement (before GREEN):** UE 5.8 `PlayerInput.cpp` accumulates `InputComponent->AxisBindings[].AxisValue` before executing action delegates, but executes the axis callbacks after actions. Dash must sample both current `InputComponent->GetAxisValue` values, not only the previous callbacks' cached values. Add a same-input-frame release/press plus Space regression before implementing this refinement.

## Files and steps

- Runtime: `Public/SeedForgeGameplayTypes.h`, `Private/SeedForgeGameplayTypes.cpp`, `Public/SeedForgeGameplayActors.h`, `Private/SeedForgeGameplayActors.cpp`.
- Tests: move B's already validated World/input fixture unchanged into test-local `SeedForgeInputTestWorld.h`, reuse it in `SeedForgeRestartInputTests.cpp` and new `SeedForgeDashInputTests.cpp`.
- Evidence: `sf-ira-004-evidence.md` and `progress.md`.

- [x] Add pure helper declaration with a safe-forward RED stub. Test diagonal, cancelled axes, release-to-aim, vertical/zero/non-finite aim and input. Example:

```cpp
TestTrue(TEXT("Diagonal dash matches both axes"),
    FSeedForgeGameplayMath::ResolveDashDirection(1, 1, FVector::ForwardVector)
        .Equals(FVector(1, 1, 0).GetSafeNormal()));
```

- [x] Add actual engine-input tests: W+D then Space yields normalized diagonal PendingLaunchVelocity; release all or press opposite keys falls back to aim; Controller FlushPressedKeys clears held intent; repeated Space before cooldown does not alter the accepted launch; re-possession does not retain a held axis. No direct assignment to launch velocity or private axis state.
- [x] Build and run `Scripts/Test.ps1 -Filter SeedForge.Audit.DashInput -TimeoutSeconds 120`; retain expected RED, while rerunning B validates the shared fixture extraction.
- [x] Implement Character axis retention and Dash resolution:

```cpp
CurrentForwardAxis = FMath::IsFinite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
// Right axis follows the same explicit assignment including zero.
const FVector Direction = FSeedForgeGameplayMath::ResolveDashDirection(
    CurrentForwardAxis, CurrentRightAxis, AimDirection);
LaunchCharacter(Direction * Tuning.DashImpulse, true, false);
```

`ResetMovementIntent()` clears only the two axes. Controller `FlushPressedKeys()` calls Super then resets its Character's intent; `PawnClientRestart` and `UnPossessed` reset it through the same boundary. Keep the existing Dash cooldown value/check.

- [x] Build; focused DashInput (5/5); RestartInput regression (3/3); full SeedForge suite (77/77); record actual counts/logs.
- [x] Read-only review and separate commit `fix(input): derive dash from combined live movement axes`.

Final packaged input-self-test coverage is still task H/gate 9; this local fix is not a substitute for it.

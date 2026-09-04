# SF-IRA-003 persistent restart input implementation plan

> **Execution:** `executing-tasks`, inline; RED/GREEN through UE input APIs, not OS macros.

**Goal:** Keep exactly one R/N binding alive on the PlayerController while the Character is absent during generation/failure/restart.

**Architecture:** Controller owns only persistent run controls and a weak coordinator reference. Movement/attack/Dash remain Character-owned. Existing coordinator generation IDs and ownership cleanup remain the state source; add read-only snapshot counters needed to prove a key dispatch starts exactly one request.

**Stack:** UE 5.8 `FInputKeyEventArgs::CreateSimulated`, configured PlayerInput/InputComponent classes, transient World, existing Runtime module.

## Scope and files

- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeGameplayActors.h` and `Private/SeedForgeGameplayActors.cpp`.
- Modify `Public/SeedForgeGameplayTypes.h` / coordinator snapshot for read-only `RunGeneration` and `PendingRequestId`; this observation prerequisite does not fix SF-IRA-007's misleading log label (task E).
- Create `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeRestartInputTests.cpp`.
- Add direct `InputCore` test-module dependency for real EKeys/InputKeyEventArgs.
- Preserve Legacy named Action/Axis mappings on Enhanced-compatible input classes.

## Test/implementation steps

- [x] Write RED tests with a real transient World, native PlayerController, `InitInputSystem`, production Coordinator, actual configured PlayerInput, and actual action mappings.

```cpp
const auto Before = Coordinator->GetSnapshot();
Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::N, IE_Pressed, 1.0f));
Controller->PlayerInput->ProcessInputStack(InputStack, 1.0f / 60.0f, false);
const auto After = Coordinator->GetSnapshot();
TestEqual(TEXT("One N event starts one run"), After.RunGeneration, Before.RunGeneration + 1);
TestEqual(TEXT("New seed transform runs once"), After.Seed,
    Before.Seed * 6364136223846793005ULL + 1442695040888963407ULL);
```

Release the key and process the next input sample between presses. Verify Controller bindings R/N count 1 after repeated initialization, Character bindings R/N count 0, no pawn during Generating/Failed, newer pending ID after rapid R, stale success ignored, exactly one fresh possessed Character after matching success, old actors no longer live, and failure recovery through a Controller R event.

- [x] Run `Scripts/Build.ps1` then `Scripts/Test.ps1 -Filter SeedForge.Audit.RestartInput -TimeoutSeconds 120`; require RED caused by absent Controller bindings/retained Character bindings, not an input fixture error.

- [x] Implement `ASeedForgePlayerController::SetupInputComponent()`:

```cpp
Super::SetupInputComponent();
InputComponent->RemoveActionBinding(TEXT("RestartSameSeed"), IE_Pressed);
InputComponent->RemoveActionBinding(TEXT("StartNewSeed"), IE_Pressed);
InputComponent->BindAction(TEXT("RestartSameSeed"), IE_Pressed, this,
    &ASeedForgePlayerController::RestartSameSeed);
InputComponent->BindAction(TEXT("StartNewSeed"), IE_Pressed, this,
    &ASeedForgePlayerController::StartNewSeed);
```

Remove Character R/N methods and bindings. Controller handlers resolve one valid Coordinator through a weak cache and a World lookup; multiple coordinators fail closed instead of choosing an unspecified actor. Reset the weak reference on EndPlay. Preserve movement/attack/Dash ownership.

- [x] Run focused `SeedForge.Audit.RestartInput`, regression `SeedForge.Audit.RunFailure`, and full `SeedForge`. Verify no old possession/actor set and no duplicate input dispatch.
- [x] Record exact source HEAD, command timestamps/results/reports in `sf-ira-003-evidence.md`; update 001's persistent recovery dependency and progress; commit only this root cause as `fix(input): keep restart controls alive across generation`.

## Later certification

Task H and final gates 9/11 must still execute R/N through the packaged executable's actual input stack and verify same/new-seed identities. Unit/controller evidence here does not substitute for those package gates.

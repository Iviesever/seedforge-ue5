# SF-IRA-003 test fixture lifecycle packet

## Observed sequence

1. Initial compile rejected a `TObjectPtr<APawn>` passed directly to a pointer-template assertion; adding `.Get()` corrected only test syntax. Log: `Artifacts/Logs/build-editor-20260904-105317.log`.
2. First test run (`automation-20260904-105504`) proved absent Controller R/N but also exposed missing Pawn InputComponent and a one-actor counting error. UE source showed local-controller classification requires the GameMode local flag (or a LocalPlayer). The fixture added `SetAsLocalPlayerController()` and counted a possessed Character as Controller-owned.
3. Corrected fixture then reached a real Character N action, destroyed its pawn, and crashed on the subsequent release dispatch. Log: `Artifacts/Logs/automation-20260904-105742.log`; callstack is `FWorldFixture::Dispatch`, line 78, null PlayerInput.

## Root cause and ownership

The transient World had no GameMode-created PlayerState. `AController::PawnPendingDestroy` unpossesses, enters Inactive, and destroys the Controller if PlayerState is null. `APlayerController::Destroyed` clears PlayerInput. This is expected Engine behavior for the incomplete fixture, not proof that a normal local PlayerController dies with a pawn.

The required fixture must represent both boundaries normally provided by GameMode: local controller designation and a PlayerState. It must also account for `Possess` transferring Pawn Owner from Coordinator to Controller.

## Rejected explanations

- Not a stale UObject capture in the product: the stack identifies a direct test dereference after Engine-owned controller destruction.
- Not evidence that Legacy mappings fail on Enhanced-compatible input: the log proves the possessed Character N handler ran and queued the next seed.
- Not a reason to suppress failures or to manually invoke R/N delegates: tests still dispatch through InputKey and the configured PlayerInput stack.

## Next discriminating correction

Create a real transient `APlayerState` and assign it through `Controller->SetPlayerState` in the fixture. Assert Controller/PlayerInput survive the first N event before sending R. Add a null guard in the test dispatcher so a failed persistence assertion cannot crash the process and hide remaining RED results. Re-run the unchanged R/N acceptance expectations before moving any production bindings.

No production R/N binding has been changed at this checkpoint.

## Outcome

The completed fixture produced assertion-only RED at `Artifacts/Reports/automation-20260904-110055/index.json` with exactly the expected missing/duplicated ownership failures and no crash. After the production ownership fix, the same tests passed 3/3 at `automation-20260904-110401`, confirming the fixture corrections did not bypass configured engine input dispatch.

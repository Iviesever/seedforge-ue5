# SF-IRA-008 production input, pathing and restart evidence plan

> **Status:** H1–H6 and storage/clean BuildPlugin checkpoints are preserved by the primary. H7 RED is now valid: strict build, focused 4 pass / 3 intended failures with physical/watchdog controls passing, and real PathObservationUnavailable/exit 2 with no captures. The earlier self-Add fixture crash is retained but excluded from RED. H7-G1 is explicitly authorized and source-prepared; stop for primary build/focused/full/Editor verification before claiming GREEN. F/H6/PS source is unchanged by this subtask. No overall release completion is claimed.

**Goal:** Prove actual enemy A* movement and configured WASD/mouse/LMB/Space/R/N behavior, loss/restart identity and actor ownership in the normal packaged gameplay path, without `-SeedForgeGameplaySmoke`.

**Architecture:** One dormant controller-owned input-self-test component injects UE input before normal input processing and observes effects after normal input/World ticks. The existing Coordinator remains the only gameplay state/request/actor authority. Small read-only diagnostics expose facts currently inaccessible to a verifier. A passive path-proof value helper is shared with gameplay smoke; it never drives gameplay.

**Stack:** Existing UE 5.8 Runtime, C++20, native input mappings on Enhanced-compatible classes, existing generated layout/encounter, current viewport and CharacterMovement.

## Constraints and accepted setup boundary

- Existing checkout only: `D:\program\SeedForge`; Engine is read-only; no clone/worktree/parallel integration tree.
- The corrected latent fixture and H5 focused/full checkpoint are green. H6 RED is valid and the primary authorized the complete validator/serializer and real input driver. H7 still requires a separate primary-observed real-input GREEN checkpoint.
- Native source identity accepts either a 40-hex declared revision or explicitly `diagnostic-<40hex>` for development runs, classifying the latter as diagnostic and preserving the prefix in trace identity. This never proves a dirty binary equals a clean revision; the authoritative PowerShell wrapper accepts only its clean 40-hex verification context.
- Evidence storage is bounded: at most 1025 path cells, 1024 waypoints, two retained movement samples, 96 input events, 24 transitions and eight queued runs. Runtime proof may aggregate validated distance/count/time and retain first/latest samples only until at least two observations and 20 units complete; completion freezes accumulation. Additional run/setup records introduced during GREEN must receive explicit small caps before use. No per-frame append-only trace arrays.
- H6 fixed additional caps are four run snapshots, 16 owned actor records per snapshot, 16 setup records and 4096 initial walkable cells. The tested ordinary scenario uses exactly four snapshots, 13 required input-effect records and four restart queue records; no per-frame history is serialized.
- No assignment of live RunState, player/enemy HP, collected counts, canonical hashes, request IDs or run generation. No fake successful async completion in the packaged runner.
- No AI tick disabling, substitute pathfinder, mocked movement, input-framework migration, or direct calls to private movement/Attack/Dash handlers.
- Primary explicitly permits logged real-actor `SetActorLocation(..., TeleportPhysics)` setup on canonical walkable cells before measurement windows, with a camera cut; and public `ApplyPlayerDamage` to cause Lost. These are labeled setup/public-damage operations, not keyboard or movement evidence.
- No OS cursor/keyboard APIs, `SetMouse`, `SetMouseLocation`, `FSlateApplication::SetCursorPos`, arbitrary settle sleeps, retry growth or timeout expansion.
- Real packaged input uses ordinary World ticks and the normal input stack. The old fixture's manual `ProcessInputStack`/completion broadcast is not claimed as packaged dispatch or worker evidence.
- Runtime input self-test has one fixed 30-second whole-run watchdog, matching the existing smoke budget. Progress is driven by observed conditions; cooldown waits use production tuning. Slow/unsupported execution fails typed rather than widening deadlines.
- F capture ownership and schema remain intact. Apply H's small smoke/path extension only after the primary declares F native files stable. G PowerShell integration remains primary-owned and disjoint from H native work.

## Source facts behind the design

- `SeedForgeGameplayActors.cpp`: PC `PlayerTick` calls `DeprojectMousePositionToWorld`; Character binds MoveForward/MoveRight, Attack and Dash; persistent PC binds R/N. Enemy `Tick` actually uses `VInterpConstantTo` at `EnemyMoveSpeed=260` and `SetActorLocation`.
- `SeedForgeGameplayCoordinator.cpp`: the normal 0.5-second replanner calls the real A* function and passes path cells `[1..]` converted to world waypoints to `Enemy::SetPath`. `StartRun` performs Restarting and Generating synchronously, so polling snapshots cannot prove the intermediate transition. Apply/queued request IDs are already distinct and observable.
- `SeedForgeInputTestWorld.h` and existing B/C tests validate useful input-stack behavior but manually pump input and broadcast completion. They do not prove viewport aim, real enemy displacement, or ordinary packaged execution.
- `SeedForgeDemoGameMode` already supplies the production controller/HUD and spawns one Coordinator; no test map or second GameMode is needed.
- Installed UE 5.8 `PlayerInput.cpp` calls `PreProcessInput` before `EvaluateKeyMapState`, then evaluates delegates and calls `PostProcessInput`. Non-axis delegate dispatch sorts by input EventIndex. Thus two distinct injected keys can supersede a request in one normal processing batch without pumping another stack.
- Installed `FSceneViewport::OnMouseMove` updates cached viewport cursor coordinates used by `GetMousePos`/deprojection. `SetMouse` explicitly calls the OS cursor path and is forbidden. Self-test mouse events require the owning real scene viewport, valid geometry, visible cursor and no mouse capture; no synthetic aim-vector fallback is permitted.

## Alternatives considered

1. **Recommended — controller-owned opt-in component and read-only diagnostics.** Survives pawn replacement; exercises normal engine dispatch; separates verification state from gameplay authority and capture ownership.
2. Extend the existing state-driving gameplay smoke to press keys. Rejected: it couples ordinary-input certification to smoke teleports/capture exit behavior and does not satisfy the no-gameplay-smoke packaged gate.
3. A new test GameMode/map or OS macro. Rejected: a second gameplay host adds unnecessary divergence; OS macros violate scope and introduce desktop timing dependencies.

## Exact file ownership after approval

**New Runtime files:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeGameplayDiagnostics.h`
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayDiagnostics.cpp`
- `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeInputSelfTest.h`
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeInputSelfTest.cpp`

**Minimal existing Runtime edits:**

- `Public/SeedForgeGameplayActors.h`, `Private/SeedForgeGameplayActors.cpp`: controller ownership/input hooks; read-only Character cooldown and Enemy path/move snapshots.
- `Public/SeedForgeGameplayCoordinator.h`, `Private/SeedForgeGameplayCoordinator.cpp`: read-only run-transition/queued/path-applied observations and resource diagnostics. No generator/state-machine algorithm change.
- `Public/SeedForgeGameplaySmoke.h`, `Private/SeedForgeGameplaySmoke.cpp`: additive passive path evidence only. Coordinator smoke gets an AwaitPathProof stage before requesting the first capture; the existing whole-smoke watchdog remains the failure bound.
- `SeedForgeRuntime.Build.cs`: explicitly add public `InputCore` and private `ApplicationCore` for the platform input-device mapper; Slate/SlateCore already exist. No Editor-only dependency in Runtime.

**Tests:**

- New `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeGameplayPathIntegrationTests.cpp`.
- New `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeInputSelfTestTests.cpp`.
- New `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeRunIntegrationTests.cpp`.
- Extend `Private/SeedForgeInputTestWorld.h` only with a bounded normal-World-tick helper/startup guards, preserving existing B/C test semantics.
- Extend `Private/SeedForgeGameplaySmokeTests.cpp` for the additive path-proof codec fields.

The primary later owns `Scripts/TestInputSelfTest.ps1` and G-bound invocation/strict trace validation in aggregate/package entry points. Those scripts are not H native-subtask edits.

## Read-only interfaces

Keep diagnostics as plain C++ values/delegates, not Blueprint mutation functions:

```cpp
// GameplayDiagnostics.h
struct FSeedForgeEnemyMoveObservation
{
    uint64 Sequence = 0, Frame = 0, PathRevision = 0;
    int32 WaypointIndex = INDEX_NONE;
    FVector From = FVector::ZeroVector, Target = FVector::ZeroVector, To = FVector::ZeroVector;
    float DeltaSeconds = 0.0f;
};
struct FSeedForgeEnemyPathSnapshot
{
    uint64 Revision = 0;
    int32 NextWaypointIndex = 0;
    TArray<FVector> Waypoints;
    FSeedForgeEnemyMoveObservation LastMove;
};
struct FSeedForgeRunResourceSnapshot
{
    bool bInteractionTimerActive = false, bRepathTimerActive = false;
    double AttackCooldownRemaining = 0.0;
};
```

- Enemy `FSeedForgeEnemyPathSnapshot GetPathSnapshot() const`: `SetPath`/`ClearPath` increment a per-instance revision; actual Tick records From/Target/To/delta/frame, a monotonic per-actor observation sequence, and the revision/index it consumed. The sequence distinguishes multiple normal World ticks within one Automation host frame without changing GFrameCounter. Clear resets stale movement evidence. These records never change motion decisions.
- Character `double GetDashCooldownRemaining() const`; no setter and no cooldown override.
- Coordinator `FSeedForgeRunResourceSnapshot GetRunResourceSnapshot() const` reads actual timer/cooldown state.
- Coordinator `OnRunStateChanged()` broadcasts `(From, To, const FSeedForgeGameplaySnapshot&)` only after successful production transitions, including Restarting before BeginGenerating. Playing notification occurs after the successful apply identity is stored. Observers only append facts; they never reenter gameplay APIs from a transition callback.
- Coordinator `OnRunQueued()` broadcasts `(StateBeforeStartRun, const FSeedForgeGameplaySnapshot&)` after the actual request ID is assigned. This exposes both rapid queued requests, including the second call's Generating entry state.
- Coordinator `OnEnemyPathApplied()` broadcasts `(ASeedForgeEnemyPawn*, RunGeneration, AppliedRequestId, RequestStart, RequestGoal, const FSeedForgePathResult&)` **after** the real `Enemy->SetPath`. The verifier compares the enemy's actual stored waypoint snapshot against that result; the signal alone is not success evidence.

`FSeedForgeEnemyPathProof` consumes these observations through `ObserveAppliedPath(...)`, `ObserveMovement(...)`, `Reset()`, `IsComplete()` and `GetEvidence() const`. Its evidence carries stable enemy ID, run/request, path revision, cells, actual waypoints, expanded nodes and actual movement samples. It rejects stale ownership/revisions, non-success/empty routes, non-walkable or non-cardinal cells, mismatched world waypoints, nonfinite samples and speed-bound violations. At least two actual movement samples and total displacement >= 20 world units are required. Per-sample distance must be <= `EnemyMoveSpeed * DeltaSeconds + 0.1`; samples must lie on the segment actually consumed for that path revision. Replanning never substitutes a different revision's waypoints for an earlier move.

## Self-test component and ordinary input boundary

```cpp
USeedForgeInputSelfTestComponent::Start(const FString& SourceIdentity, uint64 ExpectedInitialSeed);
USeedForgeInputSelfTestComponent::BeforeInput(float DeltaSeconds, bool bGamePaused);
USeedForgeInputSelfTestComponent::AfterInput(float DeltaSeconds, bool bGamePaused);
USeedForgeInputSelfTestComponent::Cancel();
USeedForgeInputSelfTestComponent::IsRunning() const;
// OnFinished(const FSeedForgeInputSelfTestTrace&) fires exactly once after cleanup.
FSeedForgeInputSelfTestCodec::ExportCanonicalJson(const FSeedForgeInputSelfTestTrace&);
FSeedForgeInputSelfTestCodec::ValidateEvidence(const FSeedForgeInputSelfTestTrace&, FString& OutError);
FSeedForgeInputSelfTestCodec::ParseSourceIdentity(const FString&, ESeedForgeInputSourceKind&, FString& OutRevision);
```

- Create/register the component on the persistent PC only for explicit `-SeedForgeInputSelfTest`. Disabled mode has no input injection, timers or delegate subscriptions. The component never creates another Coordinator or replaces the GameMode.
- PC overrides PreProcessInput/PostProcessInput and forwards to the component around the normal Super behavior. Input is queued only in BeforeInput, via the owning `UGameViewportClient::InputKey(FInputKeyEventArgs::CreateSimulated(..., SceneViewport))`. The engine itself evaluates mappings/actions and movement; the packaged driver never calls `ProcessInputStack`.
- Aim uses `GetGameViewport()`, `GetCachedGeometry()` and an `FPointerEvent` with the primary platform input device. Convert the projected viewport pixel target through viewport-local geometry to screen-space for `OnMouseMove`. Require no capture, valid nonzero geometry and a visible cursor before dispatch. Missing viewport is typed `ViewportUnavailable`; captured/invalid geometry is typed `ViewportNotReady` under the fixed watchdog. Do not replace aim with `SetAimWorldPoint` in this runner.
- Aim/attack retains component-local pointer intent and feeds the same guarded pointer path in normal PreProcessInput until AttackKill completes, then clears it; cleanup also clears it. This prevents later ordinary Slate cursor refresh from replacing a one-shot cached position. These repeated held-intent feeds are not additional proof effects or appended history; the trace still has exactly two aim effects and 13 total effects.
- Observe pending Dash launch immediately after normal input evaluation, actual motion after `FWorldDelegates::OnWorldPostActorTick`, and the normal PC aim update after its tick. Filter every global callback to the owning World. Retain an owning handle for each delegate.
- Track injected held keys and release them on finish/failure/cancel/EndPlay, including when unpossessed. Unbind World/Coordinator observers before OnFinished. Late callbacks cannot finish twice or append evidence for a later run.
- The component only returns a finished trace. The command-line PC adapter validates/writes it and requests exit 0/2 once; Automation can observe OnFinished without exiting the host process.
- `-SeedForgeInputSelfTest` is mutually exclusive with `-SeedForgeGameplaySmoke` and ordinary capture-auto-exit arguments. Require `-SeedForgeInputTrace=<path>` and `-SeedForgeGitSha=<40hex>` or explicitly diagnostic-prefixed development identity. Trace source kind distinguishes declared clean-shaped from diagnostic identity; only the external authoritative wrapper can certify the former against a clean checkout. Invalid arguments log a typed failure and exit nonzero, never silently fall back to ordinary capture success.

## Condition-driven scenario

All setup and measurement windows are separately recorded; setup displacement is excluded from movement evidence.

| Phase | Production operation and required observation |
|---|---|
| Initial ready | Observe real async startup reach Playing; one Coordinator/controller/HUD and one complete owned run set; applied request is nonzero and pending request is zero. Capture seed/layout/encounter identity and weak actor identities. |
| Arrange/prove path | Choose canonical walkable setup cells and a short cardinal route. Place the real target enemy and player before measurement; place other enemies on distinct distant walkable cells if needed, logging all moves, without disabling AI. Wait for normal ReplanEnemies to supply a successful path and normal enemy ticks to prove bounded movement. |
| Aim/attack | Project two distinct in-viewport world targets, dispatch pointer moves, and require actual GetAimDirection/rotation to agree. Arrange one live enemy in the real attack arc/range. Send exactly two mapped LMB presses separated by the production attack cooldown, with releases; require it alive after the first and destroyed after the second, with the live enemy count decreased exactly once. No direct TryPlayerAttack call. |
| WASD | On a canonical open test pad, inject each W/S/A/D separately until actual post-tick displacement reaches 20 units in the expected direction. Require finite bounded movement and capsule-safe/walkable location. Release and observe natural braking before the next logged setup; do not set velocity or disable collision. |
| Dash | Queue W+D+Space in one input frame; normal PostProcessInput must expose the diagonal PendingLaunchVelocity matching DashImpulse. Then require actual bounded post-tick displacement. Release movement, preserve the real cooldown, and record it nonzero before loss. |
| Lost/same seed | Invoke public ApplyPlayerDamage(current health), record real Playing->Lost and stopped timers/cleared paths. Dispatch R through the normal persistent controller stack. Record actual Restarting then Generating, zero pending hashes/applied ID, a live new request, no possessed old pawn and no live old owned actors. Wait for the real completion; require the same seed/layout/encounter hashes, fresh actor identities, full HP, zero Core eligibility, locked exit, zero initial cooldowns and normal timers. |
| New seed | Dispatch N. Require exactly `oldSeed * 6364136223846793005ULL + 1442695040888963407ULL` with uint64 semantics, one incremented run, the actual request/applied identity and fresh ownership. Independently compute expected layout/encounter with the public pure generator/planner and compare; never assign them into the running Coordinator. |
| Rapid restart | Queue N then R presses in the same PreProcessInput batch, then releases on the next frame. Require two OnRunQueued records in event order, the second entering from Generating, strictly newer request/run identity, and final seed equal to one next-seed transform. Natural async completion may apply only the last request; old weak actors remain destroyed and exactly one complete current run set/possession survives. |
| Final validation | Verify every required input/effect/path/restart record, one persistent HUD overlay, no duplicate controllers/Coordinators/run actors, no stale pending request or callback, and zero self-test delegate bindings after cleanup. Serialize success only after this complete validation; otherwise emit one typed failed trace. |

Initial/same-seed golden checks remain `7425849530159566348` layout and `15303214708604970503` encounter for seed 24301. Actor equality is process-local weak-object identity plus logged object path/run/stable-role ID, not pointer-address hashes or fabricated canonical identities. Ownership counts are independently enumerated from live actors and actual owners, not just Coordinator array lengths. The initial/default complete set is player + visualization + 3 cores + 5 enemies + exit; controller/HUD/Coordinator are persistent and counted separately.

## Separate input-selftest trace

Use schema `seedforge.input-selftest`, schemaVersion 1; do not relabel this as the gameplay-smoke trace. Preserve all uint64 identities/frame counters as decimal strings. Record:

- Git SHA, Engine version, mode `ordinary`, explicit absence of gameplay-smoke mode, initial/final seed/run/request/hash identities.
- Setup poses/cells and frame boundaries; injected key/pointer events and the actual effect-confirmation frames/positions/aim/launch results.
- Path proof with stable ID + run/request + path revision, finite walkable cells/waypoints and actual bounded movement samples.
- Observed transitions and queued/applied request sequences, same/new/rapid snapshots, live ownership and fresh weak-identity assertions, cooldown/Core/exit/timer observations.
- Started/completed UTC, one result/failure code/message, completion count and remaining self-test delegate bindings.

No success result is inferred merely from an event being injected or a command returning true. The pure evidence validator rejects missing/reordered effects, wrong run/request attribution, nonfinite/off-route movement, missing transient transitions, stale actor identity, duplicate completion or nonzero delegate ownership. Synthetic negative trace values are clearly unit fixtures, never live gameplay state mutation or release proof.

## RED-first implementation sequence

- [x] **H1: Write failing observability/path tests before production diagnostics.** Add declarations and empty/default diagnostic results only as compile scaffolding. In a real transient World, apply the genuine generated layout through the existing fixture boundary and begin normal actor play. A latent Automation command performs at most one `World->Tick(LEVELTICK_All, 1.f/60.f)` per actual engine frame, capped at 120 ticks; it never changes GFrameCounter. Never call Enemy::Tick or ReplanEnemies directly. Require timer progress, real bounded movement, then a real path-applied observation and proof. Empty scaffold observations must fail only the missing-evidence assertions. Pair begun-play fixture teardown with World::EndPlay.

```cpp
Fixture.ApplyCurrent(); // Focused World setup only; NOT packaged async proof.
// Inside IAutomationLatentCommand::Update; return false to yield to the engine.
if (GFrameCounter == LastEngineFrame) { return false; }
LastEngineFrame = GFrameCounter;
Fixture.World->Tick(LEVELTICK_All, 1.0f / 60.0f);
++Ticks; // at most 120 distinct host frames, never a same-frame loop
```

- [x] **H2: Add real loss/restart integration tests and empty transition/queued scaffolding.** Prove `Playing -> Lost -> Restarting -> Generating -> Playing` through public damage plus input dispatch; same/new identities; immediate absence of old ownership; queued request supersession; timer/path/cooldown reset. A missing Restarting observation or stale applied ID is a RED assertion, not an invented transition appended by the test. Retain B's injected-stale-completion fixture tests as deterministic unit coverage, but do not claim them as worker completion.
- [x] **H3: Add self-test/evidence REDs with a fail-closed component scaffold.** `Start` may report `MissingInputEvidence` until implemented; it must never invent Passed. Add pure validator negatives for missing effects/path/ownership/transitions and cancellation/exact-once completion. Add a real fixture input-binding negative: remove W, Attack, Dash or one R/N binding only from that fixture's InputComponent, dispatch engine events, and require the corresponding proof to remain absent/fail. Add the missing-scene-viewport typed-failure test; no direct aim fallback.
- [x] **H4: Primary builds and runs the RED namespace; retain report/log counts.** Run `Scripts/Build.ps1`, then `Scripts/Test.ps1 -Filter SeedForge.Audit.GameplayPath`, `-Filter SeedForge.Audit.RunIntegration`, and `-Filter SeedForge.Audit.InputSelfTest`. No assertion is softened because the new evidence is missing.
- [x] **H5: Implement only read-only seams and passive path proof; rerun H1/H2.** Record facts after their actual production boundary. Do not expose mutation hooks. Ensure diagnostics added to Tick/SetPath/ClearPath cannot alter algorithm, tie-break, speed, timing or ownership decisions.
- [x] **H6: Implement the controller-owned driver and normal input injection.** Build, run unit negatives and positive integration, then run the actual Editor `-game` ordinary self-test. If a viewport/input/physics assumption fails, retain a root-cause packet and fix that boundary; do not substitute manual stack pumping, direct aim/attack calls or longer sleeps.
- [ ] **H7: Reuse passive path proof in gameplay smoke.** Add AwaitPathProof before first capture; feed actual replanner/move observations; use the existing 30-second whole-smoke watchdog. Extend the gameplay-smoke codec with `pathEvidence`, and require it in its evidence validator. Do not alter F's rendered-capture lifecycle or fabricate extra screenshot receipts.
- [ ] **H8: Primary runs focused regressions and full Automation, then ordinary Editor and fresh packaged self-tests.** Keep all B/C/D/E/F namespaces and existing goldens. The packaged run uses the actual normal GameMode/controller and natural generation completions, without the gameplay-smoke flag.
- [ ] **H9: Independent review before a separate H commit.** Explicitly review no-op ReplanEnemies/Enemy::Tick, missing bindings, stale ownership and missing viewport failure sensitivity. The new proof requires both path receipt and physical movement, so a no-op in either boundary cannot remain Passed. Only the primary may perform a temporary source mutation experiment/build; restore the exact scoped diff and rerun GREEN if such an experiment is used. Do not leave production fault flags or weaken tests.

## Primary-owned runtime verification commands

Use revision-bound unique project-local trace/log directories; no `-NullRHI` for pointer/viewport proof. Actual paths are supplied by the primary's G wrapper, not invented by this native component.

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.GameplayPath -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.RunIntegration -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.InputSelfTest -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge -TimeoutSeconds 900

# Editor ordinary-game path; primary supplies the validated variables.
& $EditorExe $ProjectFile /Game/Maps/SeedForgeDemo -game -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -unattended -nosound -SeedForgeInputSelfTest -SeedForgeSeed=24301 "-SeedForgeInputTrace=$InputTracePath" "-SeedForgeGitSha=$ExpectedRevision" "-abslog=$InputLogPath"

# Fresh Development packaged executable, intentionally no gameplay-smoke flag.
& $PackagedExe -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -unattended -nosound -SeedForgeInputSelfTest -SeedForgeSeed=24301 "-SeedForgeInputTrace=$PackagedInputTracePath" "-SeedForgeGitSha=$ExpectedRevision" "-abslog=$PackagedInputLogPath"
```

Require native exit 0, one `SEEDFORGE_INPUT_SELFTEST_SUCCESS` marker, independently reparsed complete trace, strict whole-log audit and unchanged clean revision before/after each process. Failure tests require a typed failed trace/marker and nonzero controlled exit, not an external timeout masquerading as a negative success. The primary also reruns Editor/packaged gameplay smoke with passive path proof and F's capture validation.

## Recorded H6 RED preparation checkpoint

The original 11 tests passed, as did the primary's full 109-test run. H6 added four InputSelfTest tests: complete synthetic evidence acceptance, complete synthetic success-schema serialization, tampering/missing-effect rejection, and CLI option safety. The first two were intentional RED expectations against the fail-closed success validator/serializer and were observed to fail; no synthetic string is published as a runtime trace.

At that RED checkpoint the opt-in PC created one instance component, bound one completion callback, and forwarded normal PreProcessInput/PostProcessInput. The component injected nothing. With a real scene viewport it reported Failed/MissingInputEvidence, wrote one fresh absolute JSON file without replacement, and the dedicated process requested exit 2 once. Invalid source/path/seed or conflicting smoke/capture options fail without starting another driver. A missing/invalid output path cannot produce a trace; it still logs one failure and exits nonzero. The authoritative wrapper owns current-run path containment and process/revision checks. No UE execution occurred in this sub-agent's preparation.

## H6 JSON field contract — schemaVersion 1

This is the finalized contract for the primary's PowerShell validator. At the recorded RED checkpoint only the failure envelope was emitted. The pending-verification GREEN implementation now emits bounded evidence fields and validates success before publication. A failed early trace may have absent or empty/default success-only evidence arrays/objects. A Passed trace must contain and satisfy all fields below.

**Common envelope:** `schema="seedforge.input-selftest"`, numeric `schemaVersion=1`, strings `sourceIdentity`, `sourceRevision`, `sourceKind` (`clean`, `diagnostic`, `unverified`), actual boolean `sourceVerified=false`, `engineVersion`, `mode="ordinary"`, actual boolean `gameplaySmokeEnabled=false`, uint64 decimal-string `expectedInitialSeed`, UTC-Z `startedAtUtc`/`completedAtUtc`, uint64-string `startedFrame`/`completedFrame`, numeric `completionCount=1`, `remainingDelegateBindings=0`, `remainingPressedKeys=0`, `result` (`Passed`/`Failed`), `failureCode`, `failureMessage`. Authoritative wrappers accept only their clean 40-hex identity; native diagnostic classification is never release authority.

Passed envelope `failureCode` and `failureMessage` are exactly empty strings. Internal native success uses the None enum; nested gameplay snapshots retain their existing `failureCode="None"` representation. `sourceVerified` remains false even when the declared source is clean-shaped.

**Value encoding:** all uint64 IDs, seeds, hashes, frames, sequences and aggregate move counts are canonical decimal strings; all numeric vectors/cells/cooldowns/distances are finite JSON numbers. A vector is `{x,y,z}`; a cell is `{x,y}` with int32 coordinates. Times are valid UTC ISO strings ending in uppercase Z. Object/actor keys are opaque process-local strings, not addresses or canonical game hashes.

**Gameplay snapshot** (`initial`, `final`, `runs[].snapshot`, `transitions[].snapshot`, `queuedRuns[].snapshot`): `seed`, `layoutHash`, `encounterHash`, `runGeneration`, `pendingRequestId`, `appliedRequestId` as uint64 strings; `runState` as the exact enum name; finite numeric `playerHealth`, `playerMaxHealth`; numeric `collectedCoreCount`, `requiredCoreCount`; boolean `exitUnlocked`; strings `failureCode`, `failureMessage`. Initial/final equal the first/last run snapshot and are applied Playing states, with pending request zero and nonzero applied identity.

**`runs` (exactly four; cap four):** ordered labels `initial`, `same`, `new`, `rapid`. Each contains `label`, uint64-string `frame`, `atUtc`, `snapshot`, `resources` (`interactionTimerActive`, `repathTimerActive`, `attackCooldownRemaining`), `dashCooldownRemaining`, `coordinatorKey`, `controllerKey`, `hudKey`, numeric `coordinatorCount`, `controllerCount`, `hudCount`, `hudOverlayCount`, boolean `previousActorsDestroyed`, and `actors` (cap 16). Each actor record contains `key`, `role`, numeric uint32 `stableId`, `ownerKey`, `cell`. Roles are `player`, `visualization`, `core`, `enemy`, `exit`; visualization cell is the non-positional zero sentinel. Default fresh runs contain one player/visualization/exit, three cores and five enemies. Controller/HUD/Coordinator keys remain persistent; run actor keys are fresh and disjoint between snapshots. Counts and freshness come from actual live-owner/weak-object observations. Same-seed hashes equal initial; new/rapid seeds use the exact LCG; each fresh run resets HP/Core/exit/cooldown state and restores normal timers.

**`walkableCells` (cap 4096):** canonical initial-layout cell membership used by the recorded path proof. It must be unique and consistent with the initial layout identity and the native pure generator validation; it is not a second layout authority.

**`pathEvidence`:** numeric uint32 `stableId`; uint64-string `runGeneration`, `sourceRequestId`, `pathRevision`; `start`, `goal`; `status="Success"`; numeric `expandedNodes`; `cells` (2..1025), `waypoints` (cells minus one, cap 1024); `movementSamples` (exactly two retained first/latest samples); uint64-string `observedMoveCount` (>=2); finite `totalDistance` (>=20), `totalDeltaSeconds`; boolean `complete=true`. Each movement sample has uint64-string `sequence`, `frame`, `pathRevision`, numeric `waypointIndex`, vectors `from`, `target`, `to`, and positive finite `deltaSeconds`. Require actual matching identity/revision, increasing sequence/nondecreasing frames, cardinal walkable cells, waypoint conversion, consumed-segment membership and movement <= speed*delta+0.1. Aggregates remain bounded and complete proof stops growing.

**`inputEvents` (13 required effects in exact order; cap 96):** `AimX/MouseMove`, `AimY/MouseMove`, `AttackFirst/LeftMouseButton`, `AttackKill/LeftMouseButton`, `MoveW/W`, `MoveS/S`, `MoveA/A`, `MoveD/D`, `Dash/SpaceBar`, `RestartSameSeed/R`, `StartNewSeed/N`, `RapidNewSeed/N`, `RapidRestartSameSeed/R`. Every event has `action`, `key`, `actorKey`, uint64-string `runGeneration`, `sourceRequestId`, `injectedFrame`, `effectFrame`, `releaseFrame`, UTC-Z `injectedAtUtc`, `effectAtUtc`, vectors `before`, `after`, `launchVelocity`, finite `cooldownBefore`, `cooldownAfter`, numeric `liveEnemiesBefore`, `liveEnemiesAfter`, booleans `targetAliveBefore`, `targetAliveAfter`, `confirmed=true`. Unused effect-specific scalar/vector fields are zero. Mouse moves use releaseFrame zero; other keys have a real later release frame bounded by completion. R/N records identify the affected newly assigned run/request, not an invented equality with the prior run. First nine events belong to initial applied identity. The last rapid N/R injections share one engine input frame in N-then-R event order.

Aim before/after values are actual normalized XY aim directions and must demonstrate the requested distinct targets. Move before/after are actual post-tick positions with >=20 units in the keyed cardinal direction and speed/collision/walkable bounds. Dash records the actual diagonal pending launch at DashImpulse plus real motion/cooldown: scalar deltas and derived XY distance must be finite, distance must be 19.9..200 inclusive, direction dot with normalized positive XY diagonal must be >=0.98, and both endpoints must lie inside canonical full-cell walkable footprints (100.1 units from the cell center on each axis). The live driver waits for >=20 units and retains its stricter per-frame capsule/speed checks. Dash launch and cooldown are sampled only at the injected input frame, not overwritten after CharacterMovement consumes PendingLaunchVelocity. AttackFirst leaves the target alive and consumes the real cooldown; AttackKill occurs after that cooldown and destroys exactly that target/decrements the live count once. Merely injecting an event or setting confirmed is never enough.

**`transitions` (mandatory 10; cap 24):** `{from,to,snapshot,frame,atUtc}`. From the post-initial measurement window: Playing->Lost; Lost->Restarting->Generating->Playing; Playing->Restarting->Generating->Playing; Playing->Restarting->Generating->Playing for the rapid run, where the last Playing belongs to the final superseding request. No artificial initial transition is required if startup was first observed already Playing. Snapshot state equals `to`; queued evidence explains the run-ID change while Generating.

**`queuedRuns` (mandatory four; cap eight):** `{stateBefore,snapshot,frame,atUtc}` for same/new/rapid-N/rapid-R, with stateBefore Lost/Playing/Playing/Generating. Snapshot is Generating, applied/hash identities are zero, pending request is nonzero, and run/request ordering is strict. Rapid N/R have the same input-frame stamp and final applied identity equals the last queued request; the earlier rapid request never produces a Playing snapshot.

**`setups` (cap 16):** `{label,actorKey,runGeneration,sourceRequestId,frame,atUtc,cell,position}`. These record actual permitted canonical-cell placements before measurement; setup displacement is excluded from input/path proof. Measurement and event/frame/time order must establish that setup precedes the affected window.

The new tests use clearly named synthetic value factories, real pure-generated identities/routes, and deliberately synthetic actor/effect records. They write no runtime evidence files. Complete success acceptance and complete success JSON both failed at the primary-observed H6 RED boundary. Their original complete fixtures and rejection expectations remain intact; the schema test additionally locks empty Passed failure fields and unverified native source status.

## H6 implementation handoff after observed RED

The component retains exactly 13 effect records, four run snapshots, four queued observations and ten transitions on the successful path. It uses a monotonic fixed 30-second budget, with no waits or deadline growth. Actual key presses/releases are sent only in the normal pre-input hook; mouse projection and guarded scene-viewport cursor updates precede ordinary PlayerTick deprojection. The read-only `OnCursorQuery` additionally rejects the engine's hidden-after-capture cursor state before `OnMouseMove`, avoiding that method's OS-cursor restoration branch.

Physical movement/path samples come only from normal post-actor World observations. Setup is capped and logged, other enemies remain alive/ticking, attack cooldown and movement braking waits are actual state conditions, and loss is caused only through public damage. Restart effects observe transient/queued identities during normal dispatch and wait for the real final applied request. Independent live actor enumeration, weak old-actor identities, persistent HUD/controller/Coordinator keys, bounded tagged-widget traversal and actual post-cleanup key state support ownership/cleanup evidence.

The validator independently regenerates the four model identities and initial canonical cells, recomputes the recorded A* result, validates retained motion samples/aggregate bounds, and requires the full effect/transition/queue/run relationships. This is necessary native evidence, not sufficient release authority. The adapter writes once without replacement; only validated success plus successful write emits the success marker and requests exit 0. Otherwise it emits failure and preserves exit 2. No H7 smoke/capture files or primary-owned Scripts were changed in this handoff.

## H7 additive passive smoke-path plan — RED source released

**Current gate:** the primary observed complete focused/live RED and explicitly released H7-G1. The real bounded observer, targeted validator and additive JSON are now implemented but not yet UE-verified. Freeze for the primary's build/focused/full/Editor checkpoint. Parent owns all Scripts, UE processes, staging/commits and clean BuildPlugin/package verification.

**Goal:** gameplay-smoke cannot request its first screenshot or publish Passed before a genuine same-run A* route and at least two actual enemy moves totaling 20 units have been observed. Ordinary input remains separate; no input/actor setup or combat behavior is added here.

### Chosen ownership and exact files

Recommended: a small noncopyable RAII passive observer in the existing GameplaySmoke files. It owns the existing H5 proof and delegate handles; Coordinator only gates the first capture. This makes cancellation/restart testable without exposing private smoke stages. Raw callbacks in Coordinator are slightly shorter but harder to test independently; 0.1-second position polling is rejected because it misses actual consumed movement/revision observations.

- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeGameplaySmoke.h`: include GameplayDiagnostics, trace additions and observer declaration.
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplaySmoke.cpp`: observer, bounded path JSON and targeted evidence validator.
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeGameplayCoordinator.h`: observer member and `AwaitPathProof` stage.
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`: lifecycle/gate only; no pathfinder/Enemy::Tick/combat/capture-component algorithm edits.
- Extend `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeGameplaySmokeTests.cpp`; reuse the existing input World fixture without editing it.
- Update this plan and `sf-ira-008-evidence.md`. No new Runtime file/UObject, no H6 input-source edits, no F capture lifecycle/receipt changes.

Interfaces implemented after the observed RED gate:

```cpp
// FSeedForgeGameplaySmokeTrace additions
static constexpr int32 MaxWalkableCells = 4096;
FSeedForgeEnemyPathEvidence PathEvidence;
TArray<FIntPoint> WalkableCells;
int32 RemainingPathDelegateBindings = 0;

class FSeedForgeGameplaySmokePathObserver final
{
public:
    FSeedForgeGameplaySmokePathObserver() = default;
    ~FSeedForgeGameplaySmokePathObserver(); // idempotent Cancel
    FSeedForgeGameplaySmokePathObserver(const FSeedForgeGameplaySmokePathObserver&) = delete;
    FSeedForgeGameplaySmokePathObserver& operator=(const FSeedForgeGameplaySmokePathObserver&) = delete;
    bool Start(ASeedForgeGameplayCoordinator& Coordinator);
    void Cancel();
    bool IsComplete() const;
    const FSeedForgeEnemyPathEvidence& GetEvidence() const;
    int32 GetRemainingDelegateBindings() const;
};

// FSeedForgeGameplaySmokeCodec addition: targeted H7 evidence, not a PNG verifier.
static bool ValidatePathEvidence(const FSeedForgeGameplaySmokeTrace& Trace, FString& OutError);
```

Start first Cancels prior ownership, requires actual Playing/nonzero run+applied request, finds the real owned live lowest-stable-ID enemy, copies the actual canonical walkable cells (cap 4096) and tuning, then subscribes to `OnEnemyPathApplied`, `OnRunQueued`, and `FWorldDelegates::OnWorldPostActorTick`. It holds weak Coordinator/enemy references and exactly three handles. Path callbacks accept only the exact actor/run/request, then feed the existing `ObserveAppliedPath` with the actual enemy snapshot. World callbacks filter the exact World, recheck current Playing/applied identity and live ownership, and feed actual LastMove into `ObserveMovement`. No direct ReplanEnemies/Enemy::Tick or actor movement call is allowed.

Completed proof freezes its bounded first/latest samples and unbinds all handles while retaining evidence. Queued restart cancels/reset evidence immediately. Cancel removes handles, clears weak identities and resets proof; destructor also Cancels. Coordinator explicitly Cancels before actor destruction, trace initialization/reset, failure, completion and EndPlay. A later run cannot inherit old proof. An observer invalidated before completion cannot request a screenshot.

### Exact additive JSON/F correlation contract

Keep `seedforge.gameplay-smoke`, schemaVersion 1 and all existing fields/receipts. Add only:

- `walkableCells`: actual initial layout canonical `{x,y}` cells, unique/canonical order, cap 4096.
- `pathEvidence`: identical field names/encodings to H6: numeric `stableId`; uint64 strings `runGeneration`, `sourceRequestId`, `pathRevision`; `start`, `goal`; `status`, `expandedNodes`; `cells`, `waypoints`, `movementSamples`; uint64 string `observedMoveCount`; finite `totalDistance`, `totalDeltaSeconds`; boolean `complete`. Each sample contains `sequence`, `frame`, `pathRevision`, `waypointIndex`, `from`, `target`, `to`, `deltaSeconds`. Caps remain 1025 cells/1024 waypoints/exactly two retained first/latest moves.
- `remainingPathDelegateBindings`: numeric zero on successful publication. This counts passive observer ownership only; do not reinterpret F receipt `remainingDelegateBindings`.

Require nonzero top `runGeneration`/`appliedRequestId`, equal path run/source request and equal all three F receipt run/source IDs. Stable ID must identify an initially counted enemy. Recompute the recorded A* request using canonical cells and the existing 1024 budget; require exact status/path/expanded-node match, cardinal walkable route and 200-unit/height-58 waypoint conversion. Retained samples must be finite, same revision, increasing sequence/frames, consumed-waypoint/route-consistent, and each distance <=260*delta+0.1. Aggregate count/distance/time must be finite/consistent; count>=2 and distance>=20. Use H5's existing proof rules, not new movement timing.

For count==2, both observed moves are retained: require contiguous positions, index advance at most one, and distance from the first end to its consumed target <=4.1 when advancing. For count>2, retain nonadjacent first/latest compatibility but require the omitted gap to fit both aggregate budgets: `TotalDistance+0.1 >= retainedDistanceSum+Gap`, `HiddenDt >= -1e-6`, and `Gap <= 260*max(0,HiddenDt)+0.1*(count-2)+0.1`, where Gap is first.to to last.from and HiddenDt is total delta minus retained deltas. This is a straight-line lower bound, not invented intermediate samples. The primary aligned the external validators to these exact tolerances.

Both retained movement frames must be <= `captures[0].requestedFrame`. That request happens only after proof completion; equality is valid when proof and request occur in one engine frame. Existing F receipts stay exactly start/combat/win, with requested<=rendered<=captured<=completed and matching same-run IDs. No extra capture/receipt is manufactured. H7 does not substitute for PNG/process/source validation in parent-owned Scripts.

`ValidatePathEvidence` rejects absent/incomplete/oversized/nonfinite/off-route/wrong-revision/wrong-run proof, proof after the first request, missing/reordered/wrong-ID receipt correlation and nonzero passive bindings. CompleteGameplaySmoke validates before setting success/printing its marker. ExportCanonicalJson must fail closed if bSuccess is asserted without valid path proof, using `MissingPathEvidence`; already failed traces retain their existing failure. Invalid numeric evidence in failed JSON is null, not NaN/Infinity. Preserve exact uint64 encoding and existing string escaping.

### Coordinator stage, single budget and cleanup

```text
normal async apply + current actor checks -> AwaitPathProof
real replanner + enemy ticks -> completed frozen proof, passive handles removed
existing 0.1s AdvanceGameplaySmoke -> WaitingStartCapture -> existing start request
unchanged F capture/combat/core/exit flow -> win receipt -> H7 validation -> success
```

After existing actor-position checks, StartGameplaySmoke copies canonical cells and starts the observer. The existing stage timer starts as before, but its initial screenshot request is replaced by AwaitPathProof. That stage copies complete frozen evidence, checks zero bindings/current identity, sets WaitingStartCapture and requests start exactly once. Start failure is `PathObservationUnavailable`; invalidated state is `PathObservationInvalidated`; deadline without proof uses existing `SmokeTimeout`.

Keep one existing 30-second whole-smoke watchdog. Do not create a proof timer or rearm/extend the watchdog after proof. StartGameplaySmoke must not extend the budget already armed by StartRun. Existing F deadlines remain untouched. Copy complete evidence before Cancel resets the observer at final cleanup; trace reset/restart clears it before reuse.

### RED-first stages after explicit source release

- [x] **H7-R1 — Tests/scaffolds:** add four tests: `SeedForge.GameplaySmoke.PathEvidenceAcceptsCompleteSynthetic`, `PathEvidenceRejectsTampering`, `PathEvidenceSchema`, `PassiveObserverUsesRealWorldAndCleansUp`. Compile scaffolds return false/empty from Start/validation and produce no successful observation. The positive value fixture uses actual pure model/A* plus clearly synthetic two moves and three F-shaped receipts, never written as runtime proof. Negative cases remove path/sample, use 19 units, stale run/request/revision, nonfinite/off-route/overspeed movement, oversized arrays, wrong receipt ID, movement after first request, or nonzero bindings.
- [x] **H7-R2 — Preserve controls:** upgrade the old success-codec fixture to a complete synthetic proof/three receipts while retaining schema/hash/count/order assertions. Retain maximum uint64 encoding on a separate failed metadata value rather than pretending inconsistent seed/hash metadata is successful evidence. Failure escaping/code controls remain. The real observer test uses existing FWorldFixture/normal latent ticks once per actual GFrameCounter, cap 120; timer/physical controls must pass independently of absent observer proof. Add Cancel/restart/no-stale-proof and zero-handle assertions; never manually tick Enemy or call ReplanEnemies.
- [x] **H7-R3 — Live fail-closed gate:** wire AwaitPathProof to the scaffold and stop. Parent strict-builds, runs focused tests and real Editor gameplay smoke. Since scaffold Start returns false, expected live RED is typed PathObservationUnavailable/exit 2 with no first capture/receipt, not an outer timeout. The unchanged 30-second watchdog remains the later bound for an observer that starts but never completes. A compile/fixture error is not accepted as RED. Preserve exact reports.
- [ ] **H7-G1 — Minimum implementation after observed RED:** implement observer + targeted path validation/JSON + lifecycle/gate. No H6 input or F component edit. Stop for parent focused/full/Editor smoke.
- [ ] **H7-G2 — Parent final checks:** fresh Editor and packaged smoke must prove actual path before first capture, three fresh validated F receipts/PNGs, zero passive/capture handles, matching run/request/frames, strict logs and clean revision/process boundaries. Rerun H6 ordinary input separately. BuildPlugin/packaging/source certification remain separate G gates, not implied by H7.

Parent commands after boundary and source gates are released:

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.GameplaySmoke -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.GameplayPath -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.InputSelfTest -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge -TimeoutSeconds 900 -AllowDirtyDiagnostic
.\Scripts\TestGameplay.ps1 -AllowDirtyDiagnostic
.\Scripts\TestInputSelfTest.ps1 -AllowDirtyDiagnostic
```

These commands remain parent-only. H7-G1 source authorization does not authorize this sub-agent to execute UE or claim GREEN without primary results.

### H7 GREEN-source handoff after observed RED

The primary's corrected report `automation-20260904-185054-a42335c52d284a8094a5e63b96ea1441` is 4 pass / 0 warning-success / 3 fail: observer 8 expected assertions, acceptance 2, schema 9. Physical controls report 120 World ticks, 118 timer firings and 502.666685 units; watchdog lifecycle passes. Earlier `184620` exit 3 was a TArray self-element Add fixture assertion, not valid RED. The primary changed it to a local FIntPoint copy before Add; that correction is preserved. Real `Gameplay/20260904-184729-9b8302d66c5e4612af6b68c285792779` records PathObservationUnavailable, Generating/Playing/Failed, exit 2, zero receipts/PNGs and valid owned storage.

After authorization, Start now observes the real owned enemy through exactly three delegates. It feeds the existing H5 proof on normal path-applied/World-post boundaries, enforces strictly advancing retained frame observations, and detaches on completion while retaining frozen evidence. Cancel/queued restart/owner cleanup remove handles and reset identities. The existing Coordinator gate waits for proof; invalidated ownership fails closed. No setup, AI/input/attack call or timing growth is added.

The codec now serializes bounded path/walkable/binding fields, recomputes the native A* result and checks receipt correlation, samples, arrival and aggregate bounds. Failed nonfinite data emits null, and asserted success without valid path cannot emit Passed. Tests add a valid nonadjacent first5/hidden8/last10 observation and two reported aggregate counterexamples (distance gap and hidden time) without adding sample history. The primary had reproduced the aggregate defects as external-validator RED; no separate native result for these added cases is claimed yet.

Stop for primary strict build, all seven focused smoke tests, full Automation and fresh Editor gameplay smoke plus external path/PNG/storage validation. Source identities for that run follow current parent HEAD (now beyond the RED f235e46 checkpoint). H6 native, F components and PS files are unchanged by this implementation.

### Historical H7 RED-source handoff details

Exactly the five allocated native/test files are changed, plus this task/evidence. Start/ValidatePathEvidence return false; the observer subscribes to nothing and retains no proof. The existing JSON exporter is deliberately not extended yet, so the new schema/fail-closed codec expectations discriminate missing GREEN behavior. The real Coordinator is fail-closed independently: StartGameplaySmoke enters AwaitPathProof and rejects the false Start with PathObservationUnavailable before requesting any screenshot. CompleteGameplaySmoke also requires the false validator before success.

Focused namespace now has seven tests: existing SuccessTrace/FailureTrace, four planned path tests, and the approved watchdog lifecycle test. Source-level expectation is four passing controls and three failing tests (acceptance, schema, real observer); expected assertion failures are 2 + 9 + 8 respectively, subject to actual primary execution. Timer/physical controls must pass; compilation or fixture errors do not count as valid RED.

The primary approved a WITH_DEV_AUTOMATION_TESTS-only friend seam for the watchdog test, defined only in GameplaySmokeTests.cpp. It arms an actual World timer in the existing handle, advances two distinct engine-frame World ticks, then checks elapsed/remaining/original deadline survive preserving cleanup exactly. Default cleanup, queued restart, actual EnterRunFailure and actor-destruction EndPlay must clear the timer. No global command line or live run-state/HP/hash/identity is assigned. This is a private lifecycle unit contract, not a real 30-second smoke measurement.

Source investigation found same-run ApplyGeneratedLayout previously cleared StartRun's watchdog through ClearRunObjects, and StartGameplaySmoke then re-armed it. The approved minimal lifecycle correction adds private `ClearRunObjects(bool bPreserveSmokeWatchdog=false)`, passes true only for smoke same-run apply, preserves the original timer there, and removes the later re-arm. Restart/failure/EndPlay use default clearing. The 30-second value and all F deadlines are unchanged.

# SeedForge 0.3.0 independent release audit — findings

## Summary

| Severity | Count |
|---|---:|
| Blocker | 0 |
| High | 3 |
| Medium | 5 |
| Low | 1 |
| Note | 6 |

**Release decision at audited head `18c255db750b8edf3576fe348b47fd0b6b8d312e`: do not merge PR #1 and do not publish `v0.3.0`.**

The decision is driven by confirmed lifecycle/input defects and release-evidence integrity failures, not by style, file length, API fashion, or scope preference.

---

## SF-IRA-001 — generation/apply failures have no terminal lifecycle boundary

**Severity:** High
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`
  - `StartRun`
  - `ApplyGeneratedLayout`
  - `HandleGenerationApplied`
  - `StartGameplaySmoke`
  - `FailGameplaySmoke`

**Observable consequence:**

A matching asynchronous generation failure is logged and then abandoned. `ActiveRequestId` becomes zero, but `RunState` remains `Generating`. Layout validation, encounter planning, actor spawn, or `StartPlaying` failures similarly return `false` without moving the run to an explicit failure state. The normal game has no Player pawn after `ClearRunObjects`, and the smoke watchdog has not yet been armed because `StartGameplaySmoke` is called only after a successful layout application.

For gameplay smoke, this produces a process that hangs until the outer PowerShell timeout kills it. It does not reliably write a failure trace and does not request its own non-zero exit. For an interactive run, it leaves a permanent `Generating` state with no playable pawn.

**Trigger / minimum reproduction:**

Launch the gameplay smoke with valid trace/capture/Git-SHA arguments and an invalid generation config, for example `-SeedForgeGridWidth=0`. The generator returns a typed failure. `HandleGenerationApplied` logs it and returns. No runtime watchdog is active, no trace is written, and the outer script eventually terminates the process for timeout.

The same defect can be reproduced in Automation by delivering a matching failed `FSeedForgeAsyncCompletion`, or by forcing one spawn/application boundary to fail.

**Why existing tests did not find it:**

- The run-state tests have no generation/apply failure transition.
- The transient World gameplay test covers successful application and lethal damage only.
- Gameplay-smoke tests exercise JSON serialization, not the runtime smoke driver.
- The successful smoke arms its watchdog only after the failure window has passed.

**Suggested fix:**

Add one coordinator-owned `EnterRunFailure`/`FailRun` boundary and route every startup/application failure through it. It must:

1. verify the completion still belongs to the active request/run;
2. clear request ownership, timers, actors, paths, and pending capture state;
3. transition to an explicit failure state or equivalent fail-closed snapshot that is not `Generating`;
4. preserve a concise typed failure code/message for HUD/logging;
5. in smoke mode, initialize trace identity early enough to write a failure trace and request exit status 2 immediately;
6. leave restart/new-seed control available through the persistent PlayerController.

Add a failing World/Automation test before implementation. Cover generator failure, apply failure, stale failure completion, trace write, non-zero smoke exit request abstraction, and recovery via R/N.

**Fix risk:** Medium. The risk is state/HUD/test surface, not gameplay expansion. Keep the change centralized; do not scatter ad-hoc exits among spawn branches.

**Must block merge:** Yes.
**Must block `v0.3.0` publication:** Yes.

**Status:** Confirmed; unresolved at audited head.

---

## SF-IRA-002 — package manifests can label dirty or changing source as the clean HEAD

**Severity:** High
**Location:**

- `Scripts/AuditRepository.ps1`
- `Scripts/PackageDemo.ps1`
- `Scripts/PackageGameplay.ps1`
- `Scripts/PackagePlugin.ps1`
- `Scripts/TestGameplay.ps1`
- `Scripts/VerifyPhase3.ps1`

**Observable consequence:**

The standalone package commands are documented as authoritative evidence commands, but they do not require a clean worktree before building. They build whatever is on disk and then write `git rev-parse HEAD` into the manifest. A package compiled from uncommitted C++/config changes can therefore be labeled as if it came from the clean commit.

`VerifyPhase3.ps1` checks cleanliness only at the beginning. It does not lock the starting revision through every gate, does not repeat the clean-tree audit at the end, and does not independently re-hash package archives and manifests before writing the aggregate summary. `PackagePlugin.ps1` also writes a literal three-target `verifiedTargets` array instead of deriving proof from the produced log/artifacts.

This is a release-evidence falsification path even when every UBT/UAT command itself succeeds.

**Trigger / minimum reproduction:**

1. Check out head `18c255d...`.
2. Modify a tracked runtime `.cpp` file without committing.
3. Run `Scripts/PackageGameplay.ps1` or `Scripts/PackagePlugin.ps1` directly.
4. UAT builds the dirty file, while the resulting manifest still records `18c255d...` as `sourceRevision`.

A second variant starts `VerifyPhase3.ps1` clean and edits a tracked file after the initial repository audit while a long build/test gate is running. The aggregate has no final invariant that rejects the changed tree.

**Why existing tests did not find it:**

There is no negative harness test that intentionally dirties a tracked file and proves every authoritative build/package entry point fails before UBT/UAT. There is also no final independent re-read/re-hash in the Phase 3 aggregate comparable to the older final-delivery audit.

**Suggested fix:**

Create one small shared PowerShell verification helper and use it from every authoritative script. It should:

1. fail on any non-zero native `git` exit;
2. resolve and validate one 40-character `ExpectedRevision`;
3. require no staged, unstaged, or untracked repository changes;
4. assert `HEAD == ExpectedRevision` immediately before and after every external process;
5. make package scripts write the captured expected revision, not a late unconstrained lookup;
6. run a final clean-tree/revision assertion after all gates;
7. independently re-hash every archive and compare it with its manifest/checksum;
8. hash the machine-readable manifests themselves for final reporting;
9. derive BuildPlugin target evidence from UAT output and/or expected packaged binaries rather than a literal array;
10. scan ordinary packaged and UAT logs with an explicit error/fatal policy and a reviewed warning allow-list.

Add a deterministic script-level negative test that alters a temporary tracked fixture or uses a temporary Git repository. Do not modify an unknown user file as part of the test.

**Fix risk:** Low to medium. Most changes are fail-closed checks; the main risk is rejecting existing scripts because of PowerShell/native-process edge cases. Test under both supported PowerShell versions or narrow the documented support.

**Must block merge:** Yes, because the PR and docs present these manifests as authoritative release proof.
**Must block `v0.3.0` publication:** Yes.

**Status:** Confirmed; unresolved at audited head.

---

## SF-IRA-003 — restart input ownership dies with the pawn, so rapid R/N is not actually available

**Severity:** High
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayActors.cpp`
  - `ASeedForgePlayerCharacter::SetupPlayerInputComponent`
  - `ASeedForgePlayerController::BeginPlay`
  - `ASeedForgePlayerController::PlayerTick`
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`
  - `StartRun`
  - `ClearRunObjects`
- `docs/PHASE3_CODE_WALKTHROUGH.md`
- `docs/PHASE3_ACCEPTANCE_MATRIX.md`

**Observable consequence:**

`RestartSameSeed` and `StartNewSeed` are bound only on `ASeedForgePlayerCharacter`. The first R/N request enters generation and immediately destroys that Character in `ClearRunObjects`. The persistent PlayerController binds no restart action. Until a new Character is spawned and possessed, further R/N presses have no input recipient.

Therefore the documented scenario “press N during generation” and the requested rapid R/N newest-request-wins behavior cannot be exercised by a normal user. It also removes the only advertised recovery control in the failure mode described by SF-IRA-001.

**Trigger / minimum reproduction:**

In the ordinary packaged executable, press N and immediately press R or N again while generation is in flight. After the first press the pawn/input component is destroyed; the second action cannot invoke `StartRun`.

**Why existing tests did not find it:**

- Tests call coordinator methods directly rather than dispatching the configured input action.
- No test observes controller/pawn possession during generation and after restart.
- No packaged interaction smoke drives R/N through the engine input stack.
- Same-seed/new-seed acceptance is currently supported by pure state tests and source reasoning, not a coordinator restart integration.

**Suggested fix:**

Move R/N bindings to `ASeedForgePlayerController::SetupInputComponent`, which persists when no pawn is possessed. Remove the duplicate Character bindings to prevent one key press from starting two runs. Resolve the coordinator through a weak cached reference or a deterministic World lookup, and clear it safely at teardown.

Add RED tests that prove:

- R/N can be dispatched while the controller has no pawn;
- a second request supersedes the first while generation is active;
- stale completion cannot populate the new run;
- the controller possesses exactly one fresh Character after successful apply;
- rapid restart leaves no old actors/timers/paths/cooldowns;
- input fires exactly once per key press.

Then include an engine-level packaged input smoke or a recorded manual interaction evidence step. Do not use OS keyboard/mouse macros.

**Fix risk:** Medium. Unreal input-stack ordering can cause duplicate dispatch if the old pawn bindings are not removed. The change should remain limited to restart ownership.

**Must block merge:** Yes.
**Must block `v0.3.0` publication:** Yes.

**Status:** Confirmed; unresolved at audited head.

---

## SF-IRA-004 — diagonal/current Dash direction is overwritten by the last axis callback

**Severity:** Medium
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeGameplayActors.h`
  - `LastMoveDirection` initialization
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayActors.cpp`
  - `MoveForward`
  - `MoveRight`
  - `Dash`

**Observable consequence:**

Movement combines both axis inputs through `AddMovementInput`, but each non-zero axis callback independently replaces `LastMoveDirection`. With W+D held, movement is diagonal while Dash uses whichever callback ran last, normally a cardinal direction. `LastMoveDirection` starts as Forward and is never reset to zero, so the apparent AimDirection fallback in `Dash` is unreachable before movement and after release.

**Trigger / minimum reproduction:**

- Hold W+D and press Space: movement and Dash directions disagree.
- Aim with the mouse before moving and press Space: Dash uses initial +X instead of the aim fallback.
- Release movement and press Space: Dash uses stale last-axis state.

**Why existing tests did not find it:**

No test drives both movement axes or validates the Dash launch vector. Existing gameplay defaults tests only inspect class/component/cooldown values.

**Suggested fix:**

Track current Forward and Right axis values, including zero updates, and resolve one normalized 2D vector at Dash time. Define the contract explicitly:

1. current combined movement vector when non-zero;
2. otherwise current AimDirection;
3. otherwise Forward as the final safe fallback.

Extract a pure direction helper and test diagonal, opposite-key cancellation, input release, zero aim, and aim-only cases before changing the Character.

**Fix risk:** Low.

**Must block merge:** No independently, but it is directly in the audit scope and should be fixed before the release candidate is re-certified.

**Status:** Confirmed; unresolved at audited head.

---

## SF-IRA-005 — screenshot evidence has no render-frame/completion ownership and accepts the known clipped frame

**Severity:** Medium
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`
  - `StartGameplaySmoke`
  - `AdvanceGameplaySmoke`
  - `RequestSmokeScreenshot`
  - `IsPendingSmokeScreenshotReady`
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgePreviewActor.cpp`
  - `CaptureScreenshot`
- `Scripts/TestGameplay.ps1`
- `Scripts/PackageDemo.ps1`
- `Scripts/CaptureInspector.ps1`
- `tasks/.../root-cause-20260903-packaged-combat-capture.md`

**Observable consequence:**

The runtime advances capture stages when a path reaches 10 KiB. It does not own a specific rendered frame or screenshot-completion callback. The scripts verify existence and size but not PNG signature/decodability, exact width/height, unique expected identity, capture timestamp, or relation to the current screenshot request. Fixed 1.0/0.5-second staging delays are also used around state changes.

The repository already records an observed packaged Combat image whose left HUD edge is clipped. The current harness nevertheless treats that frame as successful release evidence.

**Trigger / minimum reproduction:**

Run the existing packaged gameplay smoke with rapid sequential start/combat/win captures under offscreen rendering. The documented observed outcome is a clipped Combat HUD frame. A stale or wrong-resolution file above 10 KiB would also satisfy the present readiness/script checks.

**Why existing tests did not find it:**

Gameplay-smoke tests cover only JSON codec success/failure. No test binds the viewport-rendered or screenshot-captured delegate, records dimensions, or validates the PNG header. Manual visual inspection was documented, but the known defective frame was accepted as a limitation.

**Suggested fix:**

Use UE 5.8 render-owned signals instead of arbitrary sleeps:

1. after each state mutation, wait for an explicit number of `UGameViewportClient::OnViewportRendered` or end-draw events;
2. issue one screenshot request with a unique capture token/path;
3. complete the stage from the screenshot-captured/request-processed signal;
4. record width, height, request token, and completion time in the trace;
5. unregister all delegates in success, failure, restart, and `EndPlay`;
6. make PowerShell parse PNG signature/IHDR, require 1280×720 for gameplay captures, require a post-request modification time, and require exactly the three unique expected files;
7. visually inspect the newly generated images before closing the finding.

If the delegate path behaves differently between Editor and standalone, use a small adapter for both documented screenshot delegates or separate one capture phase per process. Do not add sleeps/retries/timeouts as the fix.

**Fix risk:** Medium. Screenshot delegates and file-writing behavior differ between Editor and standalone; focused Editor and packaged verification is mandatory.

**Must block merge:** No independently.
**Must block release evidence acceptance:** Yes until fresh complete frames pass.

**Status:** Confirmed evidence defect; unresolved at audited head.

---

## SF-IRA-006 — A* and Encounter Manhattan arithmetic can wrap at extreme coordinates

**Severity:** Medium
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGridPathfinder.cpp`
  - `ManhattanDistance`
  - `CurrentCell + Direction`
  - 32-bit search cost/heuristic fields
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeEncounter.cpp`
  - `ManhattanDistance`

**Observable consequence:**

The code subtracts and adds signed 32-bit coordinates before widening. At integer boundaries this can overflow/wrap. In A*, `INT_MAX + 1` can become `INT_MIN`, creating a false neighbor; Manhattan distance can similarly collapse a huge separation to a small number. Encounter safety-distance checks can therefore accept or reject the wrong relationship for public extreme-coordinate inputs.

Normal generated 48×48 layouts do not reach this boundary, so this is not classified as a runtime High/Blocker.

**Trigger / minimum reproduction:**

Create a path request with walkable cells `{(INT_MAX,0), (INT_MIN,0)}`, start at the first and goal at the second. The east-neighbor addition overflows; on the audited Win64/MSVC execution model this can manifest as a false one-step route to the opposite extreme. Add an Encounter validation case with extreme player/enemy coordinates and a recomputed valid layout/encounter hash.

**Why existing tests did not find it:**

Path tests cover stable routes, already-at-goal, invalid input, unreachable, and one budget case. They do not cover coordinate limits, duplicate/unsorted walkable input, or the requested goal-discovered/budget boundary. Encounter tests do not cover extreme-coordinate distance.

**Suggested fix:**

- Compute absolute coordinate deltas in `int64` without performing a 32-bit subtraction first.
- Use safe checked neighbor construction and skip directions that leave the `int32` domain.
- Store heuristic/cost in a sufficiently wide type or reject an unsupported span with typed `InvalidInput`.
- Add explicit tests for `MaxExpandedNodes=1`, adjacent goal, goal discovered at the budget boundary, duplicate/unsorted inputs, extreme coordinates, and stable results.
- Re-run all golden layout hashes; no generator/schema/hash behavior should change for normal coordinates.

**Fix risk:** Low for the existing generated domain.

**Must block merge:** No independently, but fix before final release verification under this audit scope.

**Status:** Confirmed; unresolved at audited head.

---

## SF-IRA-007 — run generation is logged as a subsystem request ID

**Severity:** Medium
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`
  - successful `ApplyGeneratedLayout` log
- `Scripts/PackageDemo.ps1`
  - applied-layout marker search

**Observable consequence:**

The coordinator emits `Applied request=%llu` but passes `RunGeneration`, not `FSeedForgeAsyncCompletion::RequestId`. The two counters may happen to align in the simplest demo flow, but they have different ownership and can diverge after other subsystem requests, direct application, cancellation, or host integration. Logs and evidence can attribute an applied layout to the wrong request.

**Trigger / minimum reproduction:**

Issue a WorldSubsystem request outside the gameplay coordinator, or otherwise make subsystem request count diverge from run count, then start a gameplay run. Compare the queued `request=` and successful applied `request=` fields.

**Why existing tests did not find it:**

No test asserts request/run identities in logs or a structured trace. `PackageDemo.ps1` only looks for a regex marker and does not validate the ID against the queued completion.

**Suggested fix:**

Pass the completion request ID into the successful application/log boundary, or rename the field to `run=` and add a separate true request field. Keep both monotonic domains explicit in logs/trace and update script parsing. Add a focused stale/current completion identity test.

**Fix risk:** Low.

**Must block merge:** No independently; it is release-evidence correctness and should be fixed in this audit.

**Status:** Confirmed; unresolved at audited head.

---

## SF-IRA-008 — current smoke/Automation does not prove several claims recorded as passed

**Severity:** Medium
**Location:**

- `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeGameplayTests.cpp`
- `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeGameplaySmokeTests.cpp`
- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`
- `Scripts/PackageDemo.ps1`
- `docs/PHASE3_ACCEPTANCE_MATRIX.md`
- `docs/PHASE3_EVIDENCE_JOURNAL.md`
- Draft PR #1 body

**Observable consequence:**

The real gameplay smoke does use a real World, Coordinator, Character, enemy/Core/Exit actors, production attack, proximity pickup, and exit transition. It does not directly mutate RunState/health/collected counts. Those are positive findings.

However, the existing evidence does not prove:

- an enemy actually receives a non-empty A* path and moves a finite distance;
- coordinator-level Same-Seed restart reproduces layout **and encounter** identity;
- New-Seed restart changes the run identity correctly;
- rapid restart clears old possession/actors/timers/paths/cooldowns;
- configured WASD/mouse/LMB/Space/R/N actions dispatch through the packaged input stack;
- ordinary packaged “interaction” works — the current ordinary path is an offscreen capture/auto-exit startup smoke.

A regression that makes enemy replanning a no-op, breaks one input binding, or damages restart integration could still leave the current 63-test result and win smoke green.

**Trigger / minimum reproduction:**

Temporarily make `ReplanEnemies` return early, remove one input binding, or change coordinator restart cleanup while preserving pure state/generator behavior. The existing codec tests and direct-method smoke path do not necessarily fail.

**Why existing tests did not find it:**

The five gameplay tests focus on pure mapping/attack selection/defaults, one successful World apply, actor counts, and lethal damage. The two gameplay-smoke tests cover JSON serialization. Same/new seed and input acceptance rely primarily on source review.

**Suggested fix:**

Add focused, production-path integration evidence:

1. tick a transient gameplay World long enough to observe at least one live enemy receive a path and move a finite, bounded distance without leaving the walkable route;
2. perform Same-Seed restart through the coordinator and assert equal layout/encounter identity plus fresh actor ownership;
3. perform New-Seed restart and assert the expected new seed and changed run identity;
4. cover `Playing -> Lost -> Restarting -> Generating -> Playing`;
5. drive configured keys through UE's input dispatch inside a packaged self-test or controlled engine-level input harness, not an OS macro;
6. extend the trace with restart/input/path evidence only where it remains deterministic and auditable;
7. update acceptance/PR wording to exactly match what the tests prove.

**Fix risk:** Medium. Avoid brittle wall-clock or pixel-position assertions; use bounded state/identity observations.

**Must block merge:** No independently.
**Must block the current “all acceptance passed” statement:** Yes.

**Status:** Confirmed coverage/evidence gap; unresolved at audited head.

---

## SF-IRA-009 — stale layout/encounter identity remains visible during a new Generating run

**Severity:** Low
**Location:**

- `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGameplayCoordinator.cpp`
  - `StartRun`
  - `ClearRunObjects`
  - `GetSnapshot`
  - failure exits from `ApplyGeneratedLayout`

**Observable consequence:**

`ClearRunObjects` destroys actors and clears timers/health maps but does not reset `Layout` or `EncounterPlan`. `StartRun` then changes `Seed`. During generation, the HUD/snapshot can show the new seed together with the previous run's layout and encounter hashes. A failed restart can leave those stale identities indefinitely.

**Trigger / minimum reproduction:**

Complete one run, start a new seed, and read `GetSnapshot` before the new completion or after an induced generation/apply failure.

**Why existing tests did not find it:**

No test samples the coordinator snapshot between restart request and successful application.

**Suggested fix:**

Separate pending/request identity from active/applied identity, or reset active Layout/Encounter values when leaving the old run. Snapshot/HUD should show zero/“pending” hashes until the new layout is successfully applied. Test the generating and failure snapshots.

**Fix risk:** Low.

**Must block merge:** No.

**Status:** Confirmed; unresolved at audited head.

---

## Notes and hypotheses closed without a bug finding

### SF-IRA-N01 — mixed Enhanced classes and Legacy mappings are intentional compatibility, not a rewrite mandate

UE 5.8 documentation describes Enhanced Input as providing backward compatibility with the default input system. The project uses `EnhancedPlayerInput`/`EnhancedInputComponent` classes but binds named `ActionMappings`/`AxisMappings`; it does not use Input Action/Mapping Context assets. `bEnabledLegacyMappingDeprecationWarnings=False` hides migration warnings, but the architecture and Known Limitations disclose this choice.

**Decision:** retain the compatibility approach for 0.3.0, describe it precisely as Legacy mappings on Enhanced-compatible classes, and test the actual packaged path. Do not introduce an asset/input-framework migration solely for API fashion.

### SF-IRA-N02 — attack candidate/actor indexing is aligned

The coordinator builds `Candidates` and `CandidateActors` in the same loop and applies identical validity filtering. The selected candidate index maps to the corresponding actor. Dead/destroying actors are excluded.

### SF-IRA-N03 — attack miss cooldown behavior matches the documented contract

Cooldown is consumed before target selection and a no-hit attack returns accepted. The architecture explicitly documents cooldown consumption on hit or miss. No defect was filed.

### SF-IRA-N04 — terminal interaction damage is fail-closed

Interaction/repath timers are cleared on Win/Loss, and `TickInteractions` begins with a `Playing` guard. Win returns before contact damage. Win/Loss cannot both be applied by the same interaction call.

### SF-IRA-N05 — normal-domain A* tie-breaking is not dependent on `TSet`/`TMap` iteration

Walkable membership and node lookup use hash containers, but decisions come from explicit open-index scans, F/H/Y/X ordering, and fixed East/South/West/North neighbors. Duplicate walkable inputs are deduplicated. The filed arithmetic issue is separate.

### SF-IRA-N06 — layout schema/generator golden identity was not modified by the PR

The PR does not change the generator implementation or existing layout schema/hash code. Encounter identity is separate. Full UE 5.8 golden/report re-execution is still required before closing the audit.

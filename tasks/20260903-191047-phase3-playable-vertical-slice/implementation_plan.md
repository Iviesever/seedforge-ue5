# SeedForge Phase 3 Implementation Plan

## Objective

Deliver SeedForge 0.3.0 as a deterministic extraction vertical slice: the existing version-1 layout remains byte/hash compatible, while a new pure C++ encounter identity and bounded grid A* drive a code-native top-down collect/fight/extract loop in Editor and a Win64 package. Final authority comes from fresh local Automation, BuildPlugin, BuildCookRun, ordinary packaged launch, packaged gameplay smoke, reparsed JSON traces, screenshots, and a clean pushed Draft PR revision.

## Approved approach

Extend the existing `SeedForgeRuntime` module and keep `SeedForgeDemo` as the narrow startup/GameMode layer. This preserves the established dependency direction, makes independent `BuildPlugin` validate both deterministic logic and gameplay integration, avoids a new module/asset boundary, and keeps the 0.2.0 generator/codec/diff/benchmark path unchanged.

Rejected alternatives:

1. A new `SeedForgeGameplay` plugin module would create a clean conceptual layer but adds manifest, target, and packaging seams without a P0 benefit.
2. Putting all gameplay in `SeedForgeDemo` would minimize plugin edits but prevent the plugin package from proving the vertical slice and encourage a second ownership path around the world subsystem.

## Immutable 0.2.0 boundary

- `v0.2.0` and its published release stay fixed at `9a306f8ff72cb660d3c04b09806787df21191d45`.
- Generator version 1, layout schema v1, canonical JSON bytes, Layout Diff, benchmark identity, and five golden layout hashes remain unchanged.
- `FSeedForgeGenerator`, `FSeedForgeLayoutCodec`, `FSeedForgeLayoutDiffer`, and `FSeedForgeBenchmarkRunner` are not redesigned.
- Existing async newest-request-wins, weak UObject application, and non-blocking shutdown semantics remain authoritative.
- Existing 41 tests may be extended but not removed, skipped, renamed to evade execution, or softened.
- Existing committed 0.2.0 docs/screenshots and ignored timestamped deliveries are not overwritten.

## Architecture and dependency direction

```text
explicit Seed + FSeedForgeConfig
        |
        v
FSeedForgeGenerator (unchanged layout v1/hash)
        |
        v
USeedForgeWorldSubsystem (unchanged async ownership contract)
        |
        v
ASeedForgeGameplayCoordinator
        | validates layout
        | plans one encounter with pure value APIs
        | owns one run generation and every spawned gameplay actor/timer
        |
        +--> ASeedForgePreviewActor (apply-only floor/wall HISM presentation)
        +--> ASeedForgePlayerCharacter <--> ASeedForgePlayerController
        +--> ASeedForgeEnemyPawn[] (bounded periodic A*)
        +--> ASeedForgeCorePickup[]
        +--> ASeedForgeExitActor
        `--> ASeedForgeHUD (read-only snapshot rendering)

FSeedForgeEncounterPlanner ----+
FSeedForgeGridPathfinder -------+--> pure C++ / value inputs / no World
FSeedForgeRunStateMachine ------+
```

`SeedForgeRuntime` depends only on its current Core/CoreUObject/Engine/Json set unless a compile-proven direct Engine module dependency is required. `SeedForgeDemo` continues to depend on `SeedForgeRuntime` and selects the gameplay framework classes. No production module depends on `SeedForgeTests` or `SeedForgeEditor`.

## PACT-30: Baseline, blueprint, and rollback boundary

**Objective:** Lock the real remote base, 0.2.0 invariants, design, priorities, verification matrix, and failure policy before production code.

**Acceptance:**

1. Fresh `git fetch --prune origin` succeeds and local base equals `origin/main`.
2. `v0.2.0` and the published release resolve to the base revision.
3. Worktree is clean before planning; unknown user changes are absent.
4. Repository audit, Editor Development build, and all 41 baseline Automation tests pass with zero warnings/failures.
5. `issue.md`, this plan, `progress.md`, and `pact-30-evidence.md` record exact evidence before production edits.
6. Branch is `feat/phase3-playable-vertical-slice`, created directly from the fetched `origin/main` without another clone or worktree.

**Rollback:** reset is never used against user work. PACT-30's plan commit is the branch rollback point; `main` and `v0.2.0` remain untouched.

## PACT-31: Pure deterministic gameplay model

### Encounter contract

Add equivalent public value types:

- `FSeedForgeEncounterConfig`: default 3 cores, 5 enemies, Manhattan player/enemy safety radius, encounter version, and bounded placement controls.
- `ESeedForgeEncounterErrorCode` and `FSeedForgeEncounterResult`: typed invalid layout/config, insufficient cells, impossible separation, and integrity failures.
- `FSeedForgeEncounterEntity`: role, stable numeric ID, and grid cell.
- `FSeedForgeEncounterPlan`: version, seed, source layout hash, player, exit, ordered cores/enemies, and independent canonical hash.
- `FSeedForgeEncounterPlanner`: validate inputs, choose deterministic cells, validate a plan, and compute the encounter hash.

The player uses the existing entrance and the extraction point uses the existing exit. Remaining canonical walkable cells are ranked by a local SplitMix64-derived score over `Seed + layout hash + role + cell`; canonical Y/X and stable ID break ties. Cores are selected first, then enemies subject to the configured Manhattan distance from the player. Every role is ordered by stable ID. Membership containers may accelerate lookup but their iteration never affects selection, output, or hashing.

Encounter hash uses a new byte marker and explicit encounter version, source layout hash, config fields, roles, stable IDs, and ordered points. It never calls or changes `FSeedForgeGenerator::ComputeCanonicalHash`.

### Pathfinder contract

Add:

- `ESeedForgePathStatus`: `Success`, `AlreadyAtGoal`, `Unreachable`, `InvalidInput`, and `BudgetExceeded`.
- `FSeedForgePathRequest`: start, goal, canonical walkable cells, and positive maximum expanded nodes.
- `FSeedForgePathResult`: status, ordered path, and expanded-node count.
- `FSeedForgeGridPathfinder::FindPath`.

A* uses four unit-cost neighbors in a test-locked order: East, South, West, North. Manhattan distance is the heuristic. The next node is chosen by `(F score, H score, Y, X)`; improved equal-cost parents retain the first fixed-neighbor discovery. `TSet`/`TMap` iteration is never used for choice. The path includes start and goal on `Success`; `AlreadyAtGoal` returns only the start. Invalid points/budget fail before search, expansion never exceeds the request budget, and unreachable/budget results contain no misleading partial success path.

### Run-state contract

Add:

- `ESeedForgeRunState`: `Generating`, `Playing`, `Won`, `Lost`, `Restarting`.
- typed run events/results and `FSeedForgeRunStateMachine`.

Legal transitions are explicit: generation success enters `Playing`; unique stable Core IDs advance collection and unlock the exit; an unlocked exit completes `Won`; player death completes `Lost`; `Won` or `Lost` can request `Restarting`; restart begins a new `Generating` generation. Invalid state/event combinations return a typed failure and do not mutate state. Collected Core IDs remain canonically ordered so duplicate pickups cannot increment progress.

### RED/GREEN tests

Write tests first for deterministic repetition, seed/config divergence, independent encounter hash, exact counts/order, overlap rejection, safety radius, canonical walkability, impossible config failures, tamper detection, all A* statuses, locked neighbor/tie behavior, expansion bounds, shortest-path validity, and every legal/illegal state transition including duplicate Core IDs. RED must fail against explicit stubs or absent behavior without weakening the harness.

After GREEN, run focused `SeedForge.Encounter`, `SeedForge.Path`, and `SeedForge.RunState` filters, then the complete `SeedForge` suite. Record commands, counts, warnings, failures, logs, report paths, and exact HEAD in `pact-31-evidence.md`, update `progress.md`, and commit PACT-31 separately.

## PACT-32: UE gameplay vertical slice

### World ownership

`ASeedForgeGameplayCoordinator` is the sole run owner. It parses normal seed/config arguments, binds once to `USeedForgeWorldSubsystem`, tracks a monotonic run generation, validates the newest applied layout, creates the encounter, applies the existing visualization, spawns gameplay actors in stable ID order, initializes the state machine, and exposes a read-only HUD snapshot.

Restart first moves to `Restarting`, invalidates timers/callbacks, cancels active generation, unbinds delegates, destroys owned gameplay actors, clears weak references/path state, updates the seed policy, rebinds, then returns to `Generating`. Every deferred callback captures or checks the active run generation. `EndPlay` performs the same cleanup without starting another run.

### Presentation and collision

`ASeedForgePreviewActor` gains an apply-only mode so the coordinator can reuse its HISM visualization without triggering a second generation request. Floors remain lightweight; exterior wall instances gain blocking collision for gameplay while preserving existing instance counts and capture behavior. Grid/world conversion is centralized with the existing 200-unit cell scale.

Runtime-created dynamic materials and Engine cube/sphere shapes establish stable colors for player, enemy, Core, locked exit, unlocked exit, damage, and attack pulse. No external asset, skeletal mesh, animation blueprint, or manual Editor step is required.

### Player and input

`ASeedForgePlayerCharacter` uses `ACharacter`, capsule collision, CharacterMovement, a code-created mesh, and a spring-arm top-down camera. `ASeedForgePlayerController` binds text-configured action/axis names from `DefaultInput.ini`: WASD movement, mouse cursor aim projected to the gameplay plane, primary attack, Space dash, R same-seed restart, and N new-seed run. If pointer projection is unavailable, aim retains the last valid movement/forward direction as a stable fallback.

Configuration-driven mappings are preferred over binary Enhanced Input assets because they are reviewable, reproducible, and cook without an asset-generation step. This is a scoped input choice, not a claim that legacy mappings are universally preferable.

### Combat and enemies

The coordinator centralizes player HP, enemy HP, cooldown checks, damage, death, and terminal run outcomes. A player attack is a visible, short-lived directional pulse with a fixed range/arc and cooldown. Candidate enemies are evaluated in stable ID order; valid hits pass through the same coordinator damage method used by Automation/smoke. Dash has a fixed duration/impulse and cooldown.

Each `ASeedForgeEnemyPawn` stores presentation and current path only. At a bounded timer frequency (default 2 Hz), the coordinator maps player/enemy positions to canonical cells, invokes `FSeedForgeGridPathfinder` with a fixed expansion budget, and supplies the returned path. Enemies move along path waypoints between replans; no per-frame search occurs. Unreachable or budget-exceeded paths clear movement safely and record a bounded diagnostic. Contact damage is rate-limited and applied by the coordinator.

### Pickups, exit, and HUD

Cores and the exit are actors with stable IDs/cells and distinct visuals. A bounded coordinator timer checks production proximity rules: collecting an uncollected Core calls the state machine once and destroys that pickup; collecting all Cores changes the exit visual and eligibility; reaching an unlocked exit calls the state machine and wins. HP zero loses. Won/Lost stop hostile timers and show restart controls.

`ASeedForgeHUD` uses `AHUD::DrawHUD`/Canvas, avoiding a UMG asset. It renders HP, Core progress, decimal seed, run state, controls, exit lock status, and terminal prompts from one coordinator snapshot.

### Verification

Add compile-time/Automation integration contracts for framework class selection, actor defaults, apply-only visualization, stable cell/world conversion, coordinator spawn/count invariants, combat cooldown/damage/death, pickup/unlock/win, loss, same-seed restart, new-seed change, cleanup, and stale run rejection. Run focused build/tests, Editor offscreen launch smoke, an ordinary interactive launch check with bounded scripted observation, full regression, and visual inspection. Record `pact-32-evidence.md`, update progress, and commit separately.

## PACT-33: Automated gameplay smoke and package proof

### Runtime smoke

`-SeedForgeGameplaySmoke` enables an unattended driver inside the real coordinator. Scripts pass `-SeedForgeSeed`, `-SeedForgeGitSha`, `-SeedForgeGameplayTrace`, and `-SeedForgeGameplayCaptureDir`. A watchdog enforces a hard timeout.

After the newest layout is applied, smoke validates the encounter hash, actor counts, stable IDs/cells, non-overlap, walkability, and initial `Playing` state. It then uses public production interaction boundaries: exercise at least one visible attack through the coordinator, move/teleport the real player through Core proximity collection, verify unlock, move through exit proximity, and verify `Won`. Separate Automation covers `Lost` and restart. Smoke never edits state-machine fields directly.

The JSON trace records schema/version, source Git SHA, UE version, seed, layout hash, encounter hash, actor counts, ordered state transitions/actions, result, failure code/message, and screenshot paths. JSON is reparsed by PowerShell. Screenshots are requested from the real gameplay viewport after spawn and after unlock/win; scripts require PNG existence and minimum byte size. Missing actors, invalid state/hash, screenshot/trace failure, timeout, or log error calls `RequestExitWithStatus` with non-zero status.

### Scripts and gates

Extend the existing script system with `TestGameplay.ps1`, `CaptureGameplay.ps1`, `PackageGameplay.ps1`, and `VerifyPhase3.ps1` (or exact equivalents that do not duplicate lower-level build logic). Scripts keep outputs under `Artifacts/Gameplay`, `Artifacts/Logs`, `Artifacts/Media`, `Artifacts/Package`, and `Artifacts/Reports`.

PACT-33 gates, sequentially:

1. Repository audit and Editor Development build.
2. Focused gameplay Automation, then complete SeedForge Automation.
3. Editor gameplay smoke and JSON/screenshot reparse.
4. Existing Inspector and portable-evidence regression where applicable.
5. `RunUAT BuildPlugin` for Editor Development, Game Development, and Game Shipping.
6. Win64 BuildCookRun Build/Cook/Stage/Pak/Archive.
7. Ordinary packaged EXE launch with bounded observation and clean scripted exit that does not require smoke mode.
8. Packaged gameplay smoke with exit 0, reparsed JSON, non-empty screenshots, and strict warning/error audit.

Record exact commands/results/artifact paths in `pact-33-evidence.md`, update progress, and commit separately.

## PACT-34: Portfolio, interview, and final handoff

Only after P0 gameplay and packaging are stable, update version metadata to 0.3.0 and align README, `docs/PHASE3_ARCHITECTURE.md`, `docs/GAMEPLAY_LOOP.md`, `docs/PHASE3_CODE_WALKTHROUGH.md`, `docs/PHASE3_INTERVIEW_GUIDE.md`, `docs/LIVE_CHANGE_DRILLS.md`, `docs/KNOWN_LIMITATIONS.md`, `docs/AI_ASSISTANCE.md`, acceptance matrix, evidence journal, candidate notes, rollback, and final handoff.

README above the fold will show:

```text
Seed + Config
  -> Deterministic Layout
  -> Deterministic Encounter Plan
  -> Grid A*
  -> UE Gameplay World
  -> Collect / Fight / Extract
  -> Automated Packaged Evidence
```

The interview guide covers the exact questions in the Goal. Live Change Drills contains at least ten graded exercises. AI disclosure states that GPT-5.6 Sol implemented/tested/debugged/packaged the work and the user supplied goals, constraints, orchestration, acceptance, and later learning; it does not imply independent user authorship.

Run the complete clean-revision `VerifyPhase3.ps1` after documentation is committed. If manifests embed the pre-verification commit, create only the minimum evidence-finalization commit and re-run the affected clean-revision gates. Audit tracked/generated files, require a clean tree, push the feature branch, and create or update a Draft PR whose body contains product changes, architecture, exact validated HEAD, test counts, package/smoke artifacts, screenshots, limitations, AI assistance, and deferrals.

## Verification matrix

| Risk | Focused proof | Final proof |
|---|---|---|
| Layout regression | Existing core/golden/document tests | Complete SeedForge suite; five hashes unchanged |
| Encounter determinism | Repetition/divergence/hash/tamper tests | Editor and packaged trace identities |
| A* correctness/cost | Status/tie/shortest/budget tests | Bounded enemy replan diagnostics in real World |
| State ownership | Legal/illegal/duplicate transition tests | Core unlock/win smoke plus loss/restart integration |
| UE lifecycle | Spawn/cleanup/stale-run Automation | Repeated Editor smoke and packaged run |
| Player loop | Input defaults/combat/pickup/exit tests | Ordinary package launch plus visible gameplay captures |
| Distribution | Target/module compile | BuildPlugin + BuildCookRun + ordinary EXE + packaged smoke |
| Evidence integrity | JSON unit/reparse checks | Trace/artifact existence, SHA/revision, strict log audit |

## Failure and rollback policy

- After two attempted fixes for the same failure, stop editing and write a root-cause packet containing symptom, minimal reproduction, excluded hypotheses, logs, ownership boundary, and why the next repair differs.
- Never enlarge sleeps/timeouts, retry indefinitely, silence broad warnings, bypass production gameplay APIs, or mutate expected hashes to manufacture green results.
- Each completed PACT is a separately verified commit and rollback point. Revert a PACT commit if integration risk cannot be bounded; never rewrite `main`, tags, or release history.
- Build/package outputs remain ignored and timestamped. Do not commit `Artifacts`, `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, `.cache`, `.user`, or packaged archives.
- If time compresses, remove P2 and visual P1 polish first. Preserve every P0 test/build/ordinary-launch/package/smoke gate and accurately list any deferral in the Draft PR.

## Time policy

- P0 implementation must be stable before the 2026-09-05 13:00 (UTC+8) feature freeze.
- After freeze: blocker fixes, tests, packaging, screenshots, documentation calibration, checksums, PR, and audit only.
- Internal hard stop: 2026-09-05 15:00 (UTC+8).
- External deadline: 2026-09-05 15:30 (UTC+8).

## Completion condition

Do not report success until every applicable item in the Goal's 21-point completion list is supported by fresh evidence at the exact pushed Draft PR HEAD. In particular, compile-only, Editor-only, pure-model-only, partial smoke, or PR-only states are milestones, not completion.

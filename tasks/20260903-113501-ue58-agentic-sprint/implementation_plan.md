# SeedForge UE 5.8 Implementation Plan

## Objective

Deliver a reviewable UE 5.8 C++ runtime plugin and generated graybox demo that proves deterministic dungeon generation, topology validation, asynchronous request safety, automated verification, and repeatable packaging before 2026-09-05 15:30 (UTC+8).

## Architecture

```text
Seed + FSeedForgeConfig
        |
        v
FSeedForgeGenerator (pure C++, no UObject or World access)
        |
        v
FSeedForgeLayout (canonical immutable-by-contract result)
        |                  |
        v                  v
FSeedForgeValidator    Canonical FNV-1a hash
        |
        v
USeedForgeWorldSubsystem
  worker generation -> game-thread generation-id check -> apply delegate
        |
        v
ASeedForgePreviewActor -> HISM instances using Engine basic cube
```

The demo game module owns bootstrapping and command-line configuration. The plugin owns generation, validation, async orchestration, and visualization. Tests compile in a separate developer module. No production behavior depends on Blueprint or hand-authored project assets.

## PACT-00: Repository and UE smoke baseline

**Objective:** Establish a minimal UE 5.8 C++ project and runtime plugin that build and launch unattended.

**Acceptance checks:**

1. `Build.bat SeedForgeEditor Win64 Development` exits zero and records the selected MSVC/SDK.
2. `UnrealEditor-Cmd.exe` loads the project/plugin with `-unattended -nop4 -nullrhi` and exits zero.
3. All logs and generated project state are directed under the project boundary where UE supports an override.

**Non-goals:** Generator behavior, project content, UI, packaging, or engine modification.

**Changes:** Initialize Git on `codex/seedforge-implementation`; add project/module/plugin manifests, minimal modules, `.gitignore`, and deterministic build/smoke scripts.

## PACT-01: Deterministic generation core

**Objective:** Generate a canonical single-floor layout from only an explicit seed and validated configuration.

**Acceptance checks:**

1. Tests are written first and observed failing because generation is not implemented.
2. Repeating the same input produces identical ordered rooms, cells, entrance, exit, and hash.
3. Different representative seeds produce valid layouts and at least two distinct hashes.
4. Invalid dimensions, room ranges, room counts, or attempt budgets return typed errors without partial output.
5. The core contains no UObject, World, global RNG, wall-clock, or unordered iteration dependency.

**Non-goals:** Multiple algorithms, multi-floor layouts, PCG, navigation, doors, enemies, loot, or cross-version hash guarantees.

**Algorithm:** A bounded deterministic RNG proposes axis-aligned rooms on an integer grid. Accepted rooms are canonically sorted. Consecutive sorted room centers are connected with bounded L-shaped corridors. Entrance is the first canonical room center; exit is the reachable room center with greatest Manhattan distance, with canonical tie-breaking. Walkable cells and all hash inputs are sorted before hashing.

## PACT-02: Topology validation and property verification

**Objective:** Prove layout invariants across fixed and broad deterministic seed sets.

**Acceptance checks:**

1. Validator tests are written first and observed rejecting constructed overlap, out-of-bounds, disconnected, or invalid entrance/exit layouts.
2. Five golden seeds have committed canonical hashes.
3. Synchronous generation repeated 100 times per golden seed remains identical.
4. At least 1,000 seeds pass bounds, room non-overlap, corridor/walkable legality, entrance/exit membership, and single-component connectivity.
5. Failures include seed, config, error code, and a one-command reproduction hint.

**Non-goals:** Absolute cross-machine performance thresholds or randomized non-reproducible fuzzing.

## PACT-03: Asynchronous lifecycle safety

**Objective:** Run pure generation on a worker while applying only the latest live request on the game thread.

**Acceptance checks:**

1. Tests are written first for pre-cancel, in-flight cancel, stale-result suppression, worker execution, and game-thread apply.
2. A newer request invalidates an older late result.
3. Deinitialization invalidates outstanding requests; callbacks use weak UObject references.
4. Cancelled/stale work never invokes the scene-application delegate.
5. Asynchronous generation repeated 100 times per golden seed matches synchronous output and hash.

**Non-goals:** Internal parallel subdivision, networking, persistence, replay, or runtime streaming.

## PACT-04: Pure C++ graybox demo

**Objective:** Turn a valid layout into an observable engine-native scene without hand-authored gameplay assets.

**Acceptance checks:**

1. A runtime preview actor creates floor and boundary instances with HISM using Engine basic cube.
2. Seed/config come from command-line arguments with safe defaults.
3. Logs report seed, hash, room count, walkable-cell count, generation time, and terminal state.
4. An automatically generated empty map launches the demo and provides a usable camera.
5. An automated run captures a deterministic-seed screenshot under `Artifacts/Media`.

**Non-goals:** Blueprint API, UMG, complex input assets, custom materials, procedural mesh, animation, or polished art.

## PACT-05: Repeatable build, test, and distribution

**Objective:** Produce reproducible verification evidence, a distributable plugin, and a Win64 demo candidate.

**Acceptance checks:**

1. One script performs Build -> Automation -> BuildPlugin -> demo smoke and writes timestamped logs/reports.
2. `RunUAT BuildPlugin` exits zero and a plugin ZIP plus SHA-256 is created.
3. Win64 BuildCookRun exits zero and a packaged demo launches, or the final report contains complete evidence of an uncontrollable packaging blocker while preserving the verified Editor/plugin deliverables.
4. A last-known-good Git revision and artifact are retained before any release-candidate replacement.

**Non-goals:** Installer, prerequisites bundle, Marketplace submission, engine redistribution, or additional platforms.

## PACT-06: Portfolio and handoff package

**Objective:** Make the result reviewable, reproducible, and honest about AI-assisted authorship.

**Acceptance checks:**

1. README contains a 60-second quick start, screenshot, architecture, deterministic boundary, test summary, and limitations.
2. `docs/ACCEPTANCE_MATRIX.md` maps every goal requirement to fresh evidence.
3. `docs/AI_ASSISTANCE.md` accurately records Codex involvement.
4. `docs/CODE_WALKTHROUGH.md` and `docs/INTERVIEW_GUIDE.md` explain ownership, algorithms, UE lifecycle, tradeoffs, and likely interview questions.
5. `Artifacts/Release` contains source revision, plugin/demo archives, checksums, reports, and rollback notes.

## Execution order and gates

1. PACT-00 must pass before any behavior implementation.
2. PACT-01 follows strict red-green-refactor.
3. PACT-02 must be green before async work begins.
4. PACT-03 must be green before scene application is connected.
5. PACT-04 produces the first visible end-to-end slice.
6. PACT-05 creates RC1; after RC1, only verified fixes and PACT-06 work may enter unless ample time remains.
7. In the final four hours, freeze all features and perform only verification, packaging, rollback checks, and handoff.

## Failure policy

- After two failed attempts on the same issue, stop editing and produce a root-cause packet with two bounded alternatives.
- Only one UBT, UAT, UnrealEditor, Cook, or Package writer may run at once.
- Preserve the last verified build and never overwrite the only good release candidate.
- If automated map generation is blocked, deliver an Editor-loaded engine entry map demo and document the packaging limitation rather than introducing hand-authored assets.
- If Win64 packaging is blocked by unavailable prerequisites outside project control, retain the verified Editor target and packaged plugin and continue all other required evidence work.

## Planned evidence locations

- `Artifacts/Logs/`
- `Artifacts/Reports/`
- `Artifacts/Media/`
- `Artifacts/Plugin/`
- `Artifacts/Package/`
- `Artifacts/Release/`
- `docs/ACCEPTANCE_MATRIX.md`


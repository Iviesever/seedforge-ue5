# SeedForge Phase 3: Deterministic Extraction Vertical Slice

## User intent

Advance the existing verified SeedForge 0.2.0 engineering experiment into a short, genuinely playable UE 5.8 Win64 extraction loop before 2026-09-05 15:30 (UTC+8). Codex GPT-5.6 Sol owns implementation, testing, debugging, packaging, and documentation under the user's product constraints; the user owns scope, orchestration, acceptance, and later understanding practice.

## Verified starting point

- Project boundary: `D:\program\SeedForge`; the UE 5.8 installation is read-only.
- Freshly fetched `origin/main`: `9a306f8ff72cb660d3c04b09806787df21191d45`.
- Local `main` is clean and exactly matches `origin/main`.
- Immutable `v0.2.0` tag resolves to that same revision; the published 0.2.0 release exists and must not change.
- Baseline repository audit passed: 99 tracked files and 43 source/build-rule files.
- Baseline Editor Development build passed.
- Baseline complete Automation passed 41 of 41 tests with zero warnings and zero failures.
- Baseline evidence: `Artifacts/Logs/build-editor-20260903-191006.log`, `Artifacts/Logs/automation-20260903-191012.log`, and `Artifacts/Reports/automation-20260903-191012`.

## Product problem

SeedForge 0.2.0 proves deterministic layout generation, safe asynchronous application, portable evidence, and an Editor inspector, but its packaged demo is only a free-fly visualization. A reviewer cannot play a complete collect/fight/extract loop or see deterministic encounter planning and bounded grid pathfinding exercised in a real World.

## Required P0 outcome

Given an explicit `uint64 Seed + Config`, preserve the existing canonical layout and produce a separately versioned, canonically hashed encounter plan containing one player spawn, one exit, three Data Cores, and five enemies on distinct reachable walkable cells with a minimum player/enemy separation. In a code-native top-down World, the player can move, aim, use one visible cooldown-bound attack, dash, take damage, die, restart the same seed, start a new seed, collect all cores, unlock the exit, and win. Enemies pursue through SeedForge's deterministic bounded four-neighbor A* at a controlled replanning frequency.

The packaged Win64 executable must support normal interaction plus unattended `-SeedForgeGameplaySmoke -SeedForgeSeed=<uint64>`. Smoke must exercise production gameplay boundaries, emit reparsable JSON trace evidence, create non-empty gameplay screenshots, and return non-zero on missing actors, illegal state, log errors, or timeout.

## Architecture constraints

- Extend the existing Runtime plugin, demo module, world subsystem, canonical layout, and visualization path; do not create a parallel generator or gameplay framework.
- Keep encounter planning, A*, and the run state machine as pure value-oriented C++ with typed failures and tests independent of `UWorld`/`UObject`.
- Preserve generator version 1, layout schema/hash behavior, all five golden hashes, newest-request-wins, and shutdown safety.
- Gameplay actors consume the applied canonical layout and encounter plan; they do not maintain a second topology truth.
- One coordinator owns run state, spawned actors, timers, callbacks, and restart cleanup.
- Use engine basic geometry and runtime-created materials. No hand-authored Blueprint logic or required Editor setup.
- Only one UBT/UAT/Editor/Cook/Package process may run at a time.

## Delivery contracts

- PACT-30: lock the blueprint, immutable boundaries, priorities, verification matrix, failure policy, and freeze point.
- PACT-31: RED/GREEN pure C++ encounter planner, grid A*, and run state machine; focused and full regression; independent commit.
- PACT-32: RED/GREEN UE gameplay integration and normal interactive loop; compile, Automation, Editor launch, interaction evidence, full regression; independent commit.
- PACT-33: production-path automated gameplay smoke, JSON trace, screenshots, BuildPlugin, Win64 BuildCookRun, ordinary packaged launch, packaged smoke, audit; independent commit.
- PACT-34: portfolio/interview documentation aligned to the validated implementation; independent commit and final clean-revision verification.

## Strict non-goals

No new repository, clone, worktree, second layout generator, multi-floor generation, GAS, multiplayer/replication/rollback, Behavior Tree, EQS, NavMesh dual path, Mass Entity, PCG Framework, SaveGame, replay, inventory/equipment/skill tree, boss, quest system, Marketplace art, skeletal character pipeline, substantial audio, large Editor-tool rewrite, destructive layout-schema change, or validation weakening.

## Delivery and publication boundary

Work will use `feat/phase3-playable-vertical-slice` from the fetched `origin/main`. Each PACT receives a compact acceptance contract, RED/GREEN evidence where applicable, focused/full verification, an evidence journal, progress update, and its own clear commit. The final branch may be pushed and a Draft PR created or updated. Do not merge to `main`, move tags, create a formal `v0.3.0` release, or commit generated/large binary artifacts.

## Time policy

Feature freeze is 2026-09-05 13:00 (UTC+8); the internal hard stop is 15:00. Drop P2 first and visual P1 polish second. Never drop P0 core tests, ordinary packaged startup, Win64 packaging, or packaged gameplay smoke, and never describe incomplete work as complete.

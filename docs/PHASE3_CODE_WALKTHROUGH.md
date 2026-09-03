# Phase 3 code walkthrough

## Recommended reading order

1. `SeedForgeTypes.h` and `SeedForgeGenerator.cpp` — preserved layout input/output and hash boundary.
2. `SeedForgeEncounter.h/.cpp` — pure encounter config/entities/result, stable selection, typed failures, independent hash.
3. `SeedForgeGridPathfinder.h/.cpp` — pure request/result and deterministic bounded A*.
4. `SeedForgeRunState.h/.cpp` — explicit state transitions, Core identity, exit lock, restart.
5. `SeedForgeGameplayTypes.h/.cpp` — tuning, HUD snapshot, cell/world mapping, attack target selection.
6. `SeedForgeAsync.cpp` and `SeedForgeWorldSubsystem.cpp` — preserved worker/newest-request/weak UObject boundary.
7. `SeedForgeGameplayCoordinator.h/.cpp` — run ownership, spawn/cleanup, combat, pickup/exit, replan timers, smoke.
8. `SeedForgeGameplayActors.h/.cpp` — Character, Controller, Enemy, Core, Exit, and Canvas HUD.
9. `SeedForgePreviewActor.cpp` — apply-only HISM mode and blocking generated geometry.
10. `SeedForgeDemoGameMode.cpp` and `DefaultInput.ini` — code-native startup and text input mappings.
11. `SeedForgeEncounterTests.cpp`, `SeedForgeGridPathfinderTests.cpp`, `SeedForgeRunStateTests.cpp`, `SeedForgeGameplayTests.cpp`, and `SeedForgeGameplaySmokeTests.cpp` — executable contracts.
12. `TestGameplay.ps1`, `PackageGameplay.ps1`, and `VerifyPhase3.ps1` — real process and distribution evidence.

## Encounter planner

Start at `ValidateInputs`: config bounds, intact source layout hash, canonical endpoint membership, endpoint separation, and total cell capacity fail before selection. `RankCandidates` computes one deterministic score per canonical cell. Notice that `TSet` only tests occupancy; output order comes from explicit sorting and stable role loops.

Follow `ComputeCanonicalHash` byte by byte. Its marker, version, source layout hash, config, roles, IDs, and points are separate from `FSeedForgeGenerator::ComputeCanonicalHash`. This is how Phase 3 adds identity without invalidating 0.2.0 evidence.

## A* pathfinder

The open set stores node indices. Each expansion performs a visible comparator scan. Trace these keys:

```text
F = cost from start + Manhattan heuristic
then H
then cell Y
then cell X
```

The direction array locks East/South/West/North. The walkable `TSet` and point-to-node `TMap` are never iterated for a decision. Watch the budget check happen before another expansion and the result path get reconstructed through parent indices.

## State machine

Read each method as a transition contract. Every method checks the current state before mutation. `StartPlaying` sorts and rejects duplicate expected IDs. `CollectCore` rejects unknown and duplicate events. `ReachExit` checks the derived unlock predicate. `RequestRestart` deliberately supports active and terminal runs, while `BeginGenerating` accepts only `Restarting`.

## Coordinator request flow

1. `BeginPlay` parses text command arguments and binds once to the WorldSubsystem.
2. `StartRun` advances ownership, cancels old generation, clears actors/timers, resets HP/cooldowns, and requests a layout.
3. `HandleGenerationApplied` rejects any non-active request ID.
4. `ApplyGeneratedLayout` validates, plans, creates apply-only HISM visualization, resolves/spawns the Character, spawns roles in stable order, starts the state machine, and arms bounded timers.
5. `ClearRunObjects` is the restart/teardown choke point.

The Gameplay Actors do not copy layout truth. Their stable cells come from the encounter plan; enemies receive only path waypoints; HUD reads one snapshot.

## Combat and interaction

`FSeedForgeGameplayMath::SelectAttackTarget` filters live candidates by two-dimensional range/arc and selects the lowest stable ID. `TryPlayerAttack` owns the cooldown and enemy HP. `ApplyPlayerDamage` owns Player HP and invokes the state machine on death.

`TickInteractions` is shared by the normal timer and smoke driver. It invokes `CollectCore` before destroying a pickup, mirrors only the state machine's unlock value into exit presentation, and invokes `ReachExit` on proximity. `ReplanEnemies` alone invokes A*; enemy Tick never searches.

## Smoke trace

Read `StartGameplaySmoke` for identity/count validation, then the `EGameplaySmokeStage` switch. The driver teleports only to shorten verification; it still uses the real Character, attack cooldown/damage, Core actors, proximity path, exit actor, and state machine. `FSeedForgeGameplaySmokeCodec` emits fixed-order JSON with exact unsigned strings. PowerShell treats the runtime trace as untrusted output and reparses/counts/checks every artifact.

## Debugger exercises

- Break on encounter candidate sorting for seed 24301 and inspect stable role IDs.
- Change a path budget from 1024 to 1 and observe `BudgetExceeded` without a partial success path.
- Trigger `CollectCore(0)` twice and confirm the second event leaves state unchanged.
- Press N during generation and watch the old subsystem result fail the active request-ID gate.
- Break in `ClearRunObjects` during R restart and inspect timer/actor/path cleanup.
- Run smoke and break on the transition from `CollectCores` to `ReachExit`.

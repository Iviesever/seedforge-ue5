# Phase 3 architecture

## Purpose

Phase 3 turns the preserved SeedForge 0.2.0 layout/evidence experiment into one short playable extraction run without creating a second map truth or weakening the asynchronous lifetime boundary.

## Dependency direction

```text
explicit uint64 Seed + FSeedForgeConfig
  -> FSeedForgeGenerator
       output: FSeedForgeLayout + layout v1 hash
  -> USeedForgeWorldSubsystem
       worker generation -> newest live request -> Game Thread
  -> ASeedForgeGameplayCoordinator
       validate layout
       -> FSeedForgeEncounterPlanner
            output: FSeedForgeEncounterPlan + encounter v1 hash
       -> ASeedForgePreviewActor (HISM floors/walls/collision)
       -> Player / Enemy / Core / Exit / HUD
       -> FSeedForgeGridPathfinder for bounded enemy replans
       -> FSeedForgeRunStateMachine for every run transition
       -> gameplay smoke trace/screenshots/exit status
```

All Phase 3 reusable logic and actors live in the existing `SeedForgeRuntime` plugin module. `SeedForgeDemo` remains a narrow GameMode/bootstrap module. Tests are Editor-only. The Editor Inspector and report Commandlet continue consuming Runtime APIs and never own gameplay state.

## Deterministic encounter model

`FSeedForgeEncounterPlanner` accepts only an intact `FSeedForgeLayout` and explicit `FSeedForgeEncounterConfig`. It does not read time, `UWorld`, `UObject`, global randomness, input, or unordered-container iteration.

- Player cell is the existing layout entrance.
- Exit cell is the existing layout exit.
- Core and enemy candidates come from canonical walkable cells excluding occupied endpoints.
- A SplitMix64-derived score combines seed, source layout hash, role salt, and cell coordinates.
- Descending score selects cells; canonical Y/X order breaks the theoretical score tie.
- Cores are placed first. Enemies then skip occupied cells and cells inside the player safety radius.
- Stable IDs are contiguous role-local indices in selection order.
- Impossible counts, corrupt layout identity, insufficient cells, or impossible enemy separation return typed failures.

The encounter hash has its own `SFE1` byte marker and version. It hashes the source layout identity, encounter config, roles, IDs, and ordered cells. It never changes the generator's `SFG1` layout hash or schema v1 JSON.

## Deterministic grid A*

`FSeedForgeGridPathfinder` is a pure value API. A request contains start, goal, walkable cells, and a positive expansion budget.

- Movement: four neighbors, unit cost.
- Neighbor order: East, South, West, North.
- Heuristic: Manhattan distance.
- Open-set selection: lowest `(F, H, Y, X)`.
- Equal-cost parents retain first discovery; unordered map/set iteration never selects output.
- Success path includes both start and goal.
- `AlreadyAtGoal` returns the start only.
- Invalid endpoints/budget, unreachable space, and budget exhaustion have distinct statuses.
- Expansion count never exceeds the request budget.

The current implementation uses a linear scan of the open array. For the scoped map and five enemies this is deliberately simpler and easier to audit than a mutable binary heap. Worst-case selection is `O(V^2)` with `O(V)` storage. A production-scale map would use an indexed heap while retaining the same tie-break contract.

## Run-state ownership

`FSeedForgeRunStateMachine` is the only source of run terminal/collection truth:

```text
Generating -> Playing -> Won
                     `-> Lost
Playing / Won / Lost -> Restarting -> Generating
```

Expected Core IDs are sorted and validated once. Unknown or duplicate Core events fail without mutation. The exit remains locked until every expected ID is collected. Invalid state/event combinations return `InvalidTransition`; no actor maintains a competing `bWon` or `bLost` flag.

## World ownership

`ASeedForgeGameplayCoordinator` owns one run generation:

- active generation request ID and seed/config;
- applied layout and encounter plan;
- player HP, attack/contact cooldowns, and enemy HP;
- visualization, player, Core, enemy, and exit actor references;
- interaction, replan, capture, smoke, and watchdog timers;
- state machine and read-only HUD snapshot.

Restart cancels generation, clears every owned timer, destroys all owned run actors, clears health/path arrays, resets the state machine through `Restarting -> Generating`, then issues a new request. Delegate removal and subsystem cancellation occur in `EndPlay`. The subsystem still rejects stale worker results by request ID; the coordinator also rejects a completion whose request ID is no longer active.

## Gameplay framework

- `ASeedForgePlayerCharacter`: capsule/CharacterMovement, code-created mesh, absolute-rotation spring-arm camera, movement/attack/dash/restart bindings, pointer aim with forward fallback, and attack pulse.
- `ASeedForgePlayerController`: visible cursor and mouse ray-to-gameplay-plane aim.
- `ASeedForgeEnemyPawn`: stable ID/spawn cell plus current world path; Tick only moves toward supplied waypoints.
- `ASeedForgeCorePickup`: stable ID/cell and visual presentation.
- `ASeedForgeExitActor`: cell and locked/unlocked presentation. Eligibility still comes from the state machine.
- `ASeedForgeHUD`: Canvas text from a coordinator snapshot; no UMG/Blueprint asset.
- `ASeedForgeDemoGameMode`: selects native Controller/HUD and spawns the coordinator plus lighting.

Input mappings are reviewable text in `DefaultInput.ini`. The project retains UE's Enhanced PlayerInput/InputComponent defaults while intentionally using named configuration mappings, avoiding fragile binary input assets.

## Combat and pursuit

The coordinator evaluates attack candidates in stable ID order after range/arc filtering. A successful input consumes one cooldown whether it hits or misses; a hit reduces centralized enemy HP and destroys the actor at zero. Player damage is rate-limited for enemy contact and transitions through `PlayerDied` at zero HP. Dash uses a fixed impulse and cooldown.

Enemy search never runs in Tick. A 2 Hz coordinator timer maps world positions to nearest canonical walkable cells and invokes bounded A*. The Pawn receives waypoints and moves each frame. Failure clears the path safely and emits only a bounded verbose diagnostic.

## Smoke architecture

`-SeedForgeGameplaySmoke` is a production-World driver, not a mock:

1. Wait for the newest layout/encounter to apply.
2. Validate actor counts, stable IDs/cells, plan identity, and `Playing`.
3. Call the real attack/cooldown/damage path twice and verify one enemy death.
4. Teleport the real Character to each real Core, then call the same proximity evaluation used by normal timers.
5. Verify 3/3 and the real exit's unlocked state.
6. Teleport to the exit and call the same proximity rule; require `Won`.
7. Capture start/combat/win PNGs, write fixed-order JSON, and exit zero.

Any missing actor, identity/state mismatch, failed public gameplay boundary, screenshot/trace failure, or watchdog calls `RequestExitWithStatus` non-zero. Scripts independently reparse JSON, validate paths and file sizes, and apply a narrow warning allow-list.

## Determinism boundary

Guaranteed for the same algorithm/encounter version, Seed + Config, Win64, UE 5.8, and this implementation:

- canonical layout topology and layout hash;
- initial player/exit/Core/enemy stable IDs and cells;
- encounter hash;
- A* result/status/expansion count for the same request;
- state-machine transition result for the same event sequence;
- fixed-order smoke JSON fields/actions.

Not guaranteed: the complete real-time run, input timing, enemy floating-point world positions between replans, physics/collision resolution, elapsed times, screenshot pixels, GPU output, or future versions/platforms.

## Preservation boundary

Generator version 1, schema v1, the five layout golden hashes, codec/diff/benchmark behavior, newest-request-wins, weak subsystem lifetime, `v0.2.0`, and the published 0.2.0 Release remain unchanged.

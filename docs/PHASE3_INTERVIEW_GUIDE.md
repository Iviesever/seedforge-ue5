# Phase 3 interview guide

## What is deterministic?

### How is map determinism different from full-run determinism?

Map determinism means explicit Seed + generation config produces the same canonical topology and layout hash. Encounter determinism means that layout plus encounter config produces the same initial role IDs/cells and encounter hash. A real run also depends on input timing, frame rate, timer scheduling, collision, and floating-point movement, so SeedForge does not claim the whole playthrough is deterministic.

### Why is the Encounter Planner pure C++?

It makes every variable input explicit, removes World/time/global-random dependencies, permits fast value tests, and lets the same plan drive Editor and packaged Worlds. UObject actors become presentation/interaction consumers instead of a second source of spawn truth.

### Why does encounter identity have a separate version/hash?

Phase 3 must preserve generator version 1 and all 0.2.0 layout hashes. A new marker/version lets encounter rules evolve without relabeling old layout evidence or confusing geometry identity with gameplay placement identity.

## A*

### Explain the open set, heuristic, tie-break, and complexity.

The open set contains discovered nodes that are not selected for expansion. Manhattan distance is admissible for four-direction unit-cost movement. Selection minimizes F, then H, then canonical Y/X. Neighbors are East/South/West/North. The simple array implementation scans the open set, so worst-case time is `O(V^2)` and storage `O(V)`; the scoped map makes auditability more valuable than a heap.

### Why not search every frame?

Search cost would scale with frame rate and multiply across enemies, obscuring performance and creating unbounded work. SeedForge replans at 2 Hz with a fixed maximum expanded-node budget, while Pawns interpolate along existing waypoints every frame.

### What happens when a path is impossible?

The pure result is `Unreachable`, `InvalidInput`, or `BudgetExceeded`, with no misleading partial success path. The enemy clears its current path and safely waits for the next bounded replan.

## Scope choices

### Why not NavMesh, Behavior Tree, EQS, or GAS?

The portfolio goal is to expose deterministic placement, A* mechanics, bounded scheduling, state ownership, and package proof. Those frameworks would add parallel navigation/state systems and asset/editor setup without improving P0. They are valid production tools, just outside this slice.

### Who owns what?

- Actor: world representation and narrow local movement/visual state.
- PlayerController: cursor/view input and possession.
- GameMode: selects native framework classes and starts the coordinator/lights.
- WorldSubsystem: asynchronous layout request/cancel/apply boundary.
- GameplayCoordinator: the current run, HP/combat, spawned actors, timers, paths, smoke, and teardown.
- Pure models: encounter identity, A* result, and legal run transitions.

### How are stale callbacks prevented after a new run?

The async coordinator cancels the previous token and compares a monotonic request ID on the Game Thread. The Gameplay Coordinator stores its active request ID, rejects any other completion, and clears timers/actors/path state before issuing a restart. `EndPlay` removes the delegate and cancels generation.

### Why use typed failures?

`false` cannot distinguish invalid config, corrupt layout identity, insufficient cells, unreachable search, or budget exhaustion. Typed results make tests precise, logs actionable, and callers fail closed instead of silently producing illegal gameplay state.

## Verification

### How are Win and Loss tested?

Pure state tests cover legal/illegal transitions, locked exit, duplicate/unknown Cores, loss, and restart. A real transient World test spawns the coordinator and proves actor counts plus lethal damage to `Lost`. Editor and packaged smoke use production attack/pickup/exit rules to prove `Playing -> Won`.

### Why does Editor success not replace packaged EXE verification?

Editor builds can hide target eligibility, transitive includes, cooking, staged config/assets, startup class selection, and runtime dependencies. BuildPlugin verifies standalone Editor/Game/Shipping targets; BuildCookRun and two packaged launches verify the cooked map, config, modules, rendering, trace, screenshots, and process status.

### Does smoke cheat by setting the state directly?

No. It may teleport the real Character to shorten the run, which the acceptance contract permits. It still calls the same attack cooldown/damage method, collects real Core actors through the same proximity/state path, verifies the real exit unlock, and invokes the same exit proximity rule. It never assigns `Won` or collected counts directly.

## Authorship

### What did AI do?

Codex GPT-5.6 Sol chose implementation details within the user's constraints, wrote C++/tests/scripts/docs, ran UE 5.8 builds and processes, debugged failures, visually inspected images, packaged Win64 artifacts, and created verification evidence.

### What may the user honestly claim?

The user can claim they scoped and orchestrated an AI-assisted UE C++ project, set acceptance/risk constraints, reviewed evidence, and can reproduce/explain it after study. They must not claim they independently hand-wrote the implementation. After completing a personally authored test-first drill, they may accurately describe that specific contribution.

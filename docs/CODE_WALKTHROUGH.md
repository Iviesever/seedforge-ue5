# Code walkthrough

## Recommended reading order

1. `SeedForgeTypes.h` — public value types, config, errors, result contracts.
2. `SeedForgeGenerator.cpp` — RNG, bounded placement, canonical ordering, corridors, entrance/exit, hashing.
3. `SeedForgeValidator.cpp` — ordered structural checks and BFS connectivity.
4. `SeedForgeAsync.h/.cpp` — cancellation token, request generations, worker/apply boundary.
5. `SeedForgeWorldSubsystem.h/.cpp` — UObject lifetime adapter and broadcast.
6. `SeedForgeVisualization.cpp` — canonical cells to floor/exterior-wall transforms.
7. `SeedForgePreviewActor.cpp` — command-line input, async request, validation, HISM application, screenshot exit.
8. `SeedForgeDemoGameMode.cpp` — runtime-created pawn/camera/lights.
9. `SeedForge*Tests.cpp` — executable examples of every contract.
10. `Scripts/*.ps1` — reproducible build and distribution gates.

## End-to-end request

1. `ASeedForgePreviewActor::BeginPlay` reads explicit arguments and calls `USeedForgeWorldSubsystem::RequestGeneration`.
2. The subsystem constructs a value-capturing work closure and a weak-UObject apply closure.
3. `FSeedForgeAsyncCoordinator::Start` cancels the old token, assigns an id, and launches work with `UE::Tasks`.
4. The pure generator either returns a typed configuration/placement failure or a canonical layout.
5. The coordinator schedules Game Thread apply and checks lifetime, cancellation, and latest id.
6. The subsystem broadcasts; the actor validates before building transforms.
7. HISM components receive deterministic floor/wall instances.
8. Capture mode writes a screenshot and exits; interactive mode retains the free-fly pawn.

## Details worth tracing in a debugger

- Watch `FDeterministicRandom::State` advance four times per room proposal.
- Compare proposal order with the sorted `Layout.Rooms` order.
- Observe corridor cells exclude cells already contained inside rooms.
- Break on `FSeedForgeAsyncCoordinator::CancelActive` and inspect the shared token captured by a worker.
- Start two requests and observe the first Game Thread callback fail the active-id gate.
- Destroy/deinitialize the subsystem and observe `TWeakObjectPtr` plus shared-state gates suppress apply.

## Safe first personal modifications

- Add a test and configurable minimum Manhattan distance between entrance and exit.
- Replace corridor `AddUnique` with a fixed-size occupancy bitmap while preserving golden hashes or intentionally versioning them.
- Add a third HISM marker type for entrance/exit using an engine-native shape.
- Export the canonical layout to JSON with a schema version and round-trip test.

Each modification should begin with a failing Automation test and end with the complete suite plus packaged smoke.

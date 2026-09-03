# Code walkthrough

This document preserves the 0.2.0 evidence-system reading path. Continue with `PHASE3_CODE_WALKTHROUGH.md` for encounter planning, grid A*, run state, Gameplay Framework, and packaged smoke.

## Recommended reading order

1. `SeedForgeTypes.h` — configuration, layout, typed generation and validation results.
2. `SeedForgeGenerator.cpp` — SplitMix64, bounded placement, canonical sort, corridors, endpoints, FNV hash.
3. `SeedForgeValidator.cpp` — deterministic failure order and BFS connectivity.
4. `SeedForgeLayoutCodec.h/.cpp` — document contract, compact writer, strict parser, normalization, integrity checks.
5. `SeedForgeLayoutDiff.h/.cpp` — sorted merge differences and summaries.
6. `SeedForgeBenchmark.h/.cpp` — bounded methodology, timings, percentiles, aggregate identity.
7. `SeedForgeAsync.h/.cpp` — token, request id, worker and Game Thread gates.
8. `SeedForgeWorldSubsystem.h/.cpp` — weak UObject adapter and teardown.
9. `SeedForgeVisualization.cpp` and `SeedForgePreviewActor.cpp` — canonical topology to HISM scene.
10. `SeedForgeReportCommandlet.cpp` — filesystem/process boundary and three report modes.
11. `SeedForgeEditorModule.cpp` — one Slate Inspector tab and deterministic visual capture hook.
12. `SeedForge*Tests.cpp` and `Scripts/*.ps1` — executable contracts and distribution evidence.

## Runtime request flow

1. `ASeedForgeGameplayCoordinator::BeginPlay` reads explicit command-line values and requests generation from the subsystem. `ASeedForgePreviewActor` retains a compatibility auto-generation mode but gameplay uses its apply-only mode.
2. The subsystem creates a value-only work closure and weak-UObject apply closure.
3. The coordinator cancels the old token, assigns a request id, and launches `UE::Tasks` work.
4. The generator validates configuration, executes bounded proposals, sorts outputs, and computes identity.
5. The Game Thread callback checks shared lifetime, shutdown, cancellation, and newest id.
6. The coordinator revalidates, creates an encounter plan, applies HISM instances, spawns roles, and enters `Playing`.

## Document and report flow

1. Generate a layout and bind it to its configuration and version fields.
2. Export sorts collections and recomputes the canonical hash before writing fixed-order UTF-8 JSON.
3. Import checks required types and exact unsigned strings, validates config, normalizes topology, verifies hash semantics, then validates connectivity.
4. Diff turns both documents into canonical room/walkable sets and performs a two-pointer merge.
5. Benchmark warms the same code path, measures a consecutive range, calculates nearest-rank percentiles, and aggregates seed/hash pairs.
6. The Commandlet owns file I/O and exits non-zero on bad arguments, documents, generation, or writes.
7. `Report.ps1` starts independent processes and reparses every output before creating a revision-aware summary.

## Details worth tracing in a debugger

- Watch `FDeterministicRandom::State` advance for a room proposal.
- Compare proposal order with canonical room order and golden hash bytes.
- Feed an unsorted external document with a sorted hash, then with its order-dependent hash.
- Reverse a layout comparison and observe the added/removed arrays exchange.
- Break on `CancelActive`; watch a stale worker reach but fail the active-id apply gate.
- Change a benchmark sample count and verify timings change while the aggregate depends only on identities.
- Open the Inspector and follow the same Runtime APIs used by the Commandlet—there is no separate editor algorithm.
- Follow one enemy replan from world-to-cell mapping through bounded A* to Pawn waypoints.
- Run gameplay smoke and watch real attack/Core/exit APIs produce the JSON transition/action trace.

## Best first personal modifications

- Add a `--pretty`/formatted export mode outside the canonical hash representation.
- Add a Diff metric for changed configuration fields while keeping set output stable.
- Replace corridor `AddUnique` with a bitmap and prove all five golden hashes remain unchanged.
- Add a minimum entrance/exit distance with a new typed config failure and versioning decision.
- Add a CSV benchmark projection while retaining canonical JSON as authoritative evidence.
- Add one Phase 3 HUD metric through the coordinator snapshot without giving HUD ownership.

Start with a failing Automation test, make the smallest implementation, run focused tests, then run the full suite and packaged smoke.

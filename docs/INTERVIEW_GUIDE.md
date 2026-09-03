# Interview guide

## 1. Why is the generator deterministic?

It receives all variable input explicitly, owns a fixed SplitMix64 stream, uses bounded integer operations, sorts every output that affects hashing, and never reads time, global randomness, UObject state, or unordered-container iteration.

## 2. Why not use `FMath::Rand`?

It is global implicit state. Test order or unrelated code can change its sequence, so it violates reproducibility and makes failures harder to replay.

## 3. Why sort rooms after placement?

Proposal/acceptance order is an implementation detail. Canonical ordering gives stable connection order, entrance selection, tie-breaking, equality, serialization order, and hashes.

## 4. Why does the hash include the seed?

The hash identifies the complete generation identity, not only coincidentally identical geometry. Two seeds that happen to produce the same geometry remain distinguishable.

## 5. Does modulo range selection introduce bias?

Yes, `Next() % Span` has a tiny modulo bias when the 64-bit domain is not divisible by Span. It does not affect determinism or current invariants. A production statistical generator could use rejection sampling without changing the public boundary, but doing so would version golden hashes.

## 6. How is nontermination prevented?

Room proposal count is capped by `MaxPlacementAttempts`. Impossible layouts return `PlacementExhausted` with placed/required counts and the seed instead of retrying forever.

## 7. How is connectivity proved?

The validator builds the canonical walkable set and runs a four-neighbor BFS from the entrance. A valid layout requires visited count to equal total walkable count, which also proves the exit is reachable after its membership check.

## 8. Why is room overlap `O(R^2)` acceptable?

The scoped demo has ten rooms and a capped maximum. Pairwise checks are simple and auditable. A larger generator could introduce a spatial index or occupancy grid after profiling.

## 9. Why use `UE::Tasks`?

The pure generator is independent CPU work. `UE::Tasks` moves it away from the Game Thread while the coordinator explicitly returns only the immutable-by-contract value result for application.

## 10. Is cancellation immediate?

No. Cancellation is cooperative result suppression. A monolithic generator already running may finish, but the token and request-id gates prevent scene application. True mid-loop interruption would require passing a cancellation view into the generator and defining partial-work semantics.

## 11. How are stale requests prevented from winning?

Every Start assigns a monotonically increasing id and cancels the previous token. The Game Thread callback applies only when its id still equals the active id.

## 12. Why shared state plus `TWeakObjectPtr`?

Shared non-UObject state lets worker callbacks safely outlive the coordinator object without dereferencing freed memory. The weak UObject capture separately prevents callbacks from resurrecting or dereferencing a destroyed subsystem.

## 13. Why not wait for workers in `Deinitialize`?

Blocking the Game Thread could deadlock work whose completion is scheduled back to that thread and would cause teardown stalls. Shutdown invalidates state and lets task-owned values die naturally.

## 14. Why HISM instead of one Actor per tile?

HISM batches repeated mesh instances and avoids hundreds of Actor/UObject lifecycles and draw submissions. Topology remains plain data, so presentation can change without touching generation.

## 15. Where are unordered containers safe here?

They are used for membership in validation and boundary-wall detection. Their iteration order never chooses topology, output order, or hash bytes.

## 16. What did the tests catch?

They caught the initial missing behavior, test-runner false-success handling, an UE `constexpr` mismatch, latent test macro syntax, missing test-module link dependencies, and standalone plugin transitive-include issues.

## 17. Why test a packaged executable?

Editor success can hide target eligibility, module type, cooking, staged content, and runtime dependency problems. The packaged smoke proves the cooked map, Game target, plugin Runtime module, rendering, and exit path work together.

## 18. Why not use MQB to build this project?

UnrealBuildTool owns Unreal reflection, generated headers, target rules, modules, plugins, cooking, and platform packaging. Replacing it would fight the engine's build model. MQB's evidence discipline was reused, not its executable pipeline.

## 19. What would you optimize first?

Measure proposal rejection and corridor `AddUnique`. For larger grids, use an occupancy bitmap for placement/corridor membership while keeping a final canonical sorted array. Re-run golden/versioned compatibility tests and Unreal Insights.

## 20. What is the honest authorship answer?

Codex wrote and verified the implementation under a user-approved specification. The user should present it as AI-assisted orchestration and only claim C++ understanding after independently studying and modifying it.


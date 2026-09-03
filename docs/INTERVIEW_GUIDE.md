# Interview guide

## Deterministic core

### Why is generation deterministic?

All variable input is explicit, the RNG algorithm and seed are fixed, work is bounded, output-affecting arrays are sorted, and the core never reads time, global randomness, UObject state, or unordered-container iteration.

### Why not `FMath::Rand`?

It is implicit global state. Test order and unrelated callers can perturb it, making failures hard to replay.

### Why sort rooms after placement?

Proposal order is an implementation detail. Canonical order stabilizes corridor connection order, endpoint tie breaks, equality, serialization, and hashes.

### Why include the seed in the hash?

The hash represents generation identity, not only coincident geometry. Two seeds remain distinguishable even if they happen to create equal shapes.

### Is modulo selection statistically perfect?

No. `% Span` has tiny modulo bias unless the 64-bit domain divides evenly. It does not hurt determinism. Rejection sampling would be an algorithm-version change and would require new golden hashes.

### How is nontermination prevented?

`MaxPlacementAttempts` caps proposals. Impossible inputs return `PlacementExhausted` with placed/required counts and seed.

### How is connectivity proved?

The validator builds the canonical walkable set and performs four-neighbor BFS from the entrance. Validity requires every walkable cell to be visited; the exit is separately required to belong to that set.

## Async and UE boundaries

### Why `UE::Tasks`?

Generation is pure CPU work. The coordinator moves it off the Game Thread and returns only a value result for guarded application.

### Is cancellation immediate?

No. It is cooperative result suppression. Running CPU work can finish, but cancelled or stale results cannot apply. Mid-loop interruption would require a different generator contract.

### How is newest-request-wins enforced?

Each start assigns a monotonic id and cancels the previous token. Game Thread apply requires the id still equal the active id.

### Why shared state and `TWeakObjectPtr`?

Shared non-UObject state safely outlives coordinator callbacks; the weak UObject capture separately prevents access to a destroyed subsystem.

### Why not wait in `Deinitialize`?

Blocking the Game Thread risks deadlock or teardown stalls when completion is scheduled back to that thread. Invalidation is safe and non-blocking.

### Why HISM instead of one Actor per cell?

HISM batches repeated mesh instances and avoids hundreds of Actor/UObject lifecycles. Topology stays plain data and presentation remains replaceable.

## Portable evidence

### Why are seed and hash JSON strings?

Many JSON consumers represent numbers as IEEE-754 doubles, which cannot exactly encode every `uint64`. Decimal strings preserve all 64 bits across languages.

### Why write canonical JSON manually?

The schema is small and fixed. Explicit emission makes byte order visible and prevents reliance on unordered object-map iteration or serializer formatting changes.

### Why normalize before checking the hash?

Canonical identity must describe canonical topology, not an external writer's array order. A hash matching only unsorted input is reported as non-canonical instead of silently legitimized.

### Why ignore unknown fields but reject missing ones?

Ignoring optional future fields supports forward-compatible metadata. Requiring every v1 field prevents silent defaults from changing identity or validation semantics.

### Why structural diff instead of text diff?

Text diff confuses formatting/order with topology. The two-pointer set merge reports exact room/cell additions and removals, endpoints, config, and identities with reverse symmetry.

## Benchmark and delivery

### What does warm-up accomplish?

It exercises code and caches before measurement without contaminating sample statistics. The warm-up count is recorded so another run can reproduce the method.

### Why nearest-rank P95?

It has a simple declared definition and always selects an observed sample. A known-array test fixes the convention and avoids percentile-library ambiguity.

### What does aggregate hash prove?

It folds every measured seed and canonical layout hash in order. It identifies outputs independently of machine-dependent elapsed times.

### Why use an Editor Commandlet?

It provides unattended, scriptable access to UE-linked Runtime code and explicit process exit codes without introducing file I/O into the pure codec.

### Why test a packaged executable?

Editor success can hide module eligibility, cooking, staged assets, and runtime dependencies. The package smoke proves the cooked map, Game target, Runtime module, rendering, screenshot, and exit path together.

### How does finalization avoid stale artifacts?

Verification, report, plugin, and demo manifests must all name the current clean revision and version. The finalizer copies exact manifest paths, hashes every payload, hashes its manifest, then a separate script re-reads and rehashes the set.

### Why is the Inspector deliberately small?

It demonstrates an Editor module, Slate, Runtime API reuse, and export workflow without turning the sprint into unrelated tooling. The algorithm still has one implementation.

### What is the honest authorship answer?

Codex GPT-5.6 Sol implemented and verified the repository under a user-approved scope. The user should claim orchestration and learned understanding, not independent hand-written authorship.

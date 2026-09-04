# Live change drills

These are proposed learning exercises, not changes already made or claims of personal authorship. Do not run them against a frozen verification candidate. Agree one small scope, preserve the clean starting revision and use a separate approved branch in the existing checkout. Never start parallel UE writers or create an unapproved integration copy.

## Workflow for every drill

1. State the acceptance contract, non-goals and affected evidence boundary.
2. Write the smallest failing test. A crash, invalid fixture or timeout for the wrong reason is not RED.
3. Implement the minimum change; preserve unrelated edits and old golden contracts.
4. Run focused and full diagnostic verification while the worktree is dirty.
5. After review and a clean checkpoint, run the relevant authoritative wrappers again. Diagnostic results do not certify the candidate.

For supported development wrappers:

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.CoordinateSafety -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge -TimeoutSeconds 900 -AllowDirtyDiagnostic
```

Select the actual registered filter for the drill; the coordinate filter above is one example. World/input/capture changes additionally need the appropriate ordinary-input and gameplay RHI runs. BuildPlugin/package/final wrappers require clean source and remain separate gates. Keep artifacts under the repository, preserve process-local storage controls, and never increase watchdogs or suppress errors to obtain GREEN. See [DEVELOPMENT.md](DEVELOPMENT.md).

## 1. Add an explicit four-Core configuration

Keep the current three-Core default unchanged. Test four distinct stable Core IDs, exact collection eligibility and impossibility when cells are insufficient. Explain why config changes encounter identity but not layout identity. Touch encounter config/tests first, then only the consumers whose explicit four-Core contract requires change; do not silently replace seed-24301 goldens.

## 2. Extend coordinate-boundary coverage

Add translated path fixtures near both int32 limits, including a legitimate adjacent pair and a false MAX/MIN wrap pair. Require identical translated path/status/expansion behavior. Inspect checked neighbor sums and widened subtraction, not just the return status. These sparse value fixtures do not claim generated-map connectivity.

Focused starting point: `SeedForge.Audit.CoordinateSafety`.

## 3. Change replan frequency without changing search semantics

Propose 4 Hz instead of 2 Hz. Test the scheduling contract and keep A* out of Enemy Tick. Preserve path/status/expanded-node results for identical requests and discuss the CPU/responsiveness trade-off. A normal World-tick test must advance across distinct engine frames; do not directly call the private replanner.

## 4. Make expansion semantics observable in a new case

Write a graph where the goal can be selected exactly after the last permitted non-goal expansion. Contrast it with a case needing one more expansion and an unreachable graph exhausted within budget. Preserve AlreadyAtGoal/InvalidInput distinctions, empty failure paths and the `(F,H,Y,X)` tie-break. Do not count the goal as an extra expansion or call the budget a memory/time limit.

## 5. Add a read-only HUD metric

Expose live enemy count through the copied snapshot rather than scanning actors from HUD. Test update after an actual kill, then update the AHUD-owned Slate text. Preserve one overlay through restart, hide/debug behavior and EndPlay removal. HUD must not own or mutate enemy HP/counts.

Focused starting points: `SeedForge.Gameplay.Hud` and the real Coordinator World tests.

## 6. Extend stale-completion coverage

Use a controlled test fixture to deliver an old success or failure after a newer request. Assert pending/applied IDs, hashes, actor ownership and possession remain those of the newest valid run. Keep RunGeneration distinct from request ID. Explain why fixture-delivered completion is not live worker proof.

Do not remove a production guard in a release candidate. Any separately approved mutation-sensitivity experiment must be isolated, restored exactly and followed by GREEN; never leave a fault flag or publish its artifacts as success.

## 7. Test recovery while unpossessed

Extend the persistent R/N tests for a typed failure followed by rapid requests before a new pawn exists. Require exactly one binding per action, one final possession and no old run actors. Recovery belongs to PlayerController plus Coordinator, not a Character that may be destroyed.

Focused starting point: `SeedForge.Audit.RestartInput`.

## 8. Extend same-frame Dash behavior

Add a press/release/opposite-key case that distinguishes current summed axes from stale callbacks. Verify normalized direction, one real cooldown consumption and possession/focus reset. Then confirm actual movement through the ordinary input self-test, not only PendingLaunchVelocity.

Focused starting point: `SeedForge.Audit.DashInput`. Never call private Dash directly or assign a launch vector to the evidence fixture and describe it as live input.

## 9. Add a deterministic placement property sweep

Sweep at least 1,000 explicit seeds through pure encounter planning. Check stable role IDs, uniqueness, walkability, safety distances, validation and repeatability. Keep timing out of semantic assertions. A new minimum separation rule needs typed impossibility tests and an explicit compatibility/version decision.

## 10. Add capture lifecycle rejection coverage

Start with the pure lifecycle: stale token/run, wrong path/dimensions, pixels before render, processed before save, cancellation or duplicate completion. Extend the real component test only for a documented gap. Require callback ownership and cleanup under the existing eight-second bound.

Use clearly synthetic fresh PNG fixtures only for decoder/file-validation tests. Actual rendered proof still needs the original RHI process, same-run receipts, full decode and original-resolution visual review. Do not brighten, crop or regenerate an evidence image to hide a failure.

## 11. Optimize the A* open set

Replace the scan with an indexed heap only after locking exact path/status/expansion equivalence, including ties, duplicate/shuffled walkable input, exhaustion and extreme coordinates. Compare against a test-only reference over a bounded broad corpus before removing that reference. A faster result with different deterministic choices is a contract change, not a transparent optimization.

## 12. Extend evidence tamper resistance

Add a synthetic negative case where a later child step edits an earlier raw summary/log/image, or where retained first/latest path samples claim an impossible hidden gap. Require rejection before outer manifest publication. Preserve nonadjacent valid path samples and exact uint64 strings.

Synthetic script fixtures stay under a unique repository Artifacts directory and are not UE projects, release proof or a substitute for actual processes. Explain how frozen hashes, source checks, native observations and visual review prove different things.

## Rehearsal timeboxes and handoff

Allow roughly 15–25 minutes for a focused value-test exercise, 30–45 minutes for lifecycle/input work, and 60–90 minutes for a broader optimization/evidence exercise, excluding full UE verification. These are study timeboxes, not performance guarantees or permission to skip gates.

For every drill, show the failing assertion and its cause, changed ownership/arithmetic boundary, exact commands/results, limitations and rollback revision. State which edits were personally authored and which used AI assistance. The current diagnostic 119/119 and clean a9f5625 packaged checkpoint do not validate a future drill or complete the still-pending final candidate matrix.

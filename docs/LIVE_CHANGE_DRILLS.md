# Live change drills

Use one small branch per drill. Start with a failing Automation test, implement the smallest change, run the focused filter, then run the full SeedForge suite. Package only drills that change World/config/target behavior.

## 1. Change Core count

Change the default from three to four without hard-coding actor creation. Update encounter, state, HUD, smoke counts, and docs. Explain why the encounter hash must change while the layout hash must not.

## 2. Add an A* obstacle input

Add a blocked-cell list distinct from walkable cells. Specify precedence and typed invalid overlap. Test stable detours and a newly unreachable route.

## 3. Tune enemy replanning

Change the default 2 Hz interval to 4 Hz. Demonstrate that A* still never runs in Tick and discuss CPU responsiveness trade-offs.

## 4. Add a path failure

Add `StartBlocked` or an equivalent typed status rather than folding it into `InvalidInput`. Write the RED test, update callers/logs, and preserve all existing status semantics.

## 5. Add a HUD metric

Display live enemy count. Extend the snapshot instead of iterating actors from HUD, and prove the count updates after a kill.

## 6. Fix an injected stale callback

Temporarily remove the coordinator active request-ID check, add a test/reproduction that lets an old result arrive after N, then restore the guard. Explain subsystem versus run-owner defense in depth.

## 7. Add `Paused`

Extend the state machine with explicit pause/resume transitions. Decide which timers stop, how HUD changes, what smoke ignores, and why arbitrary `SetState` remains forbidden.

## 8. Increase spawn safety

Add Core-to-enemy or enemy-to-enemy minimum distance. Keep selection deterministic, return typed impossibility, and document hash/version compatibility.

## 9. Add an Automation property test

Sweep at least 1,000 seeds through encounter generation. Check counts, stable IDs, uniqueness, walkability, safety radius, validation, and repeatability without asserting meaningless timing.

## 10. Add a cooldown indicator

Expose attack readiness/progress through the snapshot and render it in Canvas HUD. Do not let HUD own or modify cooldown state.

## 11. Optimize A* open selection

Replace the linear scan with an indexed binary heap. Lock identical path/status/expansion results for all current tests and add a deterministic broad comparison against the reference implementation before removing it.

## 12. Add a new gameplay pickup

Implement a health pickup with its own stable role/ID/cell and typed placement failure. Avoid turning the task into inventory, persistence, or a general item framework.

## Timed interview format

- Easy drills 1-5: 15-25 minutes each.
- Medium drills 6-10: 30-45 minutes each.
- Advanced drills 11-12: 60-90 minutes each.

For every drill, be ready to state the acceptance contract, non-goals, failure observed in RED, exact verification command, changed ownership boundary, and rollback commit.

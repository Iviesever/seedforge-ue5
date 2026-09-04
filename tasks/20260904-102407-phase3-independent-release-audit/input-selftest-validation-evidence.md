# H6 external input-self-test validator evidence

Status: pure PowerShell validator implemented and independently runnable. All evidence below is **synthetic test evidence only**, not Editor/packaged runtime success or release readiness. No UE, UBT, UAT, installation, Engine edit, Git index/HEAD mutation, or commit was performed by this bounded subtask.

## API and authority boundary

Dot-source `Scripts/InputSelfTestValidation.ps1`, then call:

```powershell
Assert-SeedForgeInputSelfTest -Trace $parsedTrace -SourceIdentity $declaredIdentity `
    -Seed $seed -ProcessStartedAtUtc $processStart -ProcessEndedAtUtc $processEnd
```

`Trace` is the object obtained by parsing JSON, not a path or JSON string. Preserve timestamp strings when parsing on PowerShell 7.6 (`ConvertFrom-Json -DateKind String`); Windows PowerShell 5.1 preserves these strings by default. Process bounds accept the wrapper's `DateTimeOffset` values. The function throws on a missing/malformed/contradictory observation and returns one compact proof object on acceptance. It does not read/write trace files or launch processes.

The returned proof contains validation status, source identity/revision/kind, `SourceVerified=false`, run/input/transition/queue counts, initial/final seed and run generation, path cell/move counts and total distance. It deliberately contains no raw actor references, trace object, or graph. UObject keys such as `/Game/...#id` remain opaque strings in the original trace and are never treated as filesystem paths. Diagnostic prefixes remain diagnostic; clean-shaped syntax is not clean-build authority. Native revision parsing lowercases the separate revision while preserving the supplied identity's casing, and the external validator mirrors that behavior.

The wrapper remains responsible for clean/current source context, actual process exit/no timeout, unique marker, strict whole-log validation, trace path/freshness/hash, and frozen evidence indexing. This helper enforces UE `5.8.<numeric patch>` version shape but cannot attest to a binary from a version string.

## Validated contract

- Actual JSON scalar/array/object types, exact required field spelling, bounded arrays/strings, canonical uint64 strings without whitespace/leading zero/overflow, finite numbers and finite derived distance/speed bounds, int32 cells and uint32 stable IDs, UTC-Z timestamps and a <=30-second trace inside the process interval.
- Passed envelope with empty failure code/message; gameplay snapshots use the distinct native `None` failure sentinel. One completion, no remaining delegates/pressed keys, ordinary mode and no gameplay smoke.
- Four initial/same/new/rapid runs, exact uint64 LCG seeds and generation offsets, ordered request identities, same-seed hash/cell consistency, seed-24301 golden layout/encounter hashes, persistent one-controller/one-HUD/one-Coordinator ownership, exactly eleven disjoint fresh run actors and full health/core/exit/cooldown/timer resets.
- Thirteen ordered attributed input effects, real releases, unit XY aim targets, two attacks and cooldown-separated single-target destruction, cardinal 19.9..80-unit capsule-safe movement, and diagonal Dash launch/cooldown plus 19.9..200-unit positive-diagonal motion on walkable cells. Dash movement bounds reflect the primary's explicitly approved H6 review amendment; no input/runtime behavior is changed here.
- Ten state transitions, four queued requests, zero pending hashes/applied IDs during generation, final superseding request ownership, and same-frame N-then-R injection/queue correlation.
- Canonical unique walkable ordering; simple cardinal walkable path; matching waypoints; two retained first/latest movement samples with matching identity/revision, genuine waypoint targets, path-plane/route/segment membership and exact speed bounds. Aggregates support more than two observations, small retained distances, and off-center starts. Only a count of two requires adjacency and exact retained sums. Completion stops accumulation after the first eligible >=20-unit aggregate.
- Bounded setup records with known owned identities and canonical positions, before an affected measurement and outside input/path windows. Frame and UTC order are checked; later legitimate setup for a later effect is allowed.

The JSON does not contain the complete layout/configuration required to independently recompute arbitrary generator/encounter hashes. The primary explicitly retained that responsibility in native `ValidateEvidence`, which regenerates each seed's layout and encounter. This PowerShell helper does **not** claim that a plausible walkable set or nonzero new-seed hash proves native generator identity, nor does it reimplement A*. It validates all relationships that this trace exposes.

## Test fixtures

`input-selftest-synthetic-fixture.ps1` mirrors the native `MakeSyntheticSuccessEvidence` schema, timing, ownership and effect structure. It uses intentionally synthetic actor identities and non-default hash placeholders; these values are not native live-game proof. Default cell membership is copied into the explicitly marked `input-selftest-layout-synthetic-fixture.json` from the old Phase2 export. The harness is self-contained and does not depend on an existing ignored Phase2 artifact directory. No synthetic success trace is written as a runtime artifact.

Positive controls include diagnostic identity retention, opaque `/Game/...` actor keys, non-adjacent first/latest aggregate samples, retained samples totaling less than 20, off-center starts, an already-correct first aim, fractional UTC widths, process `DateTimeOffset` arguments, native source-case normalization, and an earlier setup frame sharing the millisecond-rounded injection timestamp. Strict frame separation with consistent UTC order excludes setup displacement without inventing sub-millisecond precision. Negative cases cover envelope/type/precision/ownership/reset/effect/queue/transition/setup/path/time tampering and both review regressions.

## Observed RED to GREEN

All report paths are below `Artifacts/Reports/InputSelfTestValidation/`. Every report explicitly sets `SyntheticOnly=true`.

| Checkpoint | Result | Report directory |
|---|---:|---|
| Initial permissive API stub, PS5.1 | 116 expected rejection failures; 5 controls passed | `20260904-091707-8a4989104ebc43ae9487c1f12717e2f4` |
| Additional type/ordering RED, PS7.6 | 11 failures / 144 cases | `20260904-092711-fc66ece12a3046979c4c6f72be7ea9e7` |
| Post-completion accumulation RED, PS7.6 | 1 failure / 145 cases | `20260904-092949-890bb6d74f164192a28440d1c513d70c` |
| Dash bounds/derived-finite and source-case RED, PS7.6 | 6 failures / 152 cases | `20260904-093232-64f78db9c6e94022a50a1d14fe4757fe` |
| UE version RED, PS7.6 | 3 failures / 156 cases | `20260904-093502-02ef6cf4e53d44878b0c792b1240c81c` |
| Rounded setup timestamp RED, PS7.6 | 1 failure / 157 cases | `20260904-093832-4b766e5d7c104595b46c6409ba02ff8e` |
| Final GREEN, Windows PowerShell 5.1 | **157/157** | `20260904-093918-85d7cd7ff97a45e8a96e0d2dbf4c0a50` |
| Final GREEN, PowerShell 7.6.0 | **157/157** | `20260904-093916-696ff4e5f3ee41859ed3ab01d6e3cffa` |

Intermediate GREEN attempts exposed and corrected fixture-only dictionary sorting/negative-literal typing and implementation overload selection for UTC parsing and floating-point clamping. Those failed attempts are retained in the same report root, not presented as passes.

Reproduce with either shell:

```powershell
powershell.exe -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-input-selftest-validation.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-input-selftest-validation.ps1
```

The real failed H6 trace at `Artifacts/Reports/InputSelfTest/20260904-171917-61a3c9427537408ab9715bb57dcc82a4/input-selftest.json` was read only to verify observed encoding and retained-sample shape. It was **not** accepted as successful input evidence. Actual successful Editor/packaged trace integration remains the primary's next gate.

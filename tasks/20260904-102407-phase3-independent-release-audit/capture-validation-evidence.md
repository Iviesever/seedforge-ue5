# SF-IRA-005 capture-set validator evidence

## Scoped delivery

AI-assisted implementation by the bounded capture-validator sub-agent. Only these files were created by this subtask:

- `Scripts/CaptureValidation.ps1`
- `tasks/20260904-102407-phase3-independent-release-audit/verify-capture-validation.ps1`
- This evidence document.

The existing PNG validator and its harness/evidence were not edited. No Runtime code, other scripts/plans, Git index/HEAD/branches, Engine files, global settings, dependencies, or sibling directories were changed. No UE/Editor/UBT/UAT/build/package processes or sub-agents were started.

## Interface and checks

Dot-source `Scripts/CaptureValidation.ps1`, then call:

```powershell
$records = @(Assert-SeedForgeGameplayCaptures -Trace $trace -RunDirectory $currentScreenshotFolder -ProcessStartedAtUtc $startedUtc -ProcessEndedAtUtc $endedUtc)
```

The function dot-sources the existing `PngValidation.ps1` and calls its real `Assert-SeedForgePng` for each receipt. It returns exactly three validated PNG records in start/combat/win order, enriched with `Token` and `Label`. No partial records are emitted if any later validation fails. Content SHA256 values are not required to differ; the positive fixtures deliberately reuse identical PNG bytes.

Checks include:

- Positive canonical uint64 decimal **strings** for trace run/applied-request identity and matching receipt identities; uint64 decimal strings including zero for frames, without float conversion or precision loss.
- Exactly three captures and screenshots, ordered start/combat/win labels, distinct lowercase nonzero 32-hex GUID tokens and distinct normalized paths.
- Ordered screenshot paths equal normalized receipt paths; every receipt is directly in the flat current-run folder; native `-<token>.png` suffix is mandatory. Exactly three actual PNG files must exist there and equal the receipt path set, including hidden-file counting.
- Strict valid UTC `Z` strings, with whole-second or 1–7 fractional-digit ISO timestamps. Process/request/completion bounds and within/cross-capture frame/time order allow equality, as contracted.
- Actual boolean true success; numeric integral zero delegate bindings; numeric integral 1280x720 dimensions; positive numeric integral fileBytes equal the independently validated PNG length. Strings, booleans and fractions cannot masquerade as numeric fields.
- Real PNG validation enforces freshness, containment, reparse rejection, dimensions, minimum size, CRC/zlib/decoded integrity and file metadata.

Integration must preserve JSON timestamp strings. PowerShell 7.6's `ConvertFrom-Json` supports `-DateKind String`; use it where available. Windows PowerShell 5.1 already preserves these strings. A DateTime object cannot preserve evidence of the original required UTC-Z spelling, so it is not accepted as a receipt timestamp.

This validator is only for the native three-gameplay-capture set. The ordinary single-capture mode's user-specified fixed filename intentionally does not use it.

## Exact commands and RED/GREEN evidence

Working directory: `D:\program\SeedForge`.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-capture-validation.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-capture-validation.ps1
```

All directories in this table are under `Artifacts/Reports/CaptureValidation/` and contain `summary.json` with exact per-case results. Every case keeps its synthetic trace and images; final runs also retain the exact process-boundary and run-directory parameters in `validation-input.json` and validator SHA256 identities in the root summary.

| Stage | Fixture directory | Total / passed / failed | Exit |
|---|---|---|---|
| Permissive-stub RED | `20260904-124131-540bba4e73fb46099a58088897a80b20` | 81 / 0 / 81 | 1 |
| Initial implementation; nondeterministic negative time fixture found | `20260904-124409-ffd1972e07704ce68a1abc43a52fd09f` | 81 / 80 / 1 | 1 |
| Corrected deterministic time fixture, GREEN | `20260904-124533-34c99c2c800b48e1b57f438ebfd1ec7b` | 81 / 81 / 0 | 0 |
| Expanded native-time/equality/two-file coverage, Windows PowerShell 5.1 | `20260904-124657-d341a8664c83466c8a057eb02037db92` | 84 / 84 / 0 | 0 |
| PowerShell 7.6.0 compatibility and recorded input parameters | `20260904-124828-e21af61e2e2b40a69d072f3506bd1e3f` | 84 / 84 / 0 | 0 |
| Final Windows PowerShell 5.1 rerun with complete parameter evidence | `20260904-125008-b6fe982b6a64461d9050311b67565a38` | 84 / 84 / 0 | 0 |

Final console output:

```text
CAPTURE_VALIDATION total=84 passed=84 failed=0 fixtures=D:\program\SeedForge\Artifacts\Reports\CaptureValidation\20260904-125008-b6fe982b6a64461d9050311b67565a38
```

Both scripts also passed the PowerShell parser (`parse_errors=0`) and direct trailing-whitespace checks (`trailing_whitespace_lines=0`).

The initial RED had four positive controls failing with `Valid set did not return three PNG records.` and 77 negative cases failing with `Invalid capture set was accepted.` No exception masking or missing-function error was used as RED proof.

The final 84-case harness has six positive controls and 78 rejection cases. Positives cover identical PNG hashes, uint64 maximum IDs/frames, zero/equal frame boundaries, numeric integral Double values, native millisecond UTC strings, and whole-second/equal time boundaries. Negative cases cover malformed/missing IDs/frames/times, order, counts, duplicated/wrong tokens/paths, coercion, dirty delegate state, size mismatches, wrong folder scope, stale metadata, real PNG corruption and directory reparse traversal.

### Time-fixture diagnosis

The initial `completion before its request rejected` mutation copied the first capture's request timestamp into the second capture's completion. The saved input showed start request/completion and combat request/completion were all exactly `2026-09-04T04:44:18.4850714Z`; Windows clock resolution had made the intended reversal an allowed equality.

The validator was unchanged. The fixture now explicitly subtracts one tick from the relevant request/prior-completion timestamp. A synthetic one-second process-envelope margin keeps cross-capture ordering cases independent of process-start bounds. No sleep/retry was added, and valid equality acceptance was preserved.

## Evidence boundaries

- Fixtures contain freshly re-emitted bytes from the previously inspected baseline `Artifacts/Media/Gameplay/20260904-120630/SeedForge-Gameplay-start-24301.png`. They are clearly labeled synthetic and are **not** current gameplay screenshots, lifecycle/worker evidence, visual approval or release proof.
- All fixture writes and junction targets remain beneath their unique repository-local `Artifacts/Reports/CaptureValidation/<id>` root. The two-file case retains its third synthetic PNG outside that case's flat image folder instead of deleting it.
- This layer verifies evidence consistency and present PNG contents. The parent owns real process boundary collection, trace authenticity, rendered-state/callback ownership, runtime integration, visual QA and immutable-release provenance.
- Existing reparse paths are rejected. Neither this wrapper nor the PNG validator claims atomic protection against hostile concurrent ancestor replacement.
- No overall SeedForge release-completion claim is made.

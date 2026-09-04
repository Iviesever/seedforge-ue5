# SF-IRA-005 PNG validator subtask evidence

## Scope and implementation

AI-assisted implementation by the bounded PNG-validation sub-agent. Only these source/evidence files were created by this subtask:

- `Scripts/PngValidation.ps1`
- `tasks/20260904-102407-phase3-independent-release-audit/verify-png-validation.ps1`
- This evidence document.

No Runtime files, Git index/HEAD/branches, Engine files, global environment/folder settings, or sibling directories were changed. No UE, Editor, UBT, UAT, package, compiler, or dependency-install process was run. System.Drawing is loaded from the installed Windows runtime; no compilation or temporary compiler files are required.

`Assert-SeedForgePng` implements the allocated parameter contract and returns normalized `Path`, decoded `Width`/`Height`, `Length`, lowercase `Sha256`, `CreationTimeUtc`, and `LastWriteTimeUtc`. It rejects lexical escapes/sibling prefixes, every reparse-point ancestor, missing/non-file input, stale creation or modification times, wrong extension/signature/IHDR/dimensions, undersized files, incomplete PNG chunks, invalid chunk CRCs, missing/invalid IEND, invalid zlib headers/DEFLATE/Adler checksums, invalid scanline filters, and incorrect decoded byte counts. Adam7 pass geometry is accounted for. It also performs a real System.Drawing pixel decode/draw. Streams, images, graphics, bitmap and hash objects are disposed in `finally`; validation holds a read-only file handle denying writes/deletion during content inspection.

## Reproduction commands

Working directory for every command: `D:\program\SeedForge`.

Primary RED/GREEN command (Windows PowerShell `5.1.22621.2506`):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-png-validation.ps1
```

Compatibility command:

```powershell
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-png-validation.ps1
```

The harness creates fresh unique directories under `Artifacts/Reports/PngValidation/` and keeps all fixtures and `summary.json`. The summaries contain exact case names, pass/fail counts and failure details; the final harness additionally records the actual rejection reason for negative cases. Directory-junction fixtures point only to other directories within their own synthetic fixture root.

## RED to GREEN history

All identifiers below are relative to `Artifacts/Reports/PngValidation/`; each contains `summary.json`.

| Stage | Fixture directory | Total / passed / failed | Exit |
|---|---|---|---|
| Initial scaffold (metadata property absent) | `20260904-121546-31503c53f46b4479a283ac9073ef41fc` | 23 / 2 / 21 | 1 |
| Behavioral RED with complete-shaped permissive stub | `20260904-121600-a3b16c6b488d4ecc9469073e00471a31` | 23 / 2 / 21 | 1 |
| Path/header/framing checks plus decoder only | `20260904-121753-7c0e7b136d14443aa07617ab99b968c2` | 23 / 21 / 2 | 1 |
| Expanded decoder-tolerance RED | `20260904-122024-37d4d40b199d406bbc1b2e335027c89e` | 26 / 21 / 5 | 1 |
| Integrity checks; single-pass PowerShell array bug | `20260904-122207-5870a92a99944a5e9f8c368b236bf9bf` | 26 / 23 / 3 | 1 |
| First full GREEN | `20260904-122231-58381acf277247ef8677a032dfabdb24` | 26 / 26 / 0 | 0 |
| Explicit decoded-count checks and rejection evidence | `20260904-122420-f4f9a3bc92624c31a4e311213bb5f12a` | 28 / 28 / 0 | 0 |
| PowerShell 7.6.0 compatibility | `20260904-122453-7b2e166ffb2b41d396c68a4089ca0b3a` | 28 / 28 / 0 | 0 |
| Final Windows PowerShell rerun, function-local terminating error policy | `20260904-122630-855a3c3214884f3d8de46e0ff5a07d3e` | 28 / 28 / 0 | 0 |

Final console summary:

```text
PNG_VALIDATION total=28 passed=28 failed=0 fixtures=D:\program\SeedForge\Artifacts\Reports\PngValidation\20260904-122630-855a3c3214884f3d8de46e0ff5a07d3e
```

PowerShell parser checks on both scripts reported `parse_errors=0`; `git diff --check` reported no tracked-file whitespace errors.

The meaningful initial RED output was `Wrong byte length.` for metadata and `Invalid PNG input was accepted.` for the 20 rejection cases. The two controls (normalization and no leaked success handle) passed against the permissive stub.

### Decoder-tolerance root cause

The first IDAT begins at byte 33 in the baseline fixture. Its first compressed bytes are `120,94,236,157,7,160`; the initial corruption changed the zlib header to `0,0,236,157,7,160`. System.Drawing `Image.FromStream(stream, false, true)` and a full-size `DrawImage` did not throw. Reading pixel `(1279,719)` returned transparent black (`A=0`) from the corrupt fixture, versus opaque black (`A=255`) from the valid one. Thus, successful GDI+ calls alone were not proof of valid compressed pixels.

The expanded RED preserved corruption and recomputed PNG chunk CRCs independently in the harness. Its five failures were:

1. `corrupt pixel data rejected`
2. `invalid zlib header rejected even with valid chunk CRC`
3. `invalid DEFLATE block rejected even with valid chunk CRC`
4. `bad Adler checksum rejected even with valid chunk CRC`
5. `releases file handles after rejection` (the invalid file was admitted, so rejection precondition failed)

No negative assertion was removed or softened. The implementation now validates all chunk CRCs plus the zlib header, actual DEFLATE read, exact scanline byte count and Adler-32. The valid-CRC reserved-DEFLATE fixture fails with `Unknown block type. Stream might be corrupted.`; separate valid-CRC fixtures fail explicitly on zlib header and Adler checksum. Both oversized and undersized decoded data are tested with corrected IHDR CRCs.

The intermediate three positive-case failures (`Unable to index into an object of type System.Int32.`) were a PowerShell single-nested-array flattening bug. Keeping the non-interlaced pass as a unary-comma array fixed it; the same complete suite then passed.

## Boundaries and limitations

- Every image emitted by this harness is synthetic test data, including freshly re-emitted bytes from `Artifacts/Media/Gameplay/20260904-120630/SeedForge-Gameplay-start-24301.png`. They are **not** fresh gameplay screenshots or release evidence. The baseline image was read only.
- This validator proves file containment/freshness, PNG integrity and decoding, not that the correct World/HUD/state was rendered. Capture count, labels, distinct tokens, callback/frame ownership, receipt matching and release provenance remain the primary agent's integration work.
- Windows-specific System.Drawing support is intentional. Both Windows PowerShell 5.1 and PowerShell 7 were exercised; non-Windows environments were not certified.
- Reparse checks reject existing traversal. They are not an atomic defense against a hostile process concurrently replacing ancestor directories; the capture pipeline is expected to own its output tree.
- The harness validates production-sized non-interlaced PNG fixtures. Adam7 size accounting is implemented but has not been exercised with a separate interlaced positive fixture.
- No full SeedForge build, runtime smoke, packaging or release-readiness conclusion is made by this subtask.

# BuildCookRun Pak/IoStore original-log association

Status: the approved bounded helper and synthetic tests are implemented. These are **synthetic validator results**, not a successful PackageDemo run or release evidence. The sub-agent changed only this document, `Scripts/BuildCookRunStorageValidation.ps1`, and `verify-build-cook-run-storage.ps1`. No existing RuntimeStorage helper, PackageDemo launcher, configuration, Engine file, Git index/HEAD, or UE process was changed/run by this subtask.

## API

```powershell
. ./Scripts/BuildCookRunStorageValidation.ps1
Assert-SeedForgeBuildCookRunStorage `
    -UatLog $originalUatLog `
    -DiagnosticRoot $currentUatDirectory `
    -ProjectRoot $projectRoot `
    -EngineRoot $engineRoot `
    -ProcessStartedAtUtc $uatStart `
    -ProcessEndedAtUtc $uatEnd
```

The fixed contract is one `CreateMultiplePaks` process and one `CreateIoStoreContainers` process. Other UnrealPak scenarios, including ZenAutoLaunch, are not silently accepted. Cook remains the caller's existing complete-native-log validation. The caller also retains clean-source/process-exit and whole-log error/warning checks; this helper adds no warning/error allowance.

The helper returns `Validated`, `ProcessCount=2`, an original `UatLog` file proof, and two `Processes` records. Each process includes the scenario, exit code, announce/Running/exit/SafeCopy line numbers, the **original complete lines**, original stdout/native file proofs (`Path`, `Sha256`, `Length`, `LineCount`), and the unchanged RuntimeStorage helper's proof. Raw Running lines remain prefixed strings, not standalone external Engine paths that evidence indexing could mistake for files. Their original content is bound by the UAT file SHA-256 and recorded line numbers.

No transcript is written, rewritten, normalized on disk, or supplemented with a synthetic `LogInit` header. Only in-memory stdout comparison removes trailing spaces/tabs, terminal empty lines, and line-ending differences. Every original file's byte hash is retained and rechecked before returning.

## Source and original-log facts

Installed UE 5.8 `CopyBuildToStagingDirectory.Automation.cs:485–501`:

1. Prepends the actual project file to the command.
2. Logs `Running UnrealPak with arguments: ...`.
3. Calls `RunAndLog`, which emits `Running: <executable> ...` and the process result.
4. After successful return, copies the program's complete `Saved/Logs/UnrealPak.log` to `UnrealPak_<Scenario>-<timestamp>.txt` in the isolated UAT log directory.

`ProcessUtils.cs:466–491` appends original stdout lines to captured output while logging them; `1315–1320` writes that captured output to the separate dash-named `UnrealPak-<timestamp>.txt`. `1248` emits `Took ...s to run UnrealPak.exe, ExitCode=...`.

Read-only inspection of `Artifacts/Logs/uat-demo-20260903-205654/Log.txt` established the exact associations at lines 1675/1677 -> 1711 -> 1713 and 1714/1716 -> 1846 -> 1848. The corresponding **underscore** complete logs contain `LogInit: ExecutableName: UnrealPak.exe` and the native command line at line 41. The shorter dash-named stdout files do not contain that header. Their bodies matched the UAT Running/exit intervals after the comparison-only whitespace handling above (33 and 129 lines). These historical files contain prohibited Zen activity and are not passing storage evidence.

The UAT and native command lines are not required to be byte-identical: Engine startup removes the leading project selector, changes quoting, and the observed native line includes `-nopak`. Instead, the two original UAT command records must agree exactly, and the native log must agree on the scenario's key response/container/cooked-directory paths and the explicit unique DDC controls.

## Validation algorithm

- Require an owned project, an isolated diagnostic directory beneath project `Artifacts`, nonempty regular logs, and last-write times within the caller's UAT process interval. Reject path escapes and reparse components. Traverse diagnostic directories explicitly without following junctions, rejecting a reparse entry before descending.
- Inventory **all** `UnrealPak*` files: exactly two dash-named stdout logs and the two recognized underscore full logs, all direct children of the diagnostic root. Nested, extra, unknown-scenario, missing, or reused logs fail.
- Scan all original UAT/stdout/full log text for prohibited Zen service/store, launch, installation, write, oplog, or staging activity. `-SkipZenStore` itself is not treated as prohibited activity.
- Parse exactly two serial blocks: original announce, exact configured UnrealPak executable/arguments in Running, a well-formed `Took <duration>s ... ExitCode=0`, then the original SafeCopy from the fixed project `Saved/Logs/UnrealPak.log` to that scenario's current complete-log path. Interleaved, duplicate, orphan, premature, missing, and nonzero records fail.
- Bind each stdout to exactly one complete Running/exit interval by content; each stdout and full-log target can be used once only. A copied filename or matching timestamp alone is not sufficient correlation.
- Parse Windows command-line quoting without promoting quoted or escaped switch-like text into arguments. Require the owned project selector on the UAT invocation; require exactly one owned DDC graph, bare no-default-graph switch, and exact repository-local DDC override in both UAT/native commands.
- `CreateMultiplePaks` must use this diagnostic root's `PakCommands.txt`. `CreateIoStoreContainers` must use this root's `IoStoreCommands.txt`, with owned container/cooked paths matching the native log. Ambiguous or cross-scenario switches fail.
- Apply the existing strict `Assert-SeedForgeRuntimeStorage -RequireDdc:$false` directly to each complete native log, compare its hash with the already-read original, and finally recheck every original file's digest/length. Cook, UAT invocation exit, artifact freshness and whole-log audits remain caller gates.

Response **paths** and critical scenario parameters are correlated; this helper does not implement a new response-file parser or attempt to prove package contents. Original bytes and hashes of UAT/stdout/full logs are preserved. Actual packaging and archive/product validation remain separate requirements.

## Synthetic RED to GREEN

All fixtures, including deliberately synthetic project files and bounded junction fixtures, remain under `Artifacts/Reports/BuildCookRunStorageValidation/`. Test logs visibly identify themselves as synthetic and reports set `SyntheticOnly=true`. No fixture is published as native verification evidence.

| Checkpoint | Result | Report directory |
|---|---:|---|
| Initial permissive stub RED, PS5.1 | 45 rejection failures; four passing controls | `20260904-103946-12e0215be05d4f6084a0d85e1b3acefc` |
| Malformed process-duration RED, PS5.1 | 1 failure / 54 cases | `20260904-104836-74789ddb07224e07bb1670100ffa0b5a` |
| Nested diagnostic junction RED, PS7.6 | 1 failure / 55 cases | `20260904-105012-77ca4f55fa8b4b5bb64226e05002122d` |
| Final GREEN, Windows PowerShell 5.1 | **55/55** | `20260904-105145-3912ab3e3c364420bfd05446dfaa1f60` |
| Final GREEN, PowerShell 7.6.0 | **55/55** | `20260904-105148-78dfa120c1534605ae1157fe609dbad5` |

Positive controls cover two original associations, stdout without native initialization headers, unchanged raw bytes despite comparison-only whitespace normalization, and native quoting differences. Negative cases cover every missing/extra/duplicate/orphan event/file, mismatched actual executable/arguments, exit failure, copy ordering/source/target/reuse, response/container/cooked path mismatches, missing/duplicate/wrong DDC controls, quoted switch spoofing, Zen in each original source, stale/future files, reversed intervals, active writers, root junctions, and nested junctions. Accepted proofs are checked against all original stdout/full/UAT hashes; every case checks that original file bytes remain unchanged.

Reproduce:

```powershell
powershell.exe -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-build-cook-run-storage.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-build-cook-run-storage.ps1
```

Initial GREEN attempts exposed PowerShell automatic-variable collisions (`args`/`Matches`); task-specific names corrected those implementation errors, with all failed attempt reports retained. The final code and test harness parse successfully and contain no trailing whitespace. Parent integration, independent verification, and the next real BuildCookRun remain pending.

## Parent integration

Parent independently reran 55/55 on PowerShell 7.6 (`20260904-105405-3f986fb274ba4242ab3f7d74fdf6a9e7`) and Windows PowerShell 5.1 (`20260904-105409-f5a46ce504ed4482b67a79dbc7c8f9e4`). PackageDemo now records the actual UAT end time immediately after the process, passes the original diagnostic `Log.txt` and exact process interval to this helper, and retains `pakStorageProof` in its manifest. The one full Cook log still uses RuntimeStorage separately; all original logs still undergo the existing strict warning/error checks. No abbreviated stdout is treated as a native command-line header.

Installed `LooseCookedPackageWriter.cpp` 176-182 and `CookOnTheFlyServer.cpp` 10755/10798-10801 show a full, non-worker loose cook initializes with bFullBuild and deletes its old sandbox. This explains why the supported `-SkipZenStore` full cook should not reuse the old Zen `ue.projectstore`; actual fresh Cook/Stage/Pak output must still demonstrate the intended behavior. No Engine/global file is manually removed or rewritten. Actual BuildCookRun remains pending H7's clean checkpoint.

Independent review of the PackageDemo call and nested evidence graph found no blocking issue. Its returned Path/Sha256/Length records survive PackageGameplay's packageProof and immediate snapshot boundary; comparison-only raw-line normalization does not alter original hashes. Repeated actual entry-point preflight tests passed 96/96 at `ReleaseEntrypoints/20260904-185555-6760798c40304ca2852b2431313a02f9`. This is the script integration checkpoint, not a passing native BuildCookRun.

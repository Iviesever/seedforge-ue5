# SF-IRA-001 — failure lifecycle evidence

## Starting revision and fresh baseline

- HEAD: `baa314d6b067e7661e0c131e056c1e617289ab81` (audit contract; production still audited `18c255d`).
- Fresh Editor build: exit 0, `Artifacts/Logs/build-editor-20260904-102948.log`.
- Fresh full Automation: 2026-09-04 10:30:00.458–10:30:35.334 UTC+8; 63 passed, 0 warnings/failures/not-run/in-process; HEAD unchanged.
- Baseline report: `Artifacts/Reports/automation-20260904-103000/index.json`.

## RED: real invalid-config process

- Command: `Scripts/TestRunFailure.ps1 -Case Grid -TimeoutSeconds 45`.
- Runtime began 10:34:01.686 and outer timeout ended 10:34:46.912 UTC+8.
- Runtime logged `Gameplay generation failed code=1` for grid width 0 at 10:34:12.816.
- Process remained alive until the external test killed it; observed process exit -1, test exit 1, no trace existed.
- Observation/log: `Artifacts/Reports/RunFailure/20260904-103401-609-Grid/observation.json` and `runtime.log`.
- This is diagnostic dirty-tree RED evidence, explicitly labeled `sourceTreeDirty=true`, not a release package manifest.

## RED: World/state contracts

- Only declarations/default fields, a deliberate state transition stub, and a behavior-neutral StartRun return value were added before tests; no failure-handling implementation existed.
- Compile: 10:35:00.995–10:35:21.070, exit 0; `Artifacts/Logs/build-editor-20260904-103501.log`.
- Focused command: `Scripts/Test.ps1 -Filter SeedForge.Audit.RunFailure -TimeoutSeconds 120`.
- Run: 10:35:32.593–10:35:47.421, HEAD unchanged.
- Result: 0 passed, 0 warnings, 6 failed, 0 not-run/in-process; test process wrapper exit 1.
- Report/log: `Artifacts/Reports/automation-20260904-103532/index.json`; `Artifacts/Logs/automation-20260904-103532.log`.
- Failures match missing terminal state/detail/HP cleanup and the zero-request acceptance defect. Real World spawn rejection executed and existing partial cleanup was preserved, but the run stayed Generating.

## Dependency boundary

Coordinator/state recovery is part of this fix. Recovery from configured R/N while no Pawn exists is task B / SF-IRA-003; this finding is not certified end-to-end until B's persistent input tests pass as well.

## GREEN and exit-status root cause

- Centralized `EnterRunFailure` now owns cancellation, generation delegate removal, actor/timer/capture cleanup, HP/cooldown/failed-identity reset, explicit Failed state and typed HUD detail. Recovery rebinds the generation delegate. Stale/non-owned/zero request completions do not alter the run.
- Smoke trace identity and the pre-Playing watchdog are initialized before requesting generation; all startup/apply failures serialize one failed trace.
- First GREEN compile: 10:39:58.447–10:40:45.239 UTC+8, exit 0, `Artifacts/Logs/build-editor-20260904-103958.log`.
- World/state focused GREEN: 10:41:04.039–10:41:34.134, 6/6 passed, zero warnings/failures; `Artifacts/Reports/automation-20260904-104104/index.json`.
- The first process check correctly rejected exit 0 even though a Failed trace was written. Log `Artifacts/Reports/RunFailure/20260904-104148-949-Grid/runtime.log` records `RequestExitWithStatus(0, 2)` followed by clean Engine shutdown returning 0.
- Read-only UE source diagnosis: `FWindowsPlatformMisc::RequestExitWithStatus(false, code)` posts a WM_QUIT message; an early Engine-exit path can leave the loop before processing that status. The dedicated failure subprocess now calls the force/status path only after synchronous owned-run cleanup and trace writing. The platform flushes the log and terminates with status 2. Successful/interactive paths are not force-terminated.
- Corrected compile: 10:42:55.537–10:43:31.410, exit 0, `Artifacts/Logs/build-editor-20260904-104255.log`.
- Grid process GREEN: 10:43:46.796–10:44:06.507, runtime exit 2, no outer timeout, failed trace reparsed; `Artifacts/Reports/RunFailure/20260904-104344-649-Grid/observation.json`.
- Encounter process GREEN: 10:44:06.872–10:44:41.219, runtime exit 2, no outer timeout, failed trace reparsed; `Artifacts/Reports/RunFailure/20260904-104406-718-Encounter/observation.json`.
- Both negative diagnostics explicitly retain `sourceTreeDirty=true`; they are test observations, not mislabeled release packages.

## Relevant regression

| Filter | Start/end UTC+8 | Result | Report |
|---|---|---|---|
| SeedForge.Model.RunState | 10:45:11.130–10:45:31.773 | 5 passed / 0 warnings / 0 failures | `Artifacts/Reports/automation-20260904-104511/index.json` |
| SeedForge.Gameplay | 10:45:31.822–10:45:50.990 | 7 passed / 0 warnings / 0 failures | `Artifacts/Reports/automation-20260904-104531/index.json` |
| SeedForge | 10:45:51.042–10:46:19.133 | 69 passed / 0 warnings / 0 failures | `Artifacts/Reports/automation-20260904-104551/index.json` |

All three commands returned 0, recorded unchanged HEAD `baa314d6b067e7661e0c131e056c1e617289ab81`, and had 0 not-run/in-process. Repository/diff audit passed.

## Remaining independent log review

The negative Editor process also exposed a UE `LogClass` initialization diagnostic for `FDataflowToolNodeSnapshot::Date` outside the requested test events. It is not hidden or counted as an approved exception here. SF-IRA-002/Gate 14 must classify and resolve this whole-process log issue before release; an Automation summary with zero test warnings is not sufficient to certify it.

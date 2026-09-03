# SeedForge Verification Evidence

## PACT-00 — Repository and UE smoke baseline

Status: **Passed** on 2026-09-03 (UTC+8).

### Build

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/Build.ps1`
- UE: 5.8.0, changelist 55116800
- Toolchain: MSVC 14.44.35228
- Windows SDK: 10.0.26100.0
- Cold build result: 12/12 actions succeeded in 87.84 seconds.
- Incremental script verification: target up to date, exit code 0 in 3.79 seconds.
- Evidence:
  - `Artifacts/Logs/build-editor-20260903-121245.log`
  - `Artifacts/Logs/build-editor-20260903-122144.log`
  - `Artifacts/Logs/ubt-editor-20260903-122144.log`

The local Unreal Build Accelerator could not bind its optional network listener on port 1345 and continued with the local executor. This did not affect compilation or linking.

### Headless smoke

- Initial check: ordinary Editor startup completed engine initialization in 16.76 seconds and received `Cmd: Quit`, but its process remained in the Editor main loop until the 180-second harness timeout. This proved module compatibility but failed the deterministic-exit contract.
- Root cause boundary: `-ExecCmds=Quit` was attached to ordinary Editor startup rather than a naturally terminating commandlet.
- Corrected check: the built-in `CompileAllBlueprints` commandlet receives a project-local allow list containing only a nonexistent sentinel asset. It loads the project and plugin, performs no engine-wide compilation, reports zero errors/warnings, and exits naturally.
- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/Smoke.ps1`
- Result: exit code 0; required success marker present; no fatal/plugin incompatibility marker.
- Evidence:
  - Failed contract reproduction: `Artifacts/Logs/smoke-editor-20260903-121426.log`
  - Successful probe: `Artifacts/Logs/smoke-commandlet-probe.log`
  - Passing bounded smoke: `Artifacts/Logs/smoke-editor-20260903-122058.log`

### Baseline conclusion

The project module and SeedForge runtime plugin compile with the installed UE 5.8 binary engine and load in an unattended commandlet. PACT-01 may begin from commit `codex/seedforge-implementation` after the scaffold commit is recorded.

## PACT-01 — Deterministic generation core, first red-green cycle

Status: **Initial behavior passed** on 2026-09-03 (UTC+8). Further boundary/property coverage continues under PACT-02.

### RED

- Three tests were compiled and discovered:
  - `SeedForge.Core.RejectsInvalidGrid`
  - `SeedForge.Core.RejectsInvalidRoomRange`
  - `SeedForge.Core.SameSeedIsDeterministic`
- The first correctly quoted Automation run executed all three against the `NotImplemented` stub and produced `failed=3`.
- The test harness itself initially returned a false success because UnrealEditor exits with status 0 even when Automation reports failures. The harness was corrected to parse `index.json`, require at least one executed test, and reject failed/not-run/in-process/warning counts.
- Confirmed failing harness evidence:
  - `Artifacts/Logs/automation-20260903-123026.log`
  - `Artifacts/Reports/automation-20260903-123026/index.json`

### GREEN

- Implemented only the behavior demanded by the three tests:
  - typed validation for invalid grid and room-size ranges;
  - bounded deterministic room placement;
  - canonical room/corridor ordering;
  - deterministic L-shaped connectivity;
  - explicit entrance/exit selection;
  - byte-defined FNV-1a canonical hashing.
- Build result: succeeded with MSVC 14.44.35228 and no SeedForge compile warning in 32.08 seconds.
- Automation result: `succeeded=3`, `failed=0`, `notRun=0`, `inProcess=0`.
- Evidence:
  - `Artifacts/Logs/build-editor-20260903-123236.log`
  - `Artifacts/Logs/automation-20260903-123318.log`
  - `Artifacts/Reports/automation-20260903-123318/index.json`


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


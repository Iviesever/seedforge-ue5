# Build environment confinement prelude

> **Execution:** `executing-tasks` inline with test-first changes, before further UE verification.

**Goal:** Keep UBA storage and DDC in the project for every UE/UAT/UBT entry point, including inherited Editor child UBT. Disable unused UBA remote execution where the stock build command exposes scoped UBT arguments; do not suppress diagnostics or change compilation targets.

**Authority:** User approved only `C:\Users\Iviesever\AppData\Local\UnrealBuildTool\Trace*.uba` as an internal diagnostic exception. Engine remains read-only. Do not create XML environment overrides that would change UE's global environment config file.

**Architecture (corrected after real integration RED):** A common `Scripts/BuildEnvironment.ps1` function validates the project root and sets process-scoped project-local `UBA_ROOT` and DDC paths. It does not inspect or modify `UBT_EXTRA_ARGS`: recursive metadata/tool modes reject Build-only options. Build.ps1 passes bare `-UBADisableRemote` directly to Build mode; PackageDemo passes it via BuildCookRun's documented UbtArgs. All 11 direct UE/UAT launch scripts initialize inherited cache roots. No machine/user persistent environment changes.

Stock UE 5.8 BuildPlugin does not expose an equivalent arbitrary UBT argument option; its cache root is confined now, but its remote-listener diagnostic/configuration remains part of G's actual BuildPlugin log gate. Do not invent an ignored flag, patch UAT, or waive that gate here.

## Steps

- [x] Add an empty function declaration plus a PowerShell acceptance harness at `tasks/20260904-102407-phase3-independent-release-audit/verify-build-environment.ps1`. RED proves missing environment effects and launcher wiring, not syntax failure.
- [x] Harness verifies exact project-local roots, byte-for-byte preservation of opaque/quoted UBT_EXTRA_ARGS, idempotent cache setup, child-process inheritance, all direct launcher calls and scoped Build/BuildCookRun flags. Restore original process environment in a finally block.
- [x] Implement `Initialize-SeedForgeBuildEnvironment -ProjectRoot <repo>` with native PowerShell environment operations and no Engine/global configuration edits. Reject nonexistent/non-project roots before creating directories.
- [x] Wire Build, Test, Smoke, Report, CreateDemoMap, CaptureDemo, CaptureInspector, PackageDemo, PackagePlugin, TestGameplay and TestRunFailure. Replace their repeated DDC assignment with the common call, preserving all other behavior.
- [x] GREEN PowerShell harness; parse every changed script; Build; full Automation. Check fresh build output no longer reports the unused remote socket-bind attempt and verify local cache creation. Recheck the global environment XML remains unchanged.
- [x] Record exact commands/results and authorization, review the localized diff, audit repository, and commit `fix(build): confine UBA storage and disable unused remote execution`.

This prelude does not close SF-IRA-002 revision provenance, archives/manifests, or negative release-verifier testing. Those remain G.

# PACT-31 Evidence

## RED

- Contract base: `83e76727ea19e1683d8111b48de05ccb610076b9`.
- Added public value API declarations plus deliberate non-functional stubs for encounter planning, grid A*, and run state.
- Added 15 real Automation tests under `SeedForge.Model` covering deterministic encounter identity/placement/failures/tampering, path success/tie/status/budget contracts, and run win/loss/restart/illegal transitions.
- First focused run: 1 passed and 14 failed because the path stub's default `InvalidInput` accidentally satisfied the invalid-input contract. This did not qualify as complete RED.
- First log: `Artifacts/Logs/automation-20260903-191920.log`.
- Stub-only correction: return an empty `Success` path so no requested path behavior is accidentally implemented.
- Compile command: `Scripts/Build.ps1`.
- Compile result: passed with 11 actions for the initial contract and 4 actions after the stub-only correction.
- Build logs: `Artifacts/Logs/build-editor-20260903-191851.log` and `Artifacts/Logs/build-editor-20260903-192008.log`.
- Authoritative RED command: `Scripts/Test.ps1 -Filter SeedForge.Model -TimeoutSeconds 900`.
- Authoritative RED result: 0 passed, 0 warnings, 15 failed, 0 not-run, 0 in-process.
- Authoritative RED log: `Artifacts/Logs/automation-20260903-192030.log`.
- Authoritative RED report: `Artifacts/Reports/automation-20260903-192030`.

No encounter selection, canonical encounter hash, A* search, or run-state transition behavior exists at this checkpoint.

# PACT-32 Evidence

## RED

- PACT-31 GREEN commit: `81a77a2fbcd0bc10068118f5fe29037c177a8e40`.
- Added code-native class/API contracts for a player Character, PlayerController, enemy Pawn, Core pickup, exit, UCanvas HUD, gameplay coordinator, cell/world mapping, stable attack selection, and apply-only layout visualization.
- Added four focused Automation tests under `SeedForge.Gameplay` for cell mapping/tie-breaks, attack selection/filtering, framework inheritance/components/cooldowns, and PreviewActor apply-only mode.
- First focused run: 1 passed and 3 failed because the attack stub's `INDEX_NONE` accidentally satisfied the invalid-attack contract. This did not qualify as complete RED.
- First log: `Artifacts/Logs/automation-20260903-193616.log`.
- Stub-only correction: return fixed candidate index `0`, which implements neither valid stable selection nor invalid-input rejection.
- Compile command: `Scripts/Build.ps1`.
- Compile result: passed after both initial contracts and the stub-only correction.
- Build logs: `Artifacts/Logs/build-editor-20260903-193540.log` and `Artifacts/Logs/build-editor-20260903-193647.log`.
- Authoritative RED command: `Scripts/Test.ps1 -Filter SeedForge.Gameplay -TimeoutSeconds 900`.
- Authoritative RED result: 0 passed, 0 warnings, 4 failed, 0 not-run, 0 in-process.
- Authoritative RED log/report: `Artifacts/Logs/automation-20260903-193656.log`; `Artifacts/Reports/automation-20260903-193656`.

No playable input, camera, actor presentation, combat, pursuit, pickup, exit, HUD, or coordinator behavior exists at this checkpoint.

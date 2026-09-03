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
- Before coordinator implementation, a fifth contract created a real transient Game `UWorld`, passed a generated layout through the production `ApplyGeneratedLayout` boundary, and required 3 Core/5 Enemy actors, full HP, `Playing`, and lethal-damage `Lost`.
- Coordinator World RED result: 0 passed, 1 failed against the stub.
- Coordinator World RED log/report: `Artifacts/Logs/automation-20260903-193905.log`; `Artifacts/Reports/automation-20260903-193905`.

No playable input, camera, actor presentation, combat, pursuit, pickup, exit, HUD, or coordinator behavior exists at this checkpoint.

## GREEN

- `ASeedForgeGameplayCoordinator` is the sole run owner. It consumes the existing newest-request-wins WorldSubsystem result, validates the layout, generates the encounter plan, spawns actors in stable order, owns HP/combat/proximity/path timers, and clears all owned actors/timers/delegates on restart/end play.
- Added code-native `ACharacter` player with spring-arm top-down camera, text-configured WASD/mouse/attack/dash/restart/new-seed input, visible attack pulse, and dash cooldown.
- Added stable-ID enemy Paws whose Tick only follows supplied waypoints; the coordinator performs bounded A* replans at 2 Hz and rate-limited contact damage.
- Added code-native Core pickups, locked/unlocked exit, distinct runtime-created color materials, blocking HISM floor/walls, and UCanvas HUD for HP/Core/Seed/Run/controls/terminal prompts.
- The demo GameMode now creates the gameplay coordinator and uses the code-native PlayerController/HUD; no Blueprint or hand-authored input asset is required.
- Input choice: committed Action/Axis mappings remain plain text while UE's Enhanced PlayerInput/InputComponent classes remain the engine-selected defaults. Legacy mapping deprecation logging is disabled narrowly because the mappings are intentional and asset-free.
- Initial GREEN build: passed.
- Initial GREEN build log: `Artifacts/Logs/build-editor-20260903-194557.log`.
- Initial focused result: 5 of 5 Gameplay tests passed with zero warnings/failures.
- Initial focused log/report: `Artifacts/Logs/automation-20260903-194618.log`; `Artifacts/Reports/automation-20260903-194618`.

## Normal launch and visual inspection

- Normal-path command launched `UnrealEditor.exe ... -game -RenderOffscreen -SeedForgeSeed=24301` without any smoke/capture flag, waited for the real async ready marker, then the external harness terminated the bounded observation.
- Normal path reached: `players=1 cores=3 enemies=5 exits=1`, layout hash `7425849530159566348`, encounter hash `15303214708604970503`.
- Normal path log: `Artifacts/Logs/gameplay-normal-editor-20260903-194701.log`.
- No SeedForge error, generic error, or fatal marker was present. UE emitted known machine/Editor warnings for the installed GPU driver and Editor Data Storage UI; these are not allow-listed as project success and remain visible for final log policy.
- The existing `Scripts/CaptureDemo.ps1 -Seed 24301` path was preserved by moving capture ownership to the gameplay coordinator after ready.
- Capture result: process exit 0; screenshot exists at 1280x720 and is 388,156 bytes.
- Screenshot: `Artifacts/Media/SeedForge-24301.png`.
- Capture log: `Artifacts/Logs/capture-demo-20260903-194855.log`.
- Manual image inspection: top-down player, generated floor/wall collision space, HP 100/100, Core 0/3, seed 24301, Playing state, locked exit, WASD/mouse/attack/dash and restart/new-seed controls are visible without clipping or overlap.

## Regression and standalone targets

- Post-capture build: passed; log `Artifacts/Logs/build-editor-20260903-194815.log`.
- Post-capture focused result: 5 of 5 passed with zero warnings; log/report `Artifacts/Logs/automation-20260903-194835.log`; `Artifacts/Reports/automation-20260903-194835`.
- Full command: `Scripts/Test.ps1 -Filter SeedForge -TimeoutSeconds 900`.
- Full result: 61 of 61 passed, 0 warnings, 0 failures, 0 not-run, 0 in-process. All original 41 tests and PACT-31's 15 tests remain green.
- Full log/report: `Artifacts/Logs/automation-20260903-194950.log`; `Artifacts/Reports/automation-20260903-194950`.
- Independent command: `Scripts/PackagePlugin.ps1`.
- BuildPlugin result: passed UnrealEditor Win64 Development, UnrealGame Win64 Development, and UnrealGame Win64 Shipping.
- BuildPlugin log: `Artifacts/Logs/package-plugin-20260903-195024.log`.
- Pre-version-bump plugin archive: `Artifacts/Release/SeedForgePlugin-0.2.0-20260903-195024.zip`, SHA-256 `815533b219724a7e6e01dba1bc410b533aefd841098d070ec0a5114c16786942`.
- Repository audit after GREEN: passed with 121 tracked files and 59 source/build-rule files.

PACT-33 remains responsible for the authoritative Collect -> unlock -> Won production smoke, JSON trace, multiple gameplay captures, ordinary packaged launch, packaged smoke, and full Win64 BuildCookRun gate.

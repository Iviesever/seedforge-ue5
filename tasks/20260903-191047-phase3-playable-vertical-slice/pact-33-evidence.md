# PACT-33 Evidence

## RED

- PACT-32 GREEN commit: `7f6feddfdf86f40165ac6818948c57a4ae7af488`.
- Added a gameplay smoke trace contract with Git SHA, UE version, exact uint64 seed/layout/encounter identities, actor counts, ordered state transitions/actions/screenshots, and explicit pass/failure fields.
- Added two real Automation tests under `SeedForge.GameplaySmoke` for canonical success JSON and escaped typed failure JSON.
- Deliberate serializer stub returns only `{}`.
- Compile command: `Scripts/Build.ps1`.
- Compile result: passed.
- Build log: `Artifacts/Logs/build-editor-20260903-195610.log`.
- RED command: `Scripts/Test.ps1 -Filter SeedForge.GameplaySmoke -TimeoutSeconds 900`.
- RED result: 0 passed, 0 warnings, 2 failed, 0 not-run, 0 in-process.
- RED log/report: `Artifacts/Logs/automation-20260903-195635.log`; `Artifacts/Reports/automation-20260903-195635`.

No trace serialization, runtime smoke state driver, JSON output, packaged gameplay smoke, or Phase 3 aggregate script exists at this checkpoint.

## GREEN: trace and runtime smoke

- Implemented fixed-order JSON with escaped strings and decimal-string `uint64` identities for seed, layout hash, and encounter hash.
- First smoke implementation compile reached one UE 5.8 API mismatch: `IPlatformFile::FileSize` is non-const. Changing only the local platform-file reference qualifier resolved it.
- Failed compile log: `Artifacts/Logs/build-editor-20260903-200219.log`.
- Corrected build log: `Artifacts/Logs/build-editor-20260903-200304.log`.
- Serializer GREEN: 2 of 2 `SeedForge.GameplaySmoke` tests passed with zero warnings/failures.
- Serializer GREEN log/report: `Artifacts/Logs/automation-20260903-200313.log`; `Artifacts/Reports/automation-20260903-200313`.
- Runtime `-SeedForgeGameplaySmoke` validates the production encounter and actor identities, waits for viewport readiness, performs two attacks through the real cooldown/damage API, kills one enemy, collects all three real Core actors through proximity, observes exit unlock, reaches the real exit, and verifies `Won`.
- The runtime writes an explicit failure trace where possible and calls `RequestExitWithStatus` non-zero on invalid arguments, missing/mismatched actors, attack/Core/unlock/win failures, screenshot/trace failure, or a 30-second watchdog.

## Editor smoke and visual QA

- `Scripts/TestGameplay.ps1` launches the Editor game, passes exact Git SHA/output paths, requires exit 0, reparses JSON, checks exact counts/state/actions, checks three project-contained PNGs over 10 KiB, rejects all error/fatal markers, and rejects every warning outside a minimal exact engine allow-list.
- First script invocation did not launch UE because a multiline PowerShell cast condition failed to parse. Explicit parentheses corrected only the script syntax.
- The first full runtime smoke passed its state/trace/screenshots but the warning gate rejected UE 5.8's exact `r.MotionVectorSimulation` render-thread warning. That single engine-owned text was added to the allow-list; no project warning/error pattern was relaxed.
- Visual QA on the first passing images found that screenshots were requested before viewport/camera stabilization. A one-second initial warm-up, fixed absolute camera-arm rotation, and a half-second combat staging boundary corrected the frames without changing gameplay state APIs.
- Authoritative Editor smoke command: `Scripts/TestGameplay.ps1 -Seed 24301 -TimeoutSeconds 180`.
- Authoritative Editor smoke result: passed with trace reparse, 0 unexpected warnings, and 0 error/fatal markers.
- Authoritative trace/summary: `Artifacts/Reports/Gameplay/20260903-201302/gameplay-smoke.json`; `Artifacts/Reports/Gameplay/20260903-201302/summary.json`.
- Authoritative log: `Artifacts/Logs/gameplay-smoke-editor-20260903-201302.log`.
- Trace identity: seed `24301`, layout hash `7425849530159566348`, encounter hash `15303214708604970503`, actors `1/3/5/1`, states `Generating -> Playing -> Won`, actions `Attack`, `Kill`, three `Collect`, `ExitUnlocked`, `ReachExit`.
- Visually inspected gameplay captures: `Artifacts/Media/Gameplay/20260903-200909/SeedForge-Gameplay-start-24301.png` (388,311 bytes), `SeedForge-Gameplay-combat-24301.png` (655,015 bytes), and `SeedForge-Gameplay-win-24301.png` (720,556 bytes).
- Start shows the stable HUD and spawn room; combat shows enemy/player/attack pulse in a walkable room; win shows 3/3, unlocked exit, terminal prompt, and nearby actors with no HUD clipping.

## Automation and scripts

- Post-smoke focused result: 2 of 2 passed with zero warnings; log/report `Artifacts/Logs/automation-20260903-201150.log`; `Artifacts/Reports/automation-20260903-201150`.
- Full command: `Scripts/Test.ps1 -Filter SeedForge -TimeoutSeconds 900`.
- Full result: 63 of 63 passed, 0 warnings, 0 failures, 0 not-run, 0 in-process.
- Full log/report: `Artifacts/Logs/automation-20260903-201213.log`; `Artifacts/Reports/automation-20260903-201213`.
- Added `CaptureGameplay.ps1` as the public capture wrapper, `PackageGameplay.ps1` to reuse one existing BuildCookRun then verify ordinary launch and packaged production smoke, and `VerifyPhase3.ps1` as the final sequential clean-revision gate.
- All four new PowerShell scripts parse successfully.

The runtime/scripts implementation is committed before authoritative BuildPlugin/BuildCookRun so binary manifests can name the exact source revision rather than a dirty-tree parent. Package evidence follows below after that clean checkpoint.

## Clean-revision BuildPlugin and Win64 package

- Smoke implementation commit and clean package revision: `9ff53d407e6525af96a92fedb35c51532a48b39c`.
- Independent BuildPlugin passed UnrealEditor Win64 Development, UnrealGame Win64 Development, and UnrealGame Win64 Shipping.
- BuildPlugin log: `Artifacts/Logs/package-plugin-20260903-201442.log`.
- Plugin archive: `Artifacts/Release/SeedForgePlugin-0.2.0-20260903-201442.zip`.
- Plugin SHA-256: `ef6c351fea863ac38d0403f89d5fa64108fe63860fcc8704bff790fd60d01c4e`.
- BuildCookRun passed Build, Cook, Stage, Pak, and Archive; AutomationTool reported success and Cook reported 0 errors / 0 warnings.
- BuildCookRun log: `Artifacts/Logs/package-demo-20260903-201635.log`.
- Win64 package directory: `Artifacts/Package/SeedForge-Win64-20260903-201635`.
- Demo archive: `Artifacts/Release/SeedForgeDemo-Win64-0.2.0-20260903-201635.zip`.
- Demo SHA-256: `88753b793f6f848b2ccdf760836a319b6c1a6aef8cfa81b03a451ef0b849d057`.

## Ordinary and smoke packaged execution

- The existing non-smoke `-SeedForgeCapturePath` packaged path reached gameplay ready, captured `Artifacts/Media/SeedForge-Packaged-24301.png` (400,267 bytes), and exited zero.
- Packaged gameplay smoke exited zero, reparsed its JSON, and reported 0 unexpected warnings and 0 error/fatal markers. Four exact engine/machine warnings were allowed and remain counted.
- Packaged trace/summary: `Artifacts/Reports/Gameplay/20260903-201808/gameplay-smoke.json`; `Artifacts/Reports/Gameplay/20260903-201808/summary.json`.
- Packaged smoke log: `Artifacts/Logs/gameplay-smoke-packaged-20260903-201808.log`.
- Packaged trace exactly matches revision `9ff53d407e6525af96a92fedb35c51532a48b39c`, seed `24301`, layout hash `7425849530159566348`, encounter hash `15303214708604970503`, actors `1/3/5/1`, states `Generating -> Playing -> Won`, and the seven required attack/kill/collect/unlock/exit actions.
- Packaged start/combat/win PNGs exist and exceed 400 KiB each under `Artifacts/Media/Gameplay/20260903-201808`.
- Manual packaged visual QA: start and win frames have complete HUD and world presentation. Combat shows the production enemy/player/attack pulse, but its HUD left edge is clipped. After two timing/camera corrections, further guessing stopped and `root-cause-20260903-packaged-combat-capture.md` records the non-blocking P1 limitation and a render-owned future repair.
- Authoritative manifest: `Artifacts/Package/last-gameplay-package.json`.

UAT's initial run selected its installed-engine AutomationTool `Saved` directory for internal diagnostic files despite all project-facing outputs being under SeedForge. No Engine file was edited, removed, or committed manually. `PackagePlugin.ps1` and `PackageDemo.ps1` now set `uebp_LogFolder` and `uebp_FinalLogFolder` to unique project-local `Artifacts/Logs/uat-*` directories; the final PACT-34 verification will prove this redirection.

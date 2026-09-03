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

## PACT-02 — Topology validation and property verification

Status: **Passed** on 2026-09-03 (UTC+8).

### RED

- Added five validator tests for out-of-bounds rooms, overlapping rooms, disconnected walkable space, acceptance of generated layouts, and a deterministic 1,000-seed property sweep.
- Against the validator stub, Automation discovered all five and reported `failed=5`.
- Evidence:
  - `Artifacts/Logs/automation-20260903-123547.log`
  - `Artifacts/Reports/automation-20260903-123547/index.json`

### GREEN

- Implemented ordered validation for room count, room bounds, pairwise room overlap, corridor bounds, entrance/exit membership, and four-neighbor connectivity.
- The first compile exposed that UE 5.8's `FIntPoint` constructor is not `constexpr`; changing only the local direction array from `constexpr` to `const` resolved the documented compiler error.
- The five validator tests then passed, including all 1,000 deterministic seeds.
- Evidence:
  - Compile failure: `Artifacts/Logs/build-editor-20260903-123706.log`
  - Corrected build: `Artifacts/Logs/build-editor-20260903-123742.log`
  - Passing validation: `Artifacts/Logs/automation-20260903-123807.log`
  - Passing report: `Artifacts/Reports/automation-20260903-123807/index.json`

### Golden hashes and repeatability

The canonical implementation was characterized once through UE Automation, then the resulting values were locked into regression tests:

| Seed | Canonical hash |
|---:|---:|
| 0 | 3488165859926780287 |
| 1 | 8479853380352986717 |
| 24301 (`0x5EED`) | 7425849530159566348 |
| 12648430 (`0xC0FFEE`) | 7770407528328499089 |
| 18446744073709551615 | 5108722159798011553 |

- Characterization evidence: `Artifacts/Logs/automation-20260903-123941.log`
- Final aggregate verification: Automation passed 10 tests, with zero warnings/failures/not-run/in-process tests.
- Aggregate evidence:
  - `Artifacts/Logs/automation-20260903-124130.log`
  - `Artifacts/Reports/automation-20260903-124130/index.json`
- Five golden seeds each reproduced the entire canonical layout 100 times synchronously.

## PACT-03 — Asynchronous lifecycle safety

Status: **Passed** on 2026-09-03 (UTC+8).

### Coordinator RED and GREEN

- Five initial coordinator tests covered worker/game-thread separation, in-flight cancellation, stale-result suppression, immediate cancellation, and shutdown suppression.
- With the request-id-only stub, Automation reported `succeeded=2`, `failed=3`; completion, in-flight cancellation observation, and newest-result application correctly failed.
- RED evidence:
  - `Artifacts/Logs/automation-20260903-124725.log`
  - `Artifacts/Reports/automation-20260903-124725/index.json`
- Implemented `FSeedForgeAsyncCoordinator` with `UE::Tasks::Launch`, atomic cancellation, monotonic request ids, shared lifetime state, and a Game Thread apply boundary.
- GREEN evidence:
  - `Artifacts/Logs/automation-20260903-124937.log`
  - `Artifacts/Reports/automation-20260903-124937/index.json`

### World subsystem RED and GREEN

- Added integration tests for successful `USeedForgeWorldSubsystem` forwarding and `Deinitialize` suppression.
- The subsystem stub produced `succeeded=1`, `failed=1`; the success path correctly failed on request id zero and timed out without a callback.
- RED evidence:
  - `Artifacts/Logs/automation-20260903-125308.log`
  - `Artifacts/Reports/automation-20260903-125308/index.json`
- The subsystem now lazily owns the coordinator, captures itself weakly at the apply boundary, clears delegates, shuts down outstanding work, and destroys the coordinator during deinitialization.
- GREEN evidence:
  - `Artifacts/Logs/automation-20260903-125440.log`
  - `Artifacts/Reports/automation-20260903-125440/index.json`

### Required asynchronous repetition

- `SeedForge.Async.HundredSequentialRepetitions` launches 100 distinct `UE::Tasks` requests serially.
- Every result returns to the Game Thread and compares its complete canonical layout with the synchronous `0x5EED` baseline.
- The final async suite passed 8 tests; the complete SeedForge suite passed 18 tests with zero warnings/failures/not-run/in-process tests.
- Evidence:
  - `Artifacts/Logs/automation-20260903-125625.log`
  - `Artifacts/Reports/automation-20260903-125625/index.json`
  - `Artifacts/Logs/automation-20260903-125706.log`
  - `Artifacts/Reports/automation-20260903-125706/index.json`

### Compile diagnostics resolved before GREEN

- Latent Automation command declarations initially lacked the semicolon required by UE's macro expansion; only test syntax was changed.
- Direct UObject use in `SeedForgeTests` initially exposed missing `CoreUObject` and `Engine` module dependencies at link time; only the test module manifest was corrected.
- The production runtime module linked successfully throughout those test-harness corrections.

## PACT-04 — Pure C++ graybox demo

Status: **Passed in Editor game mode** on 2026-09-03 (UTC+8). Packaged-game verification remains in PACT-05.

### Visualization RED and GREEN

- Two pure planner tests specify that one walkable cell creates one floor plus four exterior walls, while two adjacent cells create two floors plus six exterior walls with no duplicated shared edge.
- The empty planner produced `failed=2`.
- RED evidence:
  - `Artifacts/Logs/automation-20260903-125943.log`
  - `Artifacts/Reports/automation-20260903-125943/index.json`
- The canonical planner now emits deterministic floor/wall transforms and bounds from canonical walkable cells.
- GREEN evidence:
  - `Artifacts/Logs/automation-20260903-130112.log`
  - `Artifacts/Reports/automation-20260903-130112/index.json`

### Generated map and runtime path

- `Scripts/CreateDemoMap.py` created and saved the only project-owned map automatically through UE's Python commandlet path.
- Map: `Content/Maps/SeedForgeDemo.umap` (6,379 bytes).
- Generation evidence: `Artifacts/Logs/create-demo-map-20260903-130551.log`.
- `ASeedForgePreviewActor` requests generation through the world subsystem, validates the result, converts it to HISM transforms, and logs all observable metrics.
- `ASeedForgeDemoGameMode` creates engine-native light/camera actors. Interactive runs use the engine `ADefaultPawn` free-fly controls; capture runs switch to a fixed camera and hide the pawn.

### End-to-end visual verification

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/CaptureDemo.ps1`
- Final seed: 24301 (`0x5EED`)
- Canonical hash: 7425849530159566348
- Applied instances: 587 floors, 436 boundary walls
- Observed end-to-end async/apply time: 419.464 ms on the recorded run
- Screenshot: `docs/images/seedforge-24301.png` (1,280 x 720, visually inspected)
- Runtime evidence: `Artifacts/Logs/capture-demo-20260903-131138.log`
- The complete suite after scene integration passed 20 tests with zero warnings/failures/not-run/in-process tests:
  - `Artifacts/Logs/automation-20260903-130918.log`
  - `Artifacts/Reports/automation-20260903-130918/index.json`

## PACT-05 — Repeatable build, test, and distribution

Status: **RC1 passed** on 2026-09-03 (UTC+8). A clean-revision RC2 will be produced after documentation and final verification.

### Independent plugin package

- Initial `BuildPlugin` exposed two standalone-target dependencies that the project Editor build had hidden:
  - the test module was eligible for UnrealGame builds instead of being Editor-only;
  - the runtime preview actor relied on a transitive `UStaticMesh` include.
- The test module was constrained to `Editor`, and Runtime now includes its complete `UStaticMesh` dependency explicitly.
- The second `RunUAT BuildPlugin` passed independent HostProject builds for:
  - UnrealEditor Win64 Development;
  - UnrealGame Win64 Development;
  - UnrealGame Win64 Shipping.
- Plugin archive: `Artifacts/Release/SeedForgePlugin-0.1.0-20260903-131805.zip` (39,760,188 bytes).
- Plugin SHA-256: `065e75adc890221611b95754b881fc335da12950fec387a91764a269c8bf5acc`.
- Evidence:
  - Failed standalone dependency discovery: `Artifacts/Logs/package-plugin-20260903-131527.log`
  - Passing package: `Artifacts/Logs/package-plugin-20260903-131805.log`
  - UAT log: `Artifacts/Logs/uat-package-plugin-20260903-131805.log`

### Win64 RC1 and packaged smoke

- `BuildCookRun` completed Build, Cook, Stage, Pak, and Archive in 83.09 seconds.
- The packaged `SeedForge.exe` launched outside the Editor, generated seed 24301, reproduced canonical hash 7425849530159566348, applied 587 floor and 436 wall instances, captured a 1,280 x 720 PNG, and requested clean exit.
- Packaged runtime end-to-end async/apply time in this warm run: 6.855 ms.
- Demo archive: `Artifacts/Release/SeedForgeDemo-Win64-0.1.0-20260903-132041.zip` (405,934,413 bytes).
- Demo SHA-256: `6fdd8168de82dac4fafd98898e8a189968e87c1ccd1c2fe857a73b05fee063e1`.
- Evidence:
  - `Artifacts/Logs/package-demo-20260903-132041.log`
  - `Artifacts/Logs/uat-package-demo-20260903-132041.log`
  - `Artifacts/Logs/smoke-packaged-20260903-132041.log`
  - `Artifacts/Media/SeedForge-Packaged-24301.png` (visually inspected)
  - `Artifacts/Package/last-package.json`

### Post-package regression

- After the standalone-target dependency corrections, the complete Editor Automation suite passed 20 tests again with zero warnings/failures/not-run/in-process tests.
- Evidence:
  - `Artifacts/Logs/automation-20260903-132321.log`
  - `Artifacts/Reports/automation-20260903-132321/index.json`

RC1 was produced while the packaging scripts and standalone-target include corrections were still uncommitted; its manifest therefore names prior last-known-good revision `69f0a81`. RC1 is retained as a verified fallback, while RC2 must be produced from a clean committed revision before final delivery.

## Extended deterministic verification

Status: **Passed** on 2026-09-03 (UTC+8).

- Added explicit coverage for invalid room counts, negative padding, insufficient attempt budgets, bounded placement exhaustion, and representative seed divergence.
- Expanded the deterministic property sweep from 1,000 to 10,000 consecutive seeds.
- The complete suite now passes 25 tests with zero warnings/failures/not-run/in-process tests.
- Evidence:
  - `Artifacts/Logs/build-editor-20260903-132701.log`
  - `Artifacts/Logs/automation-20260903-132748.log`
  - `Artifacts/Reports/automation-20260903-132748/index.json`

## Aggregate verification diagnostic

- The first clean-revision `VerifyAll.ps1` attempt passed repository audit, Editor build, and headless smoke, then correctly stopped at Automation because one test was marked `SucceededWithWarnings`.
- The warning was an unrelated UE 5.8 Editor Home Screen request to `https://www.google.com/generate_204` timing out during the long async test event window.
- Engine source identified the owning `HomeScreen.EnableHomeScreen` CVar. The project disables the Home Screen for deterministic automation rather than weakening the warning gate.
- The same complete Automation check then passed 25 tests with zero warnings/failures/not-run/in-process tests.
- Evidence:
  - Warning run: `Artifacts/Logs/automation-20260903-133747.log`
  - Warning report: `Artifacts/Reports/automation-20260903-133747/index.json`
  - Corrected run: `Artifacts/Logs/automation-20260903-134032.log`
  - Corrected report: `Artifacts/Reports/automation-20260903-134032/index.json`

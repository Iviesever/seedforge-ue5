# SF-IRA-005 render-owned capture and PNG proof plan

> **Execution:** `executing-tasks` with TDD. A bounded independent PowerShell PNG-validator subtask may run under `dispatching-parallel-agents`; only the primary agent runs UE and changes Runtime code.

**Goal:** Exactly three fresh, decodable 1280x720 gameplay images correspond to owned rendered states and completed screenshot callbacks, with no clipped production HUD. Ordinary single-capture mode also exits only after actual capture completion.

**Starting revision:** `15d051b6e12535566a30422f0d76dec992aca265`. Clean-commit Editor smoke passed at `Artifacts/Reports/Gameplay/20260904-120630/summary.json`. All three images were inspected at original resolution: start HUD is intact, combat HUD is clipped at the left, and win geometry has transient motion blur. These images are baseline observations, not accepted final visual evidence.

## Contract and ownership

- Add a value-type capture lifecycle and one ActorComponent owned by the Coordinator. It owns exactly one token/run/label/path request and all viewport/screenshot delegate handles. No second gameplay/state authority.
- States: Idle → AwaitRenderedFrame → AwaitPixels → AwaitProcessed → Complete. Invalid/out-of-order/wrong-token/wrong-run/wrong-path/wrong-size callbacks cannot complete a request. Cancel clears ownership and prevents late completion.
- Each request gets a fresh GUID token and unique PNG filename. Record token, label, path, run generation, applied request ID, requested/rendered/captured/completed frame counters, requested/completed UTC, width/height and file bytes in the trace.
- Bind to `UGameViewportClient::OnViewportRendered()` and require the owning World viewport. UE 5.8 broadcasts this after `Canvas.Flush_GameThread()` and before `ProcessScreenShots`; only at this boundary call RequestScreenshot for the intended state.
- Bind to actual `UGameViewportClient::OnScreenshotCaptured()`. Validate current FScreenshotRequest filename, captured token/run, dimensions and pixel count; save the received production-rendered bitmap through `FImageUtils::SaveImageByExtension`. The viewport delegate intercepts default file writing, so the component owns the write.
- Complete only from `FScreenshotRequest::OnScreenshotRequestProcessed()` after matching pixels and successful PNG write. Validate file size once there, not by polling. Unregister all delegates before notifying the Coordinator. Only reset a global screenshot request if its filename is this component's owned path.
- Reject an already-busy global screenshot request or pre-existing external screenshot interceptor. Use an 8-second watchdog as a bounded failure path, never Sleep/Retry growth.
- Cancel on success, failure, restart, and EndPlay; exit requests remain idempotent. Preserve production HUD/state machine in rendered images.

## Native files / interfaces

- New `Public/SeedForgeCaptureTypes.h`, `Private/SeedForgeCaptureTypes.cpp`: request/receipt and pure lifecycle. `Public/SeedForgeGameplayCapture.h`, `Private/SeedForgeGameplayCapture.cpp`: `USeedForgeGameplayCaptureComponent` adapter.
- Lifecycle input methods: `Begin(Request)`, `OnRendered(Token, Run, Frame)`, `OnPixels(Token, Run, Path, Size, PixelCount, Frame)`, `OnProcessed(Token, Run, bSaved, CompletedUtc, Frame, FileBytes)`, `Cancel()`. Return bool and expose readonly phase/receipt. Token and run must match the active request at every step.
- Component methods: `RequestCapture(Label, OutputPath, Run, SourceRequestId, AllowedRoot)`, `CancelCapture()`, completed/failed delegates; no test-only mutation API. Create it as a default subobject on Coordinator. Reject movie/high-res and foreign screenshot ownership before registration and again at render boundary. Include UI but restrict readback to the owning game viewport; standalone frames 0..2 remain excluded by the installed Engine hidden-present boundary.
- Modify Coordinator header/cpp: delegate-driven smoke capture transitions; remove file-size polling, screenshot sleep/deadline waits, CaptureExitTimer, and premature ScreenshotPaths insertion. Record paths only after a successful owned callback. Keep gameplay cooldown waits and the existing whole-smoke watchdog.
- Modify GameplaySmoke header/codec/tests: additive `captures` receipt array alongside existing screenshots. Preserve schemaVersion 1 and existing fields; require the new capture evidence in updated verification scripts.
- Runtime Build.cs: explicit ImageCore dependency for FImageView/image saving; Slate/SlateCore for the native AHUD-owned text repair documented in `tasks/20260904-134500-capture-hud`. Production CameraComponent disables motion blur for top-down readability in all modes; no smoke-only visual substitution.

## PowerShell files / interface (independent subtask)

- New `Scripts/PngValidation.ps1` with `Assert-SeedForgePng -Path <file> -RunDirectory <current-run-dir> -RequestedAtUtc <DateTimeOffset> -Width 1280 -Height 720 -MinimumBytes 10240`. Return a record with normalized Path, Width, Height, Length, Sha256, CreationTimeUtc and LastWriteTimeUtc.
- Reject outside/sibling-prefix paths, reparse-point traversal, nonexistent/stale files, wrong extension/signature/IHDR, wrong dimensions, truncation/corruption that fails full image decoding, and too-small files. Dispose streams/images; no global settings or external directories.
- New audit-local `verify-png-validation.ps1` writes clearly synthetic fixtures only under `Artifacts/Reports/PngValidation/<unique-id>` and proves positive/negative cases RED→GREEN. It may use the already-inspected baseline PNG solely as fixture bytes, never as release evidence.
- Parent integrates validator into TestGameplay, CaptureGameplay and capture/package paths as applicable. Exactly three distinct paths/tokens/labels (start/combat/win), each under current run output, must correspond to successful callback receipts with ordered frame/time identities.

## Execution checklist

- [x] Add lifecycle and trace receipt RED tests before implementation, including duplicate request, mismatched/late callbacks, missing render acknowledgement, wrong path/dimensions, failed save, cancel/restart, and exact-once completion.
- [x] Add PNG-validator RED tests, implement minimum validator, inspect its output and rerun independently.
- [x] Implement native lifecycle/component, Coordinator transitions and receipt serialization. Build; focused capture/smoke/lifecycle tests; full suite. Do not remove/soften existing assertions.
- [ ] Run clean-revision Editor and packaged capture flows; strictly reparse receipts and PNGs. Inspect all three images in each runtime at original size. Fix rendering/camera/HUD timing based on the observed cause; never edit the PNG to conceal a runtime defect.
- [x] Verify negative capture timeout/save/ownership paths are bounded and idempotent, and delegate cleanup survives restart/EndPlay.
- [ ] Record exact RED/GREEN/visual evidence, independent review, separate commit `fix(capture): bind gameplay screenshots to rendered completion`.

Final all-gate certification still includes G's immutable-revision packaging and H's actual input/path/restart self-tests; do not infer them from capture-only smoke.

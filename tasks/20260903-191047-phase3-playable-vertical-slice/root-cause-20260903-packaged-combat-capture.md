# Root-cause packet: packaged combat screenshot edge clipping

## Symptom

The packaged gameplay smoke succeeds and writes valid start/combat/win PNGs, but the combat PNG intermittently captures only the right portion of some HUD lines at the left edge. The world scene, player, enemy, and attack pulse are visible. Packaged start and win frames are complete; the equivalent Editor combat frame is complete after stabilization changes.

## Minimal reproduction

1. Run `Scripts/PackageGameplay.ps1 -Seed 24301 -TimeoutSeconds 300`.
2. Open `Artifacts/Media/Gameplay/<latest>/SeedForge-Gameplay-combat-24301.png`.
3. Compare with the start/win images from the same run and the Editor combat image from `Artifacts/Media/Gameplay/20260903-200909`.

Evidence run: `Artifacts/Media/Gameplay/20260903-201808` with log `Artifacts/Logs/gameplay-smoke-packaged-20260903-201808.log`.

## Excluded hypotheses

- Missing/corrupt cooked HUD or font: packaged start and win render the complete same UCanvas HUD.
- Gameplay failure: JSON proves attack, kill, three pickups, unlock, and `Won`; process exit is zero.
- Camera yaw inheritance: the spring arm now uses absolute rotation and the Editor combat capture is complete.
- Initial viewport readiness: smoke waits one second before the first capture; start is complete.
- Empty/partial file: the packaged combat PNG is a valid 1280x720 file over 400 KiB.
- Project error/warning: strict packaged audit reports zero errors and zero unexpected warnings.

## Ownership boundary

The remaining symptom is at the interaction between multiple sequential `FScreenshotRequest` calls, a same-run camera teleport/attack frame, and packaged offscreen rendering. File materialization proves encoding completion but does not prove that a stable number of fully presented frames elapsed after the previous capture. Production gameplay state and HUD drawing are stable before and after that frame.

## Decision

Do not continue timing guesses or enlarge arbitrary sleeps. The Goal requires at least one normal gameplay screenshot; packaged start and win are complete, and the Editor combat frame is complete. Treat the packaged combat frame as a P1 evidence-quality limitation, not a P0 gameplay/package blocker.

## Different next repair, if prioritized later

Replace time/file-poll sequencing with a render-owned frame counter or viewport draw callback: request each screenshot only after a declared number of completed viewport frames following teleport/attack, or capture each phase in a separate process. That changes the ownership boundary rather than applying another sleep. Re-run Editor and packaged three-frame visual QA plus strict trace/log validation.

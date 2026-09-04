# SF-IRA-003 — persistent restart input evidence

## Source and contract

- Starting HEAD: `b02bd9e2e5a2e379f99d545be635c5447921eb03`.
- Positive Editor smoke at this clean A-fix commit passed: `Artifacts/Reports/Gameplay/20260904-104817/summary.json`.
- Before moving production input bindings, added read-only snapshot run/request counters and tests dispatching `FInputKeyEventArgs::CreateSimulated` through the actual Controller PlayerInput stack. No OS macro or direct handler/delegate invocation.
- Test module explicitly depends on InputCore for real key dispatch.

## Test fixture investigation

The first compile/run exposed incomplete test environment setup, not valid product RED. Full diagnosis and correction are recorded in `root-cause-003-input-fixture.md`: pointer-template syntax, local-controller designation, PlayerState, and possession's Owner transfer.

## Authoritative RED

- Build command: `Scripts/Build.ps1`, exit 0; log `Artifacts/Logs/build-editor-20260904-110029.log`.
- Test command: `Scripts/Test.ps1 -Filter SeedForge.Audit.RestartInput -TimeoutSeconds 120`.
- Result: 0 passed, 0 warnings, 3 failed, 0 not-run/in-process; wrapper exit 1.
- Report/log: `Artifacts/Reports/automation-20260904-110055/index.json`; `Artifacts/Logs/automation-20260904-110055.log`.
- Failures now match only absent persistent bindings, existing Character R/N bindings, unpossessed event loss, and the resulting stale-apply/possession outcome. Local PlayerInput exists, Controller survives Pawn destruction, one possessed N correctly queues a request, and the fixture no longer crashes.

## Cross-finding boundary

This task closes the persistent R/N portion of SF-IRA-001 recovery. Packaged input/rapid restart identity certification is still required by task H and gates 9/11.

## GREEN

- Moved R/N to `ASeedForgePlayerController::SetupInputComponent`; repeated setup removes prior bindings before re-adding exactly one each. Character no longer binds or implements R/N.
- Controller uses a weak cached Coordinator lookup, rejects an ambiguous fresh multi-owner lookup, and clears its weak reference at EndPlay.
- Coordinator explicitly unpossesses before destroying its run Character. Move/attack/Dash remain Pawn-owned.
- Build/focused command began 2026-09-04 11:02:57.051 UTC+8 and ended 11:04:37.418; both exited 0. Build log: `Artifacts/Logs/build-editor-20260904-110257.log`.
- Focused result: 3/3 passed, 0 warnings/failures/not-run/in-process; report/log `Artifacts/Reports/automation-20260904-110401/index.json`; `Artifacts/Logs/automation-20260904-110401.log`.
- The unchanged real-input tests prove one R/N per press, no Pawn duplicate binding, no-Pawn/failure dispatch, newest request ownership, stale completion suppression, no old run actors, fresh possession, reset HP/Cores, and no request on key repeat.

## Regression

- `SeedForge.Audit.RunFailure`: 11:04:48.443–11:05:09.294; 6/6 passed, zero warnings/failures; `Artifacts/Reports/automation-20260904-110448/index.json`.
- Full `SeedForge`: 11:05:09.350–11:05:38.675; 72/72 passed, zero warnings/failures/not-run/in-process; `Artifacts/Reports/automation-20260904-110509/index.json`.
- Both command wrappers exited 0 and recorded unchanged HEAD `b02bd9e2e5a2e379f99d545be635c5447921eb03` throughout.
- Repository and whitespace audits passed before the separate task B commit.

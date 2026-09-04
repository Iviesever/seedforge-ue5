# SF-IRA-004 — live combined Dash input evidence

## Source and root cause

- Starting HEAD: `5bf6bfeada7b4889ad4fc2c92988092c4bb14a1f`.
- Existing non-zero movement callbacks each replaced `LastMoveDirection`; zero samples never cleared it. The initial +X value also made the aim fallback unreachable.
- UE 5.8 `Engine/Source/Runtime/Engine/Private/UserInterface/PlayerInput.cpp` computes each input binding's current `AxisValue` before action dispatch, but dispatches axis callbacks after action callbacks. Therefore cached callback values alone would be one input frame stale for simultaneous movement changes and Space.
- Extracted the already validated restart input World fixture into a shared test-local header; `CountAction` is inline to preserve one-definition correctness across translation units. Queue/Process separation permits several real engine key events within one input frame. Production handlers are never invoked directly by these tests.

## Authoritative RED

- `Scripts/Build.ps1`: exit 0, `Artifacts/Logs/build-editor-20260904-111407.log`.
- `Scripts/Test.ps1 -Filter SeedForge.Audit.DashInput -TimeoutSeconds 120`: wrapper exit 1, 0 passed / 4 failed / 0 warnings / 0 not-run / 0 in-process; report `Artifacts/Reports/automation-20260904-111440/index.json`.
- Failure assertions proved diagonal mismatch, stale release/opposite-key direction, focus/re-possession stale direction, and the pure helper's missing directional behavior. No fixture crash or compilation failure was counted as RED.
- Fixture extraction regression: `Scripts/Test.ps1 -Filter SeedForge.Audit.RestartInput -TimeoutSeconds 120`, exit 0, 3/3 passed with zero warnings/failures, report `Artifacts/Reports/automation-20260904-111511/index.json`.
- Added same-frame press/release plus Space coverage before GREEN: build exit 0 (`build-editor-20260904-111618.log`), then DashInput wrapper exit 1, 0/5 passed, 5 expected failed, no warnings/not-run/in-process (`automation-20260904-111623/index.json`). Both same-frame assertions failed against the old Character.

## Minimal GREEN

- Character retains both axis samples including zero, clamps finite values, and clears intent on Controller key flush, PawnClientRestart, and UnPossessed without resetting Dash cooldown.
- Dash samples both already-summed input binding values at action time, avoiding UE's action-before-axis-callback ordering trap. The pure helper chooses normalized movement, then normalized flat aim, then +X; non-finite inputs have a safe fallback.
- Build: `Scripts/Build.ps1`, exit 0, `Artifacts/Logs/build-editor-20260904-111739.log`.
- Focused: `Scripts/Test.ps1 -Filter SeedForge.Audit.DashInput -TimeoutSeconds 120`, exit 0, 5/5 passed, zero warnings/failures/not-run/in-process, report `Artifacts/Reports/automation-20260904-111814/index.json`.
- Tests observe actual CharacterMovement pending launch velocity after Controller PlayerInput processing; repeat Space before cooldown preserves the first accepted velocity.

## Regression and repository checks

- `Scripts/Test.ps1 -Filter SeedForge.Audit.RestartInput -TimeoutSeconds 120`: exit 0, 3/3 passed, zero warnings/failures/not-run/in-process; `Artifacts/Reports/automation-20260904-111854/index.json`.
- `Scripts/Test.ps1 -Filter SeedForge -TimeoutSeconds 600`: exit 0, 77/77 passed, zero warnings/failures/not-run/in-process; `Artifacts/Reports/automation-20260904-111906/index.json`.
- Independently reparsed all three GREEN `index.json` files; test-array counts agree with 5, 3, and 77 executed tests.
- `Scripts/AuditRepository.ps1` and `git diff --check`: exit 0 before staging.

## Independent read-only review

- A bounded reviewer inspected this isolated working-tree diff and the UE input evaluation order without editing files or running UE processes. No blocking correctness findings.
- One P3 coverage gap was confirmed: the original re-possession branch first flushed keys, coupling that proof to the focus-loss path. The revised case holds W, unpossesses, dispatches W release while no Pawn exists, re-possesses, then dispatches Space. The focus-flush branch remains separate.
- The cooldown test now flushes keys before the second Space, verifying that flush does not bypass the accepted Dash deadline.
- After this test-only strengthening: build exit 0 (`Artifacts/Logs/build-editor-20260904-112118.log`); DashInput 5/5, zero warnings/failures/not-run/in-process, exit 0 (`Artifacts/Reports/automation-20260904-112132/index.json`).
- Final full suite after the review changes: 77/77, zero warnings/failures/not-run/in-process, exit 0 (`Artifacts/Reports/automation-20260904-112151/index.json`). This includes all three RestartInput regressions.

## Certification boundary

These are local development checks of the working tree based on the starting HEAD, not clean-revision release certification. Packaged input proof remains task H/gate 9. Fresh all-process log and provenance gates remain pending: the UBA build output reports a socket-bind diagnostic and a default user-profile `Trace.uba` path; neither is being silently certified as a strict final gate pass here. Task G must make diagnostic output provenance/boundaries explicit.

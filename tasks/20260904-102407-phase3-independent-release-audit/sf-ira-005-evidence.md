# SF-IRA-005 — in-progress capture evidence

**Status: not closed.** Native callback ownership and strict PNG/receipt validators are implemented and have local GREEN evidence, but final visual/package verification is still required. Do not promote rejected diagnostic screenshots to release evidence.

## Starting point and contract

- Starting HEAD: `15d051b6e12535566a30422f0d76dec992aca265`.
- Its clean-commit Editor smoke passed at `Artifacts/Reports/Gameplay/20260904-120630/summary.json`. Original-size inspection found combat HUD prefix loss and win motion blur.
- Plan: `task-005.md`. New pure capture types/lifecycle, a Coordinator-owned ActorComponent, matching token/run/path/frame callbacks, synchronous PNG write, processed-boundary completion, actual delegate cleanup, explicit host capture root, and per-capture receipts.
- Diagnostic runs explicitly label gitSha/sourceRevision `diagnostic-<base HEAD>` and record sourceTreeDirty; they are not clean-revision certification.

## Native RED → GREEN

| Check | Result / evidence |
|---|---|
| Pure lifecycle/receipt RED | Build `121552` exit 0; Automation `121609`: 1 pass (invalid-request rejection), 5 expected failures, zero warnings |
| Pure lifecycle/receipt GREEN | Build `121825` exit 0; Automation `121834`: 6/6, zero warnings/failures |
| Native adapter compile | Build `122621`: exit 0 |
| Cached new-source discovery gap | Build `122916` did not compile the two new component cases; report `122920` remained 6 tests and was not counted as component verification |
| Fresh graph and fixture diagnosis | `build-capture-freshgraph-20260904-123027.log` discovered the file; `123036` and `123117` failed on invalid UGameViewportClient Outer. See `root-cause-005-viewport-fixture.md` |
| Correct component fixture | Build `123233`; Automation `123238`: 8/8. Real FSceneViewport delegate simulation verifies cancellation/EndPlay cleanup, not GPU rendering |
| Trace top-level identity RED | Build `123703`; Automation `123724`: only the new runGeneration/appliedRequestId serialization assertion failed |
| Identity + full regression GREEN | Build `123921`; Capture `123926`: 8/8; full SeedForge `123938`: 94/94, zero test warnings/failures/not-run/in-process |
| Review P2 RED | Build `124945`; Automation `125006`: one component test failed on missing explicit-root confinement and movie/highres exclusion at request/render boundaries |
| Review P2 GREEN | Build `125421`; Capture `125439`: 8/8, zero warnings/failures |

Build identifiers refer to `Artifacts/Logs/build-editor-20260904-<id>.log`; Automation identifiers refer to `Artifacts/Reports/automation-20260904-<id>/index.json`.

## Script validators

- PNG validator subtask: `png-validation-evidence.md`, 28/28 on Windows PowerShell and PowerShell 7, including CRC, zlib/DEFLATE, Adler, exact decoded lengths, real pixel decoding, freshness and reparse rejection.
- Capture-set validator subtask: `capture-validation-evidence.md`, 84/84 on both shells; exact start/combat/win set, native token filename suffix, IDs, frame/time order, zero remaining bindings, file membership, bytes and actual PNG verification.
- Primary independent reruns: `Artifacts/Reports/PngValidation/20260904-125618-8bbf9104aaf043a2bd41ef44e625e5b3/summary.json` (28/28), `Artifacts/Reports/CaptureValidation/20260904-125620-b8930d977d5f46678f2218b72eee5c37/summary.json` (84/84).
- Synthetic validator fixtures are never release screenshots. The script summary explicitly leaves visual review unassessed.

## Visual diagnostics — all must be interpreted with inspection

See `root-cause-005-visual-readiness.md` for source evidence and controlled experiments.

- `Gameplay/20260904-124046-0444836d0f6e4ec8bd11c7ffbdda34e6`: callback metadata passed, but start/combat captured unready frames 0/1 and win blurred. Rejected visually.
- `Gameplay/20260904-125636-3837a31ce33b421d99f96cf62f755a0a`: PSO/proxy readiness was already zero pending; it did not fix early black frames. Standard camera-cut marking removed win motion blur. HUD still failed.
- `Gameplay/20260904-130225-a46e8e87ea494a28a0ca11158f5a0e3d`: avoiding UE 5.8's first three hidden standalone presents produced complete geometry from start frame 3. HUD prefix loss remained.
- `Gameplay/20260904-130630-e682fffe22f04929a8c2ee30c94cf5ee`: independent line batches did not fix HUD; that change was removed.
- `Gameplay/20260904-131134-7c30385596344182ac6e712e095c5a51`: per-line Canvas flush did not fix HUD; that change was removed.
- `Gameplay/20260904-132420-8f579641e2e34cd8a6aa208cac126323`: include-UI/readback restricted to game viewport still had selected missing prefixes. Rejected visually.
- The foreground-Canvas experiment also failed (`Gameplay/20260904-133336-0694596ca08d45029fe421e60dd84f41`) and was removed. The bounded native Slate HUD replacement, its RED/GREEN tests, hide/debug/EndPlay ownership and independent review are recorded in `tasks/20260904-134500-capture-hud`.
- Post-correction `Gameplay/20260904-140628-341792190b504e81a585ebf464d243eb` and `Gameplay/20260904-140750-71e02e6998a541e1b761984312001084` both strictly passed receipts/PNG checks. All six original-resolution images were inspected: complete readable HUD, complete scene and sharp visible player/enemy/exit geometry. Production CameraComponent disables motion blur; no screenshot files were edited.
- Final local native build `build-editor-20260904-140522.log` exit 0; full `automation-20260904-140554` 98/98, zero test warnings/failures/not-run/in-process. Native capture review found no blocking issue; HUD review P2 was reproduced and corrected, then independently re-reviewed.

## Negative runtime capture coverage in progress

- New cases: CapturePath (a file blocks capture-directory creation) and RenderUnavailable (null RHI).
- `RunFailure/20260904-132812-016-CapturePath` is **not accepted**: trace reached Failed and process requested exit 2, but cold Engine initialization took 41.17 seconds and the outer 45-second observation timed out. The observation retains `outerTimeout=true`; no timeout budget was increased. A same-budget rerun will distinguish startup variance from capture failure behavior.

## Remaining work

- Local implementation, repeated Editor visual diagnostics, native negative capture processes and full regression are complete for the checkpoint. Final clean-revision Editor/package certification remains open.
- Obtain final independent review, commit a clean candidate, and certify Editor + packaged captures against that immutable source. Any local checkpoint must keep packaged/final-gate status open until actually run.
- G shared contract implementation has begun independently in its allocated files; real package/aggregate integration and all 16 gates remain pending.
## Latest capture-negative process evidence

`RunFailure/20260904-133359-370-CapturePath` and `RunFailure/20260904-133410-762-RenderUnavailable` both exited 2 with failure traces and `outerTimeout=false`, in about 11.3s and 13.9s respectively. They retain the original 45s outer budget. The earlier cold-start CapturePath timeout remains a rejected attempt, not hidden or converted into a pass. Source remained explicitly diagnostic/dirty at base `15d051b6e12535566a30422f0d76dec992aca265`.

After the final HUD/camera code, `RunFailure/20260904-140812-236-CapturePath` and `RunFailure/20260904-140830-201-RenderUnavailable` again exited 2 with reparsed failure traces and no outer timeout. The 45s budget was unchanged. Eight F PowerShell files parsed without errors and `git diff --check` passed before staging the checkpoint.

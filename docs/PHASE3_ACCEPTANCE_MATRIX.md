# Phase 3 acceptance matrix

This matrix is a route to evidence, not a static certificate. The independent audit repaired the original 63-test handoff's gaps. Checkpoint results below are observed at their recorded revisions; after documentation/images change, all 16 final gates must run again on one clean candidate.

| Gate | Required proof | Observed audit checkpoint / final authority |
|---|---|---|
| 1. Repository/base | clean audit, preserved 0.2.0, fresh Git base | Local audits pass; final fresh fetch and candidate audit required |
| 2. Editor Development | actual UBT, exit 0, strict complete logs | H7 diagnostic build `190812` passes; final candidate build required |
| 3. Complete Automation | no warnings/failures/not-run/in-process | `191006`: 119/119, zero whole-log warnings/errors; diagnostic checkpoint |
| 4. Phase 2 chain | canonical JSON, Diff, 10k benchmark, old goldens | Preserved in source/tests; fresh revision-bound 10k report required |
| 5. Focused tests | gameplay, smoke, path, encounter, state and audit | H7 smoke 7/7 plus prior focused RED/GREEN; final namespace runs required |
| 6. Editor gameplay | real path before capture, attack/collect/extract | `191146`: 8 real moves / 20.9972 units, 3 decoded/inspected PNGs |
| 7. BuildPlugin | actual Editor Dev / Game Dev / Game Shipping | Clean f235e46 three-target checkpoint and 58-file index; rerun with final H7 source |
| 8. Win64 package | actual Build/Cook/Stage/Pak/Archive | Clean a9f5625 `193928` passes, local DDC/temp/loose Cook and original Pak log association |
| 9. Ordinary packaged input | no gameplay-smoke driver; real mapped input | `194029`: WASD/aim/LMB/Space/R/N, 13 effects, actual motion and controlled exit |
| 10. Packaged gameplay | path, combat, pickups, exit and failure exits | `194036` positive; four `194046..194056` expected-negative processes pass |
| 11. Restart identities | same/new/rapid, fresh actors and ownership | Editor and packaged four-run input scenarios pass; real async newest-request ownership |
| 12. JSON reparse | exact uint64/type/identity/action/path relations | Native model/A* validation plus external input/path/capture validators |
| 13. Visual materialization | three original 1280x720 frames in each environment | Both checkpoint triplets inspected; new six-image final review still required |
| 14. Strict logs/storage | no unexpected warnings/errors or escaped storage | Full-log gates and precise environment profile; filesystem DDC/temp and Cook controls |
| 15. Digests | archive, sidecar, nested records and final index | Package a9f5625 rehashed; 49-file independent index; final candidate seal required |
| 16. Clean/remote | final same HEAD, fresh PR/base/reviews/checks | Pending final candidate push, fetch/API review and readiness records |

Exact checkpoint paths and SHA values are in [the audit journal](../tasks/20260904-102407-phase3-independent-release-audit/progress.md), [H evidence](../tasks/20260904-102407-phase3-independent-release-audit/sf-ira-008-evidence.md), and the original generated files. The default seed retains layout `7425849530159566348` and encounter `15303214708604970503`; the original 41 tests and all five layout goldens remain part of the complete suite.

## Authority flow

`VerifyPhase3.ps1` captures one clean revision, checks it around every process, freezes child evidence immediately, and returns `MachinePassed` only. Its unique summary and independent index are under `Artifacts/Reports/Phase3Verification/`; `phase3-verification-last.json` is a convenience pointer, not authority merely because it exists.

The primary then inspects all six original Editor/packaged images, pushes the exact validated feature commit while PR #1 remains Draft, and freshly reads that matching remote head/base and reviews. Candidate-bound visual and remote JSON records are required by `FinalizeRelease.ps1`, which verifies indexes/digests and writes `release-readiness.json`. It does not itself merge or publish.

The user has authorized merge and a source-only v0.3.0 Release after these gates pass. Publication is a subsequent verified GitHub operation; no plugin/demo binary assets are uploaded. Dirty `DiagnosticPassed` evidence and historical handoffs cannot satisfy the final gates.

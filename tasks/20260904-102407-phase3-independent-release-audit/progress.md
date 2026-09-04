# Independent release audit — local execution

## Authority and source preservation

- Byte-identical source files are preserved under `Artifacts/IndependentAudit/20260904-102407/source-audit/`. The tracked `source-audit/` transcripts remove only Markdown trailing spaces to pass the unchanged whitespace gate; historical conclusions and `not-run` entries are not local evidence. See `source-provenance.md`.
- Latest user instruction authorizes fixes, pushes, Draft PR #1 updates, then merge and a source-only GitHub Release after all 16 gates pass. It supersedes the packet's older no-merge/no-release boundary.
- Do not upload plugin/demo ZIPs as Release assets. Local packaging and verification remain mandatory; GitHub's default source ZIP/tarball are the only release downloads requested.
- One primary agent, existing checkout only, one UE writer process at a time; Engine read-only.
- User approved the sole external internal-diagnostic exception for `C:\Users\Iviesever\AppData\Local\UnrealBuildTool\Trace*.uba` on 2026-09-04. Project artifacts/evidence remain inside the repository; Engine/global folder settings remain unchanged.

## Fresh preflight (2026-09-04 10:23–10:24 UTC+8)

- Branch: `feat/phase3-playable-vertical-slice`.
- HEAD and live PR head: `18c255db750b8edf3576fe348b47fd0b6b8d312e`.
- Freshly fetched `origin/main`: `9a306f8ff72cb660d3c04b09806787df21191d45`.
- Relation: 11 ahead / 0 behind; initial worktree clean; `git diff --check` clean.
- PR #1: open, Draft, unmerged, mergeable, 64 changed files.
- Reviews / issue comments / review comments: all three API responses are empty arrays. An initial PowerShell wrapper counted the empty array object as one; direct JSON inspection corrected that observation.
- Checks / workflow runs at head: 0 / 0.
- No active UBT/UAT/Editor/Cook/Pak writer found.
- `v0.2.0` annotated tag still peels to the immutable base; no remote `v0.3.0` tag.

## Execution ledger

- [x] Read all four source audit files completely.
- [x] Read repository rules and recover fresh local/remote state.
- [x] Preserve source audit packet without rewriting historical claims.
- [x] A — SF-IRA-001 core: RED reproduced, central failure/recovery GREEN, 6 focused + 69 full tests, Grid/Encounter real failure processes exit 2; persistent R/N recovery remains dependent on B.
- [x] B — SF-IRA-003 local: real-input RED→GREEN, 3 focused + 6 failure regression + 72 full tests; packaged input certification remains H/gates 9/11.
- [x] C — SF-IRA-004 local: 5 real-input/pure RED→GREEN tests, 3 restart regressions, final full 77/77; includes same-frame movement+Space, focus flush, unpossessed release, and cooldown preservation.
- [x] D — SF-IRA-006 local: checked neighbors and int64 distance/cost; 3 bug REDs plus 3 preservation cases, focused 6/6 and full 83/83, existing path/encounter/goldens preserved.
- [x] E — SF-IRA-007/009 local: true completion/request identity, cleared pending hashes, direct apply request 0; 3 focused, 6 failure, 3 restart and full 86/86; script marker negative checks pass.
- [ ] F — SF-IRA-005: render-owned screenshots and strict PNG proof.
- [ ] G — SF-IRA-002: immutable clean-revision evidence and negative harness.
- [x] G prelude only: project-local UBA/DDC inheritance at 11 launchers; Build/BuildCookRun scoped remote-disable flags; PowerShell RED→GREEN and full 86/86. This does not close the remaining G gates.
- [ ] H — SF-IRA-008: real path/movement/restart/input integration.
- [ ] I — documentation and disclosure calibrated to new evidence.
- [ ] Gates 1–16: all not-run for this audit until fresh results are recorded.
- [ ] Publication: final PR update, merge, source-only Release, remote re-read.

## Current boundary

Task A commit: `b02bd9e2e5a2e379f99d545be635c5447921eb03`; RED/GREEN/process evidence is in `sf-ira-001-evidence.md`. Latest full suite is 69/69 with zero test warnings/failures, not yet final certification. Positive Editor gameplay smoke also passed at this clean commit: `Artifacts/Reports/Gameplay/20260904-104817/summary.json`.

Task B commit: `5bf6bfeada7b4889ad4fc2c92988092c4bb14a1f`; real Controller input/possession evidence and its fixture root-cause packet are recorded here.

Task C commit: `554ff5ff61182343f789f4e829ac196e4226b76c`; combined live Dash and independent review evidence are in `sf-ira-004-evidence.md`. Its clean-commit Editor smoke passed at `Artifacts/Reports/Gameplay/20260904-112322/summary.json`.

Task D commit: `4b68da82c89f5dc1776a264fd94bb8dcd5d1b216`; safe-coordinate RED→GREEN and independent review are in `sf-ira-006-evidence.md`.

Task E commit: `648b776a01e98cf1408d4b33bd19d23b297380db`; request/pending identity RED→GREEN and script checks are in `sf-ira-007-009-evidence.md`.

The trace-boundary blocker is resolved by the user's narrow exception. Build-environment prelude commit `15d051b6e12535566a30422f0d76dec992aca265` passed its clean Editor smoke at `Gameplay/20260904-120630`.

F local implementation is ready for its checkpoint but **not closed**: see `sf-ira-005-evidence.md`. Native capture, Slate HUD ownership and production camera readability passed final full 98/98 at `automation-20260904-140554`. PNG/capture-set validators independently passed 28/28 and 84/84. The two final Editor diagnostic triplets (`140628`, `140750`) were fully inspected at original resolution with complete HUD and sharp objects. Final CapturePath/RenderUnavailable processes (`140812`, `140830`) exited 2 without outer timeout. Earlier rejected images and the cold-start timeout are preserved as failed diagnostic attempts. Clean-source/package certification remains required.

F checkpoint is committed as `2035e925ceb73fd2aa24de2646a0ad6197216dc8`. G foundation is committed as `ee22236d8b56c3f5954270b5a01f47b01003087a`; process integration is ready for its explicitly non-final checkpoint. Its journal is `sf-ira-002-integration-evidence.md`: latest foundation/context/index/sealing 92/92, actual entry-point preflight 96/96, strict positive log 57/57, exact negative log 36/36, target proof 38/38 and local adapter 36/36. Two independent sealing P2s are corrected and regression-tested. Actual UAT/BuildPlugin/BuildCookRun and the H6 successful-input consumer remain dependencies; G is not closed.

H passive path/restart observations and failure-only input scaffolding reached 109/109 plus zero whole-log errors/warnings at `automation-20260904-160608-31c4e8d832434422a94b162105c36274`. H6 success validator/serializer RED then produced 8 passing controls/2 intended failures; its ordinary real Editor process `InputSelfTest/20260904-164323-ba17eba96b194f6cb798e09f9055002b` exited 2 itself with exactly one Failed/MissingInputEvidence trace. Real input driving and H7 smoke path proof are next, not certified.

Strict logging exposed 13 built-in Engine startup self-test failures under zh-CN. Same-binary/same-98-tests comparison at `StartupCulture/20260904-144858` reproduced 13 at default culture and zero with `-culture=en`; verification child processes now pin English without changing OS/Engine settings or skipping tests. The separate historical Dataflow Date diagnostic remains unallowlisted. Final packaging, all 16 clean-candidate gates, current documentation, remote/PR re-read and source-only publication remain incomplete; nothing has been pushed/merged/released for this audit yet.

## 18:31 runtime boundary checkpoint (2026-09-04, UTC+8)

G process integration is committed as `be52224f56f04c69d51cd1e93bf67b523fb91b93`; H1-H6 ordinary input/path/restart is committed as `058615673ab593611d054f7b7ebd337363bb091d`. H7 remains unimplemented and is gated on the first actual clean BuildPlugin checkpoint.

The Zen metadata/global-temp violation was diagnosed from original logs, not allowed as an environment warning. `root-cause-runtime-output-boundary.md` records the explicit filesystem graph, process-local TEMP/TMP, Cook SkipZenStore and original before/after metadata. `RuntimeStorageValidation.ps1` is now invoked by all ten runtime launchers. It checks observed storage behavior and original log hashes; it is not a whole-filesystem attestation.

Fresh parent checks after integration:

- Runtime storage helper: 64/64 on both PS 5.1 and 7.6 at `RuntimeStorageValidation/20260904-102203-ea0b8d04e21c4e0fa35cc145da508071` and `20260904-102204-9144023cdb484da98648cbc06eeede54` (UTC stamps). Launcher-consumer absence and duplicate IoStore arguments each produced an observed RED before correction. Runtime provider and child-temp inheritance checks passed on both versions.
- Entry-point dirty/revision/git-failure harness: 96/96 at `ReleaseEntrypoints/20260904-182508-8e546f6deb934f7d8d176ff1f8bf831e`.
- Full Automation: 114/114, zero test/whole-log warnings and errors, at `automation-20260904-182442-b9fe06f5e6b54cd1a403fe4377218bb7`; observed owned DDC/temp proof passed.
- Ordinary input: `InputSelfTest/20260904-182403-f3c129ef902948ed981db36ddb31adb8`, exit 0, four runs/13 effects/10 transitions/four queued requests, actual enemy distance 20.4186055; original-byte strict log and storage proofs agree. Zero errors, 25 narrowly allowed existing environment warnings.
- Gameplay/captures: `Gameplay/20260904-182556-3ef8e2cb42f043ffa783bd238bc5f88d`, exit 0, three validated native receipts and PNGs, matching golden hashes and same strict storage/log proof. These fresh images passed machine validation; this triplet has not received an additional manual visual review.
- RenderUnavailable negative: `RunFailure/20260904-182748-835-RenderUnavailable`, native exit 2 without outer timeout; exactly two expected errors, no unexpected error/warning, owned storage proof passed.

All of these are explicitly diagnostic source runs, not final release certification. Independent review corrected UnrealPak full-log selection (underscore scenario logs, not abbreviated stdout logs) and duplicated IoStore arguments (UE 5.8 already inherits AdditionalPakOptions). Before actual BuildCookRun, add the original UAT Running/exit/copy correlation for those full logs and validate fresh non-Zen cooked storage. First actual BuildPlugin, H7, final all-16 gates, visual/docs/remote publication remain open. The PackagePlugin read-only metadata guard is a before/after diagnostic for the two identified infrastructure paths, not an Engine-wide attestation.

## 19:03 checkpoints and remaining native work (2026-09-04, UTC+8)

- Runtime storage checkpoint: `f235e46e9f637d706b9997e8af542873591c86f5`.
- **First actual clean BuildPlugin passed** at that exact revision: three targets, strict logs/binary metadata/products, native and wrapper exit 0; original archive rehash/checksum and 58-file independent index passed. The two identified read-only infrastructure metadata paths were unchanged. Exact paths/times/hash are in `build-plugin-live-evidence.md`. This does not count as the final gate 7 after later H7 changes.
- Nested package evidence sealing: `23f07b4759d6b0237e5abf3c916312722430b594`. Actual caller harness first reproduced eight tamper acceptances, then both PS versions passed 9/9 and shared helper regression passed 92/92. Read-only independent review had no P2; see `package-evidence-sealing-evidence.md`.
- Original UAT/Pak/full-log correlation: `cbd7ac3412fb1500d62dff9426eaed1bf30f4ec9`. Both PS versions independently passed 55/55; 96/96 entry-point preflight checks also passed. PackageDemo retains the proof with exact actual UAT interval; independent integration review had no blocking finding. Actual BuildCookRun remains unexecuted at this checkpoint.
- H7 RED build (`184515`) passed. The initial focused run `automation-20260904-184620-4f4f51fcb6cf44bdbd3df01b65384573` crashed in a synthetic duplicate-cell test (TArray self-element Add), exit 3/no report. Preserved as a fixture failure. A local FIntPoint copy corrected only the fixture; strict rebuild `184843` passed.
- Correct H7 focused RED: `automation-20260904-185054-a42335c52d284a8094a5e63b96ea1441`: four controls passed/zero warnings/three intended failures (8 observer, 2 acceptance, 9 schema assertions), notRun/inProcess zero. Real movement/timer controls passed: 120 ticks, 118 timer firings, 502.666685 units. Watchdog original-deadline/default/restart/failure/EndPlay checks passed. Owned storage proof passed.
- Real H7 fail-closed RED: `Gameplay/20260904-184729-9b8302d66c5e4612af6b68c285792779`: native exit 2, PathObservationUnavailable, Generating/Playing/Failed, no screenshot/receipt, no outer timeout, observed storage confinement passed. H7 minimum observer/codec GREEN was released only after the corrected focused RED.
- External H7 path validation is connected to TestGameplay without weakening capture/PNG/source/log gates. Review found two serialized-proof gaps: impossible count-two waypoint advance, and omitted aggregate displacement/time for first/latest samples. Both were reproduced separately in H7 and H6 external validators; correction contracts, failures and compatible positive controls are recorded in `root-cause-compressed-path-proof.md` and `gameplay-path-validation-evidence.md`. Native H7 gets equivalent aggregate tests; H6 production input/movement is unchanged.

Remaining: H7 native GREEN/review/build/focused/full/real Editor and ordinary input; clean checkpoint; actual Win64 BuildCookRun/ordinary input/positive-negative packaged smoke; inspect/replace final screenshots and calibrate current docs; then all 16 final gates on one clean candidate, fresh remote PR evidence, merge and source-only Release. No audit branch push, merge, tag or Release has occurred yet. No additional permission is needed for these authorized in-scope steps.

## 19:15 H local checkpoint ready

`sf-ira-008-evidence.md` now contains the primary's frozen-source H7 GREEN evidence: strict build, seven focused tests, **119/119 full Automation with zero whole-log warnings/errors**, real Editor H7 path before three rendered screenshots (all manually viewed at original resolution), separate ordinary-input/four-run proof, and all four exact negative runtimes. Independent native and final external path review have no blocking P2. All native runs are explicitly diagnostic-cbd7ac3; clean-source/package/final certification is still next, not implied by this checkpoint.

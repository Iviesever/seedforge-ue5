# SF-IRA-002 integration journal — not yet closed

Base foundation commit: `ee22236d8b56c3f5954270b5a01f47b01003087a`. F checkpoint: `2035e925ceb73fd2aa24de2646a0ad6197216dc8`. The changes recorded below remain development integration, not final clean-revision release certification.

## Entry point and context RED/GREEN

- Real script-only synthetic preflight harness: 15 actual entry-point sources, staged/unstaged/untracked, malformed/wrong ExpectedRevision and native Git failure. No UE binary/project is present in a fixture; these are not clones/worktrees/integration copies.
- RED `ReleaseEntrypoints/20260904-141418-4998350e8eb44218ad1d30650aeff7fd`: 90/90 failed against missing expected-revision/preflight interface.
- GREEN `ReleaseEntrypoints/20260904-142041-65a96b63192d4aee881a0b940ef7653c`: 90/90. Repeat after process integration `ReleaseEntrypoints/20260904-161942-fc80a6ea1be84884a4e875507a7b4167`: 90/90.
- Script-context diagnostic/authoritative distinction RED: `VerificationContract/20260904-141822-e46e9819d92e43e4a2016f7db373b005`, 0/6. GREEN `142036-946b8d475d3f4460920d4f3db9afc7c5`, 6/6. Explicit diagnostic results cannot certify archive manifests; default paths require clean source and frozen HEAD.
- Independent evidence-index RED `VerificationContract/20260904-155055-34562dd1589e46c2a09dc8491ee59f40`, 0/6. GREEN `155551-02f4e7541dd44bc2a1de062a6dd30e1e`, 6/6. Full combined foundation/context/index regression `162028-4a27a0315ddb4c0689b40cef2cb730c3`: 63/63.

## Strict log / target / adapter checks

- Parent independently reran LogValidation 57/57 at `LogValidation/20260904-152555-02d9270363dd49538ac70e8dd1979999` and target proof 38/38 at `BuildPluginValidation/20260904-152556-a3fcb71bc0cb458baea5235c7dad220a`.
- Parent independently reran adapter compilation/pure argument tests 36/36 at `AutomationExtension/20260904-160721-891dbccabd59414aa8a6cb59cecf135c`. No UAT/UBT was called by that harness. Actual BuildPlugin extension loading/target execution remains pending.
- Single-project external Automation discovery and no ProjectReferences are enforced before UAT. Read-only Engine source shows the custom record is project-local; Engine scratch props would be written only for multiple external projects, which this invocation rejects. See adapter evidence/plan.
- The 13 startup errors in earlier green Automation reports are attributed to three built-in localized Engine smoke tests; `root-cause-002-startup-culture.md` records the same-98-test A/B proof. Pinning only the verification child's culture to English preserves all tests and yields zero complete-log errors/warnings. No error allow-list or Engine/global language setting was changed.

## Expected-negative log contract

Positive log validation still permits no errors. The four intentional run-failure cases now have a separate `ExpectedFailure` verdict: exactly one controlled run-failure line and one controlled smoke-failure line, with exact code/message for that case. Missing/duplicate lines, unexpected errors/warnings, false success and altered messages reject. Original logs remain untouched.

- `NegativeLogValidation/20260904-163829-e25a3322fb1446b0bf61771a8218097b`: RED 32 failures / 4 preserved positive-mode rejection controls.
- `NegativeLogValidation/20260904-163927-5327281cb4524bf180f2e2e36be59604`: GREEN 36/36.
- Existing positive log regression `LogValidation/20260904-163927-9e711bde5de44915abca47a1e90b0a4d`: 57/57.

## Native use of integrated wrappers

Strict diagnostic Build/Test wrappers ran H's focused checkpoints and a complete 109-test regression (`automation-20260904-160608-31c4e8d832434422a94b162105c36274`) with zero test warnings/failures/not-run/in-process and zero whole-log errors/warnings. These are explicitly diagnostic/uncommitted-source results, not the final clean revision.

## Review corrections and remaining work

Independent review found two P2 sealing gaps: digest records were reduced to paths and could be re-hashed after tampering; outer gameplay package manifests omitted the inner BuildCookRun log proof. Both shared/caller corrections are implemented: freeze expected records immediately after each step, compare them while sealing, carry the old machine-index and checksum expectations into finalization, snapshot review files before reading, and retain nested `packageProof`. `evidence-sealing-evidence.md` records real tamper REDs. Primary independent combined regression `VerificationContract/20260904-170824-13f3b2c332ba40f19361231cf5cb278a` passed 92/92.

The new ordinary-input wrapper joins the source-preflight matrix: `ReleaseEntrypoints/20260904-165844-a51526478d6344579ce05360aca57595` passed 96/96 across 16 entry points. Its successful evidence validator is the H6 dependency and is not yet available at this checkpoint.

The updated gameplay wrapper ran successfully against the last compiled H6 fail-closed scaffold binary (ordinary gameplay mode, not the input driver): `Gameplay/20260904-171038-c5bcea3bb6414c83902069af72d43ccb` reports DiagnosticPassed, zero errors and 25 exact allowed environment warnings. Updated Grid/Encounter negative wrappers at `RunFailure/20260904-171105-962-Grid` and `171113-546-Encounter` both returned native exit 2, reparsed expected failures and passed the exact negative-log policy without outer timeout. This is wrapper regression, not H6 success or final visual certification.

Input wrapper/schema, actual ordinary input success, actual local-only BuildPlugin, BuildCookRun, final positive/negative package evidence, all original-resolution final images, document calibration, final clean revision, remote re-read and publication remain required. Machine aggregation explicitly reports `MachinePassed` with pending visual/remote review; finalization requires separate matching review records and never merges/tags/uploads by itself.

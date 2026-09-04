# Phase 3 evidence journal

## Baseline

- Base: `origin/main` at `9a306f8ff72cb660d3c04b09806787df21191d45`.
- `v0.2.0` tag and published release: same immutable revision.
- Repository audit: 99 tracked files, 43 source/build-rule files.
- Editor build: passed.
- Automation baseline: 41 passed, 0 warnings, 0 failures.
- Full detail: `tasks/20260903-191047-phase3-playable-vertical-slice/pact-30-evidence.md`.

## PACT-31: deterministic model

- RED: 0 passed / 15 failed against deliberate stubs.
- GREEN: 15 focused passed; 56 complete passed, 0 warnings/failures.
- Added encounter plan/hash, bounded A*, and run state machine.
- Commits: RED `319a302`; GREEN `81a77a2`.
- Full detail: `tasks/20260903-191047-phase3-playable-vertical-slice/pact-31-evidence.md`.

## PACT-32: gameplay World

- RED: 0 passed / 4 failed; separate coordinator World contract 0/1 failed.
- GREEN: 5 focused passed; 61 complete passed, 0 warnings/failures.
- Normal Editor path reached 1 player / 3 Cores / 5 enemies / 1 exit.
- Editor gameplay screenshot and independent BuildPlugin passed.
- Commits: RED `6e8fcbd`; GREEN `7f6fedd`.
- Full detail: `tasks/20260903-191047-phase3-playable-vertical-slice/pact-32-evidence.md`.

## PACT-33: gameplay smoke/package

- RED: 0 passed / 2 failed against empty trace serializer.
- GREEN: 2 focused passed; 63 complete passed, 0 warnings/failures.
- Editor smoke: seed 24301, layout `7425849530159566348`, encounter `15303214708604970503`, attack/kill/3 collect/unlock/win, three PNGs, JSON reparse, strict log audit.
- Clean revision `9ff53d407e6525af96a92fedb35c51532a48b39c`: BuildPlugin passed Editor/Game Development/Game Shipping; BuildCookRun and ordinary packaged launch passed; packaged smoke passed with exact trace and images.
- Plugin pre-version-bump SHA-256: `ef6c351fea863ac38d0403f89d5fa64108fe63860fcc8704bff790fd60d01c4e`.
- Demo pre-version-bump SHA-256: `88753b793f6f848b2ccdf760836a319b6c1a6aef8cfa81b03a451ef0b849d057`.
- Commits: RED `53b10f8`; smoke `9ff53d4`; package evidence `2494275`.
- Full detail: `tasks/20260903-191047-phase3-playable-vertical-slice/pact-33-evidence.md`.

## Historical PACT-34 handoff (superseded by independent audit)

- Candidate documentation/version/screenshots commit: `96f88cb`.
- Complete project-local AutomationTool log/final/commandlet-saved correction: `5157b9d`.
- A pre-final aggregate at `96f88cb` passed every functional gate but was rejected because Cook's temporary log still selected the installed Engine Saved directory. The corrected scripts set all three UAT environment boundaries; the accepted aggregate shows Cook/Pak/IoStore response paths under `Artifacts/Logs/uat-*`.
- The former handoff reported 63 passing tests and matching last-manifest pointers. The independent audit subsequently identified capture/input/lifecycle/evidence-authority gaps; those historical assertions are not current release certification.
- Its Draft PR/source history and original PACT records are retained. Remote/tag/Release status must be freshly read, not inferred from this historical section.

## Independent audit, September 4

The four source audit documents are preserved under `tasks/20260904-102407-phase3-independent-release-audit/source-audit/`; byte-identical originals and provenance hashes are retained under Artifacts. The [execution journal](../tasks/20260904-102407-phase3-independent-release-audit/progress.md) records each independent RED, minimal repair, verification, review and commit.

Repairs cover explicit failed-run cleanup/recovery; persistent Controller R/N; current-axis Dash; checked/wide A* and encounter arithmetic; separate pending/applied run/request identity; render-owned captures and Slate HUD; immutable source/process/artifact evidence; and real path/movement/ordinary-input/restart ownership. The old clipped image, invalid tests/fixtures and rejected storage runs remain historical failures, not current passing evidence.

| Observed checkpoint | Result |
|---|---|
| H7 Editor build / focused | `190812` strict build; `190920` 7/7, zero warnings/errors |
| Complete Automation | `191006` 119/119, zero test/whole-log warnings/errors; diagnostic-cbd7ac3 |
| Editor gameplay | `191146`: 8 real path moves / 20.9972 units before start frame 37; all 3 original images inspected |
| Editor ordinary input / negatives | `191243` four-run input proof; `191331..191349` four native exit-2 cases |
| Actual BuildPlugin | clean f235e46, 3 actual targets, strict product/log proof and 58-file index |
| First Cook attempt | clean d896e5c native BCR success, wrapper rejected; relative diagnostic and optional EditorDomain client corrected |
| Corrected Win64 package | clean a9f5625 `193928`, full loose Cook/Stage/Pak/Archive, no Zen initialization records |
| Packaged ordinary input | `194029`: 13 effects, four runs, ten transitions, four queues; 24 real path moves / 20.4258 units |
| Packaged gameplay | `194036`: 4 real moves / 22.3146 units before first capture frame 10; 3 original PNGs inspected |
| Packaged negative runs | `194046..194056`: Grid/Encounter/CapturePath/RenderUnavailable, native exit 2, no outer timeout |
| Package seal | `194106` outer manifest and independent 49-file index; archive SHA256 below |

The clean package source is `a9f56254f11554316302936926211e75d86d7f4d`. Its archive SHA256 is `120fdaf5b0ec4bad856bedf9ed11690ff8bc94cd0e40c50d578c58dae834967e`. Exact original paths are in `Artifacts/Package/gameplay-package-20260904-194106-88cf81446f8f4efd81b9a6aae3984815.json`; index and checksum are in `Artifacts/Logs/uat-demo-20260904-193928-48c50dfed8b345658bbfa84616782cce/`.

Native SHA declarations remain unverified by the executable itself. The external clean context, actual process results and frozen hashes provide the documented authority. The original 41 tests and default layout/encounter identities remain unchanged; render pixels and full real-time simulation are not deterministic promises.

## Final candidate and publication authority

The documentation/image update changes HEAD, so the checkpoints above cannot be merged into a fictional single final run. Run the complete `VerifyPhase3.ps1` from that new clean candidate. It returns `MachinePassed` with pending visual/remote review and an independent evidence index, not a Release.

Inspect all six images from that exact run, freshly fetch/read PR #1 and provide candidate-bound review JSON to `FinalizeRelease.ps1`. The final exact SHA, counts, timestamps and digests belong in those generated records and the PR/Release body, avoiding a self-referential tracked-SHA cycle.

The user has explicitly authorized subsequent merge and source-only publication after all 16 gates. No binary Release assets are uploaded. Publication status is verified remotely; this journal does not claim that those final operations have already happened.

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

## PACT-34 and final authority

- Candidate documentation/version/screenshots commit: `96f88cb`.
- Complete project-local AutomationTool log/final/commandlet-saved correction: `5157b9d`.
- A pre-final aggregate at `96f88cb` passed every functional gate but was rejected because Cook's temporary log still selected the installed Engine Saved directory. The corrected scripts set all three UAT environment boundaries; the accepted aggregate shows Cook/Pak/IoStore response paths under `Artifacts/Logs/uat-*`.
- At handoff, `Artifacts/Reports/phase3-verification-last.json` is `Passed`, version 0.3.0, and names the exact Git HEAD.
- The current `Artifacts/Plugin/last-plugin-package.json`, `Artifacts/Package/last-gameplay-package.json`, latest Phase 2 summary, and packaged gameplay trace/summary name that same revision.
- Final Automation is 63 passed, 0 warnings, 0 failures, 0 not-run, and 0 in-process.
- The remote feature branch resolves to the same HEAD and Draft PR #1 is open, unmerged, based on the immutable 0.2.0 `main` revision.
- No remote `v0.3.0` tag or formal Release exists.

The exact final SHA, timestamped artifact paths, and SHA-256 values are read from those machine-readable files and the Draft PR body rather than duplicated into this self-referential tracked document.

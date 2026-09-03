# PACT-34 Evidence Contract

## Candidate metadata and documentation

- Project/plugin candidate version: 0.3.0; generator version 1 and layout schema v1 remain unchanged.
- `v0.2.0` and its published Release remain fixed at `9a306f8ff72cb660d3c04b09806787df21191d45`.
- README leads with the playable Seed -> Layout -> Encounter -> A* -> World -> Extract -> packaged evidence path.
- Added current architecture, gameplay loop, Phase 3 code walkthrough, Phase 3 interview guide, 12 live-change drills, Phase 3 acceptance matrix, evidence journal, and candidate release notes.
- Updated current architecture, development, limitations, AI disclosure, historical walkthrough/interview/acceptance links, release notes, rollback, and candidate handoff.
- Committed fixed-seed start/combat/win PNGs come from the visually inspected Editor gameplay smoke for seed 24301.
- Local Markdown link audit reports no missing repository-relative targets.
- Unfinished-marker scan reports no placeholder entries in current delivery docs/task journal.
- Candidate metadata Editor build passed; log `Artifacts/Logs/build-editor-20260903-202947.log`.
- Candidate metadata full Automation passed 63 of 63 with 0 warnings/failures/not-run/in-process; log/report `Artifacts/Logs/automation-20260903-203021.log`; `Artifacts/Reports/automation-20260903-203021`.

## Accuracy boundaries

- Documentation distinguishes layout/initial-encounter determinism from whole-run behavior.
- It does not claim NavMesh, Behavior Tree, GAS, multiplayer, persistence, production art, cross-platform support, or independent user authorship.
- The packaged combat-frame edge clipping is recorded as a P1 evidence-quality limitation with a root-cause packet; packaged start/win and Editor combat frames remain the selected proof.
- 0.3.0 is candidate metadata only. This PACT does not create a tag or formal Release and does not merge `main`.

## Final authority

After this documentation/version commit, `Scripts/VerifyPhase3.ps1` must run from a clean worktree without further source/doc edits. Its authoritative report is `Artifacts/Reports/phase3-verification-last.json`; the same revision/version must appear in `Artifacts/Plugin/last-plugin-package.json`, `Artifacts/Package/last-gameplay-package.json`, the packaged gameplay trace/summary, and the preserved Phase 2 report summary.

The final Draft PR body quotes those machine-readable files for the exact HEAD, Automation counts, BuildPlugin, BuildCookRun, ordinary launch, packaged smoke, screenshots, limits, and AI disclosure. Git status must remain clean after verification, push, and PR creation.

## Rejected pre-final aggregate

An aggregate at `96f88cb3ddceb7f264873361a174ed695438b994` passed every functional gate, including 63 tests, both gameplay smokes, BuildPlugin, and BuildCookRun. It is not accepted as final because installed-build AutomationTool chose Engine `Programs/AutomationTool/Saved` for the Cook commandlet's temporary `-abslog` even though UAT/UBT logs used the project-local `uebp_LogFolder`.

Local UE 5.8 source shows `StartRunCommandlet` derives that path from the separate `uebp_EngineSavedFolder`. Both package scripts now set LogFolder, FinalLogFolder, and EngineSavedFolder to the same unique project-local UAT evidence root. A fresh clean-revision aggregate after this correction is the only final authority.

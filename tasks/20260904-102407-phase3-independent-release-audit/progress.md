# Independent release audit — local execution

## Authority and source preservation

- Byte-identical source files are preserved under `Artifacts/IndependentAudit/20260904-102407/source-audit/`. The tracked `source-audit/` transcripts remove only Markdown trailing spaces to pass the unchanged whitespace gate; historical conclusions and `not-run` entries are not local evidence. See `source-provenance.md`.
- Latest user instruction authorizes fixes, pushes, Draft PR #1 updates, then merge and a source-only GitHub Release after all 16 gates pass. It supersedes the packet's older no-merge/no-release boundary.
- Do not upload plugin/demo ZIPs as Release assets. Local packaging and verification remain mandatory; GitHub's default source ZIP/tarball are the only release downloads requested.
- One primary agent, existing checkout only, one UE writer process at a time; Engine read-only.

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
- [ ] E — SF-IRA-007/009: true request identity and pending hashes.
- [ ] F — SF-IRA-005: render-owned screenshots and strict PNG proof.
- [ ] G — SF-IRA-002: immutable clean-revision evidence and negative harness.
- [ ] H — SF-IRA-008: real path/movement/restart/input integration.
- [ ] I — documentation and disclosure calibrated to new evidence.
- [ ] Gates 1–16: all not-run for this audit until fresh results are recorded.
- [ ] Publication: final PR update, merge, source-only Release, remote re-read.

## Current boundary

Task A commit: `b02bd9e2e5a2e379f99d545be635c5447921eb03`; RED/GREEN/process evidence is in `sf-ira-001-evidence.md`. Latest full suite is 69/69 with zero test warnings/failures, not yet final certification. Positive Editor gameplay smoke also passed at this clean commit: `Artifacts/Reports/Gameplay/20260904-104817/summary.json`.

Task B commit: `5bf6bfeada7b4889ad4fc2c92988092c4bb14a1f`; real Controller input/possession evidence and its fixture root-cause packet are recorded here.

Task C commit: `554ff5ff61182343f789f4e829ac196e4226b76c`; combined live Dash and independent review evidence are in `sf-ira-004-evidence.md`. Its clean-commit Editor smoke passed at `Artifacts/Reports/Gameplay/20260904-112322/summary.json`.

Task D has passed safe-coordinate RED→GREEN and independent read-only review; see `sf-ira-006-evidence.md`. Latest full suite is 83/83, zero test warnings/failures/not-run/in-process (`automation-20260904-113017`). Next: task E / SF-IRA-007 and 009, true request/run identity and clearing stale hashes while a new run is pending. Final Gate 14 still needs the whole-process UE Dataflow initialization log diagnostic reviewed; it is not silently allow-listed. Task G must also address the observed UBA diagnostic/trace-path boundary. Previous package runs are historical, not independent audit certification.

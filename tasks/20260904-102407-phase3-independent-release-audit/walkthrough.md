# Independent release audit — machine-verification walkthrough

This required finishing-workflow audit record captures the complete machine run at clean `5a91934aff15de846b80238d0d14c9961f2dbdcb`. Adding this record creates a new documentation-only candidate, which must be fully reverified. This file does not claim that remote/visual finalization, merge or publication has already happened.

## Actual passing terminal evidence

The primary ran `pwsh -NoProfile -File Scripts/VerifyPhase3.ps1` in the existing checkout. Selected original output from that run:

```text
Repository audit passed: 254 tracked files, 81 source/build-rule files.
Automation passed 119 test(s) for 'SeedForge'. Log: D:\program\SeedForge\Artifacts\Logs\automation-20260904-201330-221b9597508f497fa24ec3e97a99b594.log Report: D:\program\SeedForge\Artifacts\Reports\automation-20260904-201330-221b9597508f497fa24ec3e97a99b594
Automation passed 16 test(s) for 'SeedForge.Gameplay'.
Automation passed 7 test(s) for 'SeedForge.GameplaySmoke'.
Automation passed 5 test(s) for 'SeedForge.Model.Path'.
Automation passed 5 test(s) for 'SeedForge.Model.Encounter'.
Automation passed 5 test(s) for 'SeedForge.Model.RunState'.
Automation passed 47 test(s) for 'SeedForge.Audit'.
Phase 2 report integration passed. Run: D:\program\SeedForge\Artifacts\Reports\Phase2\20260904-201509-3684814ba89349659dfc911ba6bb296b Summary: D:\program\SeedForge\Artifacts\Reports\Phase2\20260904-201509-3684814ba89349659dfc911ba6bb296b\report-summary.json
SeedForge plugin package passed.
SeedForge Win64 package and smoke passed.
Run failure contract passed: Grid, runtime exit 2, failed trace reparsed, no outer timeout.
Run failure contract passed: Encounter, runtime exit 2, failed trace reparsed, no outer timeout.
Run failure contract passed: CapturePath, runtime exit 2, failed trace reparsed, no outer timeout.
Run failure contract passed: RenderUnavailable, runtime exit 2, failed trace reparsed, no outer timeout.
Repository audit passed: 254 tracked files, 81 source/build-rule files.
Machine gates passed for 5a91934aff15de846b80238d0d14c9961f2dbdcb. Visual inspection and fresh remote review remain required.
Summary: D:\program\SeedForge\Artifacts\Reports\Phase3Verification\20260904-201314-2dd334478d344aab8568e0b78342231c\summary.json
```

Focused-test lines above reproduce the original leading success clause; their full original log/report paths are retained in the machine summary. The complete suite has zero warnings/failures/not-run/in-process and zero whole-log errors/warnings. Known narrowly allowed environment warnings in real RHI runs are separately counted; they are not claimed absent.

## Changed physical boundaries

- `SeedForgeRunState`, `GameplayTypes`, `GameplayCoordinator`: Failed/recovery, centralized cleanup and truthful pending/applied run/request identity.
- `SeedForgeGameplayActors`: persistent Controller restart input, live-axis Character Dash, ownership/reset and native Slate HUD.
- `SeedForgeGridPathfinder`, `SeedForgeEncounter`: checked neighbors and int64 distance/cost math without changing layout/encounter goldens.
- `SeedForgeCaptureTypes`, `SeedForgeGameplayCapture`, PreviewActor/Inspector paths: owning render/pixel/save completion, current token/path/frames and cancellation.
- `SeedForgeGameplayDiagnostics`, `SeedForgeInputSelfTest`, `SeedForgeGameplaySmoke`: actual enemy motion, ordinary UE input/async restarts, passive path-before-capture and bounded serialized proof.
- Focused Automation tests and synthetic PowerShell rejection harnesses: preserved REDs, fixture failures, source/digest/path/type/ownership counterexamples and compatible controls.
- `Scripts/VerificationContract.ps1` and existing build/test/report/package/final wrappers: frozen clean source, strict original logs, exact target/process correlation, archive/PNG checks and immediate child evidence snapshots.
- Runtime storage/BuildEnvironment and the narrow project Automation adapter: repository-owned caches/temp, supported loose Cook/disabled optional Zen attachments, direct build-only remote-disable and read-only Engine boundaries.
- Current README/architecture/workflow/limitations/disclosure/study docs and the three byte-identical inspected packaged illustrations.

Exact diffs, per-finding commits and failed attempts remain in Git and this audit directory. Historical source-audit/PACT records are not rewritten as newly passing evidence.

## How the success criteria are tested

The machine summary binds this run's Editor build/load, complete/focused tests, canonical report/diff/10k benchmark, ordinary Editor input, Editor gameplay/three PNGs/four negatives, Inspector, actual three-target BuildPlugin and full Win64 package workflow to one clean SHA. The plugin archive SHA256 is `22c4bab801b24706e8630b9985386308e64e985d2daaed19a2b4838a6f96ea56`; the Win64 archive SHA256 is `ef2f328646ad489ffef13a91a3e97d342d481f5184ee55aec5d175ade41d228a`. Adjacent checksums and an independently reread evidence index accompany the unique summary directory.

Native SHA declarations are not source attestation. Ordinary input remains separate from the state-driving smoke, setup teleports/public damage are disclosed, and real path observations precede capture. Screenshot bytes are decoded and lifecycle-checked; original-resolution visual review is still a separate required act.

The workspace is the original normal Git checkout, not a new clone/worktree. The user already chose push/update existing PR #1, then merge and source-only Release after all gates, so no new integration menu or permission is needed. Existing branches and older tags/releases are preserved; no binary Release assets are uploaded.

## Final handoff boundary

After this record is committed, rerun the full clean pipeline, inspect its six new original images, push the exact validated feature commit with PR #1 still Draft, fetch/re-read its matching head/base/reviews/checks, and supply those actual review records to FinalizeRelease. Only then execute and verify the authorized merge/tag/source-only Release. Final exact SHA/results/publication records are generated under Artifacts and in GitHub, not inserted recursively into frozen tracked source.

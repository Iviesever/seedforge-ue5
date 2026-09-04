# Phase 3 handoff — pre-final documentation checkpoint

## Status boundary

This file records the pre-final documentation checkpoint following clean `a9f56254f11554316302936926211e75d86d7f4d`. At that checkpoint, the documentation/images candidate still needed its own final same-revision machine and manual gates; `v0.3.0` had not been tagged or published. Current status comes from generated readiness evidence and the actual PR/Release state, not this static snapshot.

The user has already authorized commits/push, PR #1 merge and a source-only GitHub Release after all 16 gates. No new authorization is implied or requested here. Authorization does not mean those actions or gates have occurred. No generated binary Release assets are uploaded; publication uses GitHub's default source archives only.

## Observed checkpoints, not one final candidate

| Evidence | Source boundary | Observed result |
|---|---|---|
| Full Automation | `diagnostic-cbd7ac3412fb1500d62dff9426eaed1bf30f4ec9` | 119/119; zero test warnings/failures and whole-log errors/warnings |
| Actual BuildPlugin | clean `f235e46e9f637d706b9997e8af542873591c86f5` | UnrealEditor Win64 Development; UnrealGame Win64 Development and Shipping |
| Actual PackageGameplay | clean `a9f56254f11554316302936926211e75d86d7f4d` | BuildCookRun, ordinary single capture, ordinary input/restarts, gameplay/path/captures and four typed expected negatives |

Exact local anchors:

- Full suite: `Artifacts/Reports/automation-20260904-191006-a5020ffb51174f3f9c952add67ae2545/verification-summary.json`.
- Plugin: `Artifacts/Plugin/plugin-package-20260904-182919-167f22b4dec54a188f7bc3e792888e9f.json`.
- Packaged outer manifest: `Artifacts/Package/gameplay-package-20260904-194106-88cf81446f8f4efd81b9a6aae3984815.json`.
- Packaged 49-file index: `Artifacts/Logs/uat-demo-20260904-193928-48c50dfed8b345658bbfa84616782cce/evidence-index.json`.
- Packaged input: `Artifacts/Reports/InputSelfTest/20260904-194029-c7639a67818749e1974225b0e72bd1cd/summary.json`.
- Packaged gameplay/path/captures: `Artifacts/Reports/Gameplay/20260904-194036-01908f1a1d5c4d23a4073bd3e12307f2/summary.json`.

The four negatives are Grid, Encounter, CapturePath and RenderUnavailable: controlled exit 2 and ExpectedFailure, not an outer timeout. The failed 191653 packaging attempt remains unchanged; its native BuildCookRun exit 0 did not satisfy storage validation.

The primary inspected the a9f5625 packaged start/combat/win images at original 1280x720: complete readable Slate HUD and geometry, without cropped prefixes. Start may show initial exposure settling; images were not brightened or edited. Documentation illustrations retain original-byte provenance, but cannot replace the final candidate's new six-image review. Rendering and the complete real-time playthrough are not promised deterministic.

## Final machine and manual sequence

After the documentation/images commit, freeze that clean candidate and run one UE pipeline:

```powershell
$revision = (git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve candidate revision.' }
$machine = .\Scripts\VerifyPhase3.ps1 -ExpectedRevision $revision
```

The result is **MachinePassed**, with manual visual and remote reviews Pending. Use its timestamped `summary.json`, `evidence-index.json` and checksum under `Artifacts/Reports/Phase3Verification/`; `phase3-verification-last.json` only points to that run. Frozen child digests prevent later bytes from silently replacing earlier evidence.

Actually inspect all six new original-resolution Editor/packaged gameplay images. Push the exact machine/visual-validated feature commit with PR #1 still Draft, then perform a fresh fetch/PR API review of head/base/reviews/checks; an old remote head cannot satisfy the record. Create `seedforge.visual-review` and `seedforge.remote-review` records under `Artifacts`, tied to this exact revision and machine summary. Do not generate Passed records from old documents, images or remote state.

With `$visualReviewPath` and `$remoteReviewPath` set to those real absolute record paths:

```powershell
.\Scripts\FinalizeRelease.ps1 `
  -ExpectedRevision $revision `
  -VisualReviewPath $visualReviewPath `
  -RemoteReviewPath $remoteReviewPath
```

The finalizer verifies the machine index, exact package manifests and review records, then writes a local source archive, `release-readiness.json` and frozen final index. It explicitly records `published=false` and does not merge, tag or publish. The separately authorized GitHub workflow must then verify the merged PR, intended tag/source revision and Release state before publication is described as complete. Do not attach local plugin/demo/source ZIPs or other generated assets to the source-only Release.

Keep the final SHA, actual gate results and publication status in generated final evidence and the PR/Release record. Do not edit the frozen source merely to insert its own SHA; any further tracked change creates a new candidate requiring verification.

## Run the candidate

For this audited workspace, keep any extraction of a timestamped, manifest/hash-verified `SeedForgeDemo-Win64-0.3.0-*.zip` under project `Artifacts`. The packaged entry point is:

```text
Windows/SeedForge.exe
```

Controls: WASD move, mouse aim, Left Mouse Button attack, Space Dash, R restart same seed and N next seed. `-SeedForgeSeed=<uint64>` selects the deterministic initial layout/encounter, not deterministic live physics/rendering.

Use the verification wrappers for audited runs: they supply project-local user/log/cache paths. An uninstrumented manual launch is not process, storage or release evidence.

## Reproduce gameplay evidence

```powershell
.\Scripts\TestInputSelfTest.ps1 -Seed 24301 -ExpectedRevision $revision
.\Scripts\TestGameplay.ps1 -Seed 24301 -ExpectedRevision $revision
.\Scripts\PackageGameplay.ps1 -Seed 24301 -ExpectedRevision $revision
```

The ordinary input self-test uses UE input processing, separate from gameplay smoke. Logged setup teleports/public damage are not keyboard or path movement evidence. Gameplay smoke requires real same-run A* movement before its first render-owned screenshot, then preserves combat/core/exit behavior and three owned PNG receipts. `PackageGameplay` also runs the four expected negatives and freezes each child proof.

All project source, caches, temporary files and artifacts stay under `D:\program\SeedForge`; Engine/global configuration is not edited. Only internal UBT `Trace*.uba` diagnostics/backups have the approved LocalAppData UnrealBuildTool exception. See `DEVELOPMENT.md` for scoped filesystem DDC/temp/Cook controls and exact commands.

## Rollback

- Immutable 0.2.0 baseline/tag/release: `9a306f8ff72cb660d3c04b09806787df21191d45` / `v0.2.0`.
- Phase 3 branch: `feat/phase3-playable-vertical-slice`.
- PACT checkpoints and recovery guidance: `docs/ROLLBACK.md`.
- Do not use destructive reset on the user's checkout; revert focused commits or branch from the immutable tag.

## Portfolio warning

Codex using GPT-5.6 Sol performed implementation, testing, debugging, packaging, audit and documentation work. The user supplied goals, constraints, orchestration and acceptance decisions. Do not present the repository as independently hand-written code or presume the user already understands it. Read `AI_ASSISTANCE.md`, reproduce the evidence, explain the design and complete personally authored test-first changes before claiming those skills or contributions.

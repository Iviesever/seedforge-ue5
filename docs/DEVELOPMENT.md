# Development and verification

Observed-status statements in this file record the pre-final documentation checkpoint following clean `a9f56254f11554316302936926211e75d86d7f4d`. Current release status comes from the exact generated readiness evidence and PR/Release state, not this static checkpoint text.

## Prerequisites and working boundary

- Windows x64
- Unreal Engine 5.8 binary installation
- UE-compatible Visual Studio/MSVC and Windows SDK; record the actual selected versions from the UBT log.
- PowerShell 7 for the complete UE workflow. Pure PowerShell validators also have Windows PowerShell 5.1/PowerShell 7 test evidence; this does not imply every native-process timeout path was exercised on both hosts.

Run commands from `D:\program\SeedForge`. Scripts default to `D:\program\UnrealEngine\Epic Games\UE_5.8`; supported launchers accept `-EngineRoot`. Only one UBT/UAT/Editor/Cook/package writer may use this checkout at a time. The installed Engine is read-only infrastructure.

## Clean verification and focused commands

Verification entry points require a clean tracked/untracked source tree and freeze an exact 40-hex revision. `-ExpectedRevision` makes that expectation explicit; source changes during a step fail verification. The following are commands for a committed, clean candidate, not a claim that a final candidate already passed.

```powershell
$revision = (git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve candidate revision.' }

# Build and verify project/plugin load
.\Scripts\Build.ps1 -ExpectedRevision $revision
.\Scripts\Smoke.ps1 -ExpectedRevision $revision

# Full suite and focused namespaces; use the generated report's actual counts
.\Scripts\Test.ps1 -Filter SeedForge -TimeoutSeconds 900 -ExpectedRevision $revision
.\Scripts\Test.ps1 -Filter SeedForge.GameplaySmoke -ExpectedRevision $revision
.\Scripts\Test.ps1 -Filter SeedForge.Audit -ExpectedRevision $revision

# Separate ordinary input proof from the state-driving gameplay smoke
.\Scripts\TestInputSelfTest.ps1 -Seed 24301 -ExpectedRevision $revision
.\Scripts\TestGameplay.ps1 -Seed 24301 -ExpectedRevision $revision
.\Scripts\TestRunFailure.ps1 -Case Grid -ExpectedRevision $revision

# Produce real runtime and Slate Inspector images
.\Scripts\CaptureDemo.ps1 -Seed 24301 -ExpectedRevision $revision
.\Scripts\CaptureInspector.ps1 -Seed 24301 -ExpectedRevision $revision

# Produce two canonical documents, import/diff, and a 10k benchmark
.\Scripts\Report.ps1 -SeedCount 10000 -Warmup 100 -ExpectedRevision $revision

# Independently package plugin and Win64 demo
.\Scripts\PackagePlugin.ps1 -ExpectedRevision $revision
.\Scripts\PackageGameplay.ps1 -Seed 24301 -ExpectedRevision $revision
```

`CaptureGameplay.ps1` delegates to `TestGameplay.ps1`. `PackageDemo.ps1 -SmokeSeed 24301` performs BuildCookRun plus ordinary packaged single capture. `PackageGameplay.ps1` additionally runs ordinary input/restarts, gameplay/path/three captures, and four expected-negative cases: Grid, Encounter, CapturePath and RenderUnavailable.

For work in progress, only entry points declaring `-AllowDirtyDiagnostic` accept that mode: Build, Test, TestGameplay, TestInputSelfTest and TestRunFailure. For example:

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit -AllowDirtyDiagnostic
.\Scripts\TestInputSelfTest.ps1 -Seed 24301 -AllowDirtyDiagnostic
.\Scripts\TestGameplay.ps1 -Seed 24301 -AllowDirtyDiagnostic
```

Diagnostic identities retain their `diagnostic-` prefix and cannot certify artifact manifests or release readiness. Native `sourceVerified=false` stays false even for a clean-shaped declared identity; authority comes from the external source/process context. Never strip the prefix or relabel failed/diagnostic artifacts.

## Final machine and manual gates

After documentation and illustrative images are committed, freeze that clean candidate:

```powershell
$revision = (git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve candidate revision.' }
$machine = .\Scripts\VerifyPhase3.ps1 -ExpectedRevision $revision
```

`VerifyPhase3.ps1` runs the machine portion of the 16-gate acceptance contract: repository audits, Editor build/load, complete/focused Automation, canonical documents/diff/10k benchmark, Editor ordinary input, gameplay/path/captures and four negatives, Inspector capture, actual three-target BuildPlugin, and the complete packaged workflow. It freezes child digests immediately and writes a timestamped summary and independent evidence index. `VerifyAll.ps1` now delegates to this same pipeline; it is not a separate historical finalizer.

The result is **MachinePassed**, with visual and remote reviews Pending. At the pre-final documentation checkpoint, the latest observed full suite was 119/119 at `diagnostic-cbd7ac3...`; final certification must use the new run's actual count and exact revision.

Then actually inspect all six new original-resolution Editor/packaged start/combat/win images. After the machine/visual checks, push that exact validated feature-branch commit while PR #1 is still Draft; the remote head must match before remote review can pass. Perform a fresh fetch/PR API review of head/base, reviews, checks and freshness. Create candidate-bound records under `Artifacts` only after those reviews. Set `$visualReviewPath` and `$remoteReviewPath` to those real absolute record paths, then run:

```powershell
.\Scripts\FinalizeRelease.ps1 `
  -ExpectedRevision $revision `
  -VisualReviewPath $visualReviewPath `
  -RemoteReviewPath $remoteReviewPath
```

Both records must identify the exact candidate and timestamped machine summary with `result=Passed`. The visual schema is `seedforge.visual-review`, containing six unique current image paths/hashes at 1280x720. The remote schema is `seedforge.remote-review`, with PR 1, the same head revision and zero behind; the reviewer records the actual remote observations. The script checks supplied records; it does not perform those reviews itself.

Finalization writes `Artifacts/Final/SeedForge-0.3.0-<stamp>/release-readiness.json`, a local source archive and frozen final index. It records `published=false` and does not merge, tag, upload or publish. PR #1 merge and a source-only GitHub Release are authorized after all gates, but remain separately executed and verified actions. Use GitHub's default source archives only; no generated binary Release assets are uploaded.

Do not edit the frozen candidate merely to insert its own final SHA or publication result into tracked documentation. Put those exact results in generated final evidence and the PR/Release record; any further source/document/image change creates a new candidate requiring verification.

`AuditDelivery.ps1` is for historical directories containing `DELIVERY_MANIFEST.json`, not the new readiness schema. An old `LATEST.txt` or mutable `last-*` pointer never substitutes for current timestamped manifests and indexed bytes.

## Evidence locations

- `Artifacts/Logs/`: UBT, UAT, Automation, Commandlet, capture, and packaged-run logs.
- `Artifacts/Reports/automation-*`: UE Automation JSON/HTML.
- `Artifacts/Reports/Phase2/<timestamp>/`: canonical layouts, diff, benchmark, logs, revision-aware summary.
- `Artifacts/Reports/InputSelfTest/<run>/`: ordinary input trace, process observation, log and summary.
- `Artifacts/Reports/Gameplay/<run>/`: Editor/packaged gameplay/path trace and capture/process/log summary.
- `Artifacts/Reports/RunFailure/<run>/`: typed expected-negative trace/process/log evidence, not positive success.
- `Artifacts/Media/`: runtime, packaged, and Inspector PNG evidence.
- `Artifacts/Plugin/`: BuildPlugin directory and plugin package manifest.
- `Artifacts/Package/`: Win64 archive directory and package manifest.
- `Artifacts/Package/last-gameplay-package.json`: pointer to the full ordinary launch/input/gameplay/four-negative package evidence.
- `Artifacts/Release/`: versioned ZIPs and adjacent SHA-256 files.
- `Artifacts/Reports/Phase3Verification/<run>/`: exact machine summary, `evidence-index.json` and checksum. `phase3-verification-last.json` points to that run.
- `Artifacts/Final/SeedForge-0.3.0-<run>/`: local release readiness, source archive and final evidence index.

Generated paths, caches, binaries, and local user state are ignored by Git.

## Harness safeguards

UnrealEditor may return exit 0 even when Automation fails. `Test.ps1` reparses exported `index.json` and rejects warning-success, failed, not-run or in-process cases. Whole-log checks reject errors/unexpected warnings; narrow known environment warnings and exact expected-negative errors are explicitly classified. An outer timeout is never successful negative evidence.

Report commands use separate Unreal processes and reparse every JSON output. Benchmark timings are recorded, never thresholded. Only one UBT/UAT/Editor/Cook/package writer may target this checkout at once.

Input uses named Action/Axis mappings on Enhanced-compatible classes, not Input Action/Mapping Context assets. The opt-in self-test injects UE input and measures real effects; logged setup teleports/public damage are separate from measured input/path proof. The AHUD-owned HUD is native Slate. Capture owns viewport/render/pixel/save completion, token/run/request/frame identity, fresh PNG bytes and cleanup; it does not infer success by file-size polling. Neither render pixels/exposure nor the complete real-time playthrough is promised deterministic.

All project output stays inside `D:\program\SeedForge`. The common helper uses process-local `UBA_ROOT`, filesystem DDC and `TEMP/TMP` under `.cache`. Runtime arguments select `SeedForgeLocal`, forbid default-graph fallback and supply the exact local DDC path. Cook also requires `-SkipZenStore` and `-ini:Editor:[EditorDomain]:CookAttachmentsEnabled=False`; Pak/IoStore receive the shared controls without duplicate forwarding. Relative legacy shader comparison paths require a unique native Base Directory matching the caller's known executable directory, followed by containment/reparse checks. UAT logs use isolated `uebp_LogFolder`, `uebp_FinalLogFolder` and `uebp_EngineSavedFolder` paths.

The only approved outside-project exception is UE/UBT's own `Trace*.uba` diagnostics/backups under `C:\Users\Iviesever\AppData\Local\UnrealBuildTool`. This does not permit arbitrary Unreal user-directory writes, Zen installation metadata, global shader/XGE temporary files or Engine/global configuration edits. Unexpected storage activity remains a failure to investigate; log checks are not a universal filesystem-write attestation.

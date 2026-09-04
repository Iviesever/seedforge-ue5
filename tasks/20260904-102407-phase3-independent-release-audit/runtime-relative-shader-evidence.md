# Runtime storage: bound legacy shader paths and Cook entry contract

Scope: only `Scripts/RuntimeStorageValidation.ps1`, its existing `verify-runtime-storage-log.ps1` harness, and this independent evidence file. No launcher, Engine file, shared RCA document, Git index/HEAD, or UE process was edited/run by this subtask. The primary owns PackageDemo integration and the EditorDomain source RCA.

## Exact relative-path cause

The first clean BuildCookRun's complete Cook log is:

`Artifacts/Logs/uat-demo-20260904-191653-eaee4ebfb7c947e2817979a971c03001/Cook-2026.09.04-19.18.10.txt`

- Line 470: `LogInit: Base Directory: D:/program/UnrealEngine/Epic Games/UE_5.8/Engine/Binaries/Win64/`.
- Line 945: the legacy process-ID comparison path is `../../../../../../SeedForge/Intermediate/Shaders/WorkingDirectory/48552/`.
- Line 946: the actual cleaned shader directory is absolute and project-local under `.cache/Temp/UnrealShaderWorkingDir`.

Read-only installed UE 5.8 source explains the distinction:

- `Runtime/Engine/Private/ShaderCompiler/ShaderCompiler.cpp:856–858` builds the legacy path from `FPaths::ProjectIntermediateDir()` and prints it only to compare path lengths. The actual working directory is `ShaderBaseWorkingDirectory`, created from `FPaths::ShaderWorkingDir()` at line 857, cleaned at 861–867, and converted to an absolute write path at 869.
- `Runtime/Core/Private/Misc/Paths.cpp:496–498` constructs ProjectIntermediateDir from ProjectUserDir. At 1584–1586, default relative-to-full conversion uses `FPlatformProcess::BaseDir()`, not the shell working directory.
- `Runtime/Core/Private/Misc/App.cpp:424` logs that same `FPlatformProcess::BaseDir()` as the native Base Directory.

Combining the exact logged relative path with the independently known matching executable directory resolves to:

`D:\program\SeedForge\Intermediate\Shaders\WorkingDirectory\48552\`

This is inside the project. The change does not ignore the legacy message or reinterpret it as a write to another directory.

## Minimal API extension

```powershell
Assert-SeedForgeRuntimeStorage -Path $cookLog -ProjectRoot $projectRoot `
    -ExpectedExecutableDirectory (Join-Path $EngineRoot 'Engine/Binaries/Win64')
```

`ExpectedExecutableDirectory` is optional for existing absolute-path logs. When, and only when, the exact `Guid format shader working directory ... processId version (...)` message contains a relative path:

1. Require an explicit nonempty caller directory and exactly one native `LogInit: Base Directory` record in the same original log.
2. Require both directories to be absolute local Windows paths and equal after explicit normalization (case/slash/trailing separator normalization only follows normal Windows path semantics).
3. Reject drive-relative, root-relative, UNC, URI/colon, wildcard, and control-character syntax in the relative input.
4. Explicitly combine the relative path with the verified executable directory, then call the existing owned-path/reparse validation. No `Get-Location`, provider working directory, environment expansion, or implicit current-directory base is used.

The actual `Cleaned the shader compiler working directory` path, DDC paths, and XGE paths remain absolute-only. Base-directory metadata from CSV is not authority; duplicate native base records are rejected even when identical. A trusted base does not permit project escape, prefix escape, or a reparse target.

The compact proof adds only `RelativeLegacyShaderPathCount` and `BaseDirectoryMatched`. It deliberately does not return the external executable directory as an absolute metadata string that the evidence index might mistake for an output file. The original log SHA-256 still binds the native base record and all path observations.

## Separately approved Cook-only entry check

The primary additionally approved this exact required Cook argument:

```text
-ini:Editor:[EditorDomain]:CookAttachmentsEnabled=False
```

For native `-run=Cook` only, the helper now requires exactly one Editor ini override token, equal to that complete literal. Missing, Engine-instead-of-Editor, true/lowercase/nonprecise values, duplicates, conflicting overrides, comma-merged overrides, unrelated quoted switch text, and additional Editor overrides fail. The existing bare `-SkipZenStore` requirement remains. Non-Cook consumers are unchanged.

This validates the caller-owned Cook boundary; it does not modify configuration or disable anything itself. The primary owns the source-backed EditorDomain explanation and actual AdditionalCookerOptions insertion. **No LogZen prohibition or allowance was modified.**

## Observed synthetic RED/GREEN

All reports/fixtures are under `Artifacts/Reports/RuntimeStorageValidation/` and marked `SyntheticOnly=true`. No synthetic success is presented as runtime proof.

| Checkpoint | Result | Report directory |
|---|---:|---|
| Optional parameter placeholder + relative-path RED, PS5.1 | 82/85; three new valid relative controls fail | `20260904-112555-e11d70064c4f4b659b1c7a38482bdb66` |
| Relative-path GREEN, PS5.1 | 85/85 | `20260904-112710-2232a0f2f0a44aaa9a8a869e95000444` |
| Relative-path GREEN, PS7.6 | 85/85 | `20260904-112712-3ab5178d3cfa4abfb9b28da8e9f69a0e` |
| Cook attachments RED, PS5.1 | 85/94; nine missing/wrong/spoofed overrides fail | `20260904-112813-c94f5ecda9da477e8ac7241b4b306b0a` |
| Final GREEN, Windows PowerShell 5.1 | **94/94** | `20260904-112854-4240423aa7b04944b95a41824c5341f9` |
| Final GREEN, PowerShell 7.6.0 | **94/94** | `20260904-112856-387999fe64db42a69dfe36e67c8f163a` |

The relative controls exercise a project path containing spaces, normalized native base spelling, and an explicitly different shell working directory. Negative controls cover missing/duplicate/wrong/non-native/relative base metadata, unsafe relative syntax, boundary/reparse escapes, unchanged absolute-only cleanup/DDC/XGE rules, and unchanged Zen rejection. Every accepted result still checks the original log hash and compact binding fields.

Reproduce with either shell:

```powershell
powershell.exe -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-runtime-storage-log.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-runtime-storage-log.ps1
```

## Original Cook remains rejected

A read-only call against the unchanged original 191653 Cook log, with the correct ExpectedExecutableDirectory, now passes the legacy relative-path boundary and then rejects **line 1593: Zen service/storage activity**. The original file was not filtered, edited, or relabeled as passing. It also predates the newly required Cook attachment override. Another real process and all existing gates remain the primary's responsibility; this change does not certify the old run or grant a Zen exception.

## Independent drive-root review correction

Review found that trimming `D:\` to `D:` could make .NET Framework/Windows PowerShell 5.1 use its process current directory during a later Combine/GetFullPath. Two new cases set and finally restore the actual process current directory, using both a direct drive root and `D:\Unused/..`. Both were wrongly accepted before the fix: PS5.1 RED 94/96 at `20260904-113644-637bd996a2be4752877ee19ddf9d7af7`.

FullPath now rejects a normalized drive-colon-only result before it can reach any combination. The supported project and selected Engine/Binaries/Win64 directory are unaffected. Final GREEN is 96/96 on PS5.1 (`20260904-113728-9dc1c3f2a5fa43748d4ac20f032cd921`) and PS7.6 (`20260904-113730-d221a3c3ca694277898770fe517c2b10`). Pak/IoStore regression remains 55/55 (`20260904-113734-bc07bffcd6af460ab6e70ba42346c14f`). The independent reviewer rechecked the exact fix and both reports with no remaining issue for this finding.

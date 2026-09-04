# Runtime storage log validation

Implemented `Scripts/RuntimeStorageValidation.ps1` as an additional post-process evidence check. This subtask changed only that new helper, `verify-runtime-storage-log.ps1`, and this evidence document. It did not change launchers/configuration/existing helpers, run UE, or read/write global Zen metadata.

## API and scope

```powershell
. ./Scripts/RuntimeStorageValidation.ps1
Assert-SeedForgeRuntimeStorage -Path $runtimeLog -ProjectRoot $projectRoot
Assert-SeedForgeRuntimeStorage -Path $packagedRuntimeLog -ProjectRoot $projectRoot -RequireDdc:$false
```

`RequireDdc` defaults to true. False allows a genuinely cache-free packaged runtime, including one receiving the safe shared runtime arguments. Present DDC switches must still be complete and correct, but the writable marker is required only when `RequireDdc` is true or DDC activity is actually observed. Both modes require exactly one real `LogInit: Command Line:` record and reject unsafe observed storage. CSV profiler command-line metadata is not accepted as command-line authority.

The compact proof returns `Path`, original-byte `Sha256`, `Length`, `LineCount`, `Validated`, `RequireDdc`, `DdcStoreCount`, `ShaderWorkingPathCount`, and `XgeWorkingPathCount`. It includes no raw command line or other absolute paths that could confuse evidence-graph file indexing. The log must be nonempty and under the supplied SeedForge project's `Artifacts`. Existing reparse components are rejected. One write/delete-excluding read handle supplies both the SHA-256 and BOM-aware text, so the proof does not hash one version while validating another.

The helper validates **observed log evidence**, not every filesystem operation. It cannot attest that a log contains all writes, prove a process exited successfully, or replace source/process/path/freshness binding and whole-log error/warning checks. The caller must retain those gates. It never grants an error allowance. Its broad safety checks do not authorize filesystem writes, global configuration changes, Zen service operations, or the user-global shader/XGE paths seen in historical logs.

## Rules

- Exactly one `-DDC=SeedForgeLocal`, one bare `-DDC-NoDefaultGraph`, and one `-LocalDataCachePath=<ProjectRoot>/.cache/DerivedDataCache` in the native command line whenever DDC is required/observed or any of these switches is present. Duplicates, false-valued flags, quoted switch text nested in another argument, malformed quoting, UNC/relative/alternate/prefix/traversal escapes are rejected. Windows backslash/quote rules preserve literal escaped quotes; quoted paths and whole arguments with spaces are supported.
- Exactly one observed `Local: Using data cache path ...: Writable`, at that exact owned cache path. Delete-only/read-only filesystem stores do not satisfy this rule. Additional stores, external cache speed/write tests, inherited environment/editor overrides, writable pak caches, and fallback/default-graph selection are rejected.
- Any `LogZenServiceInstance` record is rejected: even its environment-path diagnostics indicate constructing the service path that the corrected graph must avoid. Related Zen storage/server initialization, installation, write, or launch markers and active Zen/shared/cloud DDC stores are rejected. Exact disabled shared/cloud records, installed-engine pak reads, and `zenstreaming="0"` metadata are not storage writes and remain accepted.
- Observed shader process-ID/temporary working directories and XGE cleaning directories must normalize beneath the project and contain no existing reparse component. The known Engine messages are parsed explicitly; unrecognized working-path forms fail closed instead of silently ignoring the path.
- A native `-run=Cook` command line must include exactly one bare `-SkipZenStore`. This verifies the child commandlet received the parent-owned cooker option; the helper does not run Cook or modify those arguments.

The log formats and ownership boundary were read from `root-cause-runtime-output-boundary.md`, the corrected Editor runtime log, and historical offending logs. The installed Engine filesystem remains read-only infrastructure; no global metadata snapshot was performed by this helper or harness.

## Synthetic RED/GREEN

All generated fixtures/reports stay under `Artifacts/Reports/RuntimeStorageValidation/`. Fixtures are visibly labeled `SYNTHETIC LOG FIXTURE ONLY - NOT RUNTIME OR RELEASE EVIDENCE`; each report has `SyntheticOnly=true`. A project-with-spaces fixture exercises quoting without relying on native runtime execution.

| Checkpoint | Result | Report directory |
|---|---:|---|
| Initial permissive stub RED, PS5.1 | 49 rejection failures; 8 passing controls | `20260904-101058-7150a0d3aa60456dae7006dbfc354a14` |
| Escaped-quote spoof RED, PS7.6 | 1 failure / 62 cases | `20260904-101513-6d8f8d3912c44bfea8ed5bddad82cdf7` |
| Cache-free packaged common-arguments RED, PS7.6 | 1 failure / 64 cases | `20260904-101807-4647cd3c344743e1a84371bd37dd57bc` |
| Final GREEN, Windows PowerShell 5.1 | **64/64** | `20260904-101834-e3e92d17c49f4021a26021f567e1b367` |
| Final GREEN, PowerShell 7.6.0 | **64/64** | `20260904-101835-f37aee621005470da420444934c3a5d2` |

Tests cover successful owned storage; quoted paths and complete arguments; Windows quote escapes; timestamp prefixes; path case/slash normalization; benign reads/disabled stores; cache-free packaged mode; cook forwarding; missing/duplicate/spoofed switches; missing/unsafe/wrong-access stores; Zen manifest writes/launch/initialization; shared/cloud use; graph fallback; external cache write tests; global/traversal/prefix/relative shader/XGE directories; unknown directory forms; empty logs; and exact original-byte hash preservation.

Reproduce:

```powershell
powershell.exe -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-runtime-storage-log.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-runtime-storage-log.ps1
```

## Read-only checks of parent-provided logs

- Corrected `InputSelfTest/20260904-180436-6c3a564786834aad832f67afe8c6dfde/runtime.log`: accepted; 1552 lines, 194521 bytes, one writable local DDC store, two shader working paths and one XGE path. SHA-256 `e814a3cfc8574b79533cdb8e7621fedfdb195a8bf3871fdf6907075042999466`.
- Historical `InputSelfTest/20260904-174538-5706158a156a42a78a5f65cfd5508e33/runtime.log`: rejected at line 991, `LogZenServiceInstance` activity. It is not relabeled as boundary-compliant evidence merely because its old input assertions passed.

These are checks of existing parent-produced logs, not fresh UE runs by this subtask. Parent integration, clean revision gates, real BuildPlugin/package execution, and H7 remain separate work.

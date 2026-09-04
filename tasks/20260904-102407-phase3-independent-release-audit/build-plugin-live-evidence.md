# First actual confined BuildPlugin checkpoint

This is fresh native checkpoint evidence for clean commit `f235e46e9f637d706b9997e8af542873591c86f5`, not the final all-16-gate candidate. No worktree/clone was created; stock BuildPlugin's temporary HostProject stayed under the repository Artifacts and was removed by stock UAT.

Command: `pwsh -NoProfile -File Scripts/PackagePlugin.ps1`, 2026-09-04 18:29:19 through 18:32:29 UTC+8. UAT native exit 0; wrapper exit 0. The actual extension loaded and emitted exactly three target-specific local-only adapter markers. Each actual UBT command contained direct `-UBADisableRemote`, and its log reported remote execution disabled. No inherited UBT_EXTRA_ARGS modification or Engine patch was used.

| Actual target | Native exit | UBT seconds |
|---|---:|---:|
| UnrealEditor Win64 Development | 0 | 108.69 |
| UnrealGame Win64 Development | 0 | 36.02 |
| UnrealGame Win64 Shipping | 0 | 34.79 |

The strict console/per-target log gate passed without an error/warning allowance. The target validator correlated all three invocations, process exits, fresh diagnostic logs and packaged products, including AMD64 Editor module DLLs and Development/Shipping precompiled/object records. UBT's reported installed Engine executable paths are output-target metadata; the plugin products are repository-local. This is not a claim that the packaged demo was built or launched.

Exact outputs:

- UAT console: `Artifacts/Logs/package-plugin-20260904-182919-167f22b4dec54a188f7bc3e792888e9f.log`.
- Native diagnostics: `Artifacts/Logs/uat-plugin-20260904-182919-167f22b4dec54a188f7bc3e792888e9f/`.
- Manifest: `Artifacts/Plugin/plugin-package-20260904-182919-167f22b4dec54a188f7bc3e792888e9f.json`.
- ZIP: `Artifacts/Release/SeedForgePlugin-0.3.0-20260904-182919-167f22b4dec54a188f7bc3e792888e9f.zip`, 85772051 bytes.
- SHA256: `4a257598f82f287849232cfe8c861b460e983305b99490c42619ef69bc02f7dd`; adjacent checksum, manifest and independent reread agree.
- Parent immediately snapshotted/rechecked 58 files and wrote `evidence-index.json` plus its checksum in the native diagnostic directory above. The source tree remained clean throughout and after that seal.

The two identified read-only metadata paths were unchanged before/after: Engine `EpicGames.ScriptBuild.props` remained absent; global UBT environment XML retained its January timestamp, 113 bytes and SHA256 `d3ba6666338ebc598f9b191b867ed6f8dc6301947bd79fa4b5a25ddededd59c8`. This targeted check is not a whole-Engine/filesystem attestation. The only external write exception remains the user-approved internal UBT Trace*.uba files.

H7 native RED scaffolding was released only after this checkpoint finished. BuildCookRun/Pak storage correlation, ordinary packaged input, packaged path/captures, final evidence/docs/remote review and source-only publication remain required. This local binary ZIP will not be uploaded as a Release asset.

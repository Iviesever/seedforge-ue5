# Clean Win64 PackageGameplay checkpoint

Source: `a9f56254f11554316302936926211e75d86d7f4d`, clean throughout the complete invocation and independent seal. Command: `pwsh -NoProfile -File Scripts/PackageGameplay.ps1`. No source/doc changes or second UE writer occurred during it. This is a successful clean package checkpoint, not the later final documentation candidate.

## BuildCookRun and storage

Run: `20260904-193928-48c50dfed8b345658bbfa84616782cce`. UBT returned 0 (unchanged targets up to date); UAT performed **FULL COOK**, Stage, Pak, Archive and returned 0 in 22.87 seconds. It did not use a skipped/iterative cook as a substitute.

- Native Cook received filesystem DDC/no-default/local-path, SkipZenStore and the exact EditorDomain CookAttachmentsEnabled=False override.
- Complete Cook log: `Artifacts/Logs/uat-demo-<run>/Cook-2026.09.04-19.39.49.txt`, SHA256 `4a59080cac7ad9d52b6e7813fc47e448ef34af8efd099f24678a7be436442192`.
- Storage proof: one writable owned DDC, two shader working-path records, one XGE path, one legacy relative path explicitly bound to the selected native executable Base Directory.
- Original UAT/stdout/full logs contain no LogZen initialization or failed EditorDomain connection. Both actual Pak/IoStore processes correlate through original announce/Running/exit/copy records with unique original-byte digests.
- Cooked storage is the loose package-store manifest, not the old ue.projectstore. No Engine/global configuration was edited. The five identified global Zen metadata size/hash/time triples were rechecked unchanged after this run; this is not whole-system I/O attestation.
- The project-local `Saved/Logs/AutoSDKInfo.txt` was also manually read: Win64 valid, Result Succeeded; its WriteFileIfChanged diagnostic reports zero changed files out of one requested write. Unsupported non-Win64 SDK diagnostics are display/status information, not claims of cross-platform support. That mutable auxiliary log is not represented as an immutable member of the package index.

All complete UAT/target/Cook/Pak logs passed the existing strict error/warning policy. The rejected 191653 attempt remains separate and unchanged.

## Packaged runtime sequence

| Process | Exact run suffix | Outcome |
|---|---|---|
| Ordinary single capture | 193928-48c50dfed8b345658bbfa84616782cce | Native exit 0; real gameplay ready, render-owned PNG |
| Ordinary input/self-test | 194029-c7639a67818749e1974225b0e72bd1cd | Native exit 0; 13 effects, four runs, ten transitions, four queues |
| Gameplay/path/captures | 194036-01908f1a1d5c4d23a4073bd3e12307f2 | Native exit 0; correct attack/kill/3 Core/unlock/Won, three receipts/PNGs |
| Grid negative | 194046-101-Grid | Native exit 2; exact ExpectedFailure, no outer timeout |
| Encounter negative | 194049-014-Encounter | Native exit 2; exact ExpectedFailure, no outer timeout |
| CapturePath negative | 194051-549-CapturePath | Native exit 2; exact ExpectedFailure, no outer timeout |
| RenderUnavailable negative | 194056-100-RenderUnavailable | Native exit 2; exact ExpectedFailure, no outer timeout |

All suffixes above are prefixed `20260904-` in the directories. Input and gameplay summaries are under their corresponding `Artifacts/Reports/InputSelfTest` / `Gameplay` folders; negatives are under `Artifacts/Reports/RunFailure`.

Ordinary input proves 24 actual path moves / 20.425755560396755 units and four fresh ownership snapshots. It uses no gameplay-smoke state driver. Native sourceVerified remains false; the external clean context supplies source authority. Packaged H7 proves a 129-cell A* route, four actual moves / 22.314629793167114 units, same run/request/revision 1/1/1 and proof before first capture frame 10. Passive/capture/key bindings are cleaned. Both retain the default golden identities.

The primary viewed all three gameplay PNGs at original 1280x720. HUD content is complete and readable, with no clipped prefixes; combat/win objects are sharp. The start image has darker initial exposure but visible player/room/HUD. No pixel editing or brightening was applied. `documentation-image-evidence.md` records exact copied-image hashes.

## Archive and independent seal

- Package manifest: `Artifacts/Package/package-20260904-193928-48c50dfed8b345658bbfa84616782cce.json`.
- Outer manifest: `Artifacts/Package/gameplay-package-20260904-194106-88cf81446f8f4efd81b9a6aae3984815.json`.
- Archive: `Artifacts/Release/SeedForgeDemo-Win64-0.3.0-20260904-193928-48c50dfed8b345658bbfa84616782cce.zip`, 406864401 bytes.
- SHA256: `120fdaf5b0ec4bad856bedf9ed11690ff8bc94cd0e40c50d578c58dae834967e`; archive reread, adjacent checksum and both manifests agree.
- The outer manifest retains original nested input/gameplay/package proofs and immediately frozen child records. The primary independently reread the returned graph and sealed **49 files** into `Artifacts/Logs/uat-demo-20260904-193928-48c50dfed8b345658bbfa84616782cce/evidence-index.json` and its checksum.

The source remained clean after the seal. Only then did I documentation/image work begin. Full all-16 verification must be rerun after that commit, including a fresh three-target BuildPlugin and final visual/remote review. The local archive is evidence only and will not be uploaded as a Release asset.

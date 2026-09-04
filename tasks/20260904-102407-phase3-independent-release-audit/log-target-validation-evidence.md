# SF-IRA-002 strict log and BuildPlugin target helpers

## Scope and status

AI-assisted bounded implementation in exactly these five assigned source/evidence files:

- `Scripts/LogValidation.ps1`
- `Scripts/BuildPluginValidation.ps1`
- `verify-log-validation.ps1`
- `verify-build-plugin-validation.ps1`
- This evidence document.

No Runtime, launcher, package, aggregate, Engine, Git index/HEAD, global configuration, worktree or clone changes were performed. No UE/UBT/UAT processes ran. The only executions were PowerShell fixtures, read-only source/log inspection and file hashes. Synthetic fixture files remain below their unique `Artifacts/Reports/LogValidation/` or `BuildPluginValidation/` directories. Header-only PE/COFF fixtures are not runnable builds or release products.

The parent must independently review/rerun these helpers and integrate them around real clean-revision verification. Helper GREEN is not G completion or release certification.

## Locked API and behavior

`Assert-SeedForgeLog -Path <file> [-AllowedWarnings None|UE58LocalEnvironment]` reads the complete nonempty log. The default profile permits no warnings. It rejects UE/compiler/UAT error severities, fatal/assert/ensure/crash markers, nonzero ExitCode markers, failed build/results, invalid arguments, and the severity-free `UbaServer - bind ... failed` diagnostic. Errors have no allowance. Every offending line is retained in the thrown diagnostic; successful records contain normalized path, SHA256, line/error/warning counts and complete allowed warning records with line numbers and rationale.

`UE58LocalEnvironment` matches exact observed warning text, after removing only standard UE timestamp/frame prefixes:

- Driver mismatch: installed `551.61`, suggested `591.86`.
- Exact unattended driver-warning policy fields: `bDeviceCanUpdateDriver=0, VendorHasEntries=0, IsUnattended=1, r.WarnOfBadDrivers=1`.
- Exact stock D3D12 async-queue batching fallback warning.
- Exact `r.MotionVectorSimulation` render-thread warning. This does not waive independent visual checks or change a CVar.
- Exact purpose `0` registration text for 17 explicitly enumerated EditorDataStorageUI factories: ActorLayersWidgetConstructor, AlertHeaderWidgetConstructor, AlertWidgetConstructor, AssetDataItemTypeWidgetConstructor, AssetDataLabelWidgetConstructor, AssetDataVirtualPathWidgetConstructor, AssetNameWidgetConstructor, DiskSizeWidgetConstructor, DynamicAssetDataColumnBaseWidgetConstructor, FolderCompatibilityWidgetConstructor, OutlinerUnsavedHeaderConstructor, OutlinerUnsavedWidgetConstructor, SocketWidgetConstructor, StaticMeshTrianglesWidgetConstructor, TypeInfoWidgetConstructor, UObjectLabelWidgetConstructor, VisibilityWidgetConstructor.

Unknown factories, changed purpose/driver fields, altered suffixes, and errors appended to an otherwise known warning are rejected. There is no generic UI-category or error exemption.

`Get-SeedForgeBuildPluginTargetProof -ConsoleLog <file> -DiagnosticRoot <dir> -PackageDirectory <dir> -StartedAtUtc <actual process start>` publishes exactly three records, only after all checks pass: UnrealEditor Win64 Development, UnrealGame Win64 Development, UnrealGame Win64 Shipping.

Each record includes actual target/platform/configuration, console/per-target log paths and independent hashes, UTC log start, successful native exit, and every validated packaged product's path/size/time/SHA256. Checks require:

- Exactly three ordered UBT `Running:` invocations with owned project/plugin/manifest/log arguments, successful Result then native exit, followed by successful UAT completion.
- Quote-aware argument tokenization, so field-like text inside another value cannot impersonate `-Project`/`-log`.
- Fresh nonempty per-target logs, matching command identity, UTC header and success after its command. Header comparison accounts only for UBT's second-resolution timestamp; file timestamps remain compared with the actual precise process start.
- Strict log validation on console and each target log; there is no internal error allowance.
- Packaged descriptor's actual three SeedForge module declarations. Editor `.modules` mappings plus all three fresh AMD64 PE DLLs; Runtime Development/Shipping `.precompiled` output lists plus every referenced fresh, nonempty AMD64 COFF/bigobj file.
- Owned Artifact paths, exact diagnostic/package/module-directory boundaries, no existing reparse traversal, no duplicate object outputs or escaped references.

HostProject is not required. Installed `BuildPluginCommand.Automation.cs:159-168` compiles/packages then deletes HostProject unless `-NoDeleteHostProject`; `-KeepHostProject` is not a supported substitute. Actual inspected prior products at `Artifacts/Plugin/SeedForge-20260903-205505` contain Editor DLL/module mappings and separate Development/Shipping runtime `.precompiled` manifests with relative object paths. These informed the fixture shape, not a release-success claim.

## RED to GREEN evidence

All commands ran from `D:\program\SeedForge`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-log-validation.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-build-plugin-validation.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-log-validation.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-build-plugin-validation.ps1
```

Each directory below contains the complete named-case `summary.json`:

| Stage | Fixture directory suffix | Passed / total |
|---|---|---|
| Log behavioral RED, permissive stub | `LogValidation/20260904-141055-921b07a1968541ec875dda4739feb992` | 0 / 56 |
| Log initial PS 5.1 GREEN | `LogValidation/20260904-141209-82375717308f44518f3c20241b87f3d2` | 56 / 56 |
| Target behavioral RED, empty proof stub | `BuildPluginValidation/20260904-141447-ff2970890d2c439db93a73bab24f7205` | 0 / 35 |
| Target implementation diagnostic | `BuildPluginValidation/20260904-141735-e4e365d2e54642d7bdb3b356dd602db9` | 34 / 35 |
| Target PS 5.1 GREEN | `BuildPluginValidation/20260904-141753-bd888d7da5ea423ca921f8de1d801f55` | 35 / 35 |
| Log expanded RED, whitespace-only input | `LogValidation/20260904-142358-18e32c8330b9470f83bd1d1e2e41f5eb` | 56 / 57 |
| Target expanded RED, ordered markers/quoted-value bypass | `BuildPluginValidation/20260904-142422-2552acfe02534aa7a2727e347386187f` | 35 / 38 |
| Log expanded PS 5.1 GREEN | `LogValidation/20260904-142512-6ed0e5066e394aea9df866a812b76f8f` | 57 / 57 |
| Target expanded PS 5.1 GREEN | `BuildPluginValidation/20260904-142513-0d559970111846dc9efcaa7dc30f5e23` | 38 / 38 |
| Log expanded PS 7.6 GREEN | `LogValidation/20260904-142528-5000d876d8284d3eb9e9f0d85d4a6f81` | 57 / 57 |
| Target expanded PS 7.6 GREEN | `BuildPluginValidation/20260904-142529-29057a4845e44b7ab5d641e2e8d8a2ff` | 38 / 38 |
| Target final PS 5.1 rerun, fixture move-boundary check | `BuildPluginValidation/20260904-142817-eed23fdf08db41129e3be102b23849d1` | 38 / 38 |
| Target final PS 7.6 rerun, fixture move-boundary check | `BuildPluginValidation/20260904-142819-b1aebb0e52d34905b48cb4462b09130f` | 38 / 38 |

The initial target implementation accidentally wrapped a preserved Modules array in another array, causing its positive control to fail. Correcting that PowerShell shape error produced genuine positive and negative GREEN. Self-review then added independent failing tests for ordering and quoted field impersonation before tightening the parser. One additional whitespace-only log test was likewise RED before implementation.

All four PowerShell files passed parser and trailing-whitespace checks (zero each).

Final production helper SHA256 values:

- LogValidation.ps1: `6b65daf2d5a88aa38036ccb71e0235cfbf469d2bd41c42cbb09dccd520ae0a62`
- BuildPluginValidation.ps1: `0dd1ea0570ee27d4d3a058978fc4a23a98a40eaca79aea70cca5ef6d929ffb8e`

## Real-log observations, not waived failures

- The actual `gameplay-smoke-editor-20260904-135834-1b710332b5e54dc6a953217b4719b506.log` is a positive read-only cross-check: all 25 warning occurrences match the explicitly enumerated environmental profile. Image quality remains separately assessed.
- `package-plugin-20260903-205505.log` is correctly rejected with **three errors**, lines 17, 99 and 145: severity-free UbaServer bind failures. Historical target/product existence does not make this a clean build gate.
- Contrary to the initial assumption that `automation-20260904-135813.log` was wholly clean, strict inspection rejects **13 `LogAutomationTest: Error: Condition failed` records**, lines 1550-1562. Its 97/97 result does not excuse pre-test log errors. Cause/integration handling is returned to the primary; no allowance or suppression was added.

## Limits and handoff

- This validates consistency of observed files, metadata and invocation transcripts. It cannot cryptographically attest that a compiler produced the files, and header fixtures explicitly demonstrate that distinction.
- Existing path links are rejected; hostile concurrent filesystem replacement is not claimed to be atomically prevented.
- The target contract is deliberately stock UE 5.8 SeedForge Win64 x64 Editor Development/Game Development/Game Shipping. Other target layouts must receive a separately reviewed contract rather than silent fallback.
- No main-branch commit/merge was made; the five files are left for parent review/integration. Real BuildPlugin listener configuration, pre-test error diagnosis, immutable-revision packaging and all final gates remain open.

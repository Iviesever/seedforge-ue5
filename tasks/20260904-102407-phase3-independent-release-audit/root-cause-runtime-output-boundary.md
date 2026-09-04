# Runtime output-boundary correction

## Observed failure; not accepted final evidence

`InputSelfTest/20260904-174538-5706158a156a42a78a5f65cfd5508e33/runtime.log` reports a write to the user's global Zen installation `zen_plugin_versions.json`, and shader/XGE temporary directories under the user's global Temp directory. Data-cache content itself is already project-local, but that alone does not confine Zen installation metadata or temporary working directories. The only user-approved exception is UBT's Trace*.uba diagnostics. These runs are not final boundary-compliant evidence; the user was informed. Existing external files are not deleted, rewritten or disguised.

## Ownership and supported correction

- Process TEMP/TMP: common build-environment helper now points both at repository `.cache/Temp`. The child-process probe first failed with `TEMP/TMP are not project-local` and `Child temporary path escaped project`; after the minimal process-only change it passed all existing root/argument/inheritance checks across 12 launchers.
- Zen install paths are separate from its DataPath. Installed ZenServerInterface.cpp derives its global install location from UserSettingsDir. Private SetLocalInstallPathOverride is testing-only and is not used.
- `-NoZenAutoLaunch` is not a no-side-effect guard: Initialize still creates the application settings directory and may take the install lock/uninstall a service based on config. Do not use it as a containment fix.
- Avoid constructing Zen at all. Project config defines a named graph of read-only project/installed-engine pak stores plus one explicit FileSystem store. That store has a project-local Path and CommandLineOverride, **no inherited Env/EditorOverrideSetting** (FileSystemCacheStore applies EditorOverrideSetting after the command-line path).
- All UE runtime launchers use `-DDC=SeedForgeLocal`, `-DDC-NoDefaultGraph`, and the explicit repository LocalDataCachePath. The no-default flag prevents fallback to global/Zen graphs if local stores fail.
- CookCommandlet.cpp 425–427 supports `-SkipZenStore`, selecting ordinary loose cooked output; UAT forwards it together with the same DDC flags through AdditionalCookerOptions. Normal Build/Cook/Stage/Pak/Archive remains required. No shared/cloud cache, global config, Engine patch, registry or service management is introduced.

## Acceptance before more UE execution

1. RED/GREEN argument/config/child-temp tests before the provider/config edits.
2. Inspect every generated launch argument and Cooker argument, with strict quoting.
3. Controlled Editor process: no Zen install/update/service write markers; shader/XGE temp paths remain under repository; existing global Zen metadata is unchanged; ordinary input and captures still pass.
4. Re-run full Automation and fresh UAT/package gates on a clean revision; preserve historical diagnostics as failures, not successful compliance evidence.

## First contained runtime probe

Argument/provider RED failed because Get-SeedForgeRuntimeArguments was absent. GREEN verifies the exact graph/store, path/no-fallback flags and 10 runtime launchers plus cooker forwarding. Shared environment GREEN verifies TEMP/TMP and child GetTempPath under `.cache/Temp`, unchanged opaque UBT_EXTRA_ARGS and 12 initializer call sites.

`RuntimeBoundary/20260904-180436-980fecb094494af8924401a6ddc920ea` retains before/after metadata for five known global Zen files. Size/hash/mtime/existence were unchanged. Its actual ordinary input run is `InputSelfTest/20260904-180436-6c3a564786834aad832f67afe8c6dfde`; native exit 0, full external input proof and strict whole-log checks passed.

The runtime log contains no Zen service initialization lines, identifies the explicit repository LocalDataCachePath as Writable, and reports shader/XGE working directories below `D:/program/SeedForge/.cache/Temp`. This is a controlled diagnostic probe, not a whole-filesystem attestation or final clean-source package evidence. Full Automation/capture/package repeats remain required.

## Parent integration and UAT review

Post-process storage proofs are retained beside the strict log proof in runtime summaries. The fresh integrated full suite (`automation-20260904-182442-b9fe06f5e6b54cd1a403fe4377218bb7`) passed 114/114 with no whole-log warnings/errors; ordinary input and gameplay (`182403`, `182556`) passed their explicit diagnostic contracts and observed storage checks. The two original-byte hashes match between the log and storage proof in each summary.

Installed `CopyBuildToStagingDirectory.Automation.cs` 4693-4699 already passes AdditionalPakOptions to IoStore; 4905 separately adds AdditionalIoStoreOptions. An observed duplicate-argument RED therefore led to passing the shared storage switches only through AdditionalPakOptions. Do not accept duplicate switches to conceal this forwarding mistake.

`RunUnrealPak` 485-501 has two outputs: abbreviated RunAndLog stdout named `UnrealPak-*.txt`, and a copied full native log named `UnrealPak_<Scenario>-*.txt`. Only the latter contains the native LogInit command line. The strict storage guard stays unchanged and must validate the complete CreateMultiplePaks and CreateIoStoreContainers logs. Before BuildCookRun certification, correlate these to the original UAT Running/ExitCode=0/SafeCopyFile sequence and reject extra/unmatched/Zen invocations; never manufacture a command-line record in a stdout copy. This dependency is open at this checkpoint.

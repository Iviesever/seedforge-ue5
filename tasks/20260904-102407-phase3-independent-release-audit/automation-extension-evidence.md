# Project-local stock BuildPlugin Win64 adapter

## Scope and status

Bounded AI-assisted implementation under approved `task-002-automation-adapter.md`. Only `Scripts/Automation/*`, this audit's `verify-automation-extension.ps1`, and this evidence document were created. No existing launcher, Runtime, Engine, Git index/HEAD or global configuration was modified.

Only the installed bundled .NET SDK was executed to restore/build the local C# projects and run their argument-contract harness. **No RunUAT, RunUBT, UnrealEditor, C++ build or package process was executed.** This is adapter compilation/argument evidence, not stock BuildPlugin loading or release target certification.

## Implementation

- `Scripts/Automation/SeedForgeBuild.Automation.csproj`: `net10.0` library named `SeedForgeBuild.Automation`, using only installed binary references. No PackageReferences, Engine ProjectReferences, or imported Unreal build projects.
- `BuildPlugin_Win64.Automation.cs`: public class with exact stock-discovered name, deriving from public `BuildPlugin.TargetPlatform`. Both abstract overloads compile against the installed UE 5.8 assemblies. The legacy string-executable overload explicitly throws. The active FileReference overload validates its DLL/input identity, obtains the owning repository from its loaded assembly location, calls the pure builder, registers the exact target manifest, and invokes stock `CommandUtils.RunUBT` once with the returned arguments.
- `SeedForgeBuildArguments.cs`: pure invocation builder. Supports only SeedForge's generated HostProject below the repository's Artifacts tree, default/explicit Win64 x64, stock UnrealEditor Development and UnrealGame Development/Shipping. It forwards x64 explicitly, preserves stock plugin/manifest/nohotreload/noubtmakefiles flags and the original additional-argument text, and adds the supported bare remote-disable switch directly to this build invocation. Unsupported platform/target/type/configuration/architecture, ambiguous architecture requests, conflicting owned fields/Mode/remote flags, response-file/end-of-options injection and unmatched quotes are rejected. A pre-existing single bare disable switch is not duplicated.
- `Directory.Build.props`: repository-local `Artifacts/Automation/<project>/net10.0` outputs and `.cache/AutomationExtension/obj/<project>` intermediates, plus local package cache/configuration. EngineRoot comes from a build property or the process-only `SEEDFORGE_ENGINE_ROOT` variable, never a second embedded installation.
- `NuGet.Config`: empty package-source list. The project uses bundled framework packs and installed binary references; no remote package feeds are needed.
- `Tests/`: a plain console harness links the same production pure-builder source and loads the compiled adapter for type/legacy-overload checks. It never invokes the live FileReference build overload.

The adapter contains **no environment writes**. Neither it nor the harness sets `UBT_EXTRA_ARGS` or any `UnrealBuildTool_*` variable. The harness checks that those inherited values remain unchanged.

## RED to GREEN

Commands, working directory `D:\program\SeedForge`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-automation-extension.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-automation-extension.ps1
```

The harness parameter `-EngineRoot` defaults to the existing authorized UE 5.8 installation and may be provided explicitly. It saves/restores only process environment values for .NET CLI/NuGet/temp/cache paths and disables telemetry/build-server reuse. All logs, summaries and synthetic path fixtures remain under the repository.

| Stage | Artifact directory under `Artifacts/Reports/AutomationExtension` | Result |
|---|---|---|
| Harness command assembly error | `20260904-153954-ba04f3b0bfbf47e5ba6f4743d695e1b5` | Restore MSB1008; not counted as RED |
| Compilable stubs, behavioral RED | `20260904-154024-20f2dc7dcd634476851bec7b2051fe88` | 36 total, 2 controls passed, 34 expected failures; both C# projects compiled with 0 warnings/errors |
| Implemented adapter, PS 5.1 GREEN | `20260904-154358-60d86ba4a0e24f16b33d40457945c91e` | 36/36; both projects compiled with 0 warnings/errors |
| Independent PS 7.6 rerun | `20260904-155331-68c31b2568ea46c5a62027777ef1ed99` | 36/36; both projects compiled with 0 warnings/errors |

Each completed run retains `restore-0.log`, `build-0.log`, `restore-1.log`, `build-1.log`, `tests.log`, `argument-results.json` and `summary.json`. The initial setup issue was an unparenthesized PowerShell concatenation that split `-p:EngineRoot=<path>` into separate arguments; it was fixed before behavioral RED and is not counted as test proof.

Fresh parser/trailing-whitespace checks passed. No generated bin/obj files appeared in the tracked source folder; outputs are in the explicit local directories.

Compiled adapter path:

```text
D:\program\SeedForge\Artifacts\Automation\SeedForgeBuild.Automation\net10.0\SeedForgeBuild.Automation.dll
```

SHA256 at the final harness check:

```text
3ed98ffbb2f25361a8cea929125066bb24d5dbc15050e5d6ae7ff36b3441a727
```

## Parent integration requirements

The parent owns `PackagePlugin.ps1`; no launcher changes were made here.

1. Save and restore `SEEDFORGE_ENGINE_ROOT` around this UAT process and set it to the same explicit EngineRoot used to locate RunUAT. Use the harness's process-only .NET CLI/NuGet/temp cache roots during any project-script compilation. Existing UAT diagnostic paths and UBA_ROOT/DDC confinement remain required.
2. Invoke stock BuildPlugin with the additional supported global argument `-ScriptDir=D:\program\SeedForge\Scripts\Automation`. Prefer `-ScriptsForProject=D:\program\SeedForge\SeedForge.uproject` to restrict project discovery. Keep the normal `-Plugin`, `-Package`, `-TargetPlatforms=Win64`, and `-Rocket` arguments. Optional `-Architecture_Win64=x64` is accepted and forwarded; other architectures fail explicitly.
3. Do **not** pass `-NoCompile`, `-Compile`, or `-IgnoreBuildRecords` to work around script loading. A plain .NET build does not create UAT's script-module build record; the first supported UAT script-discovery pass must validate/build the project-local script and generate its own record. Installed-engine code separately treats Engine modules as NoCompile, while building outside-Engine modules.
4. Verify UAT actually loads the local assembly and prints one `SeedForge BuildPlugin adapter: ...; x64; direct remote-disable.` marker per target. Then validate all three actual UBT invocations/results/products through the strict target proof helper. Confirm recursive WriteMetadata receives no inherited Build-only switch, and no bind failure or invalid-argument error exists anywhere in the logs.
5. Recheck all output locations, Engine/global environment XML timestamps/hashes, clean source revision and package artifact hashes. Standard HostProject deletion remains unchanged; its retention is not needed for target proof.

Relevant installed primary-source seams: `BuildPluginCommand.Automation.cs:210-243` discovers the extension name; `:393-395` dispatches actual target compilation; `Program.cs:74-101,499-520` processes ScriptDir; `CompileScriptModules.cs:193-234` separates installed Engine NoCompile from project script builds. `RunUAT.bat` defaults to the installed UAT binary without adding a global `-NoCompile` argument for project scripts.

## Remaining limits

This deliberately supports the approved SeedForge Win64 x64 profile, not arbitrary plugins/architectures. Actual extension loading, BuildPlugin execution, all three target proofs, source/process wrappers and release gates remain unverified here and belong to the parent. No synthetic argument result is a C++ build/package success claim.

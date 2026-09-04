# Build-only UBA switch leaked into recursive tool mode

The first cache/argument implementation passed the PowerShell harness and actual Editor compilation plus 86 tests, but `build-editor-20260904-115721.log` reported `Invalid argument: -UBADisableRemote` during WriteMetadata. That output is not accepted as a clean build gate.

UE source proves the boundary: UnrealBuildTool.cs appends UBT_EXTRA_ARGS for every process; WriteMetadataMode has ToolModeOptions.None and does not consume the UnrealBuildAccelerator build configuration switch. Sending that switch through the inherited environment is therefore wrong even though the parent Build mode successfully disables remote execution.

Independent review also found two defects in the proposed raw argument regex: `=true` is not the same argument key as the supported bare boolean switch, and quoted unrelated values can resemble a flag. The correction removes argument parsing/mutation from the cache helper entirely. Caller argument text remains opaque and unchanged.

Remote-disable is passed only in Build.ps1's Build arguments and in PackageDemo's documented BuildCookRun UbtArgs. Read-only BuildPlugin source inspection confirms it has no comparable arbitrary argument pass-through; no unsupported flag will be invented for it. Cache confinement still applies to all 11 entry points and inherited subprocesses. BuildPlugin diagnostics remain in task G, not silently waived.

The strengthened harness first failed against the original helper with changed/rewritten caller arguments and missing scoped build flags. It includes `=true`, a quoted false-form token, and switch-like text inside a quoted unrelated value, then requires all of them to remain byte-for-byte unchanged. It is rerun after the minimal correction before another actual build.

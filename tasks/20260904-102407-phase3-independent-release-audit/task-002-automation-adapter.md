# Stock BuildPlugin local-only UBT adapter

> **Execution:** test-driven-development and executing-tasks. Bounded helper implementation may be delegated; only primary runs UAT/UBT/UE. Installed Engine stays read-only.

## Evidence and choice

Historical BuildPlugin logs contain bare `UbaServer - bind 0.0.0.0:1345 failed (...)` diagnostics. They fail the new whole-log gate. Direct Build uses -UBADisableRemote successfully; stock BuildPlugin exposes no arbitrary UbtArgs forwarding. Global UBT_EXTRA_ARGS contaminates its recursive WriteMetadata (ModeOptions.None); XmlConfigCache in that variable has the same scope problem. No firewall/global environment XML/Engine mutation is authorized.

Installed BuildPluginCommand.Automation.cs exposes a public nested TargetPlatform subclass seam and discovers exact name BuildPlugin_Win64 from ScriptManager.AllScriptAssemblies. The supported UAT -ScriptDir option can load a project-local Automation assembly. This is a thin stock extension, not a replacement build/package pipeline.

## Contract and files

- New `Scripts/Automation/SeedForgeBuild.Automation.csproj`, `BuildPlugin_Win64.Automation.cs`; optional small same-project testable argument builder.
- Use binary References to installed AutomationScripts.Automation, AutomationUtils.Automation, EpicGames.Core, EpicGames.Build and UnrealBuildTool assemblies. No Engine ProjectReferences/imported build projects; project-local bin/obj/build records/NuGet/CLI state only. Parameterize EngineRoot rather than embedding a second installation.
- Public class `BuildPlugin_Win64 : BuildPlugin.TargetPlatform`; implement both abstract overloads. The active FileReference overload accepts only SeedForge, Win64 x64, stock UnrealEditor Development / UnrealGame Development / UnrealGame Shipping. Fail explicitly for unsupported legacy/architecture/target input.
- Preserve stock plugin/manifest/nohotreload/noubtmakefiles/architecture handling, add the returned manifest to ManifestFileNames, pass InAdditionalArgs unchanged, and append direct -UBADisableRemote only to the actual RunUBT call. Never set UBT_EXTRA_ARGS or UnrealBuildTool_* environment variables.
- Parent PackagePlugin loads -ScriptDir for this invocation only, preserves stock BuildPlugin target selection and packaging, then strictly validates all three logs/products.

## RED → GREEN and delivery

- [ ] First reproduce bare-bind rejection with existing strict helper (already covered by target/log fixtures and actual historical log).
- [ ] Add an argument-contract harness using clearly synthetic paths under Artifacts/Reports: all three exact target commands, manifest attribution, space quoting, appended direct flag, preserved additional args, invalid target/platform/plugin/legacy path rejection, no inherited environment mutation. Expose a pure argument method, not a test-only live UBT invocation.
- [ ] Observe RED against declarations/stubs, implement minimal builder/adapter, compile through a project-local harness without invoking UBT, then GREEN.
- [ ] Primary verifies actual UAT extension loading, all three targets, recursive WriteMetadata success, no bind errors and all writes confined to project plus approved Trace exception. Check Engine global environment XML unchanged.
- [ ] Independent review and exact evidence; no packaging/release claim from argument tests alone.

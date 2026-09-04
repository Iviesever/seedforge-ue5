# UE 5.8 startup trace output boundary — authority required

## Observed state

- Last verified implementation HEAD: `648b776a01e98cf1408d4b33bd19d23b297380db` (SF-IRA-007/009), full suite 86/86 with zero test warnings/failures. SF-IRA-006 is separately committed at `4b68da82c89f5dc1776a264fd94bb8dcd5d1b216`.
- Earlier build outputs explicitly reported `C:\Users\Iviesever\AppData\Local\UnrealBuildTool\Trace.uba`. A physical read confirmed this file exists and its write time advances during this workflow; at 11:42:08 it was 309 bytes after an Editor test launch's nested UBT platform check.
- `Artifacts/Logs/automation-20260904-114207.log:77` shows the Editor launches Build.bat `-Mode=ValidatePlatforms` with a project-local AutoSDK log, independently of the main Build.ps1 call.
- No new UE process is to be launched until this output-boundary conflict is resolved. No Engine file or global known-folder setting has been modified, and no trace/cache file outside the project has been deleted or moved.

## Read-only source proof

- `Engine/Source/Programs/UnrealBuildTool/UnrealBuildTool.cs:206–239`: default Trace.uba is selected, its directory created, previous trace backed up, and the global UBA trace created before normal arguments/configuration are parsed. `UBT_EXTRA_ARGS` is appended only afterward.
- The trace path uses `Unreal.EngineProgramSavedDirectory/UnrealBuildTool/Trace.uba` for an installed non-build-machine engine.
- `Engine/Source/Programs/Shared/EpicGames.Build/Unreal.cs:213–214, 486–512`: installed engines resolve this root from Windows `Environment.GetFolderPath(LocalApplicationData)`, falling back to CommonApplicationData, not the project's `-userdir` or UAT `uebp_EngineSavedFolder` setting.
- `UnrealBuildAcceleratorConfig.cs` declares `-UBATraceOutputFile`, but a source search finds no consumption of that property by the early global trace setup; `UBAExecutor.cs:312` reads the already-created global trace path. Disabling the build executor also does not skip the startup trace creation.
- A `-Session=` branch is reserved for recursive UBT actions. Do not spoof an internal session or patch Engine startup to bypass the boundary.
- Project `Saved/UnrealBuildTool/BuildConfiguration.xml` currently contains only the default empty Configuration element. It cannot affect the earlier global trace setup.

## Decision needed

The existing task requires all project outputs inside the repository and the Engine to remain read-only. No supported project setting for this early installed-engine trace root was found. The next safe step requires explicit user authority, not a silent waiver.

Recommended narrow exception: permit only UE/UBT's own default `Trace*.uba` diagnostic/backup files under `C:\Users\Iviesever\AppData\Local\UnrealBuildTool`; keep project sources, validation logs/reports, packages, screenshots and release evidence under `D:\program\SeedForge`. Then configure the separate UBA storage root inside `.cache` and disable unused remote execution through supported project/environment options before further builds.

If the user requires zero such default diagnostic writes, do not modify Engine files, global known-folder configuration, or user-directory junctions without separate approval. That stricter environment needs an authorized isolation/redirection approach before the UE verification gates can resume.

This is the first goal turn identifying the authority blocker. The goal remains active, not completed or marked blocked. Task F/G/H and all final release gates remain incomplete.

## Resolution — 2026-09-04

After the goal was marked blocked following three consecutive occurrences, the user explicitly replied `同意` to the narrow Trace*.uba exception. The goal is active again. Only UE/UBT's default trace/backup files at the named user-directory path are permitted; Engine and global folder settings remain untouched. Proceed with project-local UBA storage and supported remote-disable configuration, then resume F/G/H and the complete release audit.

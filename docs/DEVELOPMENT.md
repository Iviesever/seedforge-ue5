# Development and verification

## Prerequisites

- Windows x64
- Unreal Engine 5.8 binary installation
- Visual Studio with MSVC 14.44-compatible C++ tools
- Windows SDK compatible with UE 5.8
- PowerShell 7 or Windows PowerShell 5.1

The scripts default to `D:\program\UnrealEngine\Epic Games\UE_5.8`; override `-EngineRoot` when required.

## Commands

```powershell
# Editor target
.\Scripts\Build.ps1

# Headless project/plugin load
.\Scripts\Smoke.ps1

# Complete Automation suite
.\Scripts\Test.ps1 -Filter SeedForge

# Recreate the only project map
.\Scripts\CreateDemoMap.ps1

# Editor-game screenshot and runtime smoke
.\Scripts\CaptureDemo.ps1 -Seed 24301

# Independent HostProject plugin package
.\Scripts\PackagePlugin.ps1

# Build/Cook/Stage/Pak/Archive plus packaged EXE smoke
.\Scripts\PackageDemo.ps1 -SmokeSeed 24301
```

## Evidence locations

- `Artifacts/Logs/`: UBT, UAT, Editor, Automation, capture, and packaged-run logs.
- `Artifacts/Reports/`: UE Automation HTML/JSON reports.
- `Artifacts/Plugin/`: timestamped BuildPlugin directories.
- `Artifacts/Package/`: timestamped Win64 package directories and latest manifest.
- `Artifacts/Media/`: generated Editor and packaged screenshots.
- `Artifacts/Release/`: ZIP archives and SHA-256 files.

`Artifacts`, `.cache`, `.user`, `Binaries`, `Intermediate`, `Saved`, and generated plugin binaries are intentionally ignored by Git.

## Test harness behavior

UnrealEditor can exit with process status 0 even when Automation cases fail. `Scripts/Test.ps1` therefore parses the exported `index.json`, requires at least one executed test, and rejects warnings, failures, not-run cases, or in-process cases.

## Build concurrency

Only one UBT/UAT/UnrealEditor/Cook/Package writer should target the integration checkout at once. This machine has 16 GB RAM; competing UE processes would reduce reliability without improving throughput.

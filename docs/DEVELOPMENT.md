# Development and verification

## Prerequisites

- Windows x64
- Unreal Engine 5.8 binary installation
- Visual Studio / MSVC 14.44-compatible C++ tools
- UE-compatible Windows SDK
- PowerShell 7 or Windows PowerShell 5.1

Scripts default to `D:\program\UnrealEngine\Epic Games\UE_5.8`; use `-EngineRoot` to override it.

## Focused commands

```powershell
# Build the Editor target
.\Scripts\Build.ps1

# Verify project/plugin load
.\Scripts\Smoke.ps1

# Run all 41 UE Automation tests, or one namespace
.\Scripts\Test.ps1 -Filter SeedForge
.\Scripts\Test.ps1 -Filter SeedForge.Document
.\Scripts\Test.ps1 -Filter SeedForge.Diff
.\Scripts\Test.ps1 -Filter SeedForge.Benchmark
.\Scripts\Test.ps1 -Filter SeedForge.Editor

# Produce real runtime and Slate Inspector images
.\Scripts\CaptureDemo.ps1 -Seed 24301
.\Scripts\CaptureInspector.ps1 -Seed 24301

# Produce two canonical documents, import/diff, and a 10k benchmark
.\Scripts\Report.ps1

# Independently package plugin and Win64 demo
.\Scripts\PackagePlugin.ps1
.\Scripts\PackageDemo.ps1 -SmokeSeed 24301
```

## Complete release commands

Both commands require a clean Git worktree:

```powershell
.\Scripts\VerifyAll.ps1
.\Scripts\FinalizeRelease.ps1
```

`VerifyAll.ps1` runs repository audit, Editor build, headless smoke, all Automation tests, runtime capture, Inspector capture, Phase 2 report integration, three-config BuildPlugin, BuildCookRun, and packaged executable smoke. It records `Artifacts/Reports/verification-last.json` with the exact Git revision and evidence paths.

`FinalizeRelease.ps1` accepts only manifests that name the same revision and version, takes exact archive paths from those manifests, assembles a timestamped delivery, hashes every payload, hashes the manifest, and invokes `AuditDelivery.ps1` for an independent re-read.

To re-audit an existing delivery:

```powershell
.\Scripts\AuditDelivery.ps1 `
  -DeliveryRoot (Get-Content .\Artifacts\Final\LATEST.txt).Trim()
```

## Evidence locations

- `Artifacts/Logs/`: UBT, UAT, Automation, Commandlet, capture, and packaged-run logs.
- `Artifacts/Reports/automation-*`: UE Automation JSON/HTML.
- `Artifacts/Reports/Phase2/<timestamp>/`: canonical layouts, diff, benchmark, logs, revision-aware summary.
- `Artifacts/Media/`: runtime, packaged, and Inspector PNG evidence.
- `Artifacts/Plugin/`: BuildPlugin directory and plugin package manifest.
- `Artifacts/Package/`: Win64 archive directory and package manifest.
- `Artifacts/Release/`: versioned ZIPs and adjacent SHA-256 files.
- `Artifacts/Final/`: self-auditing delivery directories and `LATEST.txt`.

Generated paths, caches, binaries, and local user state are ignored by Git.

## Harness safeguards

UnrealEditor may return process exit 0 even when Automation cases fail. `Test.ps1` parses exported `index.json`, requires executed tests, and rejects warnings, failures, not-run, or in-process cases.

Report commands use separate Unreal processes and reparse every JSON output. Benchmark timings are recorded, never thresholded. Only one UBT/UAT/Editor/Cook/package writer may target this checkout at once.

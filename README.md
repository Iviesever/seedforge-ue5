# SeedForge 0.2.0

SeedForge is a UE 5.8 C++ engineering portfolio project: a deterministic asynchronous dungeon generator whose layouts can be exported, verified, structurally compared, benchmarked, and inspected inside the Editor.

![SeedForge deterministic graybox for seed 24301](docs/images/seedforge-24301.png)

## What it demonstrates

- Pure deterministic generation from explicit `uint64 Seed + Config`, with bounded work and five golden hashes.
- Ordered validation of bounds, overlap, endpoints, and complete four-neighbor connectivity.
- Cancellation-safe `UE::Tasks` work with newest-request-wins Game Thread application.
- A weak `UWorldSubsystem` lifetime boundary and pure C++ HISM graybox visualization.
- Versioned canonical JSON with exact unsigned 64-bit decimal strings, strict typed import failures, hash verification, and topology validation.
- Deterministic Layout Diff over room and walkable-cell sets, endpoints, configuration, seed, and hash identities.
- A headless Editor Commandlet that generates documents, diffs them, and records a reproducible 10,000-seed timing report.
- A minimal code-native Editor Inspector with seed input, identity metrics, and canonical JSON export.
- One verification pipeline covering Editor build/load, 41 UE Automation tests, two Editor captures, BuildPlugin, BuildCookRun, and packaged-EXE smoke.

## Evidence snapshot

| Gate | Verified result |
|---|---:|
| UE Automation | 41 passed, 0 warnings, 0 failures |
| Generator property sweep | 10,000 consecutive seeds |
| Canonical document round trip | 100 generated seeds plus `MAX_uint64` |
| Structural diff | stable sorted topology, reverse symmetry, endpoint isolation |
| Benchmark integration | 10,000 attempted, 10,000 succeeded, 0 failed |
| Independent plugin build | Editor Development, Game Development, Game Shipping |
| Win64 candidate | Build, Cook, Stage, Pak, Archive and real EXE smoke |

Timings are observations on one machine, never an SLA. Raw outputs live under the ignored `Artifacts/` directory; the committed evidence journals live under `tasks/`.

## Try it

Build and run the complete test suite:

```powershell
.\Scripts\Build.ps1
.\Scripts\Test.ps1 -Filter SeedForge
```

Generate two canonical layouts, import and diff them, then benchmark 10,000 seeds:

```powershell
.\Scripts\Report.ps1
```

Open the Editor and choose **Tools > Miscellaneous > SeedForge Inspector** (the exact menu placement can vary with the UE workspace layout). The tab starts with seed `24301`; generate another seed or export its canonical JSON. A deterministic visual-QA run is also available:

```powershell
.\Scripts\CaptureInspector.ps1 -Seed 24301
```

To rebuild every deliverable from one clean revision:

```powershell
.\Scripts\VerifyAll.ps1
.\Scripts\FinalizeRelease.ps1
```

## Runtime demo

Extract `SeedForgeDemo-Win64-0.2.0-*.zip`, launch `Windows/SeedForge.exe`, and fly with W/A/S/D plus mouse look; Space/Ctrl or E/Q moves vertically. Pass `-SeedForgeSeed=<uint64>` for another layout.

## Architecture at a glance

```text
Seed + Config
  -> FSeedForgeGenerator -> FSeedForgeLayout -> FSeedForgeValidator
  -> FSeedForgeAsyncCoordinator -> USeedForgeWorldSubsystem
  -> FSeedForgeVisualizationPlanner -> ASeedForgePreviewActor (HISM)

FSeedForgeLayoutDocument <-> FSeedForgeLayoutCodec (canonical JSON)
                         -> FSeedForgeLayoutDiffer (structural evidence)
                         -> FSeedForgeBenchmarkRunner / ReportCommandlet
                         -> SeedForge Inspector (Editor-only Slate UI)
```

The generator and codec do not read `UObject`, `UWorld`, wall-clock time, global randomness, or unordered output iteration. See [Architecture](docs/ARCHITECTURE.md), [Layout format](docs/LAYOUT_FORMAT.md), and [Benchmarking](docs/BENCHMARKING.md).

## Honest portfolio use

Codex GPT-5.6 Sol wrote, tested, debugged, packaged, and documented this repository under a user-approved scope. The user did not hand-write the implementation. Present it as an AI-assisted engineering project and be ready to explain or modify it live; do not claim independent authorship. See [AI assistance](docs/AI_ASSISTANCE.md), [Code walkthrough](docs/CODE_WALKTHROUGH.md), and [Interview guide](docs/INTERVIEW_GUIDE.md).

SeedForge is intentionally an engineering lab, not a complete game or a production procedural-generation framework. See [Known limitations](docs/KNOWN_LIMITATIONS.md).

## License

MIT. See [LICENSE](LICENSE).

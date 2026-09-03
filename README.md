# SeedForge

SeedForge is a deterministic asynchronous dungeon-generation lab for Unreal Engine 5.8. It separates a pure C++ generation/validation core from UE task scheduling and HISM scene application.

![SeedForge deterministic graybox for seed 0x5EED](docs/images/seedforge-24301.png)

## What it proves

- Explicit `Seed + Config` input with bounded deterministic generation.
- Canonical room/corridor ordering and byte-defined FNV-1a layout hashes.
- Bounds, overlap, entrance/exit, and full-connectivity validation.
- Cancellation-safe `UE::Tasks` execution with newest-request-wins semantics.
- `UWorldSubsystem` lifetime boundary and weak UObject apply callback.
- Pure C++ HISM graybox using only the Engine cube asset.
- Automated UE build, test, plugin packaging, Win64 cooking, packaged smoke, and screenshot capture.

## Verified evidence

| Gate | Recorded result |
|---|---:|
| UE Automation | 25 passed, 0 warnings, 0 failures |
| Property sweep | 10,000 deterministic seeds |
| Golden regression | 5 fixed seed/hash pairs |
| Synchronous repeatability | 5 seeds x 100 complete-layout comparisons |
| Asynchronous repeatability | 100 sequential worker/apply cycles |
| Independent plugin build | Editor Development, Game Development, Game Shipping |
| Win64 BuildCookRun | Build, Cook, Stage, Pak, Archive passed |
| Packaged smoke | Hash, instance counts, PNG capture, clean exit passed |

These are machine-specific recorded results, not universal performance claims. The evidence journal is in `tasks/20260903-113501-ue58-agentic-sprint/evidence.md` and raw artifacts are produced under the ignored `Artifacts/` directory.

## Run the packaged demo

1. Extract the latest `SeedForgeDemo-Win64-*.zip` from `Artifacts/Release/`.
2. Launch `Windows/SeedForge.exe`.
3. Fly with <kbd>W</kbd>/<kbd>A</kbd>/<kbd>S</kbd>/<kbd>D</kbd>, mouse look, and <kbd>Space</kbd>/<kbd>Ctrl</kbd> or <kbd>E</kbd>/<kbd>Q</kbd> for vertical movement.

Pass a different seed from a terminal:

```powershell
.\SeedForge.exe -SeedForgeSeed=2026
```

## Local verification

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/Build.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/Test.ps1 -Filter SeedForge
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/CaptureDemo.ps1
```

The scripts keep project caches, user state, logs, reports, packages, and screenshots under the project directory whenever UE provides an override. See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) for every build and packaging entry point.

## Architecture

```text
Seed + Config
    -> FSeedForgeGenerator       pure deterministic C++
    -> FSeedForgeLayout          canonical ordered result + hash
    -> FSeedForgeValidator       structural invariants + connectivity
    -> FSeedForgeAsyncCoordinator UE::Tasks worker + Game Thread apply
    -> USeedForgeWorldSubsystem  world lifetime and broadcast boundary
    -> ASeedForgePreviewActor    HISM floors/walls from Engine cube
```

The generator never reads `UObject`, `UWorld`, wall-clock time, or global randomness. Unordered containers are used only for membership during validation/visualization, never to determine canonical output or hashes. Full design and complexity notes are in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Scope boundaries

Determinism means the same plugin version, target platform, configuration, and seed produce the same canonical layout/hash. It does not claim compatibility across future algorithm, UE, compiler, or platform versions.

SeedForge intentionally does not implement multi-floor dungeons, WFC/BSP variants, enemies, loot, NavMesh, PCG, replication, persistence, or polished art. See [`docs/KNOWN_LIMITATIONS.md`](docs/KNOWN_LIMITATIONS.md).

## Authorship

This repository was implemented and verified by Codex GPT-5.6 Sol under a user-approved specification. The user did not hand-write the code. It must not be represented as independently authored C++ work. See [`docs/AI_ASSISTANCE.md`](docs/AI_ASSISTANCE.md), [`docs/CODE_WALKTHROUGH.md`](docs/CODE_WALKTHROUGH.md), and [`docs/INTERVIEW_GUIDE.md`](docs/INTERVIEW_GUIDE.md).

## License

MIT. See [`LICENSE`](LICENSE).

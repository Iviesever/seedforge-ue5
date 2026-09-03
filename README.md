# SeedForge 0.3.0

SeedForge is a UE 5.8 C++ engineering portfolio project: a deterministic procedural dungeon becomes a short, playable top-down extraction run, with local evidence from pure-model tests through a real Win64 packaged executable.

![SeedForge combat for deterministic seed 24301](docs/images/phase3-combat-24301.png)

```text
Seed + Config
  -> Deterministic Layout
  -> Deterministic Encounter Plan
  -> Grid A*
  -> UE Gameplay World
  -> Collect / Fight / Extract
  -> Automated Packaged Evidence
```

## What you can play

- Generate one dungeon and a separately versioned initial encounter from `-SeedForgeSeed=<uint64>`.
- Move with WASD, aim with the mouse, attack with Left Mouse Button, and dash with Space.
- Fight five bounded-A* enemies, collect three Data Cores, unlock the exit, and extract.
- Lose all HP, then press R to restart the same seed or N to start a deterministic next seed.
- Read HP, Core progress, exact seed, run state, exit state, controls, and terminal prompts in the code-native HUD.

The map and initial encounter are reproducible for the same Seed + Config. The complete real-time run is not claimed deterministic across input timing, frame rate, or floating-point physics.

## Evidence snapshot

| Gate | Latest Phase 3 result |
|---|---:|
| UE Automation | 63 passed, 0 warnings, 0 failures |
| Preserved 0.2.0 baseline | original 41 tests and five layout golden hashes remain green |
| Encounter plan | player, exit, 3 Cores, 5 enemies; stable IDs/cells/hash; typed failures |
| Grid A* | four-neighbor, Manhattan, fixed tie-break, bounded expansions, typed statuses |
| Gameplay smoke | attack, kill, 3 pickups, unlock, `Playing -> Won`, JSON reparse, 3 PNGs |
| Independent plugin | Editor Development, Game Development, Game Shipping |
| Win64 candidate | Build, Cook, Stage, Pak, Archive, ordinary launch, packaged gameplay smoke |

Generated evidence stays under ignored `Artifacts/`; committed PACT journals live under `tasks/20260903-191047-phase3-playable-vertical-slice/`. The final exact revision and artifact paths are authoritative only in `Artifacts/Reports/phase3-verification-last.json` after a clean `VerifyPhase3.ps1` run.

## Run and verify

```powershell
# Build and run all deterministic/UE contracts
.\Scripts\Build.ps1
.\Scripts\Test.ps1 -Filter SeedForge

# Drive the real Editor World through attack/collect/extract and capture evidence
.\Scripts\TestGameplay.ps1 -Seed 24301

# Build one Win64 package, launch its ordinary path, then run packaged gameplay smoke
.\Scripts\PackageGameplay.ps1 -Seed 24301

# Final clean-revision aggregate: build, all tests, reports, plugin, package, both smokes
.\Scripts\VerifyPhase3.ps1
```

For ordinary play, launch the packaged `Windows/SeedForge.exe`. Pass `-SeedForgeSeed=24301` or another unsigned 64-bit seed. The normal path does not depend on the smoke flag.

## Architecture at a glance

```text
FSeedForgeGenerator (layout v1, preserved)
  -> USeedForgeWorldSubsystem (worker + newest-request-wins apply)
  -> ASeedForgeGameplayCoordinator (one run owner)
       -> FSeedForgeEncounterPlanner (pure deterministic values)
       -> FSeedForgeRunStateMachine (explicit fail-closed transitions)
       -> ASeedForgePreviewActor (HISM floors/walls + collision)
       -> Player / Enemy / Core / Exit / HUD actors
       -> FSeedForgeGridPathfinder (bounded deterministic A*)
       -> Gameplay smoke JSON + screenshots + process status
```

The 0.2.0 evidence layer remains intact: canonical layout JSON, strict import, structural Layout Diff, benchmark Commandlet, and the Slate Inspector all consume the same Runtime layout APIs.

Read [Phase 3 architecture](docs/PHASE3_ARCHITECTURE.md), [gameplay loop](docs/GAMEPLAY_LOOP.md), [code walkthrough](docs/PHASE3_CODE_WALKTHROUGH.md), [acceptance matrix](docs/PHASE3_ACCEPTANCE_MATRIX.md), and [known limitations](docs/KNOWN_LIMITATIONS.md).

## Honest portfolio use

Codex GPT-5.6 Sol implemented, tested, debugged, packaged, and documented SeedForge under constraints and acceptance goals supplied by the user. The user did not independently hand-write this implementation. Present it as an AI-assisted engineering project, reproduce the evidence, understand the trade-offs, and complete personal test-first modifications before claiming coding ownership. See [AI assistance](docs/AI_ASSISTANCE.md), [Phase 3 interview guide](docs/PHASE3_INTERVIEW_GUIDE.md), and [live change drills](docs/LIVE_CHANGE_DRILLS.md).

SeedForge remains a focused single-player graybox, not a production game or general procedural framework.

## License

MIT. See [LICENSE](LICENSE).

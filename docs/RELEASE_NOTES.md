# Release notes

## SeedForge 0.3.0 candidate

- Deterministic initial encounter plan and independent encounter hash.
- Pure bounded four-neighbor A* and explicit run state machine.
- Code-native top-down Player/Controller/Enemy/Core/Exit/HUD and complete fight/collect/extract loop.
- Same-seed restart, deterministic next seed, CLI seed, HP/death, attack/dash cooldowns, and bounded enemy replans.
- Editor and packaged production-path gameplay smoke with exact JSON, strict log audit, and start/combat/win captures.
- 22 new Automation tests, bringing the complete suite to 63 while preserving all original 41 tests and five layout hashes.
- Phase 3 architecture, walkthrough, interview guide, live drills, acceptance, evidence, and AI disclosure.

This is branch/Draft PR candidate metadata. No `v0.3.0` tag or formal Release has been created.

## SeedForge 0.2.0

### Added

- Versioned compact canonical JSON export and strict import.
- Exact decimal-string preservation for seed/hash across the full `uint64` domain.
- Typed document failures for syntax, field, schema/version, unsigned, order, hash, config, and topology errors.
- Deterministic structural Layout Diff with human and JSON summaries.
- Bounded benchmark runner with warm-up, distribution metrics, failure counts, and aggregate output identity.
- `SeedForgeReport` Editor Commandlet with Generate, Diff, and Benchmark modes.
- `Scripts/Report.ps1` real-process chain producing two documents, strict-import diff, and a 10,000-seed report.
- Minimal code-native SeedForge Inspector Nomad tab and deterministic Slate screenshot verification.
- Revision-aware plugin, Win64, report, aggregate verification, and final delivery manifests.
- Independent final payload and manifest SHA-256 audit.
- Six document, five diff, four benchmark/Commandlet, and one Editor test, bringing the total to 41.

### Preserved

- Generator version 1 and all five 0.1.0 golden hashes.
- Cancellation/newest-wins behavior, subsystem lifetime boundary, HISM demo, and packaged smoke.
- Annotated `v0.1.0` rollback tag and timestamped 0.1.0 deliveries.

## SeedForge 0.1.0

- Deterministic bounded room placement and canonical L-corridor generation.
- Byte-defined FNV-1a layout identity with five golden seed/hash pairs.
- Typed generation failures and complete topology validation.
- Cancellation-safe `UE::Tasks` coordination and `UWorldSubsystem` lifetime adapter.
- Pure C++ HISM graybox, generated map, free-fly demo, and automated screenshots.
- 25 UE Automation tests, BuildPlugin, BuildCookRun, packaged smoke, ZIP, checksums, and documented AI assistance.

SeedForge remains a focused engineering lab, not a complete procedural game. Review `KNOWN_LIMITATIONS.md` and `AI_ASSISTANCE.md` before presenting it.

# SeedForge 0.2.0 Runtime and Editor Plugin

SeedForge provides deterministic C++ dungeon generation, validation, cancellation-safe asynchronous coordination, HISM visualization, portable layout evidence, structural diffing, benchmarking, and an Editor Inspector for Unreal Engine 5.8.

## Install

Copy the packaged `SeedForge` directory into a project's `Plugins/` directory, enable it, and rebuild. The Runtime module is available to games; the Commandlet, Inspector, and Automation suite are Editor-only modules.

## Main C++ entry points

- `FSeedForgeGenerator::Generate` and `ValidateConfig` — synchronous pure generation and configuration checks.
- `FSeedForgeValidator::Validate` — deterministic topology validation.
- `FSeedForgeLayoutCodec::ExportCanonicalJson` / `ImportCanonicalJson` — versioned portable evidence with typed failures.
- `FSeedForgeLayoutDiffer::Compare` / `ExportCanonicalJson` — deterministic structural comparison.
- `FSeedForgeBenchmarkRunner::Run` — bounded timing distribution and aggregate identity.
- `FSeedForgeAsyncCoordinator::Start` — generic newest-request-wins worker/Game Thread boundary.
- `USeedForgeWorldSubsystem::RequestGeneration` — world-lifetime asynchronous generation.
- `ASeedForgePreviewActor` — engine-cube HISM visualization and optional screenshot capture.

## Editor entry points

- `SeedForgeReport` Commandlet modes: `Generate`, `Diff`, and `Benchmark`.
- `SeedForge Inspector` Nomad tab: seed input, generated identity metrics, and canonical JSON export.

There is no Blueprint API in 0.2.0. See the repository `docs/` directory for schema, architecture, evidence, limitations, and the AI-authorship disclosure.

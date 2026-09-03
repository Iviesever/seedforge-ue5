# SeedForge Runtime Plugin

SeedForge provides a deterministic C++ dungeon-layout generator, invariant validator, cancellation-safe asynchronous coordinator, world subsystem, and HISM preview actor for Unreal Engine 5.8.

## Install

Copy the packaged `SeedForge` directory into a project's `Plugins/` directory, enable the plugin, and rebuild the project.

## C++ entry points

- `FSeedForgeGenerator::Generate(Seed, Config)` — synchronous pure generation.
- `FSeedForgeValidator::Validate(Layout, Config)` — deterministic invariant validation.
- `FSeedForgeAsyncCoordinator::Start(Work, Apply)` — generic newest-request-wins worker/Game Thread boundary.
- `USeedForgeWorldSubsystem::RequestGeneration(Seed, Config)` — world-lifetime asynchronous generation.
- `ASeedForgePreviewActor` — engine-cube HISM visualization and optional screenshot capture.

The plugin intentionally exposes no Blueprint API in version 0.1.0. See the repository README and `docs/` directory for the architecture, evidence, limitations, and AI-authorship disclosure.


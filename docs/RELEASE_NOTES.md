# SeedForge 0.1.0

## Added

- Deterministic bounded room placement and canonical L-corridor generation.
- Byte-defined FNV-1a layout identity with five golden seed/hash pairs.
- Typed configuration and placement failures.
- Room/corridor bounds, overlap, endpoint, and connectivity validation.
- Cancellation-safe `UE::Tasks` coordinator with newest-request-wins apply semantics.
- `USeedForgeWorldSubsystem` lifetime adapter.
- Pure C++ HISM graybox preview, generated empty map, free-fly camera, and automated screenshots.
- UE Automation coverage for error paths, 10,000 seeds, synchronous repeats, asynchronous repeats, cancellation, stale results, subsystem teardown, and visualization planning.
- Reproducible Editor build, smoke, Automation, BuildPlugin, BuildCookRun, packaged smoke, ZIP, and checksum scripts.

## Boundaries

This is a narrow engineering lab, not a complete procedural game. Review `KNOWN_LIMITATIONS.md` and `AI_ASSISTANCE.md` before evaluating or presenting the project.


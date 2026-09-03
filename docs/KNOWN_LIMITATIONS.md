# Known limitations

- UE 5.8 and Win64 are the only tested engine/platform combination.
- Layouts are one integer-grid floor with axis-aligned rectangular rooms and one L-corridor chain.
- There are no doors, enemies, loot, NavMesh, PCG, networking, persistence, replay, streaming, or gameplay loop.
- Cancellation suppresses results but does not interrupt generation inside its placement loop.
- Corridor `TArray::AddUnique` prioritizes clarity and is not intended for very large grids.
- Generator/hash semantics are version-specific. Schema v1 rejects unknown schema or generator versions and implements no migration.
- Canonical import is strict but not designed as a hostile-network parser or a general JSON framework.
- Layout Diff compares complete valid documents; it does not apply patches, visualize changes, or recursively diff arbitrary JSON.
- Benchmark timing includes normal process/machine variance and is not a cross-machine SLA. Only the aggregate identity is deterministic.
- The Inspector displays identity metrics and exports JSON; it does not render or edit topology inside the tab.
- Five golden hashes and canonical JSON tests do not guarantee identical render pixels across GPUs or drivers.
- The graybox uses Engine basic geometry and runtime-created lighting, not production art.
- Interactive mode is a free-fly inspection experience, not a character controller or polished game.
- The subsystem teardown test uses a strongly held transient instance; packaged smoke separately proves real world operation.
- Win64 archives have no installer and do not separately validate a UE prerequisite redistributable.
- UBT/UAT can write diagnostic traces under Unreal user directories even when project-facing caches/logs are redirected.
- AI-generated implementation must be disclosed and does not substitute for the user's personal C++ practice.

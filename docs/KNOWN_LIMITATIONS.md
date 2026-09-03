# Known limitations

- UE 5.8 and Win64 are the only tested engine/platform combination.
- Layouts are one integer-grid floor with axis-aligned rectangles and one L-corridor chain.
- There are no doors, enemies, loot, navigation, PCG, networking, persistence, replay, or runtime streaming.
- Cancellation suppresses results; it does not interrupt the generator inside its placement loop.
- `TArray::AddUnique` keeps corridor code clear but is not intended for very large grids.
- The hash schema is algorithm-version-specific and intentionally changes if canonical inputs or RNG behavior change.
- The five golden hashes do not guarantee identical rendering pixels across GPUs or drivers.
- The demo uses Engine basic geometry and runtime-created lighting, not production art.
- Capture mode uses a fixed camera. Interactive mode is a free-fly inspection experience, not a character game loop.
- The subsystem teardown test directly exercises `Deinitialize` on a strongly held transient instance; packaged smoke separately proves real world-subsystem operation. A future suite could add full synthetic-world destruction.
- The Win64 archive does not include an installer or a separately validated UE prerequisite redistributable.
- UAT and UBT may still write their own global diagnostic traces under the user's Unreal directories even when project-facing logs/cache paths are redirected.


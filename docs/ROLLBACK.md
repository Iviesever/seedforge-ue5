# Rollback and recovery

## Immutable release baseline

Annotated tag `v0.1.0` points exactly to:

```text
720ba4a8abc726add137d71c49572315f969f097
```

Inspect it from a new branch or worktree. Do not destroy the current checkout with `git reset --hard`.

## Phase 2 checkpoints

- `2b08e2c` — approved Phase 2 issue and implementation plan.
- `63bfc82` — canonical layout document codec green.
- `b8ab523` — deterministic structural Layout Diff green.
- `4403d8d` — benchmark/report Commandlet and real process chain green.
- `1c76e57` — 0.2.0 version metadata and dynamic package naming.
- `c68d074` — minimal Editor Inspector green.
- `59fc3d0` — deterministic Inspector capture and visual evidence green.

Later documentation/release commits build on these checkpoints. Use `git log --oneline --decorate` and the final delivery manifest for the final revision.

## Artifact fallback

Timestamped 0.1.0 final deliveries remain under `Artifacts/Final/SeedForge-0.1.0-*` and are never overwritten. Versioned plugin/demo ZIPs have adjacent SHA-256 files.

If a new verification fails, retain the newest previously verified timestamped delivery and document the failure. Never relabel or overwrite an older good artifact with newer unverified files.

## Schema compatibility

Version 0.2.0 preserves generator version 1 and all 0.1.0 golden hashes. Schema v1 import rejects unsupported generator or schema versions. A future algorithm change must increment generator version, add new golden evidence, and decide migration explicitly rather than accepting old hashes silently.

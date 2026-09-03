# Rollback and recovery

## Immutable release baseline

Annotated tag `v0.2.0` points exactly to:

```text
9a306f8ff72cb660d3c04b09806787df21191d45
```

It is the Phase 3 branch base and remains the preferred rollback for all 0.3.0 gameplay work. The published 0.2.0 Release is unchanged.

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

## Phase 3 checkpoints

- `83e7672` — PACT-30 issue/blueprint/baseline.
- `319a302` — PACT-31 deliberate RED model contracts.
- `81a77a2` — encounter planner, bounded A*, and run state GREEN.
- `6e8fcbd` — PACT-32 deliberate RED Gameplay Framework contracts.
- `7f6fedd` — playable code-native World GREEN.
- `53b10f8` — PACT-33 deliberate RED smoke trace.
- `9ff53d4` — runtime/script gameplay smoke GREEN and clean package source revision.
- `2494275` — packaged evidence and screenshot root-cause packet.

All Phase 3 work remains on `feat/phase3-playable-vertical-slice`. Revert a focused commit or return to `v0.2.0`; do not reset the user's checkout destructively.

## Artifact fallback

Timestamped 0.1.0 final deliveries remain under `Artifacts/Final/SeedForge-0.1.0-*` and are never overwritten. Versioned plugin/demo ZIPs have adjacent SHA-256 files.

If a new verification fails, retain the newest previously verified timestamped delivery and document the failure. Never relabel or overwrite an older good artifact with newer unverified files.

## Schema compatibility

Version 0.2.0 preserves generator version 1 and all 0.1.0 golden hashes. Schema v1 import rejects unsupported generator or schema versions. A future algorithm change must increment generator version, add new golden evidence, and decide migration explicitly rather than accepting old hashes silently.

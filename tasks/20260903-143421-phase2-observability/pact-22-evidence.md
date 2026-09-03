# PACT-22 Evidence

## RED

- Contract commit: `e50ea0e`
- Filter: `SeedForge.Diff`
- Result: 2 passed and 3 failed against the deliberate empty-diff stub.
- Log: `Artifacts/Logs/automation-20260903-152546.log`

## GREEN

- Editor build: passed.
- Focused filter: 5 of 5 structural diff tests passed.
- Focused log: `Artifacts/Logs/automation-20260903-152753.log`
- Full regression: 36 of 36 SeedForge tests passed with zero warnings.
- Full log: `Artifacts/Logs/automation-20260903-152817.log`

The diff records exact seed/hash identities, configuration and endpoint flags, canonically sorted room and walkable-cell set changes, reverse symmetry, a deterministic human summary, and a fixed-order machine-readable JSON summary.

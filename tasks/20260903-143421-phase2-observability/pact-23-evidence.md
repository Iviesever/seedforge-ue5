# PACT-23 Evidence

## RED

- Contract commit: `8097037`
- Filter: `SeedForge.Benchmark`
- Result: 1 passed and 3 failed against deliberate benchmark/commandlet stubs.
- Log: `Artifacts/Logs/automation-20260903-153213.log`

## Build root-cause checkpoint

- The two sequential compile diagnostics and their narrow correction are recorded in `root-cause-20260903-commandlet-build.md`.
- Corrected Editor build log: `Artifacts/Logs/build-editor-20260903-153735.log`.

## GREEN

- Focused filter: 4 of 4 benchmark/commandlet tests passed.
- Focused log: `Artifacts/Logs/automation-20260903-153756.log`.
- Full regression: 40 of 40 SeedForge tests passed with zero warnings.
- Full log: `Artifacts/Logs/automation-20260903-154115.log`.

## Real process integration

- Command: `Scripts/Report.ps1`
- Run directory: `Artifacts/Reports/Phase2/20260903-154014`
- Canonical documents: seeds 24301 and 12648430; both were imported by Diff mode.
- Structural diff: rooms +10/-10, walkable cells +447/-445.
- Benchmark: 10,000 attempted, 10,000 succeeded, 0 failed.
- Aggregate hash: `7040334277952983391`.
- Observed total/median/P95: 169.639982283 / 0.016599894 / 0.023502856 ms.
- All four Unreal commandlet processes returned zero; PowerShell reparsed and checked all JSON outputs.

Timing values are machine observations, not performance SLAs.

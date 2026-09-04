# PACT-31 Evidence

## RED

- Contract base: `83e76727ea19e1683d8111b48de05ccb610076b9`.
- RED contract commit: `319a30288fa143222a1cb3890012403181ad23ec`.
- Added public value API declarations plus deliberate non-functional stubs for encounter planning, grid A*, and run state.
- Added 15 real Automation tests under `SeedForge.Model` covering deterministic encounter identity/placement/failures/tampering, path success/tie/status/budget contracts, and run win/loss/restart/illegal transitions.
- First focused run: 1 passed and 14 failed because the path stub's default `InvalidInput` accidentally satisfied the invalid-input contract. This did not qualify as complete RED.
- First log: `Artifacts/Logs/automation-20260903-191920.log`.
- Stub-only correction: return an empty `Success` path so no requested path behavior is accidentally implemented.
- Compile command: `Scripts/Build.ps1`.
- Compile result: passed with 11 actions for the initial contract and 4 actions after the stub-only correction.
- Build logs: `Artifacts/Logs/build-editor-20260903-191851.log` and `Artifacts/Logs/build-editor-20260903-192008.log`.
- Authoritative RED command: `Scripts/Test.ps1 -Filter SeedForge.Model -TimeoutSeconds 900`.
- Authoritative RED result: 0 passed, 0 warnings, 15 failed, 0 not-run, 0 in-process.
- Authoritative RED log: `Artifacts/Logs/automation-20260903-192030.log`.
- Authoritative RED report: `Artifacts/Reports/automation-20260903-192030`.

No encounter selection, canonical encounter hash, A* search, or run-state transition behavior exists at this checkpoint.

## GREEN

- Implemented deterministic encounter ranking from explicit layout/config values, stable role IDs/order, distinct reachable cells, minimum player/enemy Manhattan distance, independent versioned FNV-1a identity, and typed validation failures.
- Implemented four-neighbor unit-cost A* with East/South/West/North neighbor order, Manhattan heuristic, `(F,H,Y,X)` open-set selection, fixed parent retention, bounded expansions, exact status results, and no unordered-container iteration controlling output.
- Implemented one fail-closed run state machine for `Generating`, `Playing`, `Won`, `Lost`, and `Restarting`, including canonical Core IDs, duplicate/unknown Core rejection, exit lock, same-run restart, and reset to generation.
- First GREEN run: 14 passed and 1 failed because the impossible-separation test supplied `10000`, outside the declared valid safety range. The test input was corrected to the maximum valid but still impossible value `4096`; no production constraint changed.
- First GREEN log: `Artifacts/Logs/automation-20260903-192737.log`.
- Corrected focused command: `Scripts/Test.ps1 -Filter SeedForge.Model -TimeoutSeconds 900`.
- Corrected focused result: 15 passed, 0 warnings, 0 failures.
- Corrected focused log/report: `Artifacts/Logs/automation-20260903-192837.log`; `Artifacts/Reports/automation-20260903-192837`.

## Refactor and full regression

- Source review found that growing the A* node array could invalidate a reference to the current node when the expansion budget reserved fewer entries than the reachable set. The loop now snapshots the current cell/cost before any append.
- Post-refactor build: passed.
- Post-refactor build log: `Artifacts/Logs/build-editor-20260903-192936.log`.
- Post-refactor focused result: 15 of 15 passed with zero warnings.
- Post-refactor focused log/report: `Artifacts/Logs/automation-20260903-192948.log`; `Artifacts/Reports/automation-20260903-192948`.
- Full command: `Scripts/Test.ps1 -Filter SeedForge -TimeoutSeconds 900`.
- Full result: 56 of 56 passed, 0 warnings, 0 failures, 0 not-run, 0 in-process. This includes all existing 41 tests plus the 15 new model contracts.
- Full log/report: `Artifacts/Logs/automation-20260903-193010.log`; `Artifacts/Reports/automation-20260903-193010`.
- Repository audit after GREEN: passed with 113 tracked files and 52 source/build-rule files.
- Existing generator/codec/diff/benchmark sources were not changed, and the complete regression retains the five golden layout hashes.

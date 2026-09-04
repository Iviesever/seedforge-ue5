# SF-IRA-006 — checked coordinate arithmetic evidence

## Source and scope

- Starting HEAD: `554ff5ff61182343f789f4e829ac196e4226b76c`, clean before this task. Its Editor Gameplay Smoke passed at `Artifacts/Reports/Gameplay/20260904-112322/summary.json`.
- Public path inputs could wrap a signed-domain boundary into a false adjacent cell. Both path and encounter Manhattan helpers subtracted int32 values before widening, and A* stored an already-overflowed heuristic in int32.
- Fix only widens subtraction, distance/cost storage, and neighbor construction; no generator, schema, canonical hash/version, neighbor order, or tie-break changes.
- Explicit sparse corridor fixtures exercise public value-type coordinate arithmetic, not a claim that a generated dungeon spanning billions of cells exists or is connected.

## Authoritative RED

- `Scripts/Build.ps1`: exit 0, `Artifacts/Logs/build-editor-20260904-112735.log`.
- `Scripts/Test.ps1 -Filter SeedForge.Audit.CoordinateSafety -TimeoutSeconds 120`: wrapper exit 1; 3 passed / 3 failed / 0 warnings / 0 not-run / 0 in-process. Report: `Artifacts/Reports/automation-20260904-112747/index.json`.
- Expected failures: four MAX/MIN neighbor wrap directions; Encounter Generate rejected a genuinely large safety separation; Encounter Validate rejected one-axis INT_MIN and full two-axis signed-domain separation despite recomputed valid hashes.
- Preservation tests already passed: budget/goal order, near-limit and normal duplicate/reordered routes, fixed-seed layout and encounter identity. They were not artificially made RED.

## GREEN and preserved contracts

- Manhattan subtraction happens in int64 before absolute value. The maximum two-axis FIntPoint distance is 8,589,934,590, representable in int64.
- A* cost and heuristic fields use int64. Neighbor coordinates are added in int64, range-checked against int32 limits, then narrowed.
- ExpandedNodes still counts non-goal nodes whose adjacency is enumerated. A goal discovered on the last allowed expansion can be selected successfully; budget failures return no partial path. Header documentation now states this existing contract.
- Build exit 0: `Artifacts/Logs/build-editor-20260904-112844.log`.
- CoordinateSafety: 6/6, exit 0, zero warnings/failures/not-run/in-process; `Artifacts/Reports/automation-20260904-112855/index.json`.
- Existing Path: 5/5, exit 0; `Artifacts/Reports/automation-20260904-112940/index.json`.
- Existing Encounter: 5/5, exit 0; `Artifacts/Reports/automation-20260904-112952/index.json`.
- Existing GoldenHashesStable: 1/1, exit 0; `Artifacts/Reports/automation-20260904-113005/index.json`.
- Full SeedForge: 83/83, exit 0, zero warnings/failures/not-run/in-process; `Artifacts/Reports/automation-20260904-113017/index.json`.
- Seed 24301 still produces layout `7425849530159566348` and encounter `15303214708604970503`. All existing golden layout cases remain unchanged.

## Independent review and boundary

- A bounded read-only review found no actionable issues; it checked range-before-narrowing, finite initialized costs before score comparisons, unchanged ordering/budget behavior, and test scope. No reviewer UE process or file mutation occurred.
- Wide heuristic arithmetic is supported by static type/range review; isolated-extreme path tests directly detect wrap adjacency, while Encounter tests directly detect wide subtraction. No claim of a billion-cell dynamic path test is made.
- Repository/whitespace audit and report count reparse precede the separate commit. These local development runs are not final clean-revision release certification; gates 1–16 and packaging still remain.

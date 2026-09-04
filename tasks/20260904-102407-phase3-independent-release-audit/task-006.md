# SF-IRA-006 safe coordinate arithmetic implementation plan

> **Execution:** `executing-tasks` inline, test-driven, one root cause and separate commit.

**Objective:** Public FIntPoint path and encounter inputs must not wrap neighbors or Manhattan separation at int32 limits.

**Acceptance:** Reproduce false MAX→MIN adjacency and incorrect extreme enemy safety rejection; preserve deterministic normal routes, expansion budget semantics, duplicate/shuffled input results, layout goldens, and the fixed-seed encounter hash.

**Non-goals:** No generator, layout schema, hash version, path algorithm, tie-break, or generated-world domain changes. Do not add an alternate pathfinder.

**Constraints:** Existing checkout `D:\program\SeedForge`; UE 5.8 read-only; one UE writer. All audit evidence stays inside this repository. Rollback boundary is clean commit `554ff5ff61182343f789f4e829ac196e4226b76c`; its Editor smoke passed at `Artifacts/Reports/Gameplay/20260904-112322/summary.json`.

## Files

- New `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeCoordinateSafetyTests.cpp`: real pure-model input tests, no production test hooks.
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeGridPathfinder.cpp`: widen distances/costs and check neighbor construction.
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeGridPathfinder.h`: document existing expansion-count and goal-before-budget contract.
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Private/SeedForgeEncounter.cpp`: widen Manhattan subtraction and result for Generate and Validate.
- Record `sf-ira-006-evidence.md` and update `progress.md`.

## Steps

- [x] Add and run RED input cases for all four wrap directions. Example:

```cpp
Request.Start = FIntPoint(MAX_int32, 0);
Request.Goal = FIntPoint(MIN_int32, 0);
Request.WalkableCells = {Request.Start, Request.Goal};
TestEqual(TEXT("Opposite extremes are not adjacent"),
    FSeedForgeGridPathfinder::FindPath(Request).Status, ESeedForgePathStatus::Unreachable);
```

- [x] Add RED Encounter Generate and Validate cases using explicitly listed corridor cells, no extreme room loops. Recompute genuine layout/encounter canonical hashes. Validate one-axis INT_MIN separation and two-axis full signed-domain separation with safety distance 4096; both distances are safely larger than the threshold.
- [x] Add preservation cases: adjacent goal at budget 1 succeeds with one non-goal expansion; a two-edge path at budget 1 fails with no partial path, budget 2 succeeds; duplicate/reversed input gives identical status/path/count at ordinary and near-limit coordinates; Seed 24301 retains layout hash `7425849530159566348` and encounter hash `15303214708604970503`.
- [x] Build and run `Scripts/Test.ps1 -Filter SeedForge.Audit.CoordinateSafety -TimeoutSeconds 120`. Separate expected failing bug tests from initially passing preservation tests.
- [x] Implement wide subtraction and storage:

```cpp
int64 ManhattanDistance(const FIntPoint& Left, const FIntPoint& Right)
{
    return FMath::Abs(static_cast<int64>(Left.X) - Right.X)
        + FMath::Abs(static_cast<int64>(Left.Y) - Right.Y);
}
// FSearchNode CostFromStart and Heuristic, plus NextCost, use int64.
const int64 NeighborX = static_cast<int64>(CurrentCell.X) + Direction.X;
const int64 NeighborY = static_cast<int64>(CurrentCell.Y) + Direction.Y;
if (NeighborX < MIN_int32 || NeighborX > MAX_int32
    || NeighborY < MIN_int32 || NeighborY > MAX_int32)
{
    continue;
}
const FIntPoint NeighborCell(static_cast<int32>(NeighborX), static_cast<int32>(NeighborY));
```

- [x] Preserve and document the existing budget definition: ExpandedNodes counts non-goal nodes whose neighbors are enumerated; selecting an already-discovered goal succeeds before another budget check. No partial path on budget failure.
- [x] Build, focused CoordinateSafety, existing `SeedForge.Model.Path`, `SeedForge.Model.Encounter`, `SeedForge.Validation.GoldenHashesStable`, and full SeedForge Automation. Review the diff, record exact counts/logs and commit `fix(path): prevent coordinate overflow in grid search`.

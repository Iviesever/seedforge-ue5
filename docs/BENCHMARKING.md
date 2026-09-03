# Benchmarking and report evidence

## Purpose

The benchmark answers a reproducible engineering question: on this machine and toolchain, how long did the current generator take for a declared consecutive seed range, and which exact outputs were produced?

It is not an absolute performance promise or a cross-machine leaderboard.

## Method

`FSeedForgeBenchmarkRunner` validates sample/warm-up bounds and prevents `uint64` seed wrap. Warm-up seeds execute first and are not timed. Each measured generation is timed with `FPlatformTime::Seconds`; failures are counted rather than hidden.

The report records:

- start seed, warm-up count, requested/attempted/success/failure counts;
- complete generation configuration;
- total, minimum, nearest-rank median, P95, and maximum milliseconds;
- UE version, platform, CPU brand, and UTC timestamp;
- a deterministic aggregate FNV identity over every measured `(seed, canonicalHash)` pair.

The aggregate hash is independent of timing. Equal aggregate/config/range values prove the same canonical outputs were observed; different timing values are expected across runs.

## Commandlet modes

```text
-run=SeedForgeReport -Mode=Generate -Seed=<uint64> -Output=<json>
-run=SeedForgeReport -Mode=Diff -Left=<json> -Right=<json> -Output=<json>
-run=SeedForgeReport -Mode=Benchmark -SeedStart=<uint64> -SeedCount=<n> -Warmup=<n> -Output=<json>
```

Invalid or missing arguments, invalid documents, generation failures, and write failures return non-zero from the Commandlet and emit a clear error.

## Reproduce the standard run

```powershell
.\Scripts\Report.ps1 -LeftSeed 24301 -RightSeed 12648430 `
  -SeedStart 0 -SeedCount 10000 -Warmup 100
```

The script runs four separate Editor Commandlets: two generates, one diff that strictly imports both documents, and one benchmark. PowerShell reparses all outputs and validates schemas, identities, counts, zero failures, non-zero aggregate hash, and ordered timing statistics.

Each timestamped directory under `Artifacts/Reports/Phase2` includes outputs, per-process logs, and `report-summary.json` naming the exact source revision. Finalization refuses a stale report summary.

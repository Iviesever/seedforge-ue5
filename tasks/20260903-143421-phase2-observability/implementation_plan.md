# SeedForge Phase 2 Implementation Plan

## Objective

Extend the verified SeedForge 0.1.0 system with portable, versioned layout evidence: canonical JSON round trips, structural diffs, reproducible benchmark reports, and a commandlet workflow. Preserve all 0.1.0 behavior and artifacts. Add a minimal Editor Inspector only after every required gate passes with ample time remaining.

## Approved approach

Use a runtime document/codec/diff/benchmark layer plus a narrow Editor commandlet module. Canonical output is written manually in fixed field order; `uint64` seed/hash values are decimal strings so JSON number precision cannot corrupt identity. Import uses UE's JSON parser, normalizes order, recomputes the hash, and runs the existing validator before returning a document.

## Schema v1

```json
{
  "schema": "seedforge.layout",
  "schemaVersion": 1,
  "generatorVersion": 1,
  "seed": "24301",
  "canonicalHash": "7425849530159566348",
  "config": {
    "gridWidth": 48,
    "gridHeight": 48,
    "roomCount": 10,
    "minRoomWidth": 4,
    "maxRoomWidth": 9,
    "minRoomHeight": 4,
    "maxRoomHeight": 9,
    "roomPadding": 1,
    "maxPlacementAttempts": 512
  },
  "layout": {
    "entrance": [0, 0],
    "exit": [0, 0],
    "rooms": [{"min": [0, 0], "size": [4, 4]}],
    "corridorCells": [[0, 0]]
  }
}
```

The actual canonical representation is compact UTF-8 JSON without insignificant whitespace. Field and array order are defined by the codec. Unknown fields are ignored for forward-compatible readers; missing/wrong required fields and unsupported schema/generator versions are rejected.

## PACT-20: Preserve 0.1.0 and baseline

**Objective:** Establish an immutable rollback point and approved Phase 2 plan.

**Acceptance:**

1. Tag `v0.1.0` points exactly to verified revision `720ba4a8abc726add137d71c49572315f969f097`.
2. Phase 2 issue and plan are committed before production edits.
3. Existing 25 tests remain green and the 0.1.0 delivery directory remains untouched.

**Non-goals:** Repackaging or rewriting 0.1.0.

## PACT-21: Canonical layout document and codec

**Objective:** Export and strictly import a schema-versioned, byte-stable layout document.

**New API:**

- `FSeedForgeLayoutDocument` — schema/generator version, config, layout.
- `ESeedForgeDocumentErrorCode` — malformed JSON, missing/invalid field, unsupported schema/generator version, invalid unsigned integer, non-canonical data, hash mismatch, layout validation failure.
- `FSeedForgeDocumentResult` — typed import result.
- `FSeedForgeLayoutCodec::ExportCanonicalJson` and `ImportCanonicalJson`.

**Acceptance tests written before implementation:**

1. Maximum `uint64` seed/hash round-trip without precision loss.
2. Repeated export of one document is byte-identical.
3. Export has exact schema/version and fixed top-level field order.
4. Malformed JSON, missing field, wrong type, unsupported schema/generator versions, unsigned overflow, tampered hash, and invalid topology return their specific error codes.
5. One hundred generated seeds export/import to complete document equality.
6. Imported arrays are canonicalized before hash comparison; an unsorted document with its original ordered hash is rejected rather than silently accepted as canonical.
7. Existing 25 tests remain green.

**Implementation boundary:** Runtime depends on `Json`; no file I/O and no UObject access in the codec.

## PACT-22: Structural layout diff

**Objective:** Produce deterministic added/removed topology and endpoint changes for two valid documents.

**New API:**

- `FSeedForgeLayoutDifference` — seed/hash/config flags, endpoint before/after, added/removed rooms, added/removed walkable cells, `IsEmpty`, human summary.
- `FSeedForgeLayoutDiffer::Compare` and canonical JSON summary export.

**Acceptance tests written before implementation:**

1. Identical documents produce an empty diff.
2. Endpoint-only changes are represented without false cell/room differences.
3. Different generated seeds produce stable sorted added/removed collections.
4. Reversing comparison swaps added/removed sets.
5. Human and JSON summaries contain deterministic counts and identities.

**Non-goals:** Visual mesh diff, patch application, generic recursive JSON diff, or edit/migration operations.

## PACT-23: Benchmark statistics and report commandlet

**Objective:** Generate portable evidence files through a single unattended commandlet.

**Runtime statistics API:**

- `FSeedForgeBenchmarkConfig` — seed start/count, warm-up count, generation config.
- `FSeedForgeBenchmarkReport` — attempts, successes/failures, aggregate hash, total/min/median/P95/max milliseconds.
- `FSeedForgeBenchmarkRunner::Run` and deterministic percentile calculation.

**Editor module:**

`USeedForgeReportCommandlet` supports:

- `-Mode=Generate -Seed=<u64> -Output=<json>`
- `-Mode=Diff -Left=<json> -Right=<json> -Output=<json>`
- `-Mode=Benchmark -SeedStart=<u64> -SeedCount=<n> -Warmup=<n> -Output=<json>`

Output paths are supplied by project scripts and remain under `Artifacts/Reports/Phase2`. The benchmark report includes UE version, platform, CPU, timestamp, configuration, and distribution metrics. Timing values are observations, not pass/fail SLAs.

**Acceptance tests written before implementation:**

1. Known sample arrays yield exact median/P95 values.
2. A bounded benchmark returns the requested sample count, zero failures, ordered statistics, and non-zero aggregate hash.
3. Invalid command arguments return non-zero and a clear log error.
4. `Scripts/Report.ps1` generates two documents, imports them, writes a diff, runs a 10,000-seed benchmark, reparses all JSON, and records commandlet logs.

## PACT-24: Optional minimal Editor Inspector

**Entry gate:** PACT-21 through PACT-23, the complete test suite, BuildPlugin, and Win64 packaged smoke all pass; no unresolved issue; at least eight hours remain.

**Scope if entered:** A code-native Nomad tab with seed input, Generate button, hash/room/cell/time text, and Export JSON action. It consumes existing runtime APIs and creates no project asset.

**Required validation:** Module/tab registration test plus one manual visual inspection screenshot. If automated UI reliability or packaging regresses, omit the Inspector and document the gate decision.

## PACT-25: 0.2.0 distribution and handoff

**Objective:** Deliver source, plugin, Win64 demo, Phase 2 reports, docs, tests, and checksums from one clean revision.

**Acceptance:**

1. Bump project/plugin version to 0.2.0 without changing 0.1.0 tag/artifacts.
2. Repository audit and full UE Automation are clean.
3. `RunUAT BuildPlugin` passes Editor Development, Game Development, and Game Shipping.
4. Win64 BuildCookRun and packaged hash/screenshot/exit smoke pass.
5. Phase 2 commandlet integration produces canonical documents, diff JSON, 10,000-seed benchmark JSON, and logs.
6. README, architecture, limitations, release notes, AI disclosure, code walkthrough, interview guide, acceptance matrix, evidence journal, and walkthrough describe Phase 2 accurately.
7. Final delivery manifest rehashes every included file and names the same clean Git revision as verification/package manifests.

## Execution gates

1. Complete and commit PACT-20.
2. PACT-21 RED -> GREEN -> full regression -> commit.
3. PACT-22 RED -> GREEN -> full regression -> commit.
4. PACT-23 RED -> GREEN -> commandlet integration -> commit.
5. Run full BuildPlugin and Win64 package gate before deciding PACT-24.
6. If PACT-24 is entered, keep it one module/tab and re-run full distribution.
7. Complete PACT-25 docs, clean-revision VerifyAll/Phase2 report, FinalizeRelease, and independent completion audit.

## Failure policy

- Two failed fixes for the same issue trigger a root-cause packet before another change.
- Never loosen JSON validation, test warning policy, or checksum/revision gates to obtain green output.
- Preserve every timestamped 0.1.0 artifact and the `v0.1.0` tag.
- Do not modify generation behavior or golden hashes. Any observed golden change is a blocker, not an expected migration.
- Do not begin the optional Inspector while any required gate is incomplete.

# SeedForge Phase 2: Portable Layout Evidence

## User intent

Continue high-value work in the existing `D:\program\SeedForge` project before 2026-09-05 15:30 (UTC+8). Preserve the verified 0.1.0 delivery, keep all work inside the project, use one primary agent and no sub-agents, and avoid unrelated feature growth.

## Problem

SeedForge 0.1.0 proves deterministic generation inside one UE process, but its layout evidence is not portable: a reviewer cannot export a versioned layout, round-trip it, compare two layouts structurally, or reproduce a machine-readable performance run without reading test logs.

## Required outcomes

1. A versioned canonical JSON document containing configuration, layout, seed, endpoints, corridor cells, and hash without losing 64-bit integer precision.
2. Strict import with typed malformed/schema/version/hash/validation failures and a verified round trip.
3. A deterministic structural Layout Diff with machine-readable and human-readable output.
4. A commandlet-driven benchmark that emits warm-up, sample count, failures, min/median/P95/max, machine/toolchain identity, and configuration.
5. Tests, scripts, documentation, plugin/demo packages, and final delivery evidence for one clean revision.

## Optional outcome

A minimal code-native Editor Inspector may be added only after all required outcomes and distribution gates pass, with at least eight hours remaining and no unresolved regression.

## Non-goals

- Changing the generation algorithm or existing golden hashes.
- Cross-version migration beyond rejecting unsupported schema versions.
- Network transport, database/storage service, cloud dashboards, or web UI.
- General-purpose JSON reflection or diff framework.
- Absolute cross-machine performance promises.
- Hand-authored Blueprint or content assets.

## Preservation boundary

- Tag the verified revision `720ba4a8abc726add137d71c49572315f969f097` as `v0.1.0` before Phase 2 implementation.
- New work remains on `codex/seedforge-implementation` until the user chooses publication/integration.
- Existing timestamped 0.1.0 artifacts remain untouched.

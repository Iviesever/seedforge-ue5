# Acceptance matrix

| Requirement | Authoritative check | Evidence path | Status |
|---|---|---|---|
| Preserve verified 0.1.0 | annotated `v0.1.0` resolves to `720ba4a8...`; old final directories untouched | Phase 2 PACT-20 evidence | Passed |
| UE 5.8 Editor builds and loads | `Build.ps1`, `Smoke.ps1` | final verification logs | Passed |
| Original generation identity unchanged | five golden hashes plus 10,000-seed property sweep | `SeedForge.Validation.*` | Passed |
| Sync/async repeatability and cancellation | 5x100 sync, 100 async, cancellation/stale/shutdown tests | `SeedForge.Core.*`, `SeedForge.Async.*` | Passed |
| UObject lifetime and HISM visualization | subsystem and visualization suites; real runtime captures | Automation + PNG evidence | Passed |
| Exact versioned JSON | `MAX_uint64`, fixed byte order, typed errors, 100 round trips | six `SeedForge.Document.*` tests | Passed |
| Canonical import integrity | normalization, order-dependent hash rejection, config/topology validation | document tests | Passed |
| Deterministic structural diff | identical, endpoint-only, stable sort, reverse symmetry, summaries | five `SeedForge.Diff.*` tests | Passed |
| Reproducible benchmark statistics | exact nearest-rank samples and bounded real generations | `SeedForge.Benchmark.*` tests | Passed |
| Real report process chain | two Generate, strict-import Diff, 10k Benchmark, JSON reparse | `Scripts/Report.ps1`, Phase 2 report summary | Passed |
| Invalid Commandlet arguments fail | expected log error plus non-zero `Main` result | `InvalidCommandletArguments` | Passed |
| Minimal Editor Inspector | module/tab registration and code-native Slate implementation | `SeedForge.Editor.*` | Passed |
| Inspector visual quality | real Slate screenshot, marker, size check, manual inspection | `CaptureInspector.ps1`, Inspector PNG | Passed |
| Independent plugin distribution | Editor Development, Game Development, Game Shipping | BuildPlugin log + revision-aware plugin manifest | Passed |
| Win64 distribution | BuildCookRun and packaged hash/instance/screenshot/exit smoke | package manifest + logs + PNG | Passed |
| One aggregate verification entry | all required gates from a clean revision | `VerifyAll.ps1`, `verification-last.json` | Passed |
| Same-revision final delivery | verification, report, plugin, demo, source, docs and hashes agree | `FinalizeRelease.ps1`, `DELIVERY_MANIFEST.json` | Passed |
| Independent payload rehash | exact payload set, sizes, SHA-256, manifest SHA-256 | `AuditDelivery.ps1` | Passed |
| Scope and authorship honesty | limitations and explicit AI disclosure | `KNOWN_LIMITATIONS.md`, `AI_ASSISTANCE.md` | Passed |

Final verification is authoritative only when `verification-last.json`, both package manifests, the Phase 2 report summary, and the delivery manifest name the same clean Git revision and version.

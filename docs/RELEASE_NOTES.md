# Release notes

## SeedForge 0.3.0 — pre-final documentation checkpoint

These notes record the checkpoint following clean `a9f56254f11554316302936926211e75d86d7f4d`, before the documentation/images candidate's final gates. They are not a live assertion about remote publication; use generated release-readiness evidence and the PR/Release record for the final SHA, results and current status.

- Deterministic initial encounter plan and independent encounter hash.
- Pure bounded four-neighbor A* with checked/wide coordinate arithmetic and an explicit Failed/recovery-capable run state machine.
- Code-native top-down Player/Controller/Enemy/Core/Exit, an AHUD-owned native Slate HUD, and the fight/collect/extract loop.
- Persistent R/N restart bindings, actual run/request ownership, exact next-seed LCG, CLI seed, HP/death, cooldowns and bounded enemy replans. Dash uses current combined movement, then flat current aim, then +X; input release/focus/reset lifecycle is covered.
- Separate ordinary Editor/packaged input self-test: UE input injection and measured WASD/aim/attack/Dash/restarts, with setup teleports/public damage disclosed separately.
- Passive same-run A* movement proof before gameplay smoke's first capture; bounded first/latest movement evidence and aggregate distance/time checks.
- Render-owned screenshot token/run/request/frame/pixel/save completion, fresh PNG validation and delegate cleanup on success/failure/restart/EndPlay. The complete readable HUD replaces the earlier clipped-prefix result; original failed evidence remains historical.
- Strict source/process/log gates, explicit diagnostic identities, original UAT/Pak/IoStore log correlation, project-local filesystem DDC/temp and Cook attachment/loose-store controls, plus frozen evidence digests.
- The latest full Automation observed at this checkpoint was **119/119** at `diagnostic-cbd7ac3...`, with zero test/whole-log warnings/errors. The final clean candidate needs its own complete run; the old 63-test milestone is not the current checkpoint count.
- Phase 3 architecture, walkthrough, interview guide, live drills, acceptance, evidence, and AI disclosure.

Generator version 1, layout-document schema v1 and all five original layout golden hashes are preserved. Determinism applies to initial model identities, not every render pixel, exposure frame or real-time playthrough. The packaged start capture may show initial exposure settling; it is not post-processed into a brighter image or presented as production polish.

Observed package checkpoints are deliberately separate: clean `f235e46...` passed the actual three-target BuildPlugin matrix; clean `a9f56254f11554316302936926211e75d86d7f4d` passed the 193928–194106 BuildCookRun/ordinary capture/input/gameplay/four-negative chain and a 49-file index. The failed 191653 attempt remains retained. See `FINAL_HANDOFF.md` for exact artifacts; these checkpoints do not constitute one final all-16-gate revision.

After the documentation/images commit, `VerifyPhase3.ps1` must produce a fresh **MachinePassed** result for that clean SHA. Actual six-image visual review and fresh remote PR review then supply the explicit records required by `FinalizeRelease.ps1`. Finalization is local readiness, not publication. PR #1 merge and a source-only `v0.3.0` GitHub Release are already authorized after all gates. At this pre-final documentation checkpoint, no `v0.3.0` tag or formal Release had been created. Only GitHub's default source archives are distributed; generated binary Release assets are not uploaded. Publication results belong in the final external/generated records, not a recursive tracked edit to insert this commit's own SHA.

## SeedForge 0.2.0

### Added

- Versioned compact canonical JSON export and strict import.
- Exact decimal-string preservation for seed/hash across the full `uint64` domain.
- Typed document failures for syntax, field, schema/version, unsigned, order, hash, config, and topology errors.
- Deterministic structural Layout Diff with human and JSON summaries.
- Bounded benchmark runner with warm-up, distribution metrics, failure counts, and aggregate output identity.
- `SeedForgeReport` Editor Commandlet with Generate, Diff, and Benchmark modes.
- `Scripts/Report.ps1` real-process chain producing two documents, strict-import diff, and a 10,000-seed report.
- Minimal code-native SeedForge Inspector Nomad tab and deterministic Slate screenshot verification.
- Revision-aware plugin, Win64, report, aggregate verification, and final delivery manifests.
- Independent final payload and manifest SHA-256 audit.
- Six document, five diff, four benchmark/Commandlet, and one Editor test, bringing the total to 41.

### Preserved

- Generator version 1 and all five 0.1.0 golden hashes.
- Cancellation/newest-wins behavior, subsystem lifetime boundary, HISM demo, and packaged smoke.
- Annotated `v0.1.0` rollback tag and timestamped 0.1.0 deliveries.

## SeedForge 0.1.0

- Deterministic bounded room placement and canonical L-corridor generation.
- Byte-defined FNV-1a layout identity with five golden seed/hash pairs.
- Typed generation failures and complete topology validation.
- Cancellation-safe `UE::Tasks` coordination and `UWorldSubsystem` lifetime adapter.
- Pure C++ HISM graybox, generated map, free-fly demo, and automated screenshots.
- 25 UE Automation tests, BuildPlugin, BuildCookRun, packaged smoke, ZIP, checksums, and documented AI assistance.

SeedForge remains a focused engineering lab, not a complete procedural game. Review `KNOWN_LIMITATIONS.md` and `AI_ASSISTANCE.md` before presenting it.

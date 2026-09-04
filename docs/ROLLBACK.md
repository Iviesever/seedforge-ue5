# Rollback and recovery

## Historical immutable baselines

Annotated tag `v0.2.0` points exactly to:

```text
9a306f8ff72cb660d3c04b09806787df21191d45
```

It is the original Phase 3 branch base. Preserve the tag and recorded 0.2.0 delivery history; it is not proof that today's 0.3.0 verification or storage contract passed.

Annotated tag `v0.1.0` points exactly to:

```text
720ba4a8abc726add137d71c49572315f969f097
```

Inspect historical code without changing the checkout:

```powershell
git status --short
git log --oneline --decorate -15
git show --stat v0.2.0
```

Preserve user edits before an approved recovery operation. Prefer a reviewed, focused revert that adds history, or a deliberate recovery branch in this repository; do not destroy the checkout with `git reset --hard` or create a second integration checkout. A rollback revision must be freshly verified before it becomes current evidence.

## Historical Phase 2 checkpoints

- `2b08e2c` — approved Phase 2 issue and implementation plan.
- `63bfc82` — canonical layout document codec green.
- `b8ab523` — deterministic structural Layout Diff green.
- `4403d8d` — benchmark/report Commandlet and real process chain green.
- `1c76e57` — 0.2.0 version metadata and dynamic package naming.
- `c68d074` — minimal Editor Inspector green.
- `59fc3d0` — deterministic Inspector capture and visual evidence green.

Later documentation/release commits build on these checkpoints. Use `git log --oneline --decorate` and the final delivery manifest for the final revision.

## Historical PACT Phase 3 checkpoints

- `83e7672` — PACT-30 issue/blueprint/baseline.
- `319a302` — PACT-31 deliberate RED model contracts.
- `81a77a2` — encounter planner, bounded A*, and run state GREEN.
- `6e8fcbd` — PACT-32 deliberate RED Gameplay Framework contracts.
- `7f6fedd` — playable code-native World GREEN.
- `53b10f8` — PACT-33 deliberate RED smoke trace.
- `9ff53d4` — runtime/script gameplay smoke GREEN and clean package source revision.
- `2494275` — packaged evidence and screenshot root-cause packet.

These are historical PACT checkpoints on the original `feat/phase3-playable-vertical-slice` branch, not the final audit result. Later audit corrections supersede their current-behavior descriptions without erasing the original records.

## Independent-audit checkpoints

This section records the pre-final documentation checkpoint following a9f5625. It is historical evidence, not a live remote-release status page.

- `f235e46e9f637d706b9997e8af542873591c86f5` — project-local runtime storage controls; actual clean BuildPlugin subsequently passed UnrealEditor Development and UnrealGame Development/Shipping.
- `cbd7ac3412fb1500d62dff9426eaed1bf30f4ec9` — original Pak/IoStore log correlation checkpoint; H7's observed full 119/119 suite is explicitly `diagnostic-cbd7ac3...`, not a clean final certification.
- `d896e5c1d2a9ac0c79a666f2d24ea4167b328f1a` — H7 path-before-capture checkpoint; its first 191653 packaging attempt remains failed wrapper evidence after native BuildCookRun exit 0.
- `a9f56254f11554316302936926211e75d86d7f4d` — Cook attachment/storage correction; clean packaged ordinary capture, input, gameplay/path/three captures and four expected negatives passed in the 193928–194106 run.

The packaged checkpoint has a 49-file independent index. It does not combine with another SHA's BuildPlugin or diagnostic suite to certify one final candidate. Documentation/images must be committed and the final machine/manual gates rerun together. See `FINAL_HANDOFF.md` for exact artifact paths.

## Artifact fallback

Timestamped 0.1.0 final deliveries remain under `Artifacts/Final/SeedForge-0.1.0-*` and are never overwritten. Versioned plugin/demo ZIPs have adjacent SHA-256 files.

If verification fails, preserve its log, trace, source identity and original bytes. Keep prior timestamped artifacts for inspection; never relabel or overwrite them with unverified files, remove diagnostic prefixes, or edit images to make a failed review appear successful. Mutable `last-*` pointers are discovery aids, not immutable release authority.

Do not rerun historical scripts blindly: older code may predate the current filesystem-DDC/temp, Cook attachment, strict-log and evidence-sealing safeguards. Recovery still obeys the project boundary and read-only Engine rule. Only internal UBT `Trace*.uba` diagnostics/backups have the narrow approved user-directory exception; unknown external writes remain failures, not rollback permissions.

For 0.3.0 readiness, rerun `VerifyPhase3.ps1` on the recovered clean revision, then perform fresh visual/remote reviews and call `FinalizeRelease.ps1` with both explicit review paths. `MachinePassed` alone is not release readiness. Source-only publication after the full gate sequence is already authorized; it had not occurred at this pre-final documentation checkpoint. Consult generated readiness and PR/Release records for current status. Generated plugin/demo binaries must not be uploaded as Release assets.

## Schema compatibility

Versions 0.2.0 and the 0.3.0 candidate preserve generator version 1 and the five original layout golden hashes. Layout-document schema v1 rejects unsupported generator/schema versions. Gameplay-smoke and input-selftest are separate schema-v1 evidence formats, not interchangeable documents. A future algorithm/schema change needs explicit versioning, new evidence and a migration decision; do not accept old identities silently.

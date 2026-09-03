# Phase 3 candidate handoff

## Status boundary

SeedForge 0.3.0 is a feature-branch/Draft PR candidate. The Goal authorizes commit, push, and Draft PR, but not merge to `main`, a `v0.3.0` tag, or a formal GitHub Release.

## Authoritative local evidence

After the final documentation commit, run from a clean worktree:

```powershell
.\Scripts\VerifyPhase3.ps1
```

Use these files as sources of truth:

- `Artifacts/Reports/phase3-verification-last.json` — exact revision, version, test counts, and aggregate gates.
- `Artifacts/Plugin/last-plugin-package.json` — three-target plugin package and SHA-256.
- `Artifacts/Package/last-gameplay-package.json` — BuildCookRun output, ordinary packaged launch, packaged gameplay smoke, archive, hashes, trace, and screenshots.
- latest `Artifacts/Reports/Gameplay/*/summary.json` — independently reparsed gameplay trace and strict log audit.
- latest `Artifacts/Reports/Phase2/*/report-summary.json` — preserved canonical document/diff/benchmark chain.

All must name the same clean revision for the final Draft PR body.

## Run the candidate

Extract `SeedForgeDemo-Win64-0.3.0-*.zip` and launch:

```text
Windows/SeedForge.exe
```

Controls: WASD move, mouse aim, Left Mouse Button attack, Space dash, R restart same seed, N new seed. Pass `-SeedForgeSeed=<uint64>` for a chosen deterministic map/initial encounter.

## Reproduce gameplay evidence

```powershell
.\Scripts\TestGameplay.ps1 -Seed 24301
.\Scripts\PackageGameplay.ps1 -Seed 24301
```

The second command performs one BuildCookRun, verifies the ordinary non-smoke packaged path, then drives the packaged production gameplay loop through attack, kill, three Cores, unlock, and win.

## Rollback

- Immutable 0.2.0 baseline/tag/release: `9a306f8ff72cb660d3c04b09806787df21191d45` / `v0.2.0`.
- Phase 3 branch: `feat/phase3-playable-vertical-slice`.
- PACT checkpoints and recovery guidance: `docs/ROLLBACK.md`.
- Do not use destructive reset on the user's checkout; revert focused commits or branch from the immutable tag.

## Portfolio warning

Do not present the repository as independently hand-written C++. Read `AI_ASSISTANCE.md`, reproduce the evidence, explain the architecture without notes, and complete a personally authored test-first live-change drill before claiming technical ownership.

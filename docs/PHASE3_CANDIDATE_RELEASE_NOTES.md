# SeedForge 0.3.0 candidate release notes

Status: Draft PR candidate. This file does not create a tag or formal GitHub Release.

## Added

- Separately versioned deterministic encounter plan with stable player/exit/Core/enemy IDs and cells.
- Independent canonical encounter hash that preserves layout schema/hash version 1.
- Pure C++ four-neighbor bounded A* with Manhattan heuristic, fixed neighbor/tie rules, and typed statuses.
- Fail-closed run state machine for Generating, Playing, Won, Lost, and Restarting.
- Code-native top-down Character, PlayerController, five enemy Pawns, three Core actors, locked/unlocked exit, Canvas HUD, and single run coordinator.
- WASD movement, mouse aim/fallback, visible cooldown attack, dash, damage/death, same-seed restart, and deterministic next-seed run.
- Real Editor and packaged gameplay smoke using production attack/pickup/exit paths.
- Fixed-order JSON trace with Git/UE/seed/layout/encounter/count/state/action/screenshot/result evidence.
- Three gameplay captures and strict log/JSON/file validation.
- `TestGameplay.ps1`, `CaptureGameplay.ps1`, `PackageGameplay.ps1`, and `VerifyPhase3.ps1`.
- Phase 3 architecture, gameplay, code walkthrough, interview guide, live drills, acceptance, evidence, and honest AI disclosure.

## Preserved

- `v0.2.0` tag and published Release.
- Generator version 1, schema v1, five golden layout hashes, canonical codec, Layout Diff, benchmark, Inspector, async newest-request-wins, and shutdown safety.
- Original 41 Automation tests, now included in the 63-test suite.

## Known limitations

- UE 5.8/Win64 only; single-player graybox.
- Whole-run determinism is not claimed.
- Five enemies use a deliberately simple linear-open-set A* and direct waypoint movement.
- No NavMesh, Behavior Tree, EQS, GAS, networking, persistence, replay, production art, audio pass, or accessibility settings screen.
- Packaged combat evidence may clip the left edge of one HUD frame during rapid multi-capture smoke; packaged start/win and Editor combat frames are complete. See the root-cause packet.

## Verification

Run `Scripts/VerifyPhase3.ps1` from a clean revision. The authoritative exact revision and artifact paths live in `Artifacts/Reports/phase3-verification-last.json`; Draft PR text must quote that file rather than this candidate document.

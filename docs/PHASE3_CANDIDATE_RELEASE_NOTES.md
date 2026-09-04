# SeedForge 0.3.0 candidate release notes

Status: independently audited implementation and packaging checkpoints are available; the final clean-candidate machine/visual/remote sequence is required before publication. This document does not create a tag or Release.

## Added

- Separately versioned deterministic encounter plan, stable player/exit/Core/enemy IDs/cells and independent canonical hash.
- Pure four-neighbor A* with fixed tie-break, bounded expansions, checked neighbors and wide distance/cost arithmetic.
- Explicit Generating/Playing/Won/Lost/Restarting/Failed run lifecycle with typed failures and cleanup.
- Native top-down player, persistent PlayerController, five enemies, three Data Cores, exit, single Coordinator and AHUD-owned Slate text.
- WASD, mouse aim, visible cooldown attack, live-axis diagonal Dash, damage/loss, same-seed restart and deterministic next-seed runs.
- Separate ordinary-input self-test through UE viewport/input dispatch, including actual movement, loss, same/new/rapid async restart ownership and cleanup.
- Gameplay smoke with passive actual A* movement proof before its first capture, followed by production combat/pickup/exit rules.
- Render/pixel/save-owned screenshots with unique tokens, run/request/frame association, zero owned bindings and strict PNG decoding.
- Immutable clean-revision process guards, exact UAT target/log association, strict warning/error/storage gates, archive rehash and frozen nested evidence.
- Project-local filesystem DDC/TEMP, non-Zen loose cooking and a Cook-only override preventing optional EditorDomain Zen attachments.
- Updated walkthroughs, interview/drills, acceptance/evidence, honest AI disclosure and source-only release workflow.

## Preserved

- Immutable v0.2.0 tag/release and generator/layout schema/hash version 1.
- Five layout goldens, canonical codec, Diff, benchmark, Inspector, newest-request-wins and shutdown safety.
- Original 41 Automation tests. Latest observed complete audit checkpoint is 119/119 with zero warnings/failures; the final report determines final counts.

## Verified checkpoints

- Actual clean BuildPlugin: Editor Development, Game Development and Game Shipping at f235e46.
- Actual clean Win64 full package at a9f56254f11554316302936926211e75d86d7f4d, including ordinary input, positive gameplay, four controlled failure processes, original PNG inspection and a 49-file independent evidence index.
- The rejected first Cook and all earlier failed attempts remain recorded; they are not rewritten as successful results.

These are separate checkpoints. After the documentation/image commit, rerun the complete clean `VerifyPhase3.ps1`, inspect its six images, provide fresh visual/remote records, and run `FinalizeRelease.ps1`. Quote the resulting exact source revision and digests in the PR/Release rather than inferring them from this tracked file.

## Limitations and distribution

UE 5.8/Win64 only; single-player graybox with simple bounded A* and direct waypoint movement. Map/initial encounter determinism is not whole-playthrough or pixel determinism. No NavMesh, Behavior Tree, GAS, networking, persistence, replay or production art/audio pass. The old clipped-HUD frame is excluded and replaced by inspected native captures; initial exposure can still vary.

The requested v0.3.0 distribution uses GitHub's default source ZIP/tarball only. Locally verified plugin/demo archives are retained as evidence, not uploaded as Release assets. See [known limitations](KNOWN_LIMITATIONS.md) and [AI assistance](AI_ASSISTANCE.md).

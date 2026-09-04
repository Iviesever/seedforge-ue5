# Native HUD rendering blueprint

## Decision

Use a native Slate overlay owned by the existing ASeedForgeHUD. It reads the existing Coordinator snapshot and displays the same title, HP, Core progress, Seed, run state, exit lock, failure and input/restart guidance. It is hit-test-invisible and introduces no second state authority or Blueprint assets.

Alternatives considered: keep Canvas with batch/flush/layer changes (all reproduced the defect); replace with UMG (unneeded widget/asset framework); native Slate (selected bounded rendering-backend replacement). The internal Canvas defect is not claimed as fully diagnosed. The ownership boundary and repeated failing captures justify testing a different renderer, not asserting success in advance.

## Ownership and errors

AHUD creates one overlay for its World GameViewport, updates text during DrawHUD, and removes it on EndPlay or viewport change. Missing World/Coordinator/viewport means no stale overlay content. Weak UObject references and shared widget ownership cannot extend gameplay object lifetime. The whole overlay is hit-test-invisible, preserving real gameplay input.

Extract a pure FSeedForgeGameplayPresentation::BuildHudText(snapshot) for exact content tests. Use Slate's standard runtime font, white text with a black shadow and 32-pixel margin. No decorative panel, animation or menu. Include-UI screenshots remain restricted to the owning viewport.

## Acceptance

RED/GREEN tests cover exact metrics including uint64 Seed, all six run states, locked/unlocked exit, terminal replay hint and typed failure text. Existing state/input tests remain unchanged. Real repeated start/combat/win captures must contain every line at 1280x720; package proof remains required. Restart/EndPlay must not duplicate or retain an overlay.

## Blueprint checklist

- [x] Existing source, viewport hierarchy and rejected visual evidence inspected.
- [x] User intent and non-goals documented; no unresolved product question.
- [x] Three alternatives compared; bounded Slate backend chosen under Goal section I.10 autonomy.
- [x] Blueprint communicated to user; no added authority required.
- [x] Self-review: no placeholders, added product features or competing state owner.
- [x] Visual companion not needed: backend repair, not an open design/layout choice.

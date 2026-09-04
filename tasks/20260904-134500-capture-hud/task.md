# Native HUD implementation tasks

> **Execution:** executing-tasks in the existing checkout; test-driven-development and verification-before-completion. Primary alone runs UE.

**Goal:** Replace the defective legacy Canvas text path while retaining exact gameplay information and input ownership.

**Architecture:** Pure snapshot-to-text presentation in GameplayTypes; ASeedForgeHUD owns one Slate overlay and removes it at EndPlay. Slate/SlateCore are private Runtime dependencies.

**Constraints:** All project output in D:\program\SeedForge; installed UE 5.8 read-only; only the previously authorized UBT Trace exception; no second checkout.

## Task 1: Content contract

Files: Runtime Public/SeedForgeGameplayTypes.h, Private/SeedForgeGameplayTypes.cpp; Tests Private/SeedForgeGameplayTests.cpp.

- [x] Declare `static FString FSeedForgeGameplayPresentation::BuildHudText(const FSeedForgeGameplaySnapshot&)`; initial empty stub permits RED to execute.
- [x] Assert exact title/metrics/controls, canonical uint64 Seed, six state labels, failure details, terminal-only replay hint and exit lock in Automation.
- [x] Run `Scripts/Build.ps1`, then `Scripts/Test.ps1 -Filter SeedForge.Gameplay.Hud`; observe expected content failures.
- [x] Implement formatter using existing text and state labels; rerun focused tests GREEN.

## Task 2: Rendering ownership

Files: Runtime Public/SeedForgeGameplayActors.h, Private/SeedForgeGameplayActors.cpp, SeedForgeRuntime.Build.cs; Tests Private/SeedForgeGameplayTests.cpp.

- [x] Remove the failed temporary DebugCanvas pointer guard and old line drawing.
- [x] Store one `TSharedPtr<SWidget>` root, one `TSharedPtr<STextBlock>` text and weak GameViewport owner. DrawHUD resolves Coordinator and creates the root once with a 32px margin; text uses `FCoreStyle::GetDefaultFontStyle("Regular", 14)`, white color/black shadow and 440px wrap width.
- [x] Update text from the formatter on each DrawHUD; root `EVisibility::HitTestInvisible`. Remove the root from the owning viewport on EndPlay or changed viewport, then reset references. PostRender also preserves inherited hidden/debug visibility behavior.
- [x] Add a real Slate-overlay fixture regression: repeated draws have exactly one child, EndPlay removes it and hit-test visibility remains disabled. Verify RED before attachment implementation.
- [x] Build and run focused Hud tests, full SeedForge suite, and two real `Scripts/TestGameplay.ps1 -AllowDirtyDiagnostic` runs. Inspect all PNGs at original resolution.
- [ ] Record failure and success paths accurately; request independent review. Commit only with evidence, leaving package certification explicitly pending if not yet run.

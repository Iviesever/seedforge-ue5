# Strict startup-log failure: verification process culture

## Symptom and ownership

The 97/98-test SeedForge reports were green, but whole-log validation exposed 13 `LogAutomationTest: Error: Condition failed` records before SeedForgeTests loaded. These are not accepted final log evidence.

Installed read-only Engine source: LaunchEngineLoop.cpp runs built-in SmokeFilter tests before Engine initialization; LowLevelTestAdapter.h emits the generic CHECK failure; AutomationTest.cpp prints owning test names at Log verbosity, normally hidden by the category's Warning threshold.

Verbose diagnostic `StartupCulture/20260904-143524/baseline.log` attributes all 13: FUnifiedErrorTest_CreateErrorMessage (7), FUnifiedErrorTest_CreateErrorMessageWithContext (4), FStructuredLogFormatTest (2). UnifiedErrorTests.cpp compares localized CreateErrorMessage output with English literals; StructuredLogFormatTest likewise checks localized formatted strings against English literals. The process selected zh-CN from the OS. This first probe logged `Cmd: Quit` but did not exit and was killed at its unchanged 120s budget; it proves attribution only and is not accepted runtime verification.

## Discriminating A/B run

Same existing F binary, same `Automation RunTests SeedForge` and `TestExit=Automation Test Queue Empty`, same verbose LogAutomationTest setting, only added `-culture=en` in the second process. H's new source scaffolds were not compiled; neither probe claims H coverage or a clean current revision.

`Artifacts/Reports/StartupCulture/20260904-144858`:

- baseline: native exit 0, 98 SeedForge tests passed / 0 failed, 13 startup errors.
- en: native exit 0, the same 98 tests passed / 0 failed, 0 startup errors.

## Minimal remedy and invariant

Pin verification child-process culture to English. No test is removed/renamed/skipped, no log level is lowered, no error is allow-listed, no Engine/global/OS locale setting is changed. Normal game UI was already native English. Whole-log validation remains mandatory after each fresh process. The earlier occasional Dataflow Date initializer issue is distinct and remains unallowlisted if it reappears.

# Root-cause packet: commandlet compile failures

## Symptom sequence

1. Build `20260903-153555` failed because `Misc/LexFromString.h` does not exist in UE 5.8.
2. After locating and using `String/LexFromString.h`, build `20260903-153652` reached the next translation stage and failed because `EscapeJsonString(FString)` was ambiguous.

## Root cause

The commandlet used one stale include path and one helper name that collides with UE's global `EscapeJsonString` declared by `Serialization/JsonWriter.h` through transitive includes. Argument-dependent lookup exposed both the project helper and the engine helper.

## Evidence

- Local engine source resolves `LexFromString.h` only at `Runtime/Core/Public/String/LexFromString.h`.
- Compiler C2668 lists both `SeedForge::Report::Private::EscapeJsonString` and the engine global `EscapeJsonString` as candidates.

## Narrow corrective action

Keep the verified include correction and rename the project helper to `EscapeReportJsonString`, including its four call sites. No interface, command semantics, warning policy, or test expectation changes.

## Regression guard

Re-run the Editor build, focused `SeedForge.Benchmark` tests, then the full `SeedForge` filter before committing the implementation.

# Scoped log observation — fixture correction

- Initial identity RED `automation-20260904-113508` correctly proved missing applied ID, stale hashes, and uncleared pending ID. Its two log-substring failures alone were not sufficient log-format evidence because the test sink was later found incomplete.
- Initial GREEN attempt `automation-20260904-113649`: 2/3 passed. Actual engine report entries already showed `Gameplay ready run=2 request=3` and `Applied request=3 run=2`, while the sink's assertions failed.
- Root cause: UE 5.8 OutputDeviceRedirector registers devices with default `CanBeUsedOnMultipleThreads=false` as buffered; the dedicated primary logging thread delivers their callbacks. The test sink required `IsInGameThread()`, discarding those buffered records.
- Correction is test-only: register as an unbuffered multi-thread-capable output device, still discard non-game-thread emissions so the TArray is touched only on the test/game thread. Clear automatic backlog after registration to prevent historical records from satisfying assertions. No waits, timeouts, or production logging changes are used to fix observation.
- Revalidate the log-specific RED with corrected observation and temporarily restored pre-fix production log formats; then restore the actual request/run logging and rerun GREEN. This separates real format regression proof from sink failure.

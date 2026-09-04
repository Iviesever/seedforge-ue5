# SF-IRA-007/009 — request and pending identity evidence

## Source and acceptance

- Starting HEAD: `4b68da82c89f5dc1776a264fd94bb8dcd5d1b216`.
- Actual completion RequestId must remain distinct from Coordinator RunGeneration in snapshots/logs. Pending new seeds and failed restarts must not retain an old applied identity. Direct public value application is explicitly not an asynchronous subsystem completion.

## RED and fixture investigation

- Build exit 0: `Artifacts/Logs/build-editor-20260904-113451.log`.
- RunIdentity RED: 0/3 passed, 3 failed, no warnings/not-run/in-process, wrapper exit 1; `Artifacts/Reports/automation-20260904-113508/index.json`.
- Proven defects: expected applied request 3 but snapshot had 0; pending seed 202 retained old layout/encounter hashes; direct apply retained pending request 1.
- Initial GREEN attempt had correct production output (`run=2 request=3`, `request=3 run=2`) but the scoped log sink discarded buffered callbacks on the dedicated logging thread. Root cause and correction: `root-cause-007-log-fixture.md`. No production workaround or timeout increase was used.
- Corrected sink is unbuffered, records only game-thread emissions, and clears automatic historical backlog. Positive-control assertions prove actual ready/apply messages were observed.
- Authoritative log-specific RED with pre-fix formats temporarily restored: build exit 0 (`build-editor-20260904-114044.log`); targeted test wrapper exit 1, only the two expected format/identity assertions failed, while both observation controls passed (`Artifacts/Reports/automation-20260904-114048/index.json`). The actual fixed formats were then restored.

## GREEN and regression

- Private apply overload receives the matching completion RequestId. Only a successful apply publishes AppliedRequestId. Direct public apply cancels pending generation and uses request 0; it cannot invent an asynchronous source identity.
- StartRun clears layout/encounter hashes and pending/applied request IDs before externally observable actor cleanup; failure clears them; RunGeneration and requested Seed remain separate.
- Build exit 0: `Artifacts/Logs/build-editor-20260904-114128.log`.
- RunIdentity: 3/3, zero warnings/failures/not-run/in-process, exit 0; `Artifacts/Reports/automation-20260904-114133/index.json`.
- RunFailure: 6/6, exit 0; `Artifacts/Reports/automation-20260904-114144/index.json`.
- RestartInput: 3/3, exit 0; `Artifacts/Reports/automation-20260904-114156/index.json`.
- Full SeedForge: 86/86, zero warnings/failures/not-run/in-process, exit 0; `Artifacts/Reports/automation-20260904-114207/index.json`.

## Script marker validation

- Before edits, AST-extracted marker patterns in both CaptureDemo and PackageDemo accepted `Applied request=2 seed=24301 hash=7425849530159566348 floors=587 walls=436 gameplay=true.` despite the absent run ID.
- Both patterns now require positive decimal request/run IDs, a nonzero hash and geometry counts, and a terminal literal `gameplay=true.`.
- Reproducible command: `tasks/20260904-102407-phase3-independent-release-audit/verify-applied-marker.ps1`. Each actual script pattern accepts the correct divergent request/run example and rejects seven malformed examples; both scripts parse without errors. No package success is inferred from these pattern checks.

## Review and remaining scope

- Independent read-only review found no production blockers. One P3 limitation remains explicit: the direct-apply test observes cleared pending bookkeeping; the CancelGeneration call itself is statically reviewed rather than independently exercising cancellation delivery through the async pipeline. Existing async tests remain unchanged; these World tests manually deliver completions and do not claim to certify worker scheduling.
- Snapshot/production logging is now truthful; final packaged input/trace/revision certification remains in later tasks and all 16 gates.
- A physical check confirmed the pre-existing UBA default trace at `C:\Users\Iviesever\AppData\Local\UnrealBuildTool\Trace.uba` was updated during this workflow (including Editor's nested platform-validation UBT). A build-output confinement prelude must precede further UE runs; it is part of the pending G boundary work, not waived by these GREEN tests.

# SeedForge independent release audit implementation plan

> **Agent execution:** Use `executing-tasks` inline, one root cause at a time, with checkbox tracking and `test-driven-development`. No worktree or subagent is required or created.

**Goal:** Close the supplied audit findings, independently prove all 16 UE 5.8 gates, then merge PR #1 and publish a source-only Release under the latest user authorization.

**Architecture:** Preserve the existing Runtime/Demo/WorldSubsystem direction. Add fail-closed ownership boundaries and observability to the production path, not fake-test state mutation. Keep process/revision integrity in a shared PowerShell helper and capture completion at the UE render boundary.

**Stack:** UE 5.8, Win64, MSVC, C++20, PowerShell, existing Git checkout/PR.

## Global constraints

- Existing checkout: `D:\program\SeedForge`.
- Existing branch: `feat/phase3-playable-vertical-slice`.
- Installed UE 5.8 tree is read-only infrastructure.
- No second clone, worktree, or parallel UE integration directory.
- At most one UBT, UAT, UnrealEditor, Cook, BuildPlugin, or Package process.
- Generator/schema/golden hashes and `v0.2.0` remain unchanged.
- Repair order is A(001), B(003), C(004), D(006), E(007/009), F(005), G(002), H(008), I(docs).
- All verification outputs stay inside the checkout. No OS keyboard/mouse macros.
- Do not upload packaged binaries to GitHub Release; retain local package proof.
- Two failed corrections of the same fault require a root-cause packet before another edit.

## Task A — SF-IRA-001: explicit failed-run lifecycle

**Files:**
- Modify `Plugins/SeedForge/Source/SeedForgeRuntime/Public/SeedForgeRunState.h` and `Private/SeedForgeRunState.cpp`.
- Modify `Public/SeedForgeGameplayTypes.h`, `Public/SeedForgeGameplayCoordinator.h`, `Private/SeedForgeGameplayCoordinator.cpp`, and the HUD in `Private/SeedForgeGameplayActors.cpp`.
- Create `Plugins/SeedForge/Source/SeedForgeTests/Private/SeedForgeRunFailureTests.cpp`.
- Create test-local World fixture support if needed; do not add a test-only production mutation API.
- Create `Scripts/TestRunFailure.ps1` to execute the real pre-Playing negative smoke process.

**Contract/interface:**
- Preserve `StartRun(uint64)` call sites; return the actual assigned `uint64` request ID so callers/tests can correlate the public generation request boundary. Return 0 for synchronous rejection.
- Add `ESeedForgeRunState::Failed` and `FSeedForgeRunStateMachine::FailRun()`; failure is explicit and restartable, clears Core eligibility, and is idempotent.
- Add typed `ESeedForgeRunFailureCode` plus message to `FSeedForgeGameplaySnapshot` for HUD/diagnostics.
- Add private `EnterRunFailure(Code, Message)`: one cleanup path for current generation, validation, encounter, spawn, and state-start failures. Do not treat a stale completion or an invalid unsolicited apply call as a failure of the current run.
- Initialize smoke identity and arm its deadline before generation; request non-zero exit once after serializing pre-Playing failure. Actual process exit is verified with a subprocess, not mocked.
- Task B owns persistent R/N dispatch; task A verifies coordinator/state recovery and leaves that dependency explicit.

- [x] **Step A1 — write RED tests against real production boundaries.**

```cpp
const uint64 Current = Coordinator->StartRun(24301);
FSeedForgeAsyncCompletion Failed;
Failed.RequestId = Current;
Failed.Result = FSeedForgeResult::Failure(
    ESeedForgeErrorCode::InvalidGridSize, TEXT("audit generation failure"));
Subsystem->OnGenerationApplied().Broadcast(Failed);
TestEqual(TEXT("Failure is terminal"), Coordinator->GetSnapshot().RunState,
    ESeedForgeRunState::Failed);
TestEqual(TEXT("No partial Core actors"), Coordinator->GetLiveCoreActorCount(), 0);
```

Also test invalid-layout direct apply, destroyed-on-spawn Core rollback through the World actor-spawn delegate, stale failed completion while a newer request is active, duplicate failure idempotence, pure failure/restart transitions, and recovery to successful apply. Match expected error logs narrowly rather than silencing warnings.

- [x] **Step A2 — compile declarations/stubs and observe RED.**

```powershell
.\Scripts\Build.ps1
.\Scripts\Test.ps1 -Filter SeedForge.Audit.RunFailure -TimeoutSeconds 120
```

Expected: test assertions fail for retained Generating/partial objects/missing failure detail, not a compile typo. Independently launch invalid-grid smoke with valid output arguments and require a failed trace plus exit 2; at the old behavior it reaches the bounded external timeout without a trace.

- [x] **Step A3 — implement the centralized minimal GREEN path.**

```cpp
if (ActiveRequestId == 0 || Completion.RequestId != ActiveRequestId)
{
    return;
}
ActiveRequestId = 0;
if (!Completion.Result.IsSuccess())
{
    EnterRunFailure(ESeedForgeRunFailureCode::GenerationFailed,
        Completion.Result.ErrorMessage);
    return;
}
ApplyGeneratedLayout(Completion.Result.Layout);
```

The failure boundary cancels active generation, cleans owned actors/timers/path/capture state, clears failure-sensitive identity/HP/cooldowns, calls `RunState.FailRun()`, records typed details, and (for smoke) writes one failed JSON trace and requests exit 2 once. Rebind the coordinator generation delegate when recovering; leave stale request suppression intact.

- [x] **Step A4 — rerun focused and relevant regression.**

```powershell
.\Scripts\Build.ps1
.\Scripts\Test.ps1 -Filter SeedForge.Audit.RunFailure -TimeoutSeconds 120
.\Scripts\Test.ps1 -Filter SeedForge.Model.RunState
.\Scripts\Test.ps1 -Filter SeedForge.Gameplay
.\Scripts\TestRunFailure.ps1
```

Expected: all focused assertions green; the real invalid-config process emits a reparsed Failed trace and exits 2 itself, with no outer timeout/kill. Existing gameplay positive paths remain green.

- [x] **Step A5 — evidence and separate commit.**

Record exact command timestamps, before/after HEAD, process exits, reports/counts, and open dependency on B in `sf-ira-001-evidence.md` and `progress.md`. Commit only task A's files as `fix(gameplay): close failed run generation deterministically`.

## Remaining task boundaries

The unchanged `source-audit/verification_plan.md` supplies every acceptance row and the exact 16 final gates. Before each subsequent code task, write its focused executable subplan with concrete interfaces/tests into this directory; do not preimplement adjacent findings during task A.

- B: Character/Controller headers and implementations + World/input tests; one configured R/N dispatch persists without a pawn and supersedes one request exactly once.
- C: GameplayMath + Character axis state + pure direction tests; combine both live axes, reset zero/focus-loss, then aim, then safe forward.
- D: Path/Encounter arithmetic + their tests; widen before subtract/abs, checked neighbor construction, budget and input-order boundaries.
- E: Coordinator snapshot/log/trace and script correlation; preserve true completion request ID and reset pending active hashes.
- F: capture adapter, coordinator/preview/Inspector capture paths, PNG helper/tests; render-frame readiness and screenshot-completion ownership replace delay/file-size readiness.
- G: shared PowerShell evidence helper and temporary-repository negative tests; one captured ExpectedRevision, before/after process assertions, target/log evidence, archive/manifest hashes.
- H: production World and engine-input self-test harness; verify actual paths/movement, loss/restart identity, rapid requests, and ordinary packaged input with no OS macro.
- I: update current claims only from new machine evidence, run all 16 gates from one clean final revision, then publish under current user authority.

## Plan self-review

- All nine findings map to one repair task; no generator/gameplay feature expansion is proposed.
- Source packet's no-release text is historical; current release authority is recorded separately rather than editing the packet.
- A and B overlap only at recovery: failure/state cleanup stays A, input ownership stays B, and full recovery is not certified until both tests pass.
- Capture and evidence integrity require real Editor/package and script-negative proofs; green unit tests alone cannot close F/G/H or publication.

# SF-IRA-008 H1/H2/H3 RED-source handoff

## Status

The primary verified H6/storage/clean BuildPlugin checkpoints and then valid H7 RED: focused four pass/three intended failures and real PathObservationUnavailable/exit 2 with no captures. **No UE build or test was run by the sub-agent.** H7-G1 is now explicitly authorized and source-prepared; build/focused/full/Editor GREEN remains pending the primary. The earlier TArray fixture crash is retained and excluded from RED. These checkpoints are not overall release certification.

Starting HEAD observed: `ee22236d8b56c3f5954270b5a01f47b01003087a`. F native checkpoint supplied by the primary: `2035e925ceb73fd2aa24de2646a0ad6197216dc8`. Concurrent Script changes belong to the primary and were not edited by this subtask.

## Exact changes

New Runtime declarations/scaffolds:

- `Public/SeedForgeGameplayDiagnostics.h`: read-only path/move/resource/transition/queued observation values; bounded path evidence and proof API. A separate monotonic movement sequence supports multiple normal transient World ticks without altering global frame counters.
- `Private/SeedForgeGameplayDiagnostics.cpp`: proof always rejects observations and never completes; Reset only clears its own evidence value.
- `Public/SeedForgeInputSelfTest.h`: source-kind/failure enums, bounded trace declarations, codec and opt-in component interfaces.
- `Private/SeedForgeInputSelfTest.cpp`: source parsing/evidence validation reject; export always says Failed/MissingInputEvidence; component stays inactive and injects no input or callbacks. Diagnostic classification is intentionally a RED expectation, not partially implemented as a clean claim.

Existing Runtime scaffolds:

- `Public/Private SeedForgeGameplayActors`: new read-only Dash remaining-cooldown/path-snapshot getters return default zero/empty values. Existing input and movement methods are unchanged.
- `Public/Private SeedForgeGameplayCoordinator`: declaration/storage for three native observation delegates, and a default-empty resource snapshot getter. No production broadcasts, state-machine calls, timers, request lifecycle or capture behavior were changed.

Tests:

- New `SeedForgeGameplayPathIntegrationTests.cpp`: three tests.
- New `SeedForgeRunIntegrationTests.cpp`: two tests.
- New `SeedForgeInputSelfTestTests.cpp`: six tests.
- `SeedForgeInputTestWorld.h`: additive BeginWorldTicks/TickWorld/OwnedRunActors helpers. Only the new path test calls the World ticking helpers. Begin play uses the engine's `AWorldSettings::NotifyBeginPlay`, and movement uses normal `World->Tick`, never direct Enemy::Tick/ReplanEnemies calls. Existing fixture methods remain unchanged.

No Build.cs, gameplay-smoke schema/flow, player input hooks, CLI adapter, scripts, Git index/HEAD/branches or Engine files were changed.

## Expected assertion boundary (not executed)

| Namespace/test | Intended observation |
|---|---|
| GameplayPath.RealReplanAndMovement | Existing finite bounded physical movement is a control; missing path-applied event/proof must fail. |
| GameplayPath.ProofRequiresActualBoundedSamples | A real A* value plus two valid samples must be accepted/complete; rejecting proof scaffold must fail. Completed proof must retain at most two samples after 4096 further calls. |
| GameplayPath.RejectsInvalidAttributionAndBounds | Wrong run/request/revision, nonfinite/off-route motion and oversized paths must remain rejected; fail-closed control. |
| RunIntegration.LostSameSeedFreshOwnership | Existing public damage/R/same-hash/fresh-actor behavior is controlled; absent resource/cooldown/transition observations must fail. |
| RunIntegration.NewAndRapidQueuedIdentity | Existing deterministic next seed/rapid latest apply are controlled; absent two queued observations must fail. |
| InputSelfTest.SourceIdentityClassification | Clean-shaped and `diagnostic-<40hex>` must parse distinctly; rejecting parser must fail positive expectations. Malformed inputs remain rejected. |
| InputSelfTest.DiagnosticTraceCannotClaimClean | Diagnostic kind and prefix must survive serialization; fixed failure exporter must fail those missing fields and cannot publish Passed. |
| InputSelfTest.MissingViewportFailsOnceAndCancels | Real viewport absence must produce typed ViewportUnavailable and one completion; default MissingInputEvidence/no-notification scaffold must fail. Repeated cancel/late hooks may not fabricate input or completion. |
| InputSelfTest.DisabledRunnerHasNoEffects | Disabled component causes no run change or observation growth; control. |
| InputSelfTest.RealBindingRemovalCannotProduceEffects | Remove W/Attack/Dash/R/N bindings only from fixture InputComponents; verify actual mapped effects disappear while unmodified D still produces real movement intent. No live state/hash/HP is assigned. |
| InputSelfTest.TraceRejectsMissingAndUnboundedEvidence | Synthetic codec values forcing success or over-limit event/queued arrays remain rejected; control, not runtime proof. |

Expected source-level division is seven RED tests and four controls, **subject to the primary's actual compile/test report**. A compilation failure or a transient-world setup error is not accepted as the intended RED proof. The tests using fixture-delivered completions are explicitly not ordinary packaged async/worker proof; that remains H6/H8.

## Primary commands

From `D:\program\SeedForge`, using the primary's current diagnostic/verification wrapper policy:

```powershell
.\Scripts\Build.ps1
.\Scripts\Test.ps1 -Filter SeedForge.Audit.GameplayPath -TimeoutSeconds 120
.\Scripts\Test.ps1 -Filter SeedForge.Audit.RunIntegration -TimeoutSeconds 120
.\Scripts\Test.ps1 -Filter SeedForge.Audit.InputSelfTest -TimeoutSeconds 120
```

The sub-agent ran only read-only source/Engine inspection, file edits through apply_patch and static diff/name checks. `git diff --check` reported no whitespace errors at the checkpoint. No C++ compiler/UHT validation has occurred here.

## Primary RED results and fixture diagnosis

The primary reported strict diagnostic build `145301` as `DiagnosticPassed`, with both audited build logs at zero errors/warnings. The following full `index.json` files were read directly:

| Namespace | Report directory under `Artifacts/Reports/` | Passed / failed | Relevant observations |
|---|---|---|---|
| GameplayPath | `automation-20260904-150552-48c734353bf94193bb32a9e68f5b82d7` | 1 / 2 | Pure proof: four intended missing-acceptance errors. World test: missing event/proof plus failed existing-movement control and one EndPlay warning. |
| RunIntegration | `automation-20260904-150606-d2c18316166d46fe839664d6260ba60c` | 0 / 2 | Missing resource/cooldown/transition observations (four errors) and absent queued observations (one error). Existing same/new/rapid identity controls passed. |
| InputSelfTest | `automation-20260904-150618-56350c7319bc474c9ff856d2d1d7a74c` | 3 / 3 | Diagnostic serialization: two errors; typed viewport/exact-once completion: three; source classification: six. Disabled/binding-removal/fail-closed controls passed. |

Raw overall outcome: seven failed tests and four passing controls, 23 assertion errors and one fixture cleanup warning. The World test's `Real enemy moved without directly ticking it` failure is **not** accepted as intended RED evidence.

UE 5.8 source confirms the fixture problem:

- `Engine/Public/TimerManager.h:466-468`: `HasBeenTickedThisFrame()` compares `LastTickedFrame == GFrameCounter`.
- `Engine/Private/TimerManager.cpp:1136`: repeated same-frame Tick calls return immediately. Lines 1374-1388 mark the first tick's frame and activate pending timers only at that tick's end. The old synchronous 120-iteration World loop therefore did not supply 120 advancing timer frames for the normal 0.05-second first replanning delay.
- `Core/Private/Misc/AutomationTest.cpp:679-715`: a latent command returning false yields until later framework processing; a GFrameCounter equality guard can enforce one World tick per actual host frame without modifying that counter.
- `Engine/Private/World.cpp:6189-6215` provides the normal `UWorld::EndPlay` boundary and clears begun-play state; `CleanupWorld` warns at line 6236 when that boundary is missing.

Only test fixture code changed for this experiment:

- `RealReplanAndMovement` now enqueues an owning latent command. It retains one fixture, target weak pointer and proof, checks actual engine-frame progression, advances one normal World tick per distinct frame, and retains the unchanged 120 ticks / two seconds of simulated time cap. No Sleep, direct Enemy::Tick, direct ReplanEnemies, global frame alteration or timeout growth is used.
- A small fixture-owned repeating timer counts actual timer firings; final test Info reports ticks, timer firings, engine-frame endpoints and displacement. This discriminates timer progression from missing evidence production.
- Fixture teardown calls `World->EndPlay(EEndPlayReason::Quit)` only when play began, before the existing destruction path. No warning suppression was added.
- Runtime getters, events, proof/source parsers, self-test component and exporter remain the same RED scaffolds.

Next primary commands:

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.GameplayPath -TimeoutSeconds 120 -AllowDirtyDiagnostic
```

The discriminating result is now observed below. The initial invalid movement-control failure is retained as history, not repurposed as valid RED.

## Corrected RED and minimal GREEN implementation handoff

The primary reported strict build `152545-3f1a17fce9964e17a4efc28caa703315` passed with zero errors/warnings. The full corrected report `Artifacts/Reports/automation-20260904-152614-dd14b4ce94a746f2b51e6514076aae0b/index.json` was read:

- GameplayPath: one pass, two expected failures, zero test warnings.
- `RealReplanAndMovement`: `ticks=120 timer_firings=118 engine_frames=308..428 displacement=502.666685`. All physical/timer/frame/bound controls passed; only absent actual path-applied observation and proof failed.
- `ProofRequiresActualBoundedSamples`: the four expected scaffold acceptance/completion assertions failed.
- The rejection control passed. The previous World EndPlay warning disappeared.

This establishes valid RED for the movement/proof boundary without weakening its assertions or changing production gameplay.

Authorized implementation now prepared (not UE-verified yet):

- Enemy path revisions and actual Tick From/Target/To/delta/frame/sequence observations, with reset on SetPath/ClearPath. Existing interpolation, movement speed, target advancement and gameplay decisions are unchanged. Snapshot copying refuses paths above the diagnostic cap without changing the actual path.
- Read-only real Dash/attack cooldown and interaction/repath timer queries.
- Actual run-transition observations (including transient Restarting), queued request observations and successful A* observations published after the real SetPath. Snapshot values are copied at their production boundary; no live state references are exposed for mutation.
- Bounded passive path proof: at most 1025 cells/1024 waypoints; first/latest movement samples only (maximum two). It validates identity/revision, walkable cardinal path cells, actual waypoint conversion, contiguous finite movement toward the consumed waypoint, route-cell footprint, waypoint advancement and speed bounds. Aggregation freezes after at least two observations and 20 units.
- Strict native source parsing accepts 40 ASCII hex digits or explicit `diagnostic-<40hex>`, preserving raw diagnostic identity and distinct source kind. Failed JSON is deterministic/escaped and always sets `sourceVerified=false`; native string classification never certifies the checkout or binary as clean.
- Minimal component startup rejects missing real viewport with typed failure and one completion; cancellation/EndPlay cannot duplicate completion. The component subscribes to no input/World delegates yet.
- `BeforeInput`/`AfterInput` remain no-ops, `IsRunning` remains false, and complete ordinary-input evidence validation remains fail-closed. No CLI wiring, input driving, positive input-selftest success serializer or H7 capture/smoke change is enabled.

No test assertions were removed or softened for this implementation. Static diff/whitespace checks passed; C++ compilation and runtime GREEN remain pending the primary.

Requested primary checkpoint:

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.GameplayPath -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.RunIntegration -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.InputSelfTest -TimeoutSeconds 120 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge -TimeoutSeconds 900 -AllowDirtyDiagnostic
```

## H5 GREEN checkpoint verified by the primary

The primary reported strict build `160049-8e741022f34a4805b4ce5443b728fd2d` and all whole-log audits at zero errors/warnings. Report counters were read directly:

| Report under `Artifacts/Reports/` | Passed / failed / warning-success |
|---|---|
| `automation-20260904-160130-81205467ecd944f78353fb6f3d2dbc6c` | 3 / 0 / 0 |
| `automation-20260904-160149-9cf2762573e94bb385978f9786b64990` | 2 / 0 / 0 |
| `automation-20260904-160201-483b41c369ad48d684b7e818eced8803` | 6 / 0 / 0 |
| `automation-20260904-160608-31c4e8d832434422a94b162105c36274` | 109 / 0 / 0 |

These are development results bound to diagnostic `ee22236...` source identity, not a clean-release claim.

## H6 RED preparation

Native edits only, with no Scripts changes:

- Persistent controller: opt-in component creation; normal PreProcessInput/PostProcessInput forwarding; one OnFinished binding; cancellation/unbinding on EndPlay; one trace-publication/exit fence.
- Pure CLI parsing: opt-in absent leaves startup unchanged. Enabled mode requires an absolute `.json` trace path, valid source identity and canonical uint64 seed (default 24301); smoke/capture-auto-exit combinations are rejected. Coordinator startup declines invalid mixed modes so another smoke/capture driver cannot start.
- Completion adapter: writes the explicitly supplied fresh trace path using UTF-8/no-replacement semantics, emits one failure marker and requests failure status 2 after the synchronous save. This uses the established pre-first-frame failure-exit boundary, not a sleep or external kill. If the destination is missing/invalid or cannot be written, it fails nonzero without overwriting existing data. The authoritative wrapper owns run-directory containment/freshness and clean-revision checks.
- Component remains fail-closed: with a real scene viewport Start reports `MissingInputEvidence`; no input is injected, and no successful evidence is manufactured.
- Bounded success-value declarations are now complete: four run snapshots, at most 16 actors each, at most 16 setups, at most 4096 initial walkable cells, existing capped path/movement evidence, 96 input events, 24 transitions and eight queued records.
- Four new tests extend InputSelfTest to 10 tests: `CompleteSyntheticEvidenceAccepted`, `CompleteSyntheticSchemaSerialized`, `CompleteSyntheticEvidenceRejectsTampering`, and `CliOptionsSafety`.

The synthetic positive factory uses real pure-generated layout/encounter hashes and a real pure A* path, then explicitly synthetic actor identities/movement/input/transition values. It runs in an Automation value-test namespace, labels its purpose, and never writes a runtime evidence file. It covers four snapshots, 13 ordered effects, four queue records, 10 transitions and valid bounded path samples. The negative cases remove each required effect and corrupt direction/aim/Dash/kill results, identities, freshness, lifecycle, finite path data, completion or cleanup. Missing success acceptance/schema serialization are intentional RED expectations; their implementations remain disabled.

The exact final JSON field contract, numeric encoding, scenario ordering, caps and failed-vs-passed shape are documented in `task-008.md`, section **H6 JSON field contract — schemaVersion 1**, for the primary's PowerShell implementation. At this checkpoint the native failure envelope is emitted; success-only objects/arrays are intentionally absent and the schema test must detect that.

Expected focused outcome (not run here): InputSelfTest has eight passing controls and two failing tests for complete synthetic acceptance/schema. Compilation errors or malformed fixture data are not valid RED and must be separated from those expectations.

Primary commands:

```powershell
.\Scripts\Build.ps1 -AllowDirtyDiagnostic
.\Scripts\Test.ps1 -Filter SeedForge.Audit.InputSelfTest -TimeoutSeconds 120 -AllowDirtyDiagnostic

# Primary supplies fresh absolute project-local paths and the diagnostic identity.
& $EditorExe $ProjectFile /Game/Maps/SeedForgeDemo -game -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 -unattended -nosound -culture=en -SeedForgeInputSelfTest -SeedForgeSeed=24301 "-SeedForgeInputTrace=$FreshInputTracePath" "-SeedForgeGitSha=$DiagnosticSourceIdentity" "-abslog=$FreshInputLogPath"
```

The real Editor RED requires native exit 2, exactly one `SEEDFORGE_INPUT_SELFTEST_FAILURE code=MissingInputEvidence ... written=true`, a reparsed fresh Failed trace with that code, diagnostic source classification, `sourceVerified=false`, `mode=ordinary`, completionCount 1 and zero remaining owned bindings/pressed keys. It must not include `-SeedForgeGameplaySmoke` or capture-auto-exit options. Missing/invalid arguments may have no trace when no safe destination exists, but must still fail nonzero; an existing trace must never be replaced.

Static diff/whitespace checks passed. No H6 build, focused test or Editor process has been executed by this sub-agent.

## H6 observed RED and authorized GREEN source handoff

Primary-reported strict build `164019-bd8e3868591645259020a7c16dda1371` passed with zero errors/warnings. The actual report `Artifacts/Reports/automation-20260904-164100-171d64eafbaa4e78a33741717f43d20a/index.json` was read: 8 succeeded, 0 warning-success, 2 failed, 0 not-run. The only failures are `CompleteSyntheticEvidenceAccepted` (four missing-success assertions) and `CompleteSyntheticSchemaSerialized` (ten absent-success-schema assertions). The rejection and CLI controls pass. These are the intended RED boundary, not compilation/fixture errors.

The real ordinary Editor RED is retained under `Artifacts/Reports/InputSelfTest/20260904-164323-ba17eba96b194f6cb798e09f9055002b`. Its process observation records exitCode 2, outerTimeout false, start `2026-09-04T08:43:23.5304673+00:00`, end `2026-09-04T08:43:31.8311781+00:00` (about 8.3 seconds). The trace contains Failed/MissingInputEvidence, diagnostic source `diagnostic-ee22236d8b56c3f5954270b5a01f47b01003087a`, sourceVerified false, mode ordinary, gameplaySmokeEnabled false, completionCount 1, remainingDelegateBindings/remainingPressedKeys 0, and started/completed frame 0. The primary observed one failure marker with written=true. This was not an outer timeout or gameplay-smoke execution.

After that RED, the primary explicitly authorized H6 GREEN. Prepared source changes:

- Complete bounded success validator/serializer, with independent pure regeneration of all layout/encounter identities, exact canonical cells, A* result comparison, finite matching path/sample bounds, strict four-run/13-effect/four-queue/ten-transition relationships, ownership/freshness, cooldown and cleanup checks.
- Actual condition-driven controller-owned driver: simulated keys in PreProcessInput; real scene-viewport pointer updates and normal PC deprojection; actual post-input Dash launch and post-World physical effects; real cooldown/braking conditions; no manual input-stack pump, direct attack/aim/Dash call, AI disable, arbitrary sleep or timeout increase.
- Canonical logged setup poses only before measurements, capped at 16. At most two retained path samples, four run snapshots with at most 16 actors each, 4096 initial walkable cells, and the existing capped effect/queue/transition arrays. Live ownership is independently enumerated, old weak actor identities must be gone, and tagged HUD widget traversal is bounded at depth 64 / 4096 visited widgets.
- Source-hidden cursor protection: the engine `OnMouseMove` contains an OS cursor restoration branch for hidden-after-capture state. The driver rejects capture, invisible cursor, invalid geometry/projection, and a read-only `OnCursorQuery` result of None before dispatch, so it never deliberately enters that branch. Engine files were read only.
- R/N travel through the persistent controller mappings. Loss uses only public ApplyPlayerDamage; immediate restart checks require unpossession/old actors gone and zero stale hashes/applied request, then wait for actual async application. Rapid N/R share one engine input frame; only the latest queued request can satisfy the final run.
- Cleanup removes owned World/Coordinator delegates, releases owned keys, flushes normal controller key state and checks required keys are no longer down before completion. One completion/write/exit fence remains. Only native validation plus a fresh successful file write can emit the success marker/exit 0.
- Explicit Runtime `InputCore` and `ApplicationCore` dependencies; no Editor-only Runtime dependency. Passed envelope failureCode/failureMessage are empty strings, and sourceVerified is always false, including clean-shaped declared identities. Existing synthetic success fixture expectations were preserved; two schema assertions additionally lock these envelope fields.

Static verification performed by this sub-agent: read current repo rules and approved plan, inspect Engine input/viewport signatures and cursor branch, run `git diff --check` (exit 0, no output), and search the self-test source for prohibited direct handler/manual tick/sleep calls (none). These are static checks only, not a C++ build or runtime pass.

## Current stop condition

H6 GREEN source is ready for the primary's strict build, focused InputSelfTest (10 tests), H5 regressions/full Automation, and fresh ordinary Editor self-test. No new GREEN counts are claimed. If actual viewport/input/physics assumptions fail, preserve the failed trace/log and diagnose the production boundary without weakening synthetic expectations or extending the 30-second budget. H7 still requires the primary's real H6 GREEN checkpoint. No Scripts, UE processes, Git index/HEAD/branches or commits were changed by this sub-agent.

## H6 first live GREEN attempt — observed failure and diagnostic checkpoint

The primary reported strict build `171829-35798512e81f4244ad83d8e9f58d2dee` and focused InputSelfTest `automation-20260904-171901-fca2f82c20a249a9803ba5cbf1b99b7f`: build passed with zero errors/warnings; focused tests 10/10 with a clean log. This proves native synthetic acceptance, not ordinary live completion.

The first live attempt is preserved without alteration at `Artifacts/Reports/InputSelfTest/20260904-171917-61a3c9427537408ab9715bb57dcc82a4`. It exits 2 without outer timeout, with native SelfTestTimeout after 30 seconds (`phase=3 effect=3`). The actual trace records 23 observed enemy moves totaling 20.081073462960905 units, two retained samples, completed AimX/AimY in frames 83/84, and AttackFirst in frame 85 (release 86). The production attack log confirms enemy 0 reached 50 HP. AttackKill is injected at frame 224/released 225 but remains unconfirmed; no second successful-hit log exists. Later normal enemy contact causes Lost. This is the first live failure, not a second failed fix.

The full trace was parsed and the relevant log, input dispatch, target selection and driver source boundaries inspected. The existing trace cannot discriminate input nondelivery from an attack miss: its attack `cooldownAfter` was overwritten every subsequent frame until it returned to zero. Production `TryPlayerAttack` consumes cooldown even when no candidate matches, so the immediate post-input value is essential. Target geometry at frame 224 was not retained. The target was only arranged before the first attack; it remained under real AI during the 0.45-second cooldown, but target drift is still a hypothesis, not an observed root cause.

The next checkpoint is diagnostic only: exactly two pre-attack and two immediate post-input log records (bounded by the fixed scenario) expose real player/target/aim, distance/dot, target path revision/index, key-down/just-pressed, run/request and immediate cooldown/live count. Attack fields now retain the synchronous result at their injection frame instead of erasing it later. No setup, input sequence, attack retry, AI, tuning, timeout or success expectation is changed. The parent must rerun to identify the boundary before any target-placement correction is justified; H7 remains paused.

## H6 attack RCA packet — discriminating run and held-pointer correction

**Retained reproduction:** primary build `172705-6510e6c742de4ef1b0abc5a3d436e6bd` passed with clean diagnostics; focused report `automation-20260904-172713-12bbb69446cf4de68a4328b9bed11797` is 10/10, zero warning-success. The unchanged scenario plus diagnostic sampling ran under `Artifacts/Reports/InputSelfTest/20260904-172726-53a93e523eac4d389974833045997183`, exit 2, outerTimeout false, same 30-second phase 3/effect 3 failure. This diagnostic rerun did not attempt a gameplay/input fix.

**Discriminating facts:** frame 106 AttackFirst has player `(8800,400,84.15)`, target `(8800,600,58)`, aim approximately `(0,1,0)`, distance 200 and dot 1. The engine reports key-down and just-pressed true, cooldown 0.45, and the production hit log shows enemy 0 at 50 HP. At frame 243 AttackKill has target `(8870.897,514.573,58)`, still only 134.734117 units away, but actual player aim `(0.603,-0.798,0)` gives dot `-0.360739`, below the real attack threshold 0.25. The second key is down/just-pressed and consumes cooldown 0.45, yet the target survives. Thus input delivery, cooldown availability and distance are not the failure boundary; the requested positive-Y pointer intent was no longer the actual aim. Re-teleporting the enemy would not address that observed aim drift.

**Engine/source trace:** read-only UE 5.8 `SceneViewport.cpp:261` converts each incoming pointer event into `CachedCursorPos`; `OnMouseMove` at line 895 refreshes that cache. The self-test called that method only once for each aim proof and intentionally never moved the OS cursor. `SlateApplication.cpp:1731` invokes `FSlateUser::SynthesizeCursorMoveIfNeeded`; `SlateUser.cpp:736` builds another pointer event from the Slate user's cursor position and routes it through `ProcessMouseMoveEvent`, which can call widget OnMouseMove (`SlateApplication.cpp:5969`). Therefore a one-shot viewport cache update is not persistent input ownership: ordinary Slate cursor events can replace it between game frames. The changed live aim is observed; the exact individual hardware-versus-synthetic overwrite callback was not separately instrumented, so that narrower event-origin detail is not claimed.

**Single bounded correction:** hold the requested X/Y pointer direction as component-local intent and re-project/re-feed it through the same guarded real scene `OnMouseMove` in every normal PreProcessInput while the aim/attack phase is active. Ordinary PlayerTick still performs deprojection and the real input stack still calls Attack. The first two aim proof records remain exactly two; repeated held-intent feeds append no trace events or per-frame history. Disable held intent immediately after AttackKill and in every cleanup path. No direct aim assignment, OS cursor call, enemy placement change, AI disable, extra attack press/retry or deadline increase is added. The original failed attempts remain unmodified. This is the first input-boundary correction, pending primary live verification.

## H6 Dash P2 — new RED boundary

Review against the current validator confirms Dash only checked finite endpoints, `Dist2D > 1`, launch and cooldown. That admits short/reversed/perpendicular/off-map/overlong movement, and finite input coordinates whose derived distance overflows to infinity. New test `SeedForge.Audit.InputSelfTest.DashRejectsUnboundedAndOffMapMotion` preserves the complete synthetic positive control and adds eight rejection expectations: 2-unit motion; negative diagonal; wrong diagonal; both endpoints off-map; finite-coordinate derived-distance overflow; 201-unit motion; off-map start; off-map end. No validator or physical Dash threshold change is made before the primary observes this RED.

Expected next focused checkpoint: existing ten tests remain passing, the new Dash bounds test fails its eight currently accepted tamper assertions (subject to actual primary execution). After that RED, the proposed minimum GREEN is finite derived delta/distance, distance 19.9..200, positive diagonal alignment, and capsule-safe walkable endpoints, preserving actual launch/cooldown checks; the physical driver must observe at least 20 units before completing Dash. The parent-owned PowerShell validator mirrors that contract. Static `git diff --check` is clean. No UE or Scripts/index/commit actions were performed here; H7 remains paused.

## Dash observed RED and minimum GREEN handoff

The primary reported strict build `173733-d84553c84eaa4fbeb65a6c3e44b8da84` passed. Actual focused report `automation-20260904-173748-f68dafc6fcbb42d195dd539eca0eaa9b/index.json` was read: 10 succeeded, zero warning-success, one failed, zero not-run. The only failure is `DashRejectsUnboundedAndOffMapMotion` with exactly eight errors: TooShort, NegativeDiagonal, WrongDirection, OffMapBothEndpoints, DerivedDistanceOverflow, TooLong, OffMapStart and OffMapEnd. Its complete positive control remains passing. This establishes the intended P2 RED without weakening the fixture.

The same checkpoint's held-pointer live run at `Artifacts/Reports/InputSelfTest/20260904-173800-5e69704a1e3f406cafc174ec51033182` has native exit 0, no outer timeout, one success marker, 13 effects / four runs / ten transitions / four queues / zero remaining bindings and pressed keys. AttackKill at frame 275 retains positive-Y aim and dot 0.850333; immediate key-down/just-pressed are true, cooldown 0.45, enemy count becomes four and the target is dead. This is direct confirmation of the input-intent boundary correction; no enemy teleport or extra attack was required.

That native Passed trace is **not final H6 GREEN**: Dash was recorded at frame 542 with only `3.67835909128154` XY units because the original driver completed at >1 unit. The independent parent-owned PowerShell validator correctly rejects it. The artifact remains retained as diagnostic evidence, not edited into a passing trace.

After explicit primary authorization, the native validator now rejects nonfinite scalar deltas and derived distance; requires 19.9..200 XY units inclusive, positive diagonal dot >=0.98, and both endpoints in full-cell canonical walkable footprints. Full-cell endpoint semantics are the primary's final contract (the driver's separate per-frame capsule check remains stricter). Existing actual 1200-unit diagonal launch and cooldown constraints are unchanged. The driver completion threshold is now >=20 units. Source inspection confirmed `AfterInput` already guarded Dash launch/cooldown sampling with `GFrameCounter == InjectedFrame`; that guard is preserved and documented, so later movement frames cannot overwrite consumed PendingLaunchVelocity with zero. No extra event or retry is added.

Static `git diff --check` exits 0. New native GREEN counts and external-validator success remain pending the primary's fresh build/focused/live rerun. H7 remains paused; no Scripts, UE process or Git mutation was performed by this sub-agent.

## H6 functional GREEN checkpoint and separate boundary blocker

The primary reported strict build `174458-3755e487bb634e8b891004fb5480308b` passed. Actual focused report `automation-20260904-174505-d32fba0ed4c84161b18c8b37ad293aaa` has 11 succeeded, zero warning-success/failures. Full `automation-20260904-174517-6f2d45e50e314b51858e5aaa5ac74099` has 114 succeeded, zero warning-success/failures. Whole build/test logs have zero errors/warnings according to the parent gate.

Fresh ordinary Editor run `Artifacts/Reports/InputSelfTest/20260904-174538-5706158a156a42a78a5f65cfd5508e33` is native Passed/exit 0 with one completion, 13 effects, four runs, ten transitions, four queued requests and zero remaining bindings/pressed keys. Path proof totals 20.5814444422722 units over 25 actual observations. Dash injection frame 548 reaches confirmation frame 553, preserving actual launch `(848.528137423857,848.528137423857,0)` and injection cooldown 1.25 after release frame 549. The parent external validator passes; its independent negative harness is reported at 157 cases. The saved summary says DiagnosticPassed for `diagnostic-be52224f56f04c69d51cd1e93bf67b523fb91b93`; logProof records 1571 lines, zero errors, 25 explicitly allowed UE58LocalEnvironment warnings, zero unexpected warnings. The summary's trace SHA-256 is `5bbd58ab9f8bf420543d379e427673e00707b2306ae4b585eba4815c14a6ae0a`.

This establishes the H6 functional gate, not release readiness. The parent subsequently found global AppData Zen install/version writes and shader/XGE temp paths in that live log. Those paths are outside the user-approved Trace*.uba exception. UE execution is paused while the parent fixes process-local temp handling and investigates supported Zen/DDC/cook configuration. Do not relabel this diagnostic functional run as boundary-clean certification.

H1..H6 functional checklist entries in task-008 are now checked; H7/H8/H9 remain open. The concrete additive H7 plan is appended to task-008 for parent review. No H7 native source changed, and the primary alone may checkpoint H6 and later authorize H7 RED.

## Read-only Zen/DDC/cook containment diagnosis

No Engine, project configuration, Scripts, registry, environment or processes were changed by this sub-agent. The source paths below are under the read-only UE 5.8 `Engine/Source` tree. This is a supported configuration proposal for the parent's implementation/probe, not an executed containment pass.

1. **Avoid constructing Zen, rather than merely suppressing launch.** `Developer/Zen/Private/ZenServerInterface.cpp:1753` implements `-NoZenAutoLaunch` by switching to ConnectExisting, but `Initialize` at 2815 still calls MakeDirectory(ApplicationSettingsDir) at 2831, and its first-time block at 2836 uses config AutoLaunch (not the CLI override), locks ZenServerInstall and can call UninstallSystemService. ConditionalUpdateLocalInstall at 2856 and security updates at 2877 are skipped for ConnectExisting, but the earlier side effects are not. Thus `-NoZenAutoLaunch` is not a filesystem-safe no-Zen guard and is not recommended here.
2. **Use a project named graph containing only read-only pak nodes and one explicit local filesystem store.** `Developer/DerivedDataCache/Private/DerivedDataConfig.cpp:378` resolves `[DerivedDataCacheGraphs]` via GEngineIni; stores resolve via `[DerivedDataCacheStores]`. Project DefaultEngine.ini participates in that normal hierarchy. `Runtime/Core/Private/Misc/ConfigCacheIni.cpp:218` expands `%GAMEDIR%` to FPaths::ProjectDir. Existing ProjectPak/InstalledProjectPak/InstalledEnginePak are Type=ReadPak (`Engine/Config/BaseEngine.ini:2874..2877`). The built-in InstalledNoZenLocalFallback still includes ZenShared/Shared/Cloud and so is less tightly bounded than a custom graph.
3. **Do not inherit InstalledLocal's late user override.** `FileSystemCacheStore.cpp:3022..3055` applies environment, then registry, then command-line LocalDataCachePath, then EditorOverrideSetting. Therefore a CLI path does not defeat a configured editor override. Use a new store with no Base, EnvPathOverride or EditorOverrideSetting:

```ini
[DerivedDataCacheGraphs]
SeedForgeLocal=(ProjectPak,InstalledProjectPak,EnginePak=InstalledEnginePak,Local=SeedForgeLocalStore)

[DerivedDataCacheStores]
SeedForgeLocalStore=(Type=FileSystem,Path="%GAMEDIR%.cache/DerivedDataCache",CommandLineOverride=LocalDataCachePath,UnusedFileAge=34,PromptIfMissing=true)
```

4. **Pass explicit graph/no-default/path to each relevant native child.** `DerivedDataBackends.cpp:698` reads `-DDC=`; line 711 honors `-DDC-NoDefaultGraph`. The writable path should be a validated absolute project-local `-LocalDataCachePath=...`. A nonempty `DDC.Graph` CVar remains an additional candidate at line 705; its source default is empty (48..52), and no project override was found during this check. Parent must verify resolved graph/path and reject fallback/Zen logs in its first contained probe, not assume flags alone prove all writes.
5. **Disable Zen cook output separately.** `Editor/UnrealEd/Private/Commandlets/CookCommandlet.cpp:425..427` gives `-SkipZenStore` precedence over both `-ZenStore` and packaging settings. `CookOnTheFlyServer.cpp:10920..10940` then chooses FLooseCookedPackageWriter instead of FZenStoreWriter/FZenCookArtifactReader. This is a normal supported cook path, not a substitute cook. Do not pass UAT `-zenstore`; do not treat pre-existing Zen project-store markers as fresh loose-cook evidence.
6. **UAT forwarding is explicit.** `Programs/AutomationTool/Scripts/CookCommand.Automation.cs:103..107` appends AdditionalCookerOptions verbatim after outer quote trimming; include the graph/no-default/path and `-SkipZenStore` together in that single quoted UAT argument, preserving culture=en. UAT `-ddc=` also becomes the cooker graph at lines 47..50. For packaging child tools, `CopyBuildToStagingDirectory.Automation.cs:4335/4699` forwards AdditionalPakOptions and line 4905 forwards AdditionalIoStoreOptions. Those children may use compression/virtualization DDC; cooker-only flags are not a universal child-process setting. Parent's shared argument provider should cover them as applicable.

The old live log directly demonstrated why only project-local `UE-LocalDataCachePath` was insufficient: Zen used a repo-local data directory but still wrote its plugin-version file and used install/security paths under global AppData. Root owns process-scoped TEMP/TMP remediation and the final no-outside-write probe. No further UE execution is authorized by this diagnosis.

## H7 corrected RED and GREEN-source handoff

The first focused attempt (`184620`) exited 3 before an index because test case 19 called `TArray::Add` with a reference to its own element. This is a fixture error, not valid RED. The primary retained its log and changed the case to copy a local FIntPoint before Add. That exact correction is preserved in this sub-agent's GREEN diff.

The corrected strict build `184843` passed with zero errors/warnings. Actual `Artifacts/Reports/automation-20260904-185054-a42335c52d284a8094a5e63b96ea1441/index.json` was read: four succeeded, zero warning-success, three failed, zero not-run. Failures are exactly PassiveObserverUsesRealWorldAndCleansUp (8), PathEvidenceAcceptsCompleteSynthetic (2), and PathEvidenceSchema (9). Watchdog lifecycle, rejection and old schema/failure controls pass. The real observer's independent controls report 120 ticks, 118 timer firings, frames 298..418 and 502.666685 units. Storage checks passed according to the primary; no assertion was weakened to classify a fixture fault as RED.

Real `Artifacts/Reports/Gameplay/20260904-184729-9b8302d66c5e4612af6b68c285792779/gameplay-smoke.json` was read: Failed/PathObservationUnavailable with Generating, Playing, Failed; zero captures/screenshots. The primary observed native exit 2, zero PNGs and owned-DDC proof, not an outer timeout. This establishes the first-capture fail-closed boundary.

After explicit H7-G1 authorization, implementation is source-prepared:

- A noncopyable observer owns exactly three handles (real path-applied, queued-run, World-post), weak Coordinator/target/World identities, copied bounded actual canonical cells, and the existing H5 proof. It selects the real owned lowest-stable-ID enemy without setup/movement calls. Normal same-run path/World callbacks alone accumulate evidence; strictly newer retained frames are required. Completion detaches all handles but keeps frozen first/latest evidence; cancellation resets proof/identity. No per-frame trace array, new timer or AI/input/attack change is added.
- Coordinator's AwaitPathProof retains the original run/request guard and now fails PathObservationInvalidated when an incomplete observer loses its required three handles, rather than waiting 30 seconds after target invalidation. Completed proof with zero handles remains valid. A private const predicate is exercised through the existing test-only friend: destroy the real target while its run stays Playing, advance one actual-frame World tick, then require invalidation/zero handles. No production public getter or state/ID assignment is introduced.
- The codec appends bounded `walkableCells`, H6-shaped `pathEvidence`, and `remainingPathDelegateBindings`. It recomputes native A* with the existing budget, validates exact stored waypoints/identities, frame ordering before first capture, F receipt correlation, finite segment/speed/plane bounds, aggregates, and zero passive bindings. Invalid asserted success serializes Failed/MissingPathEvidence; nonfinite fields in failed evidence become JSON null.
- Count==2 enforces contiguous samples, index advance at most one and the 4.1-unit arrival rule. Count>2 retains nonadjacent first/latest behavior but validates the hidden gap: totalDistance+0.1 >= retainedDistanceSum+Gap; HiddenDt >= -1e-6; Gap <=260*max(0,HiddenDt)+0.1*(count-2)+0.1. The primary approved those exact tolerances and aligned both PS helpers after observed external RED. Native tests now include the reported first5/hidden145/last10 falsely-small aggregate, an insufficient-hidden-time case, and a valid first5/hidden8/last10 positive. These added native cases await the primary's next run; no extra native RED outcome is invented.

The successful-RED fixture correction and every existing assertion are retained. F capture components, H6 native input and all PS scripts are untouched by this implementation. The main agent's concurrent commits advanced HEAD through `23f07b4` and `cbd7ac3412fb1500d62dff9426eaed1bf30f4ec9`; fresh diagnostic runs must use the current verification context, not the older RED identity.

Static scoped `git diff --check` is clean. Stop for the primary's strict build, all seven GameplaySmoke tests, full Automation, and fresh Editor smoke with independent path/PNG/storage checks. No H7 GREEN/runtime/package or release completion is claimed yet.

## Historical H7-R1/R2/R3 authorized RED-source handoff

The primary reports H6 committed at `058615673ab593611d054f7b7ebd337363bb091d`, storage containment proven by later ordinary-input/full-114/gameplay runs, and the first actual clean BuildPlugin at `f235e46e9f637d706b9997e8af542873591c86f5` passed all three targets, strict logs/binary proof and a 58-file index. That is reported parent evidence, not a UE run by this sub-agent. Starting HEAD was read directly as the full f235e46 revision; the primary then explicitly authorized only H7 RED scaffolds.

Changed native/test files:

- GameplaySmoke.h: bounded trace additions, noncopyable observer and ValidatePathEvidence declarations.
- GameplaySmoke.cpp: Start/ValidatePathEvidence return false; Cancel resets empty proof, zero owned bindings. No path observation producer or new JSON field implementation exists at this stage.
- Coordinator.h/.cpp: AwaitPathProof and observer ownership; false Start produces PathObservationUnavailable before any capture request; final success additionally requires path validation; cancellation is wired to trace/run cleanup/failure/completion/EndPlay. No F capture component, H6 input or gameplay algorithm changed.
- GameplaySmokeTests.cpp: actual pure-generated model/A* with explicitly synthetic moves and three receipt-shaped values; no runtime evidence file is written. Existing schema/hash/count/order and failure-escaping assertions remain; max uint64 seed encoding is now checked as failed metadata rather than inconsistent successful evidence.

New tests are PathEvidenceAcceptsCompleteSynthetic, PathEvidenceRejectsTampering (21 corruptions), PathEvidenceSchema, PassiveObserverUsesRealWorldAndCleansUp, and WatchdogPreservesApplyDeadlineAndClearsLifecycle. Total namespace: seven tests. Expected source-level split is four controls passing and three intended failures, with 2 acceptance + 9 schema + 8 observer assertion failures; actual counts await the primary. Real observer test independently requires timer advancement, distinct engine frames, finite real enemy movement and the two-second speed bound while the empty observer fails its own proof assertions. It never calls Enemy::Tick or ReplanEnemies directly.

The primary explicitly approved a test-only friend seam under WITH_DEV_AUTOMATION_TESTS, defined only in the existing test cpp. The watchdog unit arms an actual World timer in the existing private handle, advances two World ticks on distinct engine frames, and asserts the same remaining time/elapsed/deadline across preserving cleanup. Default cleanup, StartRun restart, actual failure and destruction/EndPlay must clear it. The one expected production failure log is scoped using the existing Automation expected-error mechanism. This is lifecycle unit proof only, not a live 30-second deadline result.

The accompanying approved lifecycle correction preserves the StartRun watchdog only through same-run smoke apply: private ClearRunObjects now defaults to clearing it but accepts a preserve flag at that one call. StartGameplaySmoke no longer re-arms the timer. Restart/failure/EndPlay keep the default clearing behavior; no timeout grows and F capture deadlines stay intact.

Static checks: `git diff --check` exits 0; scoped diff inspection shows only the five allowed native/test files plus task/evidence. Concurrent PackageGameplay.ps1 changes belong to the parent/other subtask and were not edited here. No UE, global settings, filesystem fixture creation, staging, commit or branch operation occurred.

Stop for parent strict build and `SeedForge.GameplaySmoke` focused run, then real gameplay smoke. The expected real RED is native PathObservationUnavailable/exit 2, no screenshots/receipts and no outer timeout. Preserve full output and verify physical/watchdog controls rather than treating compile/fixture failures as RED. Only after observed RED may H7 GREEN be authorized.
# H6 checkpoint summary (not H7 / package closure)

Primary strict GREEN after Dash tightening: `build-editor-20260904-174458-3755e487bb634e8b891004fb5480308b.log`; InputSelfTest `automation-20260904-174505-d32fba0ed4c84161b18c8b37ad293aaa` 11/11; full `automation-20260904-174517-6f2d45e50e314b51858e5aaa5ac74099` 114/114 with zero whole-log errors/warnings. Independent native/PS/lifecycle review found no blocking issue.

Real ordinary input processes `InputSelfTest/20260904-174538-5706158a156a42a78a5f65cfd5508e33` and `174909-3f51b010f7924ee78c86f9b372b2914b` both passed native exit 0, external JSON relations and strict positive log checks: 13 effects, four fresh runs, ten transitions, four queues, real path movement, zero owned handles/keys. Parent independently ran the external validator's 157-case synthetic suite at `InputSelfTestValidation/20260904-094201-2af9619948424b389e94d5214c51cfb6`; 157/157. These are explicitly diagnostic source runs.

Those earlier runs exposed a separate Zen/temp output-boundary issue and are not final compliant release evidence. After project-local storage correction, `InputSelfTest/20260904-180436-6c3a564786834aad832f67afe8c6dfde` again passed, the five known global Zen metadata files remained unchanged, and shader/XGE temporary paths were project-local. Full `automation-20260904-180722-43b1cf47b03e4e54a47d1c858b0481e7` passed 114/114 with the confined filesystem DDC. See the separate runtime-output-boundary packet. H7 path-in-smoke, clean-revision build/package and all final gates remain open.

## Primary H7 GREEN verification (19:08-19:14 UTC+8)

All following runs identify `diagnostic-cbd7ac3412fb1500d62dff9426eaed1bf30f4ec9`, not a clean release candidate. Source was frozen during native execution. Independent read-only native review found no blocking P2 in observer ownership/cleanup, same-run timer budget, stage gating, native A*/sample/aggregate validation or serialization. Original F/H6 production input/capture/actors files remain unchanged by H7.

- Strict Build: `build-editor-20260904-190812-c79e275f4e864154a5f9c8710dea2776`, native/wrapper exit 0 and no log warning/error.
- Focused GameplaySmoke: `automation-20260904-190920-c956af58619f438f91b93bdc45c0e8ae`, 7/7, no warnings/errors/not-run/in-process. Real observer control: 9 ticks, 8 timer firings, 21.666667 units. Original-deadline/default/restart/failure/EndPlay and destroyed-target invalidation assertions pass.
- Full Automation: `automation-20260904-191006-a5020ffb51174f3f9c952add67ae2545`, **119/119**, zero test and whole-log warnings/errors, notRun/inProcess zero. Observed filesystem DDC/temp proof passes.
- Real gameplay: `Gameplay/20260904-191146-b009b430f8944fe0ba110d9990626086`, native/wrapper exit 0, unchanged default goldens, correct state/actions and three original rendered receipts/PNGs. Path stable ID 0, run/request/revision 1/1/1, 129 cells, 8 actual moves, total 20.99721074104309 units; retained movement frames 2 and 9 precede first capture frame 37 (combat 77, win 361). Path and all capture bindings zero. External path, PNG, full-log and storage validators pass; 25 exact environment warnings, no unexpected warnings/errors.
- The primary viewed **all three original 1280x720 images** from that exact gameplay run. Start/combat/win HUD text is complete and readable, objects are sharp, and win shows 3/3/unlocked/extracted. No image edits or crop repair was performed. These are diagnostic checkpoint visuals, not final packaged images.
- Ordinary input: `InputSelfTest/20260904-191243-b48d2d44525140f789653d37fa0d2bb4`, native/wrapper exit 0, four runs/13 input effects/10 transitions/four queues, actual path 25 moves/20.0063245296481 units, external tightened validation, no extra actors/keys/bindings, strict logs/storage pass. This process did not enable gameplay smoke.
- Four negative runtimes: `RunFailure/20260904-191331-912-Grid`, `191337-252-Encounter`, `191342-442-CapturePath`, `191349-423-RenderUnavailable`. Each exits 2 itself, matches the exact failed trace/log policy and has no outer timeout; storage proof passes. No error allowance was added for H7.

External serialized-proof reviews are recorded in `gameplay-path-validation-evidence.md` and `root-cause-compressed-path-proof.md`: H7 78/78 and H6 162/162 on both PS versions after observed arrival/gap REDs; final gap correction review passes. Unchanged PNG/capture/log/negative-log harnesses freshly pass 28/28, 84/84, 57/57 and 36/36 (`190611`, `190605`, `190613`, `190614`).

H local implementation is ready for a separate checkpoint. Clean-source BuildCookRun, ordinary packaged input, packaged H7 path/captures/negatives, final all-16 gates, documentation/visual replacement and publication remain open. No local Editor success is substituted for packaged evidence.

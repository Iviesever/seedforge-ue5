# Phase 3 code walkthrough

## Reading map

Runtime files below are under `Plugins/SeedForge/Source/SeedForgeRuntime`; test files are under `Plugins/SeedForge/Source/SeedForgeTests/Private`.

| Read | What to trace |
|---|---|
| SeedForgeTypes / Generator / Validator | Preserved seed/config, canonical layout and SFG1 identity |
| SeedForgeEncounter | Ranked spawn selection, stable IDs, wide safety distance, independent SFE1 hash |
| SeedForgeGridPathfinder | Checked neighbors, int64 costs, deterministic open ordering and budget semantics |
| SeedForgeRunState / GameplayTypes | Failed versus Lost, Core eligibility, copied snapshots, Dash/attack math |
| SeedForgeAsync / WorldSubsystem | Cancellation, newest request and weak UObject/Game Thread application |
| SeedForgeGameplayCoordinator | Actual request attribution, run ownership, cleanup and smoke gate |
| SeedForgeGameplayActors / PreviewActor | Persistent controller, Character/Enemy movement, AHUD-owned Slate, HISM collision |
| SeedForgeCaptureTypes / GameplayCapture | Pure capture lifecycle and real viewport/render/pixel/processed adapter |
| SeedForgeGameplayDiagnostics / GameplaySmoke | Bounded passive path proof, observer lifetime and additive smoke codec |
| SeedForgeInputSelfTest | Opt-in ordinary input driver, exact-once cleanup and complete evidence validation |

Then read `SeedForgeDemoGameMode.cpp`, `Config/DefaultInput.ini`, the focused audit tests, and the wrappers `TestInputSelfTest.ps1`, `TestGameplay.ps1`, `PackageGameplay.ps1`, `VerifyPhase3.ps1` and `FinalizeRelease.ps1`.

## Pure placement and A*

Encounter validation fails before selection for invalid config/layout identity, endpoint membership or insufficient cells. Candidate sorting, not TSet iteration, fixes output order. Both generation and validation widen coordinate subtraction before Manhattan safety comparisons. Hashing includes versioned model inputs; do not confuse a new encounter hash with a changed generator hash.

In A*, inspect `FSearchNode`, `ManhattanDistance` and `IsPreferred`. Costs/heuristics/F scores are int64; coordinates remain int32. Each neighbor sum is evaluated in int64 and range-checked before constructing FIntPoint. Direction order is `(+1,0), (0,+1), (-1,0), (0,-1)`; the comparator is `(F,H,Y,X)`.

The loop removes the preferred open node, checks for goal, then checks the expansion budget. Only a non-goal expansion increments ExpandedNodes. Thus budget 1 can discover and select an adjacent goal successfully; `AlreadyAtGoal` uses zero expansions after input validation. BudgetExceeded/Unreachable return no partial successful path. Array Reserve is an allocation hint, not a cap on discovered nodes.

## State and actual completion identity

Follow these Coordinator boundaries:

1. StartRun increments RunGeneration, clears applied ID/old hashes/failure detail, requests legal restart transitions where needed, cancels work and clears run objects.
2. It issues a WorldSubsystem request and records the returned pending request ID. A request during Generating supersedes the old one in that state.
3. HandleGenerationApplied ignores zero/non-active completions. A matching failure enters typed Failed; a matching success passes `Completion.RequestId` to the private apply path.
4. Apply validates and creates the real run, then records AppliedRequestId and logs `Applied request=... run=... gameplay=true.`
5. The public value-apply overload cancels pending work and uses request 0. World fixtures can exercise this or fixture-delivered completions; neither alone proves a real packaged worker completion.

Read EnterRunFailure and ClearRunObjects together. Failure cancels generation/delegates, destroys owned run actors, clears layout/encounter/HP/cooldowns, calls FailRun and exposes a typed snapshot. EndPlay uses the same cleanup boundary. Same-run smoke apply alone preserves the original watchdog; default cleanup clears it. That preservation is not a second 30-second timer.

## Controller, aim and Dash

R/N live in `ASeedForgePlayerController::SetupInputComponent`, which removes duplicate named restart bindings before binding once. The controller resolves one Coordinator even when no pawn is possessed. Character bindings are only movement, Attack and Dash.

Dash reads current InputComponent axis sums at action time; axis delegates have not yet updated cached movement fields. Inspect ResolveDashDirection for finite checks, clamp/normalization and movement→aim→+X fallback. PawnClientRestart, UnPossessed and FlushPressedKeys clear cached intent.

PlayerTick deprojects the viewport cursor to the Character-height plane. SetAimWorldPoint changes aim/rotation only for a valid normalized XY direction. Failed deprojection leaves last aim intact; it does not follow movement automatically.

## Combat, HUD and actor boundaries

SelectAttackTarget filters range/arc and chooses the lowest stable ID. TryPlayerAttack consumes a valid attack cooldown even on a miss, owns enemy HP and destroys a killed actor. ApplyPlayerDamage owns player HP and enters Lost through the state machine. TickInteractions validates a Core transition before destruction and derives exit presentation from state-machine eligibility.

ReplanEnemies calls A* from the timer and publishes an observation after SetPath. Enemy Tick records the exact revision, consumed waypoint, from/target/to, delta, frame and sequence after its normal movement. The observation must not change speed, arrival or path decisions.

AHUD's DrawHUD now creates/updates native Slate text. PostRender visibility checks and RemoveOverlay/EndPlay own removal. Read the tagged SBox and its padding/wrapping; do not describe this as Canvas text. HUD reads a snapshot rather than mutating gameplay.

## Capture lifecycle: more than a PNG header

The pure lifecycle is `AwaitRenderedFrame -> AwaitPixels -> AwaitProcessed -> Complete`. Tokens/run/path/dimensions/frame order must match. The adapter:

- binds the owning viewport's rendered callback and waits for world/render primitive readiness;
- requests a screenshot including UI only after that boundary;
- verifies callback pixels and saves them;
- requires processed completion and sufficient saved bytes;
- cancels owned callbacks/watchdog before publishing the receipt.

Cancel/timeout/EndPlay never steal an unrelated screenshot request. The single-capture mode keeps a requested fixed filename; the three gameplay filenames include their unique token. PowerShell independently verifies receipt order, exact current-run PNG cardinality, fresh timestamps, containment/reparse rules, dimensions, integrity and full decode. A valid header, delayed sleep or zero native exit is insufficient.

## Ordinary input proof versus passive gameplay proof

H6's controller-owned component is dormant unless explicitly enabled. BeforeInput queues simulated keys through the viewport; normal UE dispatch handles mappings. Pointer intent is repeatedly fed through guarded scene OnMouseMove so ordinary Slate cursor refresh cannot erase a one-shot aim. There is no OS cursor API or direct aim fallback when a real viewport is unavailable.

AfterInput latches the actual launch/cooldown at injection time. World-post observations measure physical displacement. The driver records 13 effects/four runs/ten transitions/four queued requests, including Lost/same/new/rapid restart. Setup poses and public damage are disclosed separately. Cleanup releases keys and observers before one completion.

H7's noncopyable passive observer owns path/queued-run/World handles. It selects a real owned enemy, consumes the existing H5 proof and freezes after two or more observations/20 units. It detaches before the first capture. A destroyed target cancels observation; AwaitPathProof treats lost bindings on incomplete proof as PathObservationInvalidated, not success.

ValidatePathEvidence recomputes native A* and compares route/waypoints, exact identities and frames before first capture. Two retained samples with count 2 must be contiguous and respect waypoint arrival. With count >2, the straight gap between retained samples must fit total distance and hidden-time speed bounds; missing samples are not fabricated. External PS checks these serialized relationships but does not claim to recompute the generator/A*.

## Tests and verification reading

Useful focused files (with the SeedForge prefix) include CoordinateSafetyTests, RunIdentityTests, RestartInputTests, DashInputTests, GameplayTests (including HUD lifecycle), CaptureLifecycleTests, CaptureComponentTests, GameplayPathIntegrationTests, RunIntegrationTests, InputSelfTestTests and GameplaySmokeTests. Locate exact registered filter names before invoking a focused test. Latent World fixtures advance once per real GFrameCounter; same-frame loops do not prove TimerManager progression.

The latest full result is diagnostic 119/119, not a final clean candidate result. Clean a9f5625 separately passed the actual packaged sequence. The 194106 outer package manifest freezes earlier child summaries/logs/trace/PNG digests so a later child cannot silently rewrite their baseline. FinalizeRelease requires exact-candidate machine, visual and remote review evidence; static docs and last-pointer files alone are not authority.

## Debugger exercises

- Inspect MAX/MIN int32 neighbor rejection without changing tie-breaking.
- Select an adjacent goal at budget 1 and observe that the goal itself is not charged.
- During R/N, compare RunGeneration, pending request and actual applied request; do not assume equal counters.
- Break on Failed cleanup, then recover via the controller while unpossessed.
- Step through same-frame W+D+Space and latch PendingLaunchVelocity before movement consumes it.
- Break at render, pixels and processed callbacks; inspect token/frame ownership and cleanup.
- Observe H7 detaching its three handles before the start capture request.

For implementation drills, use [LIVE_CHANGE_DRILLS.md](LIVE_CHANGE_DRILLS.md); do not modify a frozen verification candidate just to demonstrate a debugger exercise.

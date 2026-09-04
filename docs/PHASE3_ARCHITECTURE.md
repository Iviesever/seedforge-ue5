# Phase 3 architecture

## Purpose and dependency direction

Phase 3 adds a short playable extraction run to SeedForge's deterministic layout model. Layout, encounter, gameplay ownership and verification evidence remain separate responsibilities.

```text
Seed + FSeedForgeConfig -> pure generator -> FSeedForgeLayout / SFG1 identity
                                 |
                    worker + newest-request gate
                                 |
                    USeedForgeWorldSubsystem
                                 |
                    GameplayCoordinator (Game Thread)
                       |            |             |
                 encounter plan   run state    owned run actors
                    / SFE1        / failure     / generated collision
                       |                          |
                       +---- bounded grid A* -----+
                                                  |
                              normal input, HUD and capture observations
```

Reusable models and actors live in the Runtime plugin module. SeedForgeDemo supplies the native GameMode/bootstrap. Editor tools consume Runtime APIs; they are not another gameplay authority. Tests are Editor-only, while capture and opt-in runtime verification are usable in the ordinary packaged application.

## Pure models and preserved identities

The encounter planner consumes an intact layout and explicit encounter config. It ranks canonical walkable cells using a SplitMix64-derived score from seed, source layout hash, role salt and coordinates; descending score then canonical Y/X order determines selection. Cores are selected first, enemies skip occupied cells and the player safety radius, and stable IDs are contiguous within each role. Impossible inputs return typed failures.

Layout identity retains generator version 1, schema v1 and the SFG1 hash contract. Encounter identity uses its separate SFE1 marker/version and includes source layout identity, config, roles, IDs and ordered cells. The default seed 24301 remains:

| Identity | Value |
|---|---|
| Layout hash | `7425849530159566348` |
| Encounter hash | `15303214708604970503` |

Encounter safety-distance subtraction is widened before arithmetic. Sparse extreme-coordinate tests exercise public coordinate math; they do not establish generated-layout connectivity.

## Bounded, coordinate-safe A*

`FSeedForgeGridPathfinder` accepts start, goal, walkable cells and a positive expansion budget. Its contract is four-neighbor unit-cost movement, East/South/West/North discovery order, and minimum `(F, H, Y, X)` open-node selection. Equal-cost parents retain first discovery; hash-container iteration does not choose output.

Coordinates remain int32, but neighbor addition is checked in int64 before narrowing. An out-of-domain neighbor is skipped, never wrapped from MAX to MIN. Manhattan subtraction, heuristic, path cost and F score use int64. These changes preserve ordinary ordering and goldens.

Budget semantics matter: the selected goal is checked before charging another expansion. Only non-goal nodes whose neighbors are explored increment `ExpandedNodes`. An adjacent goal therefore succeeds with budget 1 and one expansion; an already-valid start equal to goal returns `AlreadyAtGoal`, one cell and zero expansions. Invalid input, exhaustion and unreachable space are distinct results; failures do not return a partial success path. The budget limits expansions, not all allocations or wall-clock time.

The open array uses a comparator scan: worst-case selection is O(V²), storage O(V). The production replan budget is 1024. Replanning occurs every 0.5 seconds after the first 0.05-second callback; enemy Tick follows existing waypoints and never searches.

## Run state, request identity and cleanup

```text
Generating -> Playing -> Won or Lost
active/terminal run -> Restarting -> Generating
operational failure -> Failed -> Restarting -> Generating
Generating + another R/N request -> a newer request, still Generating
```

The state machine owns Core IDs, exit eligibility and legal transitions. `Failed` is not player defeat: it records generation, validation, encounter, spawn or smoke/capture failure. `FailRun` clears collection eligibility; the Coordinator records the typed cause and message.

Three identities must not be conflated:

| Value | Meaning |
|---|---|
| `RunGeneration` | Coordinator ownership generation, advanced by StartRun |
| pending request ID | Current WorldSubsystem generation request |
| `AppliedRequestId` | Actual completion request that produced this applied run |

StartRun clears applied identity and old hashes before publishing pending/restart observations. A matching completion supplies its real request ID through the private apply/log path. The public value-apply API cancels outstanding generation and explicitly attributes the result to request 0; it is not worker-completion evidence. Authoritative runtime traces require positive actual applied identity.

Restart/failure cleanup cancels work and capture/path observers, removes timers, unpossesses and destroys the old Character, destroys visualization/Core/enemy/exit actors, and clears references/HP/path data as appropriate. Failure clears pending/applied IDs and hashes. Normal interactive Failed can recover through R/N because the controller survives without a pawn. EndPlay removes generation delegates and cancels remaining work.

The smoke's 30-second watchdog is preserved only across same-run apply cleanup; it is not restarted when path observation or rendering begins. Restart, failure and EndPlay clear the old watchdog.

## Framework ownership and input

| Owner | Responsibility |
|---|---|
| Persistent PlayerController | R/N bindings, possession, cursor deprojection, optional input-self-test component |
| PlayerCharacter | CharacterMovement, movement/Attack/Dash bindings, aim/visual pulse and Dash cooldown |
| GameplayCoordinator | Run state, combat HP/cooldowns, model application, run actor references, timers and smoke/capture ownership |
| EnemyPawn | Stable ID/spawn cell, current waypoints and actual movement observations |
| ASeedForgeHUD | One native Slate overlay, updated from a copied Coordinator snapshot |
| Pure models | Layout/encounter identity, A* and transition rules |

Input is named Action/Axis mappings in DefaultInput.ini on Enhanced-compatible PlayerInput/InputComponent classes, not Input Action or Mapping Context assets. R/N bindings are deduplicated on the persistent controller. They work through terminal states and while a new pawn is absent.

Dash samples InputComponent's current summed MoveForward/MoveRight values at action dispatch, because axis callbacks occur later. Same-frame press/release plus Space uses live input, not stale callback caches. A nonzero movement vector is normalized; otherwise Dash uses valid aim, then +X. Possession changes and FlushPressedKeys reset cached movement intent.

Mouse deprojection updates aim on the Character's plane. Invalid deprojection leaves the last valid aim unchanged; initial aim is +X. Movement does not independently rewrite aim.

## HUD and rendered capture

The HUD text is native Slate owned by AHUD, not Canvas-drawn text or a UMG asset. DrawHUD updates a hit-test-invisible SBox/STextBlock overlay; hide/debug/no-Canvas, viewport changes and EndPlay remove it. The HUD displays state/snapshot data and does not own HP, Core counts or cooldowns.

`USeedForgeGameplayCaptureComponent` owns a fresh path/token/run request and an eight-second callback watchdog. It waits for the owning viewport's rendered callback, enabled world rendering, standalone startup visibility and ready render primitives. It then owns screenshot pixels, saves those pixels, waits for processed completion, removes its bindings and publishes a receipt. A startup DrawHUD call or an existing PNG is not capture completion.

Receipts retain requested/rendered/captured/completed frames, UTC timestamps, token, path, run/request IDs, dimensions, saved bytes and remaining bindings. Gameplay's three ordered paths include distinct tokens. External checks enforce fresh direct-child PNGs, exact cardinality, timestamps, 1280×720 dimensions, size, signature/chunk/integrity checks and full decode. Pixels/exposure remain nondeterministic; the inspected packaged start image includes initial exposure settling and is not brightened or retouched.

## Two complementary live proofs

**Ordinary input (H6):** `-SeedForgeInputSelfTest`, mutually exclusive with gameplay smoke/capture-auto-exit, uses the normal GameMode/controller. It queues simulated UE keys in PreProcessInput, feeds guarded real scene-viewport pointer intent, observes normal PostProcessInput and World ticks, and releases input/delegates before one completion. It does not use OS macros, direct private handlers or manual input-stack pumping. Canonical setup teleports and public damage to cause Lost are separately recorded, not counted as input motion.

Its 13 effects cover two aims, two attacks, WASD, diagonal Dash, same-seed restart, new seed and rapid N/R supersession. Four run snapshots, ten transitions and four queued requests check actual identities, reset state, fresh actor ownership and one persistent HUD/controller. Missing viewport or incomplete evidence fails; native source syntax never proves a clean build.

**Gameplay smoke plus passive path (H7):** normal replanner/Enemy ticks must first produce at least two real movement observations and 20 units of same-run A* motion. The observer retains at most two first/latest samples, freezes aggregates and removes its three delegates before the first capture request. Native validation recomputes A*, checks consumed-waypoint/arrival and hidden-gap distance/time bounds, and links movement frames to the first capture receipt. The smoke then uses its disclosed production attack/proximity/teleport shortcuts to reach Won and capture start/combat/win. It never assigns HP, collected counts or Won directly.

## Evidence authority and current checkpoint

Deterministic model output does not imply deterministic timers, physics, request numbers, frame counters, trace bytes or rendered pixels. Canonical JSON field order and uint64 strings preserve representation, not repeatable whole-run evidence.

The latest observed full suite is **119/119 at diagnostic-cbd7ac3**, with zero test/whole-log warnings/errors. A distinct **clean a9f56254f11554316302936926211e75d86d7f4d** PackageGameplay checkpoint passed build/cook, ordinary packaged input, H7 captures and four typed negative processes. Its local outer manifest is `Artifacts/Package/gameplay-package-20260904-194106-88cf81446f8f4efd81b9a6aae3984815.json`; the associated UAT evidence index contains 49 files. These are not one final candidate.

Authoritative scripts require a clean pinned revision before/after processes and frozen artifact digests. Dirty development runs use explicit diagnostic identity and cannot certify release. Native input traces always say `sourceVerified=false`. VerifyPhase3 produces machine evidence; final candidate-bound visual and remote review are separate gates. The fresh all-16 final candidate matrix has not yet run at this documentation checkpoint.

Project output stays under the repository using scoped filesystem DDC/temp and supported loose-cook/EditorDomain attachment controls. Only UBT's internal Trace*.uba diagnostics have the user's narrow outside-root exception. Engine/global configuration is not edited. See [DEVELOPMENT.md](DEVELOPMENT.md) and [PHASE3_ACCEPTANCE_MATRIX.md](PHASE3_ACCEPTANCE_MATRIX.md) for current workflow/evidence navigation.

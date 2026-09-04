# Phase 3 interview guide

Use this as a source-reading and rehearsal guide, not proof that the user already understands or independently wrote the implementation. Pair each answer with the relevant code, an executable contract and a real artifact when claiming verification.

## Models and determinism

### What is deterministic, and what is not?

Seed plus explicit generator config produces canonical layout topology/hash; layout plus encounter config produces stable initial role IDs/cells and a separate encounter hash. A* gives the same path/status/expansion count for the same request and version. These contracts do not make input timing, timers, physics, request IDs, frame numbers, whole trace bytes or pixels deterministic.

Default seed 24301 retains layout hash `7425849530159566348` and encounter hash `15303214708604970503`. SFG1 layout identity and SFE1 encounter identity are separate. Changing a placement rule is not permission to relabel old layout evidence.

### Why keep encounter planning pure?

It makes all inputs explicit and removes time, World, UObject and global-random dependencies. Canonically sorted values select cells; sets only answer membership. Actors consume the result instead of becoming another spawn authority. Impossible counts or safety constraints return typed failure.

### What does coordinate-safe A* add?

Neighbor sums are computed in int64, checked against the int32 coordinate domain, then narrowed. Manhattan subtraction is widened before subtraction; G, H and F scores are int64. Without that distinction, MAX and MIN can become false neighbors or distant cells can look nearby. Extreme sparse-coordinate tests validate these public arithmetic boundaries, not generated map connectivity.

### Explain ordering, budget and complexity precisely.

Discovery order is East/South/West/North. Open selection minimizes F, H, Y, X; equal-cost parents keep first discovery. A linear open-array scan gives O(V²) worst-case selection and O(V) storage.

The budget counts expanded non-goal nodes. Goal selection is checked before the next budget charge, so an adjacent goal can succeed with budget 1. AlreadyAtGoal returns one cell and zero expansions after validating endpoints/budget. Exhaustion and unreachable space are different outcomes, with no partial successful path. A node budget is not a wall-clock or complete allocation bound.

## Ownership and failure

### What are RunGeneration and AppliedRequestId?

RunGeneration tracks Coordinator ownership cycles. The WorldSubsystem returns a separate pending generation request ID. AppliedRequestId records the actual successful completion request, passed through the private apply/log path; they need not equal.

Restart clears old hashes/applied ID before pending observations. Stale completions are rejected at the subsystem and Coordinator boundaries. Direct public value application cancels the outstanding generation and uses request 0—it is not evidence that a worker completed.

### How is Failed different from Lost?

Lost is a legal gameplay defeat after player health reaches zero. Failed means an operational boundary failed: generation, layout/encounter validation, spawning or opt-in smoke/capture verification. It preserves a typed failure message while clearing stale model/request/collection state and run actors. Normal interactive Failed can recover through R/N.

### Why put R/N on the controller?

The Character is replaced on restart and may be absent in Generating or after failure. The persistent PlayerController binds R/N once, resolves the Coordinator and survives unpossession. A new request while Generating supersedes the pending request without a fictitious intermediate Playing state.

### What actually gets cleaned up?

Coordinator cleanup cancels capture/path observation, clears interaction/replan/smoke timers, unpossesses/destroys the run Character and destroys visualization/Core/enemy/exit actors. Failure additionally clears identities/hashes/HP and cancels generation delegates. EndPlay removes remaining work. The same-run smoke apply preserves the already armed 30-second watchdog; a restart/failure/EndPlay clears it. There is no deadline reset to hide slow progress.

## Input and presentation

### Is this an Enhanced Input asset setup?

No. It uses named Action/Axis mappings in DefaultInput.ini on Enhanced-compatible classes. There are no Input Action/Mapping Context assets in this implementation. That distinction makes bindings inspectable without misrepresenting the API being used.

### Why did Dash need a live-axis fix?

UE has summed current axes before dispatching actions, but invokes axis callbacks afterward. Reading only cached callback fields during Space can use last frame's keys. Dash therefore samples InputComponent's current MoveForward/MoveRight sums. Same-frame W+D+Space launches diagonally; release/opposite keys can fall back to aim. Possession and key flush reset cached intent.

### Is the HUD Canvas or Slate?

AHUD owns a native Slate SBox/STextBlock overlay and updates it from the Coordinator snapshot. Its lifecycle still follows AHUD visibility/render callbacks, but the text is not Canvas-drawn. Hide/debug/no-Canvas, viewport changes and EndPlay remove the owned overlay. HUD does not own gameplay state; no UMG asset is required.

### Why not NavMesh, Behavior Tree, EQS or GAS?

This bounded portfolio slice exposes placement, A*, scheduling, ownership and package verification directly. Those are valid production frameworks, but introducing parallel navigation/state/asset systems would not improve this acceptance contract.

## Proofs and their limitations

### What does ordinary input self-test prove?

With `-SeedForgeInputSelfTest` and no gameplay-smoke flag, the real controller queues simulated UE keys in PreProcessInput and normal engine dispatch handles them. Guarded scene-viewport pointer events feed real deprojection; held pointer intent prevents Slate cursor refresh from erasing aim. Normal World ticks supply physical effects.

The trace covers 13 effects, four fresh runs, ten transitions and four queued requests, including Lost/same/new/rapid restart and persistent ownership. It checks real pending launch before movement consumes it and later requires actual Dash displacement. Setup teleports and public damage are disclosed separately. Missing viewport fails; there is no direct aim or private-handler substitute, OS macro or manual stack pump in the live runner.

### What does gameplay smoke prove that input alone does not?

Smoke is a disclosed shortcut through real attack/cooldown/proximity/Core/exit rules. Before its first capture, a passive observer must receive an actual A* application and at least two real enemy moves totaling 20 units. It then detaches its three handles. Native validation recomputes A*, checks consumed waypoints and matches run/request/frame evidence to capture receipts.

Only first/latest samples are retained. With two moves, they must be contiguous and respect the arrival rule. With more moves, the omitted straight-line gap must fit aggregate distance and hidden time. This checks consistency without inventing missing observations. External PS checks serialized relationships but does not claim independent generator/A* recomputation.

### Why is a PNG file or zero exit insufficient?

A stale, wrong-sized, corrupt or blank image can exist without proving the intended World rendered. Native capture owns viewport-render, pixels, save and processed completion with a fresh token/path/run and bounded callback watchdog; completion follows cleanup. External validation checks exact three-file cardinality, token/path correlation, timestamps, dimensions, PNG integrity/full decode and process identity. Original-resolution visual review remains necessary for actual content/HUD.

The inspected clean packaged triplet has readable complete HUD and geometry; initial exposure can still settle in the start image. It is not pixel-identical rendering or a production-polish claim.

### Why do Editor and packaged verification both matter?

Editor success can hide target eligibility, cooking/staging, config/assets and runtime dependency problems. Stock BuildPlugin target/binary proof and BuildCookRun are separate checks. PackageGameplay also runs ordinary launch/capture, ordinary input, H7 gameplay and four typed negative processes against the actual cooked executable. A correctly rejected negative run must fail for its expected cause, not merely time out externally.

### What is the current evidence authority?

The latest full suite is 119/119 at a diagnostic cbd7ac3 checkpoint, zero test/whole-log warnings/errors. A distinct clean a9f5625 PackageGameplay checkpoint passed its complete packaged sequence; its local 194106 manifest freezes nested child evidence. Neither is the final all-16 candidate run, which remains pending after the documentation/image update.

Native input source parsing accepts explicit diagnostic identity but always records `sourceVerified=false`. Authoritative wrappers require an unchanged clean pinned revision and recheck artifact digests. MachinePassed is not final readiness: exact-candidate original images and remote review must be supplied to FinalizeRelease. A previous SHA, last-pointer file or edited summary cannot replace fresh evidence.

### What was learned from the audit failures?

A fixture crash is not a valid RED assertion. Same-frame World loops did not exercise TimerManager progression. A one-shot pointer cache update did not persist aim. Sampling an attack's cooldown for many later frames erased its immediate result. Header-only PNG checks did not establish full image integrity. SeedForge's corrections use discriminating observations and fixed bounds, not assertion removal, arbitrary sleeps or timeout growth.

Storage is another independent boundary: project-local DDC data did not by itself prevent Zen installation metadata writes. The wrappers now select explicit filesystem DDC/temp and disable both Zen cooked storage and optional EditorDomain cook attachments without editing Engine/global configuration. Only the user's internal UBT Trace*.uba exception permits those diagnostics outside the repository.

## Authorship and personal study

### What did AI contribute?

GPT-5.6 Sol through Codex contributed implementation, tests, scripts, documentation, debugging, UE process execution, packaging and evidence/visual review work. The user set goals, constraints and acceptance priorities and orchestrated the work. This is an AI-assisted project, not an independently hand-written user implementation.

### What can the user demonstrate personally?

After study, explain a specific ownership boundary, reproduce a focused contract, interpret a real failed trace and complete a small personally authored test-first change. Describe that contribution precisely. Do not infer personal understanding from the existence of these guides or retroactively claim authorship of AI-produced code.

See [PHASE3_CODE_WALKTHROUGH.md](PHASE3_CODE_WALKTHROUGH.md) and [LIVE_CHANGE_DRILLS.md](LIVE_CHANGE_DRILLS.md). Publication is a separate authorized workflow after all final gates, not something this guide says has already happened. That authorized publication is source-only; a local Win64 ZIP does not imply an uploaded binary Release asset.

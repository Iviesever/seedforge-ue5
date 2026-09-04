# SeedForge 0.3.0 independent release audit — verification plan

## Purpose

This plan closes the confirmed findings in `audit_findings.md` without adding gameplay, changing the generator, or expanding the product boundary. It is intended to be executed in the existing Windows checkout and existing Phase 3 branch only.

Authoritative environment:

- existing local checkout: `D:\program\SeedForge` (or the actual existing SeedForge checkout if this path differs);
- existing branch: `feat/phase3-playable-vertical-slice`;
- Unreal Engine: the installed UE 5.8 binary tree, treated as read-only infrastructure;
- no second clone, no worktree, no parallel UE integration directory;
- at most one UBT, UAT, UnrealEditor, Cook, BuildPlugin, or Package process at a time.

Do not merge PR #1, close it, create/move `v0.3.0`, or publish a Release.

---

## 0. Mandatory preflight and state recovery

Run from the existing checkout before copying or editing any file:

```powershell
Set-Location 'D:\program\SeedForge'

# Recover remote facts rather than trusting this packet's historical SHA.
git status --short --branch
git remote -v
git fetch --prune origin

git rev-parse --verify HEAD
git rev-parse --verify origin/main
git rev-list --left-right --count origin/main...HEAD
git diff --check
git status --porcelain=v1 --untracked-files=all
```

Then re-read PR #1 through the GitHub connector/API and record:

- open/Draft/merged state;
- base branch and base SHA;
- head branch and head SHA;
- mergeability;
- changed files;
- reviews, review threads, comments, checks, and workflow runs.

Stop without changing files if:

- the checkout is on a different feature line;
- `HEAD` is not the live PR head;
- the worktree contains unknown user modifications;
- a UBT/UAT/Editor/Cook/package process is already using the checkout;
- the PR is merged/closed or the requested release boundary changed.

Copy this four-file task packet into the repository at:

```text
tasks/<fresh-timestamp>-phase3-independent-release-audit/
```

Use a fresh timestamp based on the local run. Preserve this packet as prior independent evidence; do not rewrite it to pretend its `not-run` rows were executed here.

Record the fresh state in the in-repository `progress.md` before production changes.

---

## 1. Repair sequence and commit discipline

Each confirmed issue follows this loop:

1. add a test or minimal reproduction that fails for the stated root cause;
2. run the focused test and retain the RED evidence;
3. implement the smallest correction;
4. run focused verification;
5. update the finding status and `progress.md`;
6. commit that root cause separately;
7. push only the existing PR branch.

Do not combine unrelated fixes. Do not weaken, skip, rename, or silence tests. If the same issue remains after two corrective attempts, stop patching it and create a root-cause packet containing the exact failing command, log, observed state, rejected hypotheses, and next discriminating experiment.

### Commit A — close startup/generation failure lifecycle (`SF-IRA-001`)

Add RED coverage first:

- matching generation failure cannot remain indefinitely `Generating`;
- layout-validation failure closes the run;
- encounter/spawn/start-state application failure rolls back all partially created objects;
- a stale failed completion cannot fail a newer run;
- smoke mode with valid trace arguments writes a failed JSON trace and requests a non-zero exit on pre-Playing failure;
- a failed interactive run remains recoverable through persistent restart/new-seed controls.

Minimal implementation boundary:

- introduce one coordinator-owned typed run-failure path;
- initialize smoke trace identity before the asynchronous request so pre-Playing failures are serializable;
- clear request, timers, delegates owned by the run, actors, paths, pending screenshots, HP/cooldown and smoke state exactly once;
- expose failure in the snapshot/HUD or an explicit state without presenting stale `Generating` as live work;
- never let stale completions alter the active run;
- preserve restart/new-seed recovery;
- do not add retries, Sleeps, or automatic gameplay changes.

Suggested commit subject:

```text
fix(gameplay): close failed run generation deterministically
```

### Commit B — move restart ownership to the persistent controller (`SF-IRA-003`)

Add RED coverage first:

- R/N dispatch while the controller has no possessed pawn;
- one key press causes exactly one request;
- a second R/N during `Generating` supersedes the active request;
- old completion cannot populate the new run;
- successful application possesses exactly one fresh player;
- failure/restart does not leave a controller pointing at a destroyed pawn.

Minimal implementation boundary:

- bind `RestartSameSeed` and `StartNewSeed` from `ASeedForgePlayerController::SetupInputComponent` or another persistent controller-owned input boundary;
- resolve the single live coordinator safely instead of storing an unsafe raw lifetime dependency;
- remove the duplicate pawn bindings so one key does not fire twice;
- leave move/attack/dash on the pawn;
- explicitly verify possession/unpossession during clear and reapply.

Suggested commit subject:

```text
fix(input): keep restart controls alive across generation
```

### Commit C — preserve current diagonal Dash intent (`SF-IRA-004`)

Add a pure helper or directly testable state calculation before changing Character behavior. RED cases:

- W+D resolves to a normalized diagonal;
- W+S cancels forward input;
- releasing one axis leaves the still-held axis active;
- releasing all movement falls back to current aim;
- zero/invalid aim falls back to a safe forward direction;
- focus-loss/input flush cannot retain a phantom held axis;
- Dash cooldown remains unchanged.

Minimal implementation boundary:

- retain current forward/right axis values rather than allowing each callback to overwrite a single vector;
- resolve Dash direction from the combined current input, then aim, then a safe fixed fallback;
- reset transient input state on the appropriate possession/input-reset boundary;
- do not migrate to Input Action assets as part of this fix.

Suggested commit subject:

```text
fix(input): derive dash from combined live movement axes
```

### Commit D — make A* and Encounter coordinate arithmetic safe (`SF-IRA-006`)

Add RED tests first:

- `INT_MAX` must not wrap east to `INT_MIN`;
- Manhattan distance uses a wide subtraction before absolute value;
- extreme spans either work safely or return typed `InvalidInput`;
- `MaxExpandedNodes=1` with adjacent Goal has the documented result;
- Goal discovered but not yet expanded obeys the chosen budget definition;
- duplicates and shuffled `WalkableCells` do not change path/status/expansion count;
- normal-domain stable path and all existing hashes remain unchanged.

Minimal implementation boundary:

- checked neighbor construction;
- `int64` distance/cost math or an explicit supported-span rejection;
- no generator/schema/hash-version change;
- document the exact definition of `ExpandedNodes` and the Goal/budget order.

Suggested commit subject:

```text
fix(path): prevent coordinate overflow in grid search
```

### Commit E — separate run and request identities (`SF-IRA-007`, `SF-IRA-009`)

RED coverage:

- make subsystem RequestId diverge from RunGeneration and assert both remain correct;
- during a pending new run, the snapshot cannot combine the new seed with old layout/encounter hashes;
- a failed restart retains no stale active identity.

Minimal implementation boundary:

- pass the true completion RequestId through the apply/log boundary;
- log explicit `run=` and `request=` fields;
- separate pending seed/request identity from applied layout/encounter identity, or reset active hashes while pending;
- update strict script parsing accordingly.

Suggested commit subject:

```text
fix(gameplay): separate pending run and request identity
```

### Commit F — own screenshot completion and validate the PNG (`SF-IRA-005`)

Before implementation, write the capture contract:

- one logical request token/label at a time;
- capture is requested only after a known rendered frame for the intended state;
- completion comes from the UE screenshot completion boundary, not file-size polling;
- callback corresponds to the expected request/path;
- callback/delegate is unregistered on completion, failure, restart, and EndPlay;
- file is fresh, decodable PNG, exactly the requested resolution, and above a minimum size;
- watchdog and exit completion are idempotent.

Use UE 5.8 viewport/screenshot delegates (`OnViewportRendered`, `OnScreenshotCaptured`, or the nearest supported frame-end equivalent) rather than arbitrary Sleep/Retry growth. Keep production HUD and production state machine in the image path.

Add PowerShell-side PNG validation that reads the PNG signature/IHDR width and height, verifies the path is inside the current run directory, verifies creation/modification is newer than the request start, and rejects duplicate/stale files.

Run Editor and packaged captures and visually inspect all three at 100% scale. The known clipped combat frame must either be fixed and replaced or explicitly excluded from release evidence; it may not remain a “passed” visual artifact.

Suggested commit subject:

```text
fix(capture): bind gameplay screenshots to rendered completion
```

### Commit G — make release evidence bind to an immutable clean revision (`SF-IRA-002`)

Write negative Pester/PowerShell tests or a deterministic script harness first. It must prove every authoritative package/aggregate entry point rejects:

- staged changes;
- unstaged changes;
- untracked files not under explicitly ignored generated roots;
- HEAD movement after preflight;
- source changes while an external verification process is running;
- missing/invalid 40-character revision;
- stale package manifest;
- archive SHA mismatch;
- manifest revision mismatch;
- native `git` failure.

Minimal implementation boundary:

- one shared helper returns a validated `ExpectedRevision` and clean-tree fingerprint;
- pass `ExpectedRevision` to build/test/report/package scripts instead of re-reading and trusting whatever HEAD is at the end;
- assert the same HEAD and clean status before and after every external process and before publishing each manifest;
- final aggregate repeats repository audit and invariant checks;
- package scripts re-read and re-hash the produced archive and adjacent checksum before accepting it;
- derive BuildPlugin target proof from UAT output/artifact inspection rather than writing an unconditional literal list;
- audit Error/Fatal/Ensure and Warning lines with a narrow, documented allow-list; do not silence new warnings;
- bind all manifests and summaries to the exact same clean revision.

Suggested commit subject:

```text
fix(release): bind artifacts to one clean immutable head
```

### Commit H — strengthen production-path integration evidence (`SF-IRA-008`)

Add focused integration coverage without OS-level keyboard/mouse macros:

- real transient World/Coordinator/actors;
- an enemy receives a successful non-empty A* path and moves a finite bounded distance;
- movement waypoints remain derived from walkable path cells;
- `Playing -> Lost -> Restarting -> Generating -> Playing`;
- Same-Seed restart returns equal layout and encounter hashes and fresh actor instances;
- New-Seed restart uses the specified deterministic next seed and changes the expected run identity;
- repeated rapid R/N leaves one active request and one owned actor set;
- engine-level input dispatch proves WASD, aim, attack, Dash, R, and N mappings in a packaged self-test path;
- no direct assignment of RunState, collected count, enemy HP, or canonical identities.

The input self-test may call UE input injection/dispatch APIs inside the executable; do not automate the desktop cursor or timing-sensitive OS key macros.

Suggested commit subject:

```text
test(gameplay): cover restart input pathing and ownership
```

### Commit I — update evidence and disclosure

Only after all focused fixes are green:

- update this audit's finding statuses with exact test/log references;
- update `README.md`, `PHASE3_ACCEPTANCE_MATRIX.md`, `KNOWN_LIMITATIONS.md`, candidate notes, evidence journal, final handoff, and PR body to match observed evidence;
- remove the clipped-HUD limitation only after fresh complete images prove it is fixed;
- describe the input implementation accurately as named Action/Axis mappings running on Enhanced-compatible input classes unless it was deliberately migrated under a separately approved scope;
- update the Automation count from the parsed final report, not from a hard-coded expectation;
- extend `AI_ASSISTANCE.md` with the independent audit, fixes, tests, visual review, and remaining human ownership boundary.

Suggested commit subject:

```text
docs: record independent 0.3.0 release audit evidence
```

---

## 2. Focused verification after each repair

Use the smallest relevant filters first, always after an Editor Development build when C++ changed:

```powershell
.\Scripts\Build.ps1
.\Scripts\Test.ps1 -Filter SeedForge.Model.Path
.\Scripts\Test.ps1 -Filter SeedForge.Model.Encounter
.\Scripts\Test.ps1 -Filter SeedForge.Model.RunState
.\Scripts\Test.ps1 -Filter SeedForge.Gameplay
.\Scripts\Test.ps1 -Filter SeedForge.GameplaySmoke
```

For smoke/capture/evidence changes:

```powershell
.\Scripts\TestGameplay.ps1 -Seed 24301 -RunLabel editor
.\Scripts\PackageGameplay.ps1 -Seed 24301
```

For script-integrity changes, execute the new negative harness before a full UAT run. It should fail before invoking UBT/UAT when the tree/revision invariant is deliberately violated.

Record for every command:

- exact start/end timestamp;
- exact `HEAD` before and after;
- process exit code;
- machine-readable report path;
- passed/warnings/failed/not-run/in-process counts;
- unexpected Error/Fatal/Ensure/Warning lines;
- artifact paths and SHA-256;
- clean-tree result.

---

## 3. Complete final verification on UE 5.8

Run only when all findings selected for repair are focused-green and the working tree is clean at the committed final head.

### Gate 1 — repository audit

```powershell
.\Scripts\AuditRepository.ps1 -RequireClean
```

Also verify:

```powershell
git diff --check
git status --porcelain=v1 --untracked-files=all
git rev-parse HEAD
git rev-list --left-right --count origin/main...HEAD
```

### Gate 2 — Editor Development build

```powershell
.\Scripts\Build.ps1 -Configuration Development
```

Require exit 0 and strict Error/Fatal/Ensure/Warning audit.

### Gate 3 — complete SeedForge Automation

```powershell
.\Scripts\Test.ps1 -Filter SeedForge -TimeoutSeconds 900
```

Parse `index.json`; require:

- `succeeded > 0`;
- `succeededWithWarnings = 0`;
- `failed = 0`;
- `notRun = 0`;
- `inProcess = 0`.

The final reported test count is whatever this fresh machine report proves; do not assume 63 after adding tests.

### Gate 4 — preserved Phase 2 evidence chain

```powershell
.\Scripts\Report.ps1 -SeedCount 10000 -Warmup 100
```

Require fresh and revision-bound:

- both canonical layout JSON documents;
- Diff JSON;
- 10,000 requested/attempted/succeeded, zero failures;
- non-zero aggregate hash;
- ordered timing distribution;
- exact reparse;
- five original golden hashes unchanged.

### Gate 5 — focused Phase 3 gameplay tests

Run the focused namespaces again after the complete suite so their exact logs are easy to audit:

```powershell
.\Scripts\Test.ps1 -Filter SeedForge.Gameplay
.\Scripts\Test.ps1 -Filter SeedForge.GameplaySmoke
.\Scripts\Test.ps1 -Filter SeedForge.Model.Path
.\Scripts\Test.ps1 -Filter SeedForge.Model.Encounter
.\Scripts\Test.ps1 -Filter SeedForge.Model.RunState
```

### Gate 6 — Editor gameplay smoke

```powershell
.\Scripts\TestGameplay.ps1 -Seed 24301 -RunLabel editor -TimeoutSeconds 180
```

Require production actors/APIs, real A* path/movement proof, exact trace identity, success marker, one completion, strict logs, and three complete images.

### Gate 7 — BuildPlugin target matrix

```powershell
.\Scripts\PackagePlugin.ps1
```

Require evidence for all three actual targets:

- UnrealEditor Win64 Development;
- UnrealGame Win64 Development;
- UnrealGame Win64 Shipping.

Do not accept an unconditional manifest list without matching UAT target evidence.

### Gate 8 — Win64 BuildCookRun

Run through the repaired package entry point:

```powershell
.\Scripts\PackageGameplay.ps1 -Seed 24301 -TimeoutSeconds 300
```

Require one clean-revision Build/Cook/Stage/Pak/Archive run, local UAT diagnostic paths, strict log audit, and a fresh executable under the new timestamped package directory.

### Gate 9 — ordinary packaged executable

Launch the ordinary packaged EXE without gameplay-smoke state-driving arguments. Prove:

- startup reaches a playable run;
- real WASD movement;
- mouse aim;
- attack and Dash;
- Same-Seed R;
- New-Seed N;
- no duplicated HUD/actors or stale possession;
- clean controlled exit.

An internal UE input-dispatch self-test is acceptable. A capture-only auto-exit path is not sufficient to claim ordinary interaction.

### Gate 10 — packaged gameplay smoke

Require the production attack/kill/Core/Exit path, path movement evidence, strict trace reparse, failure-trace negative case, non-zero failure exit, and no double completion.

### Gate 11 — restart identities

For both Editor and packaged paths:

- Same-Seed: equal seed, layout hash, encounter hash; fresh actor ownership; reset HP/cooldowns/Core/Exit/path state;
- New-Seed: exact deterministic next seed, correct new layout/encounter identities, no stale completion;
- rapid R/N: one active request and one live run object set.

### Gate 12 — JSON trace reparse

Treat runtime JSON as untrusted. Validate schema/version, exact unsigned strings, state/action sequence, actor counts, input/path/restart evidence, failure fields, source revision, and all screenshot paths.

### Gate 13 — screenshot materialization and visual inspection

For start/combat/win:

- unique current-run path;
- valid PNG signature and IHDR;
- exact 1280×720 dimensions (or the explicitly requested dimensions);
- fresh modification time after request;
- sane minimum size;
- screenshot-completion callback recorded;
- no crop, partial HUD, stale frame, missing actor, or clipped left edge on visual inspection.

Materialize the three final images into the repository only after this inspection, then commit them and re-run the revision-bound verification if the committed bytes are part of the candidate.

### Gate 14 — Error/Warning audit

Audit every fresh UBT/UAT/Editor/Automation/Cook/package/smoke log. Reject:

- fatal/assert/ensure/crash/error markers;
- unexpected warnings;
- legacy mapping warnings being hidden without the documented compatibility boundary;
- logs outside the intended project-local evidence roots where a script claims local materialization.

Every allowed warning needs an exact narrow pattern and written rationale. Do not add a broad suppression.

### Gate 15 — artifact and manifest SHA-256

Independently compute and compare SHA-256 for:

- plugin archive;
- Win64 demo archive;
- adjacent checksum files;
- gameplay trace and summary;
- three screenshots;
- Phase 2 summary and report JSONs;
- final aggregate manifest.

Re-read each file after hashing. All manifests must name the same exact clean final HEAD and version `0.3.0`.

### Gate 16 — final clean tree and remote state

After every generated artifact/manifest is complete:

```powershell
git diff --check
git status --porcelain=v1 --untracked-files=all
git rev-parse HEAD
git rev-list --left-right --count origin/main...HEAD
```

Then fetch and re-read PR #1 again. Record final base/head, Draft/open state, mergeability, reviews/threads/checks, and ahead/behind. Push the existing branch and update the Draft PR body only after the final evidence commit and its full verification are complete.

---

## 4. Exit criteria

The audit may recommend merge and `v0.3.0` publication only when all are true:

- zero unresolved Blocker or High findings;
- every directly related, low-risk Medium finding is fixed or explicitly reclassified with new evidence;
- no release-evidence falsification path remains;
- all 16 final gates pass on UE 5.8;
- Automation has zero warnings/failures/not-run/in-process;
- all artifacts/manifests bind to one clean final HEAD;
- all three screenshots are visually complete;
- final worktree is clean;
- PR #1 remains open/Draft and unmerged pending user authorization;
- no `v0.3.0` Tag or Release has been created.

Until those conditions are met, the correct recommendation is **do not merge and do not publish `v0.3.0`**.

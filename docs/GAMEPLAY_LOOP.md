# Gameplay loop

## Start and identity

Launch the Editor game or packaged `Windows/SeedForge.exe`. The default seed is 24301; `-SeedForgeSeed=<uint64>` selects another seed.

The run starts in Generating. Layout work runs off the Game Thread; only the newest live request may apply. The Coordinator validates the layout, creates the encounter, applies floor/wall collision, spawns roles, initializes HP/Core IDs and enters Playing. RunGeneration and actual applied request ID are separate identities. Pending restart/failure snapshots do not retain the previous layout/encounter hashes.

## Controls

| Input | Result |
|---|---|
| W / S | Move along +X / −X |
| A / D | Move along −Y / +Y |
| Mouse | Aim on the Character's plane; invalid deprojection retains last valid aim, initially +X |
| Left Mouse Button | Directional pulse attack with range/arc and cooldown |
| Space | Dash along current summed movement axes, otherwise aim, otherwise +X |
| R | Restart the same seed |
| N | Request the deterministic next seed |

Mappings are text Action/Axis mappings on Enhanced-compatible classes. The persistent PlayerController owns R/N, so recovery does not depend on a surviving Character. The Character owns movement/Attack/Dash. Same-frame movement plus Space uses current summed axes; possession and key flush clear stale movement intent.

## Fight, collect and extract

Five enemies start on distinct walkable cells outside the configured player safety radius. A Coordinator timer replans with deterministic four-neighbor A* every 0.5 seconds (first callback after 0.05 seconds), at most 1024 non-goal expansions per search. Enemy Tick only follows waypoints. Checked int64 neighbor/distance arithmetic prevents int32 coordinate wraparound.

An accepted attack consumes its cooldown even when it misses. Range/arc filtering then chooses the lowest stable-ID live target. Default damage is 50 against 100 enemy HP, so two hits kill one enemy. Enemy contact damage is rate-limited separately.

Three cyan Data Cores have deterministic stable IDs/cells. The proximity timer asks the state machine to collect an ID before destroying that actor. Unknown or duplicate IDs cannot increment progress. Collecting all expected IDs unlocks the real exit and changes its presentation from red to green. Exit proximity then enters Won and stops interaction/repath timers.

No NavMesh, Behavior Tree, EQS or GAS is involved in this slice.

## Lost, Failed and recovery

Zero player HP produces Lost: a gameplay defeat. Failed is an operational error such as invalid generation/encounter, spawn failure or an opt-in smoke/capture failure. Failure cleanup removes old actors/work/timers and clears stale identities, hashes and collection eligibility; the AHUD-owned Slate prompt reads that failure snapshot.

R/N from active or terminal states moves through Restarting to Generating. When already Generating, another R/N supersedes the pending request without inventing an extra Playing transition. R preserves the seed. N uses uint64 wraparound:

```text
next = seed * 6364136223846793005 + 1442695040888963407
```

The controller and HUD persist while the run Character and other run actors are replaced. Real completion must apply only the latest request and restore fresh HP, Core/exit state and cooldowns. Interactive recovery is distinct from dedicated verification processes, which exit nonzero on a failed proof.

## Automated verification paths

From a clean checkout, run the normal wrappers:

```powershell
.\Scripts\TestInputSelfTest.ps1 -Seed 24301
.\Scripts\TestGameplay.ps1 -Seed 24301
```

Use `-AllowDirtyDiagnostic` only for supported development wrappers when changes are uncommitted; that evidence cannot certify a release. Build/package/final verification remain separate gates; see [DEVELOPMENT.md](DEVELOPMENT.md).

The ordinary input self-test runs without `-SeedForgeGameplaySmoke`. It drives the UE input stack and real viewport aim, not desktop macros or private gameplay handlers. It checks WASD, two aims/attacks, diagonal Dash, public-damage Lost, R/N and rapid restart ownership. Permitted setup teleports/public damage are labeled separately from measured input motion. Missing viewport or incomplete effects fail under one 30-second budget.

Gameplay smoke first waits passively for a real A* path and at least two enemy move observations totaling 20 units, before requesting start capture. It then shortens the run using real attack/cooldown/proximity rules and disclosed Character teleports to Cores/exit. It does not assign Won or collected counts. Start/combat/win captures require owning render/pixel/save callbacks, fresh 1280×720 PNGs and clean receipt ownership—not merely files appearing on disk.

PackageGameplay combines ordinary launch/capture, ordinary input/restarts, H7 gameplay captures and four typed negative processes against the cooked executable. The clean a9f5625 checkpoint completed that sequence; final all-16 candidate verification after this documentation update remains pending. The inspected packaged images are unedited evidence illustrations, and the start image can show exposure settling.

Neither automated path promises a deterministic real-time playthrough, pixel-identical rendering or production-level visual polish.

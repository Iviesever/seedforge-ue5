# Gameplay loop

## Start

Launch the Editor game or packaged `Windows/SeedForge.exe`. The default seed is 24301; override it with:

```text
-SeedForgeSeed=<uint64>
```

The run begins in `Generating`. Layout work runs off the Game Thread. Only the newest live request may apply. The coordinator validates the layout, creates the deterministic encounter, builds floor/wall collision, spawns actors, initializes HP/Core IDs, and enters `Playing`.

## Controls

| Input | Action |
|---|---|
| W / A / S / D | Move on the top-down plane |
| Mouse | Aim; last movement/forward direction is the stable fallback |
| Left Mouse Button | Directional pulse attack; fixed range/arc and cooldown |
| Space | Dash; fixed impulse and cooldown |
| R | Restart the same seed |
| N | Start a deterministic next seed |

## Fight

Five enemies begin on distinct walkable cells outside the configured player safety radius. Every 0.5 seconds, the coordinator maps player/enemy positions to canonical cells and runs bounded four-neighbor A*. Pawns follow the resulting waypoint list between replans. Contact deals rate-limited damage. Two player hits kill one enemy under the default tuning.

The game does not use NavMesh, Behavior Tree, EQS, or GAS. This keeps the path, timing boundary, and failure modes visible in the portfolio code.

## Collect

Three cyan Data Cores have stable IDs and deterministic spawn cells. The interaction timer uses world distance to invoke `FSeedForgeRunStateMachine::CollectCore`. The actor is destroyed only after the transition succeeds. Unknown/duplicate IDs cannot increment progress.

## Extract

The exit is red while locked. Collecting every expected Core unlocks it and changes it to green. Reaching it then invokes `ReachExit`, transitions `Playing -> Won`, stops hostile timers, and presents the terminal HUD prompt.

## Lose and restart

HP reaching zero invokes the state machine and enters `Lost`. R or N moves the run through `Restarting -> Generating`. The coordinator cancels generation, removes old actors/timers/path state, and starts from a clean ownership generation. R preserves the seed; N applies a fixed unsigned next-seed transform.

## Automated path

```powershell
.\Scripts\TestGameplay.ps1 -Seed 24301
```

This starts the same GameMode/Coordinator/actors as normal play, performs attack/kill/collect/extract through production rules, captures three screenshots, writes a JSON trace, reparses it, audits warnings/errors, and exits. `PackageGameplay.ps1` repeats that proof against the packaged executable after first launching its ordinary non-smoke path.

Smoke is a bounded verification shortcut, not a claim that gameplay movement or a complete run is deterministic.

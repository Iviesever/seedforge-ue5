# AI assistance and authorship

## Disclosure

SeedForge 0.1.0, 0.2.0, and the 0.3.0 candidate were specified and constrained by the user and implemented, tested, debugged, packaged, visually inspected, and documented by Codex using GPT-5.6 Sol. The user did not participate in writing the delivered code during these delivery windows.

The user supplied the career/product goal, deadline, repository/engine boundaries, agent/process limits, non-goals, acceptance gates, branch/PR authority and, later, explicit authority to merge and publish a source-only Release after verification. The user also approved only the narrow UBT Trace*.uba outside-root exception. Codex selected implementation details, wrote C++/Automation tests/PowerShell/docs, ran UE 5.8 and Git/GitHub workflows, investigated failures, inspected screenshots, and assembled local artifacts.

Phase 3 AI work specifically includes:

- deterministic encounter API, placement rules, version/hash, and tests;
- pure bounded A* contract/implementation/tests;
- fail-closed run state machine/tests;
- native Player/Controller/Enemy/Core/Exit/HUD/Coordinator and input/config;
- combat, damage/death, dash, pickup/unlock/win, restart cleanup, and enemy replan integration;
- gameplay smoke state driver, exact JSON trace, screenshot/log validation, BuildPlugin, BuildCookRun, and packaged runs;
- portfolio architecture, walkthrough, interview, drills, acceptance, evidence, limitations, and PR material.
- the independent release audit: failed-run recovery, persistent input, live diagonal Dash, safe coordinate math, run/request identity, render-owned capture/Slate HUD, actual path/restart/ordinary-input proof, storage confinement and immutable evidence sealing;
- retained REDs, unsuccessful diagnostics and root-cause packets; independent read-only reviews by bounded sub-agents; original-resolution inspection without modifying generated screenshot pixels.

## What the user may honestly claim

An accurate portfolio description is:

> I scoped and orchestrated an AI-assisted UE 5.8 C++ extraction-slice project. Its repository includes locally executed tests, packaged input/gameplay evidence and an independent release audit. The implementation was produced with Codex; I use its reproducible checks and live-change drills to develop and demonstrate my own understanding and modifications.

After the user personally reproduces the checks, studies and modifies the system, they may describe that specific personally authored change and the understanding demonstrated by it. The audit's latest observed full suite is 119 tests; a final exact-revision result must be read from its current machine report, not inferred from that checkpoint count.

## What must not be claimed

- The user independently hand-wrote this implementation.
- The user personally designed every low-level API or debugged every compiler/runtime issue.
- Generated tests alone prove the user's C++ or Unreal skill.
- The full playthrough is deterministic because the initial map/encounter is deterministic.
- Editor/Automation success alone proves the packaged executable.
- The project is production-ready, balanced, cross-platform, multiplayer-capable, or defect-free.
- AI assistance can be omitted where an employer, course, contest, or platform requires disclosure or forbids it.

## Responsible interview preparation

Before presenting Phase 3, the user should be able to:

1. Reproduce build, complete Automation, Editor path/capture and ordinary-input checks, BuildPlugin, BuildCookRun, ordinary packaged input/restarts and positive/negative packaged smoke.
2. Explain layout versus encounter versus whole-run determinism.
3. Derive the encounter selection order and hash inputs for one role/cell.
4. Explain A* open/closed data, Manhattan admissibility, neighbor/tie order, budget, and current complexity.
5. Explain why searching at 2 Hz is bounded while movement can Tick.
6. Explain every state transition and why duplicate Core events fail closed.
7. Explain WorldSubsystem versus Gameplay Coordinator ownership and stale-result protection.
8. Explain why smoke teleporting is permitted but direct state assignment would be a fake test.
9. Explain why BuildPlugin and packaged smoke find defects that Editor tests cannot.
10. Complete at least one `LIVE_CHANGE_DRILLS.md` task test-first and defend the trade-offs without AI-generated talking points.

The source/trace SHA declared inside the executable is not self-verification. Explain why clean Git/process guards, original-log and archive hashes, frozen child records, original-resolution visual review and fresh remote review are separate evidence boundaries. `MachinePassed` alone does not mean published or human-reviewed.

The last item is the strongest bridge from an AI-produced repository to credible personal engineering skill.

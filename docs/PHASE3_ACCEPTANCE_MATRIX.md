# Phase 3 acceptance matrix

Final status is authoritative only when `Artifacts/Reports/phase3-verification-last.json`, plugin/package manifests, gameplay trace, and Draft PR all name the same clean revision.

| Goal condition | Proof | Evidence | Status before final aggregate |
|---|---|---|---|
| Latest `origin/main` base | fresh fetch; base `9a306f8...` | PACT-30 evidence | Passed |
| Independent feature branch | `feat/phase3-playable-vertical-slice` from base | Git history | Passed |
| Preserve 0.2.0 | tag/release fixed; original tests/goldens green | PACT-30/31 evidence | Passed |
| Pure encounter plan | deterministic counts/IDs/cells/hash and typed failures | `SeedForge.Model.Encounter.*` | Passed |
| Pure bounded A* | all statuses, exact tie route, budget | `SeedForge.Model.Path.*` | Passed |
| Pure state machine | win/loss/restart/illegal/Core identity | `SeedForge.Model.RunState.*` | Passed |
| Code-native player loop | Character/Controller/input/camera/attack/dash/HP | Gameplay tests + Editor capture | Passed |
| Five enemy pursuit | stable encounter + 2 Hz bounded A* handoff | source audit + Gameplay smoke | Passed |
| Three Cores and exit | production proximity, unlock, win | Editor + packaged JSON actions | Passed |
| HUD requirements | HP/Core/Seed/Run/controls/terminal/exit | inspected start/win PNGs | Passed |
| Same/new seed support | R/N bindings and run cleanup/next-seed path | state tests + source audit | Passed |
| CLI seed | `-SeedForgeSeed=<uint64>` | Editor/package traces for 24301 | Passed |
| Normal interaction mode | launch without smoke flag reaches ready | normal Editor/package logs | Passed |
| Automated gameplay smoke | production path, watchdog/non-zero failures | `TestGameplay.ps1` + trace | Passed |
| JSON reparse | strict schema/identity/count/state/action checks | gameplay summaries | Passed |
| Screenshots | Editor and packaged start/combat/win files | `docs/images` + `Artifacts/Media` | Passed with documented packaged combat-frame limitation |
| Existing 41 tests | included in complete 63-test run | Automation report | Passed |
| New tests | 22 new model/gameplay/smoke tests | Automation report | Passed |
| Editor Development | UBT | build log | Passed |
| Runtime/Game/Shipping plugin | BuildPlugin three targets | plugin manifest/log | Passed |
| Win64 Build/Cook/Stage/Pak/Archive | BuildCookRun | gameplay package manifest/log | Passed |
| Ordinary packaged EXE | non-smoke capture path exit 0 | `last-gameplay-package.json` | Passed |
| Packaged gameplay smoke | exit 0, trace, images, strict log audit | packaged summary/trace | Passed |
| Documentation/AI honesty | Phase 3 docs and disclosure | tracked docs | Pending final review |
| Repository clean/auditable | no generated paths, placeholders, diff errors | AuditRepository + Git | Pending final aggregate |
| Exact final revision | all manifests agree after docs commit | `phase3-verification-last.json` | Pending final aggregate |
| Branch pushed / Draft PR | remote branch and PR body evidence | GitHub | Pending |

`v0.3.0` is candidate metadata only. No tag, formal Release, or merge is authorized.

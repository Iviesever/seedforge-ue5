# Project-local build environment prelude evidence

## Authority and starting revision

- Starting HEAD: `e461863508767403fd4739630f175a1b6dea71d7`, clean.
- User explicitly approved the narrow default `Trace*.uba` exception after the three-turn authority blocker. The goal resumed active. The exception is recorded in `.agents/AGENTS.md`; it does not authorize Engine edits or external project artifacts.
- No Engine or global known-folder setting was changed.

## RED and initial integration finding

- Added the acceptance harness against an empty common helper. It failed on project-local roots, inherited child settings, remote configuration and all 11 missing launcher integrations; invalid-root and conflict cases also failed. This was behavior RED, not a syntax error.
- Initial helper used UBT_EXTRA_ARGS for a Build-only option. Although its unit harness passed and `build-editor-20260904-115721.log` compiled 7 actions successfully, WriteMetadata reported `Invalid argument: -UBADisableRemote`. Full Automation was 86/86 (`automation-20260904-115737`), but that build output was not accepted as a clean gate.
- Independent review additionally found `=true` and quoted-value problems in the raw argument regex. Root cause: `root-cause-build-argument-scope.md`.

## Corrected contract and GREEN

- The helper now only validates the actual project and sets process-scoped UBA_ROOT/DDC paths under `.cache`. It does not inspect or alter caller UBT_EXTRA_ARGS.
- All 11 direct UE/UAT launchers use it before children launch; nested children inherit the cache roots.
- Build.ps1 passes bare `-UBADisableRemote` only to Build mode. PackageDemo uses BuildCookRun's supported UbtArgs. Stock BuildPlugin has no matching arbitrary CLI pass-through; its storage is confined but its listener/log proof remains in task G.
- The strengthened harness first failed against the old helper, then passed after the correction. Command: `tasks/20260904-102407-phase3-independent-release-audit/verify-build-environment.ps1`. It verifies paths, invalid-root rejection, repeated setup, exact opaque argument preservation including quoted cases, a real PowerShell child process, all 11 launcher calls, and scoped build flags. Original environment is restored in finally.
- All Scripts/*.ps1 parsed without errors; whitespace audit passed.
- Fresh corrected `Scripts/Build.ps1`: exit 0, incremental target up-to-date / 0 actions; `Artifacts/Logs/build-editor-20260904-120347.log`. It explicitly reports remote execution disabled, with no `Invalid argument:` or remote bind failure. This incremental check is not presented as new full target compilation.
- Fresh full `Scripts/Test.ps1 -Filter SeedForge -TimeoutSeconds 600`: exit 0, 86/86, zero warnings/failures/not-run/in-process; `Artifacts/Reports/automation-20260904-120348/index.json`.
- Physical UBA cache files/directories now exist under `D:\program\SeedForge\.cache\UnrealBuildAccelerator` (castemp, sessions, memgroups).
- The global UBT environment XML remained unchanged in both SHA256 and last-write ticks across build and full Automation: `d3ba6666338ebc598f9b191b867ed6f8dc6301947bd79fa4b5a25dded59c8`. No XmlConfig environment override was introduced.
- Independent read-only re-review approved the corrected isolated diff, with both earlier P2s resolved. No reviewer file mutation or UE process occurred.

## Remaining scope

After this isolated commit, run clean-commit Editor Gameplay Smoke, then continue F screenshot ownership/visual proof. G's immutable revision, packaging target proof, archive checks and strict aggregate/log checks, H's actual input/path integration, final documentation, all 16 gates, PR update, merge and source-only Release remain incomplete.

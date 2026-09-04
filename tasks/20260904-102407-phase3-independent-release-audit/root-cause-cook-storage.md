# First BuildCookRun storage validation failure

## Original evidence; no passing wrapper manifest

Clean source `d896e5c1d2a9ac0c79a666f2d24ea4167b328f1a`; original UAT directory `Artifacts/Logs/uat-demo-20260904-191653-eaee4ebfb7c947e2817979a971c03001`. UBT Build and UAT Cook/Stage/Pak/Archive all returned 0; UAT completed in 78.19 seconds. The wrapper then rejected storage evidence and did not start its packaged ordinary-input/capture flow or write an accepted package manifest/ZIP. Original logs and timestamped archived executable are retained as incomplete diagnostic evidence.

The complete Cook log is `Cook-2026.09.04-19.18.10.txt`. Both Pak and IoStore commands received the unique owned DDC arguments. IoStore consumed a loose package-store manifest, reported Zen input bytes 0 and Loose File bytes 318167130. The old `Saved/Cooked/Windows/ue.projectstore` is absent after the full loose cook, as predicted by the installed writer's clean-sandbox path. This does not prove the entire Cook avoided Zen: a separate EditorDomain initialization remained.

## Failure 1: valid engine-relative legacy diagnostic

Cook log 945 contains `../../../../../../SeedForge/Intermediate/Shaders/WorkingDirectory/48552/` in the exact Guid-format-versus-processId comparison message. The initial storage validator accepted only absolute working paths and stopped here.

Installed `ShaderCompiler.cpp` 856-858 constructs that legacy string solely for a length comparison. Actual cleanup/worker output uses the other GUID directory, logged at 946 as repository `.cache/Temp/UnrealShaderWorkingDir/...`. `Paths.cpp` 1584-1586 resolves relative paths against `FPlatformProcess::BaseDir`; `App.cpp` 424 prints the same value. Native Base Directory at Cook line 470 is the selected installed Engine's `Engine/Binaries/Win64`, and explicit resolution lands inside this project's Intermediate directory.

Correction contract: only this precise legacy message may use a relative path, and only with a caller-supplied expected executable directory matching exactly one native Base Directory after normalization. Explicitly combine with that base (never shell cwd), then retain owned-path/reparse checks. DDC, XGE and actual shader cleanup paths remain absolute-only. Do not ignore the legacy line, manufacture an absolute line, or rewrite the log. Synthetic missing/duplicate/spoofed/wrong-base/escape/reparse cases precede implementation.

## Failure 2: independent EditorDomain Cook attachments

Cook lines 1593-1594 launch the installed `zen.exe service status` utility and report that EditorDomain could not connect. `TargetDomainUtils.cpp` 492-505 reads `CookAttachmentsEnabled` from **GEditorIni**, defaulting true, and constructs FEditorDomainOplog. Its 145-180 constructor creates a Zen HTTP client and may create project/oplog records. This is independent of the filesystem DDC graph and `-SkipZenStore` package writer.

The Zen service initializer can touch the global application settings directory and service configuration before it connects. Therefore the status line is not granted a broad or exact log allowance. The minimal supported prevention is the Cook-only command-line override `-ini:Editor:[EditorDomain]:CookAttachmentsEnabled=False`. `ConfigCacheIni.cpp` 2317 onward documents/parses this per-process override form. It prevents construction of the optional EditorDomain client; the commit/read helpers safely return without optional attachments when the pointer is absent. Normal full platform cooking and its loose PackageWriter are unchanged.

Correction acceptance: native Cook command line must carry the exact unique Editor override plus the existing DDC/no-fallback/local-path/SkipZenStore switches. No LogZen initialization/service/client or EditorDomain connection attempt may remain. Recheck all original UAT/full/stdout logs and known metadata; do not merely remove the line or widen a timeout.

## Read-only boundary recheck

The five previously enumerated global Zen metadata files retain their original size, SHA256 and last-write UTC compared with the 18:04 RuntimeBoundary snapshot. `C:/ProgramData/Epic` exists with creation/last-write times predating this Cook. This is a check of identified state, not whole-filesystem/service-state attestation. An initial parent comparison mistakenly used ConvertFrom-Json's automatic date conversion and falsely reported every timestamp unequal; reparse with DateKind String confirms all five size/time/hash triples agree. No external state was repaired, deleted or rewritten.

Next: implement/review both bounded script corrections, observe RED/GREEN, commit a clean checkpoint and rerun actual BuildCookRun. Neither native exit 0 nor synthetic validator GREEN alone closes the package gates.

## Parent implementation checks

The helper's observed RED/GREEN is recorded in `runtime-relative-shader-evidence.md`. Parent independent re-execution passed 94/94 under PowerShell 7.6 (`RuntimeStorageValidation/20260904-113033-fe316e8c764b432b87c128de08f500fd`) and Windows PowerShell 5.1 (`20260904-113037-e9c1e92e8643492ba9c5d1cf8d453454`). Pak/IoStore regression remained 55/55 at `BuildCookRunStorageValidation/20260904-113038-1952c09385d9483aa13f987c37af90c2`.

The PackageDemo source-contract harness first failed because the optional attachment override was absent, then passed after the single Cook-only argument was appended. PackageDemo also passes its actual selected Engine/Binaries/Win64 directory to the Cook log validator. No RuntimeStorage Zen rejection was relaxed, no global configuration was edited, and the original 191653 log still rejects its Zen-client activity after the legacy-path check succeeds.

Independent review exposed a PS5.1-specific drive-root normalization issue in the new base-directory path. After two actual process-CWD RED cases, normalized drive-colon-only values are rejected before Combine. Final parent runs passed 96/96 on both PS versions (`113728`, `113730`) and 55/55 Pak regression (`113734`); read-only review confirmed that correction. See `runtime-relative-shader-evidence.md` for exact paths. The next real PackageGameplay run, not these synthetic results, must establish native Cook confinement and packaged behavior.

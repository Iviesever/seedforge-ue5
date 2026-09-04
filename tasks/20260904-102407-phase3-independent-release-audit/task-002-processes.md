# SF-IRA-002 process and artifact integration

> **Execution:** executing-tasks; primary integrates launchers and runs all UE. One disjoint helper/test subtask may run under dispatching-parallel-agents.

**Goal:** Every authoritative verification entry point binds its processes and output to a frozen clean 40-hex revision, rejecting dirty/changed source, stale evidence and unsupported target claims.

**Foundation:** VerificationContract.ps1, 51 negative/positive cases on PS 5.1/7.6 including masked index flags and real child mutations. Parent independent rerun `VerificationContract/20260904-135702-9f6ec8244af743729a3e43ffffeb4886` passed 51/51. This does not yet certify launcher integration.

## Interfaces / owned files

- Existing Build, Test, Smoke, Report, CaptureDemo, CaptureInspector, TestGameplay, TestRunFailure, PackagePlugin, PackageDemo, PackageGameplay, VerifyAll, VerifyPhase3 and FinalizeRelease gain an optional exact ExpectedRevision. CaptureGameplay remains a forwarding wrapper. Before output/process launch capture `New-SeedForgeVerificationContext`; pass its frozen revision to child entry points. Wrap each actual UE/UBT/UAT process and relevant packaging/copy operation in Invoke-SeedForgeVerifiedStep, preserving action/postcondition failures. Assert again immediately before publishing a summary/manifest.
- Only development Build/Test/TestGameplay/TestRunFailure may expose explicit AllowDirtyDiagnostic for RED/debug work. Such results must carry a diagnostic-prefixed revision and cannot enter authoritative package/aggregate acceptance. Package and aggregate paths have no diagnostic bypass.
- Each process runner produces a unique current-run summary with frozen revision, actual log/report paths, exit, start/end UTC, and hashes. Aggregators consume returned exact paths, never search for arbitrary latest reports by mtime.
- Package scripts independently call Assert-SeedForgeArtifactManifest after writing archive/checksum/manifest. Gameplay manifest uses the same `sha256` archive field, not a differently named unverified alias. Aggregates revalidate both package archives/checksums and hash every referenced trace/report/capture/log into an evidence index. Final audit repeats clean HEAD and preserves existing v0.2.0/goldens.
- New Scripts/LogValidation.ps1: `Assert-SeedForgeLog -Path <file> -AllowedWarnings <documented-profile>` returns counts and allowed records; rejects UE/compiler/UAT errors, fatal/assert/ensure/crash and unexpected warnings. Known environmental warnings must match complete observed text or a narrowly enumerated field set; no generic LogEditorDataStorageUI prefix exemption.
- New Scripts/BuildPluginValidation.ps1: `Get-SeedForgeBuildPluginTargetProof -ConsoleLog -DiagnosticRoot -PackageDirectory -StartedAtUtc` returns the three actual target records only after matching successful UBT invocations, fresh per-target logs and real packaged per-target products. No unconditional verifiedTargets literal. Read installed BuildPlugin source and actual prior products before choosing exact markers.

## Test and execution sequence

- [ ] Write real-entry-point preflight tests that fail before launching UE for staged/unstaged/untracked/invalid expected revision/native git failure. Synthetic minimal Git fixtures remain under Artifacts/Reports, never a clone/worktree/integration copy. The shared 51-case harness supplies actual child HEAD/source mutation and archive tamper coverage.
- [ ] Lock exact log/target validation fixtures; execute RED; implement helpers; execute GREEN with tampered target/log/artifact cases. Parent independently reruns and reviews.
- [ ] Integrate all process boundaries and final summaries; parser/whitespace checks plus preflight harness.
- [ ] Commit focused implementation checkpoint, then run fresh native build/Automation/report/BuildPlugin/BuildCookRun and all package/input/capture gates on one clean exact revision.
- [ ] If native launch changes source/HEAD, rejects a log or finds a stale artifact, retain failure observation and fix the cause. Do not grow timeouts, suppress errors or rewrite historical evidence.
- [ ] Independently review, record exact evidence and commit integration. Release certification remains all 16 gates, not helper success.

## Known environment constraints

BuildPlugin does not forward arbitrary UbtArgs. Do not invent such flags. Existing UBT_EXTRA_ARGS=-UBADisableRemote failed recursive WriteMetadata and was removed. UBT global XmlConfigCache support or another installed supported boundary must be proven before using any scoped alternative. No global environment XML writes or Engine modifications; only approved startup Trace*.uba exception.

# SF-IRA-002 verification-contract foundation evidence

## Scope and delivered files

AI-assisted implementation by the bounded verification-contract sub-agent. Only these source/evidence files were created:

- `Scripts/VerificationContract.ps1`
- `tasks/20260904-102407-phase3-independent-release-audit/verify-verification-contract.ps1`
- This evidence document.

No real launchers/package scripts, Runtime code, other plans/evidence, or integration-checkout Git index/HEAD/branches were changed. No UE, UBT, UAT, Editor, build/package, install, clone, worktree, remote or sub-agent operations were run.

All test repositories are separately initialized **artificial sentinel repositories**, each tracking only `.gitignore` and `sentinel.txt`; they are not copies of SeedForge source. Fixture-local Git init/config/add/commit operations, child-induced HEAD movement, and the P2 follow-up's fixture-local `update-index` flags are the explicitly authorized negative-test operations. All repositories, child scripts, changed sentinels, synthetic ZIPs, checksums, manifests, junctions and summaries remain inside their unique `Artifacts/Reports/VerificationContract/<id>` directory.

## API contract

`New-SeedForgeVerificationContext -ProjectRoot [-ExpectedRevision]` returns `ProjectRoot`, `ExpectedRevision`, `TreeId`, and `CleanFingerprint`. Explicit revisions must be complete 40-hex identities, not empty strings, null, abbreviated hashes or refs. Omitting the argument captures current HEAD only after validating the exact worktree root, native Git success and clean staged/unstaged/untracked status. The fingerprint binds normalized root, revision, tree and the clean-state marker using SHA256.

`Assert-SeedForgeVerificationContext -Context` validates the context shape/self-binding and independently reruns Git root/HEAD/tree/index-flag/status checks. It does not adopt a new current HEAD. Ignored generated files are allowed; non-ignored changes fail. The P2 follow-up also requires every NUL-delimited `git ls-files -v -z` entry to carry the ordinary `H` tag, rejecting assume-unchanged, skip-worktree and unknown/nonordinary tags without editing the index. Git uses `--no-optional-locks`; status disables fsmonitor for the invocation only. Native Git errors retain command, numeric exit and diagnostic output, without environment/config dumps.

`Invoke-SeedForgeVerifiedStep -Context -Name -Action` copies the expected identity before action execution, checks it immediately before the action, and verifies the frozen identity plus caller-context integrity in `finally`. Caller attempts to update the context to a new HEAD do not rebase the expected identity. Action output is returned only after postconditions pass. An action-only failure retains its ErrorRecord; simultaneous action/postcondition failures are retained in an AggregateException, including both messages and inner exceptions.

`Assert-SeedForgeArtifactManifest -Context -ManifestPath [-ChecksumPath]` executes under that verified-step guard. It validates `sourceRevision`, `archive` and `sha256`; preserves other manifest properties; independently hashes the archive; and requires a matching one-line checksum naming the exact archive basename. The default checksum is `<archive>.sha256`; an explicit checksum may have a different filename but must remain adjacent to the archive. Returned data includes normalized archive/hash plus `ManifestPath` and `ChecksumPath`.

Manifest/archive/checksum paths must be strict descendants of `ProjectRoot/Artifacts`; sibling-prefix escapes and reparse-point traversal are rejected. Relative paths resolve against ProjectRoot. This matches the current package manifests' `sourceRevision`, `archive`, and `sha256` fields, without changing those package scripts.

## Exact commands and results

Working directory: `D:\program\SeedForge`.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-verification-contract.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-verification-contract.ps1
```

Every completed run below has a `summary.json` with exact case names/results and error details. Final summaries include fixture baseline revisions/trees and the production helper SHA256.

| Stage | Directory under `Artifacts/Reports/VerificationContract/` | Total / passed / failed | Exit |
|---|---|---|---|
| Initial fixture setup stopped by legacy path-length limit | `20260904-133225-0d3e2937cbac4ed6894affb5b8399989` | Incomplete; not RED proof | 1 |
| Behavioral RED against permissive stubs | `20260904-133250-25224f6f49714568a8196e0638faee80` | 39 / 3 / 36 | 1 |
| Initial implementation exposed multiple Git command matches | `20260904-133639-ba95966c5c294722b8b79c5445756436` | 39 / 10 / 29 | 1 |
| First Windows PowerShell GREEN | `20260904-133707-b7dba7355e6b406d8c7ab835c078b110` | 39 / 39 / 0 | 0 |
| Expanded PowerShell 7.6.0 verification | `20260904-133934-4f62f7c3f017416abb5f691cfdc0bc6f` | 43 / 43 / 0 | 0 |
| Final Windows PowerShell 5.1.22621.2506 verification | `20260904-134055-7079f27c804843d2bc14d33e9d8e79b7` | 43 / 43 / 0 | 0 |

Final console summary:

```text
VERIFICATION_CONTRACT total=43 passed=43 failed=0 fixtures=D:\program\SeedForge\Artifacts\Reports\VerificationContract\20260904-134055-7079f27c804843d2bc14d33e9d8e79b7
```

Both scripts passed PowerShell parser checks (`parse_errors=0`) and direct trailing-whitespace checks (`trailing_whitespace_lines=0`).

### Root causes found during implementation

- Long descriptive fixture-directory names pushed Git object paths beyond Windows' legacy path limit. Setup was changed to short `case-000` IDs; descriptive names remain in the summaries. No long-path/global setting was changed, and the aborted setup was not counted as valid RED evidence.
- `Get-Command git -CommandType Application` returned multiple installed applications. Treating their `.Source` values as one command created a concatenated invalid executable path. Selecting the first application matches normal PATH resolution and fixed the invocation; no PATH or global setting was edited.

### Distinct evidence covered

- Staged, unstaged and non-ignored untracked changes fail; ignored generated output succeeds.
- Missing/non-Git roots, malformed or wrong explicit revisions, malformed context fields, and HEAD changes fail.
- A deliberately malformed fixture-local Git config produces real native exit `128` and `fatal: bad config line ... in file .git/config`; a nonexistent root produces real native exit `128` and its directory error.
- Real child `powershell.exe` processes append to the tracked sentinel or create an empty fixture-local commit. Both fail post-step verification; the empty commit preserves the tree while changing HEAD, independently testing revision binding.
- An action that throws `ACTION_SENTINEL` after a child source edit reports both the original action failure and the dirty postcondition. A context rewritten to the newly committed HEAD still fails against the frozen expected revision and reports context mutation.
- A valid synthetic ZIP/checksum/manifest succeeds, including an explicit adjacent checksum. Wrong revision, manifest hash, checksum hash/name, modified archive bytes, missing archive/checksum/manifest, malformed JSON, escaped paths, non-adjacent checksum and reparse traversal all fail.

## Independent P2 follow-up: index flags can hide changed source

The review finding was reproduced before changing production code: `git status --porcelain=v1 --untracked-files=all` returned an empty result for tracked sentinels marked assume-unchanged (`h sentinel.txt` from `ls-files -v`) or skip-worktree (`S sentinel.txt`), even after the actual file contents changed.

Eight new cases were added: for each flag, reject a preexisting flag on an otherwise untouched file, reject a preexisting hidden file modification, reject a flag introduced during a verified step without editing the file, and reject a real child PowerShell process that sets the flag and edits the tracked file during the step. Fixture observations independently verify empty porcelain status and the expected retained flag. No flag is reset or cleared in the main checkout or in a fixture to make validation pass.

The minimal production change calls read-only `git ls-files -v -z` before status and fails closed on malformed records or any non-`H` entry. This check is shared by context creation and every pre/post assertion. Lowercase assume-unchanged tags and uppercase/lowercase skip-worktree tags are therefore rejected even when status appears clean.

The same exact harness commands above produced:

| P2 stage | Directory under `Artifacts/Reports/VerificationContract/` | Total / passed / failed | Exit |
|---|---|---|---|
| Real RED before index-flag check | `20260904-135138-295fb82b1213422693f534e0323b4bb3` | 51 / 43 / 8 | 1 |
| Windows PowerShell 5.1 GREEN | `20260904-135317-8fb3d8c39a6941549d0fc516630da623` | 51 / 51 / 0 | 0 |
| PowerShell 7.6.0 GREEN | `20260904-135433-9f7b3782b2b94b3790162f5839a79c21` | 51 / 51 / 0 | 0 |

All eight RED failures reported `Invalid verification evidence was accepted.`; all original 43 cases still passed. GREEN summaries retain explicit diagnostics such as `Verification index flags are not ordinary H entries; assume-unchanged/skip-worktree are forbidden: h sentinel.txt` and show that the flags remain present after rejection. Both scripts again passed parser and trailing-whitespace checks.

Current full-suite result:

```text
VERIFICATION_CONTRACT total=51 passed=51 failed=0 fixtures=D:\program\SeedForge\Artifacts\Reports\VerificationContract\20260904-135433-9f7b3782b2b94b3790162f5839a79c21
```

## Limits and integration work remaining

Primary independent rerun after the P2 correction: `Artifacts/Reports/VerificationContract/20260904-135702-9f6ec8244af743729a3e43ffffeb4886/summary.json`, 51/51, process exit 0. This foundation is committed separately; process integration remains the next task.

- These are shared foundations, **not completion of G or release certification**. The parent must integrate every real external process, preserve existing native exit/log handling, prove BuildPlugin Editor/Game Development/Game Shipping targets, audit whole logs, rehash final aggregates/artifacts, and complete all release gates.
- Action scriptblocks remain responsible for their own native process exit-code and log policies; this wrapper adds revision/precondition/postcondition enforcement and preserves thrown errors. It is not a sandbox for hostile PowerShell code.
- Pre/post Git checks cannot detect a transient source edit that is completely reverted before the postcondition. The clean fingerprint is an identity binding backed by Git status, not a signed attestation or independent recursive filesystem hash.
- Archive validation proves current bytes and manifest/checksum/revision agreement. It does not inspect package target contents or prove which build process produced the archive. The synthetic ZIPs are explicitly not release artifacts.
- Existing reparse traversal is rejected; atomic protection against hostile concurrent ancestor replacement is not claimed.
- Fixture-local failing repositories and synthetic files are retained for evidence. No user source or release artifact was deleted.

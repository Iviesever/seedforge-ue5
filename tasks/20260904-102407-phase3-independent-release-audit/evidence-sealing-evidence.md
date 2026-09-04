# Frozen evidence graph and seal repair

## Scope

AI-assisted shared-helper repair for the two independently identified G evidence-chain findings. Only `Scripts/VerificationContract.ps1`, `verify-verification-contract.ps1`, and this document were changed in this subtask. No launcher/aggregate, UE, Engine, main Git index/HEAD or global configuration changes were performed. Native Git mutations are confined to the pre-authorized artificial sentinel repositories under unique `Artifacts/Reports/VerificationContract` fixture directories.

Parent-owned caller integration remains necessary: snapshot immediately after each successful step, preserve the nested original PackageDemo proof in PackageGameplay, accumulate frozen expected records, and carry the checked machine index's expectations into final sealing. This document does not claim a real release pipeline has passed.

## Root causes reproduced

1. The old index writer took only paths and recomputed a new baseline. A log/report changed after its successful step digest could be silently sealed with its new bytes while the step still claimed success for old bytes.
2. Finalization could similarly validate a machine index, then silently rebase changed machine payloads when constructing the final index from bare paths.

The new tests actually modify ignored fixture payload bytes during both intervals. They require rejection before a new index file is published. The old writer, with the new ExpectedRecords parameter present but ignored, accepted both; this was observed RED, not inferred from a mock.

## API and behavior

### Get-SeedForgeEvidenceSnapshot -Context -Evidence

Walks the returned in-memory evidence graph immediately after a successful step. It validates supplied child digests against current file bytes for these pairs:

- `Path` + `Sha256`, including optional numeric `Length`/`size` claims;
- `reportIndex` + `reportSha256`;
- `trace` + `traceSha256`;
- `archive` + `sha256`;
- `ConsoleLog` + `ConsoleLogSha256`;
- `Log` + `LogSha256`;
- `IndexPath` + `Sha256`, for the checked index itself.

Both size aliases must agree when present. Missing paired paths, malformed/false hashes, size disagreement, fractional sizes, missing files, escaped evidence paths and reparse traversal fail closed. Consistent repeated graph references coalesce; false/conflicting claims do not. Recursion is bounded at 64 levels to reject cycles.

Returned records are independent `ReadOnlyDictionary<string, object>` values with exactly `path`, `size`, and `sha256`. They neither alias mutable child objects nor permit property mutation. An informational/no-output step may return an empty snapshot; this does not invent evidence or allow an empty index.

Absolute Artifact file references without producer digests (for example a newly written summaryPath) receive their baseline at this immediate snapshot boundary. Ordinary Artifact directory metadata is checked for reparse traversal but not treated as a file. Outside-Artifact `executable`, `EngineRoot`, and `ProjectRoot` metadata is not output evidence; actual claimed digest-pair paths remain subject to Artifact confinement.

**The helper does not deserialize every referenced JSON file recursively.** Callers must retain the relevant child proof object graph. The parent has preserved PackageDemo's original proof as nested `packageProof`; its buildLogProof paths/hashes are therefore traversed instead of disappearing behind a JSON filename.

### Assert-SeedForgeEvidenceRecords -Context -Records

Copies the explicit expected triples, rejects duplicate/conflicting normalized paths and malformed values, and independently re-reads/hashes every file. It returns validated read-only copies only after source and byte checks succeed. Diagnostic contexts cannot certify these records.

### Write-SeedForgeEvidenceIndex -Context -Path -Paths -ExpectedRecords

Explicit, nonempty ExpectedRecords are now required. Omission fails directly without a prompting Mandatory parameter. The Path set must equal the unique ExpectedRecords set exactly; missing/extra paths, duplicate expectations and conflicts are rejected.

The writer compares current bytes with the frozen expectations before publication and again immediately before writing. It serializes those **frozen** hashes/sizes, not a newly computed replacement baseline, and then performs the existing independent index/checksum/payload reread. Context checks still bracket the operation. Existing manifest behavior is unchanged; the internal Artifact path checker gained an opt-in directory-metadata mode for graph traversal only.

## Caller integration pattern

```powershell
$stepRecords = @(Get-SeedForgeEvidenceSnapshot -Context $context -Evidence $stepResult)
# Store these records now. When merging snapshots, coalesce only identical
# frozen path/size/hash triples and reject conflicts; do not recapture old paths.
Assert-SeedForgeEvidenceRecords -Context $context -Records $allExpected | Out-Null
Write-SeedForgeEvidenceIndex -Context $context -Path $indexPath `
    -Paths @($allExpected | ForEach-Object path) -ExpectedRecords $allExpected
```

For finalization, retain `machineIndex.files` from the independently checked machine index. Freeze the index's own `IndexPath`/`Sha256` claim and checksum sidecar too, then add only genuinely new files (final readiness record/source archive/review records) at their verified boundaries. Pass the merged frozen records to the final writer. Do not replace old machine expectations with fresh hashes of old paths.

## RED/GREEN evidence

Commands from `D:\program\SeedForge`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tasks/20260904-102407-phase3-independent-release-audit/verify-verification-contract.ps1
pwsh -NoProfile -File tasks/20260904-102407-phase3-independent-release-audit/verify-verification-contract.ps1
```

Every listed fixture directory contains full named-case results and helper SHA256 in `summary.json`.

| Stage | Directory under Artifacts/Reports/VerificationContract | Passed / total |
|---|---|---|
| New graph/sealing behavior RED | `20260904-164507-f029663be20a44279749266e85ff4680` | 64 / 87 (23 expected failures; prior 63 remained passing) |
| Initial shared implementation GREEN, PS 5.1 | `20260904-164937-6318a05b2644414198da8cdf4a4763c7` | 87 / 87 |
| Added conflicting-size claim RED | `20260904-165634-b5153f8f86834b879630309dc0d67614` | 90 / 91 |
| Final PS 5.1 GREEN | `20260904-165827-9a7bfd366aa4411cb919a692af03dad6` | 92 / 92 |
| Final PS 7.6 GREEN | `20260904-170030-5ad38cbda141474589b83c66d13255e2` | 92 / 92 |

A direct no-output snapshot call also reproduced a binding rejection before adding the nullable Evidence boundary and its permanent regression case. AuditRepository returns host-only output, so generic step capture must support a null result without inventing files.

Fresh parser/trailing-whitespace checks and scoped `git diff --check` passed. Helper SHA256 at handoff:

`aa201d87c60552c086e163426f05551f3b0998a320af10d38959845536908168`

## Remaining boundaries

- These helpers validate file/digest consistency, not the business truth of arbitrary caller claims or cryptographic build provenance.
- The first snapshot is the baseline for a raw file that has no producer digest; therefore capture immediately at the producer/step boundary.
- The parent must integrate the new required ExpectedRecords argument at both aggregate and final seals, and preserve nested proof metadata. Real process/package/visual/remote gates remain separate.
- Existing-path reparse checks and verified rereads do not claim atomic protection against hostile filesystem replacement after the helper has returned.
- No historical failed evidence was rewritten or promoted to success.

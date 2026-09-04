# SF-IRA-002 shared verification contract subtask

> **Execution:** bounded independent `dispatching-parallel-agents` implementation while the primary owns F Runtime/capture integration. TDD; no UE processes in this subtask.

**Goal:** Shared PowerShell helpers bind a verification step and its archive manifest to one validated clean Git revision. This is G's foundation, not completion of all release gates.

**Files owned by subtask:** `Scripts/VerificationContract.ps1`, this audit's `verify-verification-contract.ps1`, and `verification-contract-evidence.md`. Do not alter existing launch/package/aggregate scripts or Runtime files yet.

**Interfaces:**

- `New-SeedForgeVerificationContext -ProjectRoot <path> [-ExpectedRevision <40-hex>]`: verify native Git success, exact HEAD, clean staged/unstaged/untracked status and return ProjectRoot, ExpectedRevision, TreeId and clean fingerprint.
- `Assert-SeedForgeVerificationContext -Context <context>`: fail if context/revision is malformed, HEAD/tree/fingerprint changed, native git fails, or non-ignored changes exist.
- `Invoke-SeedForgeVerifiedStep -Context <context> -Name <label> -Action <scriptblock>`: assert immediately before and in finally after the action; freeze the input expected identity across the action, preserve action errors, and do not swallow state-change failures. Integration will wrap each existing external process separately, preserving its established argument/log handling.
- `Assert-SeedForgeArtifactManifest -Context <context> -ManifestPath <json> [-ChecksumPath <path>]`: validate manifest sourceRevision, archive path/hash and adjacent checksum; independently compute SHA256 and require exact archive basename in checksum. Reject stale/wrong revision, missing archive/checksum, hash mismatches and escaped artifact paths. Return verified manifest data, never replace ExpectedRevision with current HEAD.

## Acceptance and scope

- [ ] RED harness with minimal artificial Git repositories only under `Artifacts/Reports/VerificationContract/<unique-id>`. No source clone, worktree or parallel project; fixture content is a sentinel text file and explicit ignored generated roots.
- [ ] Prove rejection of staged, unstaged and non-ignored untracked changes; HEAD movement; malformed/missing explicit revision; nonexistent/non-Git root/native git failure; changes created by an actual child PowerShell process during a verified step; context identity mutation.
- [ ] Prove a clean step succeeds and ignored generated outputs do not make the source dirty. Preserve native exit/error information without dumping sensitive environment data.
- [ ] Prove artifact success plus wrong manifest revision, wrong archive SHA, altered checksum, wrong checksum basename and missing files. Fixtures and synthetic archives are not release evidence.
- [ ] Implement minimum helpers, rerun full harness, parse source and inspect diff. Record exact commands and results, without staging/committing or touching other files.

The primary must later integrate these helpers before/after every real external verification process, derive actual BuildPlugin Editor/Game Development/Game Shipping proof, audit whole logs with exact justified allowances, rehash final artifacts/aggregate records, and complete all 16 gates. This subtask must not claim those integrations or release readiness.

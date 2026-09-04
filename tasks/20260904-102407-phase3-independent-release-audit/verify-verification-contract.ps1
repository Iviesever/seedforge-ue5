[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/VerificationContract.ps1')
$fixtureId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N')
$fixtureRoot = Join-Path $projectRoot "Artifacts/Reports/VerificationContract/$fixtureId"
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$results = [Collections.Generic.List[object]]::new()

function Invoke-FixtureGit([string]$Root, [string[]]$Arguments) {
    $output = @(& git -C $Root @Arguments 2>&1)
    if ($LASTEXITCODE -ne 0) { throw "Fixture git exit ${LASTEXITCODE}: $($output -join ' ')" }
    return ($output -join "`n").Trim()
}

function New-ContractFixture([string]$Name) {
    # Short case IDs keep Git's object paths within legacy Windows path limits.
    $casePath = Join-Path $fixtureRoot ('case-{0:D3}' -f $results.Count)
    $repo = Join-Path $casePath 'minimal-repo'
    New-Item -ItemType Directory -Path $repo | Out-Null
    & git init --object-format=sha1 --template= -q $repo
    if ($LASTEXITCODE -ne 0) { throw 'Fixture git init failed.' }
    Invoke-FixtureGit $repo @('config', 'user.name', 'SeedForge Synthetic Fixture') | Out-Null
    Invoke-FixtureGit $repo @('config', 'user.email', 'fixture@seedforge.invalid') | Out-Null
    Invoke-FixtureGit $repo @('config', 'commit.gpgsign', 'false') | Out-Null
    Invoke-FixtureGit $repo @('config', 'core.autocrlf', 'false') | Out-Null
    Invoke-FixtureGit $repo @('config', 'core.hooksPath', (Join-Path $repo '.git/disabled-hooks')) | Out-Null
    [IO.File]::WriteAllText((Join-Path $repo '.gitignore'), "Artifacts/`n.cache/`nArtifactsSibling/`n")
    [IO.File]::WriteAllText((Join-Path $repo 'sentinel.txt'), "SYNTHETIC TEST REPOSITORY; NOT A SOURCE COPY OR RELEASE.`n")
    Invoke-FixtureGit $repo @('add', '--', '.gitignore', 'sentinel.txt') | Out-Null
    Invoke-FixtureGit $repo @('commit', '-q', '-m', 'synthetic baseline') | Out-Null
    $revision = Invoke-FixtureGit $repo @('rev-parse', 'HEAD')
    $tree = Invoke-FixtureGit $repo @('rev-parse', 'HEAD^{tree}')
    $artifacts = Join-Path $repo 'Artifacts'
    New-Item -ItemType Directory -Path $artifacts | Out-Null
    $childPath = Join-Path $artifacts 'mutate-fixture.ps1'
    [IO.File]::WriteAllText($childPath, @'
param([string]$Root, [ValidateSet('source', 'head', 'assume-unchanged-source', 'skip-worktree-source')][string]$Mode)
$ErrorActionPreference = 'Stop'
if ($Mode -eq 'source') { [IO.File]::AppendAllText((Join-Path $Root 'sentinel.txt'), 'CHILD SOURCE MUTATION') }
elseif ($Mode -in @('assume-unchanged-source', 'skip-worktree-source')) {
    $flag = $Mode.Substring(0, $Mode.Length - '-source'.Length)
    & git -C $Root update-index "--$flag" -- sentinel.txt
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    [IO.File]::AppendAllText((Join-Path $Root 'sentinel.txt'), 'CHILD HIDDEN SOURCE MUTATION')
}
else {
    & git -C $Root commit --allow-empty -q -m 'synthetic child HEAD movement'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
'@)
    return [pscustomobject]@{ Root = $repo; CasePath = $casePath; Revision = $revision; Tree = $tree; Artifacts = $artifacts; ChildPath = $childPath }
}

function Invoke-ContractCase([string]$Name, [scriptblock]$Check) {
    $fixture = New-ContractFixture $Name
    try {
        $detail = @(& $Check $fixture) -join [Environment]::NewLine
        $results.Add([pscustomobject]@{ name = $Name; passed = $true; detail = $detail; fixture = $fixture.CasePath; initialRevision = $fixture.Revision; initialTree = $fixture.Tree })
        Write-Host "PASS $Name"
    }
    catch {
        $results.Add([pscustomobject]@{ name = $Name; passed = $false; detail = $_.Exception.Message; fixture = $fixture.CasePath; initialRevision = $fixture.Revision; initialTree = $fixture.Tree })
        Write-Host "FAIL $Name : $($_.Exception.Message)"
    }
}

function Assert-ContractRejected([scriptblock]$Action, [string[]]$Patterns = @()) {
    $reason = ''
    try { & $Action | Out-Null } catch { $reason = $_.Exception.Message }
    if (-not $reason) { throw 'Invalid verification evidence was accepted.' }
    foreach ($pattern in $Patterns) { if ($reason -notmatch $pattern) { throw "Missing required error '$pattern' in: $reason" } }
    return $reason
}

function Invoke-FixtureChild($Fixture, [string]$Mode) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Fixture.ChildPath -Root $Fixture.Root -Mode $Mode
    if ($LASTEXITCODE -ne 0) { throw "Synthetic child failed with exit $LASTEXITCODE." }
}

function Assert-FixtureHiddenIndexFlag($Fixture, [string]$Flag) {
    $tag = Invoke-FixtureGit $Fixture.Root @('ls-files', '-v', '--', 'sentinel.txt')
    $expected = if ($Flag -eq 'assume-unchanged') { 'h sentinel.txt' } else { 'S sentinel.txt' }
    if ($tag -cne $expected) { throw "Fixture flag was not retained: expected '$expected', got '$tag'." }
    $status = Invoke-FixtureGit $Fixture.Root @('status', '--porcelain=v1', '--untracked-files=all')
    if ($status) { throw "Fixture did not reproduce hidden clean status: $status" }
    return "Fixture ls-files -v: $tag; porcelain status is empty; flag retained."
}

function New-ArtifactFixture($Fixture, $Context) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $payload = Join-Path $Fixture.Artifacts 'payload'
    $release = Join-Path $Fixture.Artifacts 'Release'
    New-Item -ItemType Directory -Path $payload, $release | Out-Null
    [IO.File]::WriteAllText((Join-Path $payload 'SYNTHETIC.txt'), 'Synthetic archive; NOT SeedForge release evidence.')
    $archive = Join-Path $release 'synthetic evidence.zip'
    [IO.Compression.ZipFile]::CreateFromDirectory($payload, $archive)
    $sha = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    $checksum = $archive + '.sha256'
    [IO.File]::WriteAllText($checksum, "$sha  $([IO.Path]::GetFileName($archive))`r`n")
    $manifest = [ordered]@{ sourceRevision = $Fixture.Revision; archive = $archive; sha256 = $sha }
    $manifestPath = Join-Path $Fixture.Artifacts 'manifest.json'
    [IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json))
    return [pscustomobject]@{ Manifest = $manifest; ManifestPath = $manifestPath; Archive = $archive; Sha = $sha; Checksum = $checksum }
}

function Save-ArtifactFixture($Artifact) {
    [IO.File]::WriteAllText($Artifact.ManifestPath, ($Artifact.Manifest | ConvertTo-Json))
}

Invoke-ContractCase 'clean context captures exact revision tree and fingerprint' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    if ($context.ExpectedRevision -cne $f.Revision -or $context.TreeId -cne $f.Tree -or $context.CleanFingerprint -cnotmatch '^[0-9a-f]{64}$') { throw 'Context identity or clean fingerprint is incorrect.' }
    Assert-SeedForgeVerificationContext -Context $context
    $again = New-SeedForgeVerificationContext -ProjectRoot $f.Root -ExpectedRevision $f.Revision
    if ($again.CleanFingerprint -cne $context.CleanFingerprint) { throw 'Clean fingerprint is not stable.' }
}
foreach ($bad in @('', 'HEAD', 'abc', ('0' * 40), ('a' * 41))) {
    Invoke-ContractCase "explicit revision rejects length $($bad.Length) value $bad" { param($f) Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot $f.Root -ExpectedRevision $bad } }
}
Invoke-ContractCase 'missing explicit revision rejected' { param($f) Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot $f.Root -ExpectedRevision $null } }
Invoke-ContractCase 'nonexistent root reports native git failure' {
    param($f)
    Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot (Join-Path $f.CasePath 'missing') } @('Git', 'exit')
}
Invoke-ContractCase 'non-Git directory cannot inherit parent repository' {
    param($f)
    $root = Join-Path $f.CasePath 'not-a-repository'; New-Item -ItemType Directory -Path $root | Out-Null
    Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot $root }
}
foreach ($mode in @('unstaged', 'staged', 'untracked')) {
    Invoke-ContractCase "new context rejects $mode changes" {
        param($f)
        if ($mode -eq 'untracked') { [IO.File]::WriteAllText((Join-Path $f.Root 'unexpected.txt'), 'untracked') }
        else {
            [IO.File]::AppendAllText((Join-Path $f.Root 'sentinel.txt'), 'dirty')
            if ($mode -eq 'staged') { Invoke-FixtureGit $f.Root @('add', '--', 'sentinel.txt') | Out-Null }
        }
        Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot $f.Root }
    }
}
Invoke-ContractCase 'ignored generated outputs preserve clean identity' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    [IO.File]::WriteAllText((Join-Path $f.Artifacts 'generated.txt'), 'ignored output')
    Assert-SeedForgeVerificationContext -Context $context
}
foreach ($indexFlag in @('assume-unchanged', 'skip-worktree')) {
    Invoke-ContractCase "new context rejects preexisting $indexFlag flag" {
        param($f)
        Invoke-FixtureGit $f.Root @('update-index', "--$indexFlag", '--', 'sentinel.txt') | Out-Null
        Assert-FixtureHiddenIndexFlag $f $indexFlag
        Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot $f.Root }
        Assert-FixtureHiddenIndexFlag $f $indexFlag
    }
    Invoke-ContractCase "new context rejects hidden modification with $indexFlag" {
        param($f)
        Invoke-FixtureGit $f.Root @('update-index', "--$indexFlag", '--', 'sentinel.txt') | Out-Null
        [IO.File]::AppendAllText((Join-Path $f.Root 'sentinel.txt'), 'HIDDEN BEFORE CONTEXT')
        Assert-FixtureHiddenIndexFlag $f $indexFlag
        Assert-ContractRejected { New-SeedForgeVerificationContext -ProjectRoot $f.Root }
        Assert-FixtureHiddenIndexFlag $f $indexFlag
    }
    Invoke-ContractCase "verified step rejects introducing $indexFlag without file edit" {
        param($f)
        $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
        Assert-ContractRejected {
            Invoke-SeedForgeVerifiedStep -Context $context -Name "introduce $indexFlag" -Action {
                Invoke-FixtureGit $f.Root @('update-index', "--$indexFlag", '--', 'sentinel.txt') | Out-Null
            }
        }
        Assert-FixtureHiddenIndexFlag $f $indexFlag
    }
    Invoke-ContractCase "actual child hides modified source with $indexFlag during step" {
        param($f)
        $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
        Assert-ContractRejected {
            Invoke-SeedForgeVerifiedStep -Context $context -Name "hidden child $indexFlag" -Action { Invoke-FixtureChild $f "$indexFlag-source" }
        }
        Assert-FixtureHiddenIndexFlag $f $indexFlag
        if ((Get-Content -Raw -LiteralPath (Join-Path $f.Root 'sentinel.txt')) -notmatch 'CHILD HIDDEN SOURCE MUTATION') { throw 'Child did not modify the actual tracked sentinel.' }
    }
}
Invoke-ContractCase 'assert rejects native git config parse failure' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    [IO.File]::AppendAllText((Join-Path $f.Root '.git/config'), "`n[malformed")
    Assert-ContractRejected { Assert-SeedForgeVerificationContext -Context $context } @('Git', 'exit', 'config')
}
foreach ($field in @('ExpectedRevision', 'TreeId', 'CleanFingerprint', 'ProjectRoot')) {
    Invoke-ContractCase "assert rejects context mutation $field" {
        param($f)
        $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
        $context.$field = 'tampered'
        Assert-ContractRejected { Assert-SeedForgeVerificationContext -Context $context }
    }
}
Invoke-ContractCase 'assert rejects HEAD movement after clean capture' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    Invoke-FixtureGit $f.Root @('commit', '--allow-empty', '-q', '-m', 'synthetic HEAD movement') | Out-Null
    Assert-ContractRejected { Assert-SeedForgeVerificationContext -Context $context }
}
Invoke-ContractCase 'clean verified step preserves action result' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $result = Invoke-SeedForgeVerifiedStep -Context $context -Name 'clean action' -Action { 'ACTION_RESULT' }
    if ($result -cne 'ACTION_RESULT') { throw 'Action result was not preserved.' }
}
Invoke-ContractCase 'dirty before step prevents action execution' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    [IO.File]::AppendAllText((Join-Path $f.Root 'sentinel.txt'), 'dirty before')
    $marker = Join-Path $f.Artifacts 'must-not-run.txt'
    Assert-ContractRejected { Invoke-SeedForgeVerifiedStep -Context $context -Name 'precondition' -Action { [IO.File]::WriteAllText($marker, 'unexpected') } }
    if (Test-Path -LiteralPath $marker) { throw 'Action executed despite a failed precondition.' }
}
foreach ($mode in @('source', 'head')) {
    Invoke-ContractCase "actual child $mode mutation fails post-step" {
        param($f)
        $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
        Assert-ContractRejected { Invoke-SeedForgeVerifiedStep -Context $context -Name "child $mode" -Action { Invoke-FixtureChild $f $mode } }
    }
}
Invoke-ContractCase 'action error preserved when postcondition is clean' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    Assert-ContractRejected { Invoke-SeedForgeVerifiedStep -Context $context -Name 'action failure' -Action { throw 'ACTION_SENTINEL' } } @('ACTION_SENTINEL')
}
Invoke-ContractCase 'action and postcondition failures are both preserved' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    Assert-ContractRejected { Invoke-SeedForgeVerifiedStep -Context $context -Name 'dual failure' -Action { Invoke-FixtureChild $f 'source'; throw 'ACTION_SENTINEL' } } @('ACTION_SENTINEL', 'clean|dirty|changed')
}
Invoke-ContractCase 'mutable context cannot rebase expected identity during step' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    Assert-ContractRejected {
        Invoke-SeedForgeVerifiedStep -Context $context -Name 'identity mutation' -Action {
            Invoke-FixtureChild $f 'head'
            $replacement = New-SeedForgeVerificationContext -ProjectRoot $f.Root
            foreach ($field in @('ProjectRoot', 'ExpectedRevision', 'TreeId', 'CleanFingerprint')) { $context.$field = $replacement.$field }
        }
    }
}
Invoke-ContractCase 'context-only action mutation is rejected' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    Assert-ContractRejected { Invoke-SeedForgeVerifiedStep -Context $context -Name 'context-only mutation' -Action { $context.ExpectedRevision = '0' * 40 } }
}
Invoke-ContractCase 'valid synthetic archive manifest verifies independent hash' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $artifact = New-ArtifactFixture $f $context
    $result = Assert-SeedForgeArtifactManifest -Context $context -ManifestPath $artifact.ManifestPath
    if ($result.sourceRevision -cne $f.Revision -or $result.archive -ine $artifact.Archive -or $result.sha256 -cne $artifact.Sha) { throw 'Verified artifact identity is incorrect.' }
}
Invoke-ContractCase 'explicit adjacent checksum path is accepted' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $a = New-ArtifactFixture $f $context
    $alternate = Join-Path ([IO.Path]::GetDirectoryName($a.Archive)) 'explicit.sha256'
    Copy-Item -LiteralPath $a.Checksum -Destination $alternate
    $result = Assert-SeedForgeArtifactManifest -Context $context -ManifestPath $a.ManifestPath -ChecksumPath $alternate
    if ($result.ChecksumPath -ine $alternate -or $result.sha256 -cne $a.Sha) { throw 'Explicit adjacent checksum was not independently verified.' }
}
foreach ($mode in @('revision', 'hash', 'archive-bytes', 'checksum-hash', 'checksum-name', 'missing-archive', 'missing-checksum', 'missing-manifest', 'malformed-json', 'outside-artifacts', 'manifest-outside-artifacts', 'sibling-prefix', 'checksum-not-adjacent', 'reparse')) {
    Invoke-ContractCase "artifact rejects $mode" {
        param($f)
        $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
        $a = New-ArtifactFixture $f $context
        $explicitChecksum = $null
        switch ($mode) {
            'revision' { $a.Manifest.sourceRevision = '0' * 40; Save-ArtifactFixture $a }
            'hash' { $a.Manifest.sha256 = '0' * 64; Save-ArtifactFixture $a }
            'archive-bytes' { [IO.File]::AppendAllText($a.Archive, 'SYNTHETIC ARCHIVE MUTATION AFTER HASHING') }
            'checksum-hash' { [IO.File]::WriteAllText($a.Checksum, "$('0' * 64)  $([IO.Path]::GetFileName($a.Archive))`n") }
            'checksum-name' { [IO.File]::WriteAllText($a.Checksum, "$($a.Sha)  wrong.zip`n") }
            'missing-archive' { Move-Item -LiteralPath $a.Archive -Destination (Join-Path $f.Artifacts 'held-synthetic.zip') }
            'missing-checksum' { Move-Item -LiteralPath $a.Checksum -Destination (Join-Path $f.Artifacts 'held.sha256') }
            'missing-manifest' { Move-Item -LiteralPath $a.ManifestPath -Destination (Join-Path $f.Artifacts 'held-manifest.json') }
            'malformed-json' { [IO.File]::WriteAllText($a.ManifestPath, '{broken-json') }
            'outside-artifacts' {
                $cache = Join-Path $f.Root '.cache'; New-Item -ItemType Directory -Path $cache | Out-Null
                $escaped = Join-Path $cache 'escaped.zip'; Copy-Item -LiteralPath $a.Archive -Destination $escaped
                $a.Manifest.archive = $escaped; Save-ArtifactFixture $a
            }
            'manifest-outside-artifacts' {
                $cache = Join-Path $f.Root '.cache'; New-Item -ItemType Directory -Path $cache | Out-Null
                $escaped = Join-Path $cache 'escaped-manifest.json'; Copy-Item -LiteralPath $a.ManifestPath -Destination $escaped
                $a.ManifestPath = $escaped
            }
            'sibling-prefix' {
                $sibling = Join-Path $f.Root 'ArtifactsSibling'; New-Item -ItemType Directory -Path $sibling | Out-Null
                $escaped = Join-Path $sibling 'escaped.zip'; Copy-Item -LiteralPath $a.Archive -Destination $escaped
                $a.Manifest.archive = $escaped; Save-ArtifactFixture $a
            }
            'checksum-not-adjacent' { $explicitChecksum = Join-Path $f.Artifacts 'elsewhere.sha256'; Copy-Item -LiteralPath $a.Checksum -Destination $explicitChecksum }
            'reparse' {
                $link = Join-Path $f.Artifacts 'linked'; New-Item -ItemType Junction -Path $link -Target ([IO.Path]::GetDirectoryName($a.Archive)) | Out-Null
                $a.Manifest.archive = Join-Path $link ([IO.Path]::GetFileName($a.Archive)); Save-ArtifactFixture $a
            }
        }
        if ($explicitChecksum) { Assert-ContractRejected { Assert-SeedForgeArtifactManifest -Context $context -ManifestPath $a.ManifestPath -ChecksumPath $explicitChecksum } }
        else { Assert-ContractRejected { Assert-SeedForgeArtifactManifest -Context $context -ManifestPath $a.ManifestPath } }
    }
}

$passed = @($results | Where-Object { $_.passed }).Count
$failed = $results.Count - $passed
$summary = [ordered]@{
    purpose = 'Artificial sentinel Git repositories and synthetic archives; NOT clones, worktrees, integration copies or release proof.'
    fixtureDirectory = $fixtureRoot; powershellVersion = $PSVersionTable.PSVersion.ToString()
    verificationContractSha256 = (Get-FileHash -LiteralPath (Join-Path $projectRoot 'Scripts/VerificationContract.ps1') -Algorithm SHA256).Hash.ToLowerInvariant()
    total = $results.Count; passed = $passed; failed = $failed; results = @($results.ToArray())
}
[IO.File]::WriteAllText((Join-Path $fixtureRoot 'summary.json'), ($summary | ConvertTo-Json -Depth 6) + [Environment]::NewLine)
Write-Host "VERIFICATION_CONTRACT total=$($results.Count) passed=$passed failed=$failed fixtures=$fixtureRoot"
if ($failed -gt 0) { exit 1 }

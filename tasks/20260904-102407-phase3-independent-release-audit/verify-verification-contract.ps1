[CmdletBinding()]
param([string]$CaseFilter = '*')
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
    if ($Name -notlike $CaseFilter) { return }
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

Invoke-ContractCase 'script context clean default binds authoritative revision' {
    param($f)
    $context = New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{}
    if ($context.IsDiagnostic -or $context.SourceRevision -cne $f.Revision) { throw 'Default script context is not authoritative clean source.' }
    $result = Invoke-SeedForgeScriptStep -Context $context -Name 'clean step' -Action { 'ACTUAL_RESULT' }
    if ($result -cne 'ACTUAL_RESULT') { throw 'Script step lost action result.' }
}
Invoke-ContractCase 'script context diagnostic is explicit and cannot certify archives' {
    param($f)
    $clean = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $artifact = New-ArtifactFixture $f $clean
    $context = New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{ AllowDirtyDiagnostic=$true }
    if (-not $context.IsDiagnostic -or $context.SourceRevision -cne "diagnostic-$($f.Revision)") { throw 'Diagnostic identity is not explicit.' }
    Assert-ContractRejected { Assert-SeedForgeArtifactManifest -Context $context -ManifestPath $artifact.ManifestPath } @('diagnostic')
}
Invoke-ContractCase 'script context dirty default fails while explicit diagnostic executes' {
    param($f)
    [IO.File]::AppendAllText((Join-Path $f.Root 'sentinel.txt'),'SYNTHETIC DIRTY')
    Assert-ContractRejected { New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{} } @('not clean')
    $context = New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{ AllowDirtyDiagnostic=$true; ExpectedRevision=$f.Revision }
    if (-not $context.IsDiagnostic) { throw 'Dirty context is not diagnostic.' }
    if ((Invoke-SeedForgeScriptStep -Context $context -Name 'diagnostic' -Action { 42 }) -ne 42) { throw 'Diagnostic action did not execute.' }
}
Invoke-ContractCase 'script context diagnostic still rejects invalid expected revision' {
    param($f)
    Assert-ContractRejected { New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{ AllowDirtyDiagnostic=$true; ExpectedRevision='abc' } } @('40-hex')
}
Invoke-ContractCase 'script context step detects real source change' {
    param($f)
    $context = New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{}
    Assert-ContractRejected { Invoke-SeedForgeScriptStep -Context $context -Name 'actual child' -Action { Invoke-FixtureChild $f 'source' } } @('not clean')
}
Invoke-ContractCase 'script context diagnostic step still rejects HEAD movement' {
    param($f)
    $context = New-SeedForgeScriptContext -ProjectRoot $f.Root -Parameters @{ AllowDirtyDiagnostic=$true }
    Assert-ContractRejected { Invoke-SeedForgeScriptStep -Context $context -Name 'diagnostic child' -Action { Invoke-FixtureChild $f 'head' } } @('HEAD changed')
}

Invoke-ContractCase 'evidence index rehashes each owned file and itself' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file = Join-Path $f.Artifacts 'evidence.txt'; [IO.File]::WriteAllText($file,'SYNTHETIC EVIDENCE')
    $path = Join-Path $f.Artifacts 'index.json'
    $expected = @(Get-SeedForgeEvidenceDigest $f.Root $file)
    $written = Write-SeedForgeEvidenceIndex -Context $context -Path $path -Paths @($file) -ExpectedRecords $expected
    $checked = Assert-SeedForgeEvidenceIndex -Context $context -Path $path
    if ($checked.files.Count -ne 1 -or $checked.files[0].sha256 -cne (Get-FileHash $file -Algorithm SHA256).Hash.ToLowerInvariant()) { throw 'Evidence bytes were not independently hashed.' }
    if (-not (Test-Path -LiteralPath "$path.sha256")) { throw 'Index checksum missing.' }
}
foreach ($mode in @('changed-payload','changed-index','missing-payload','duplicate-path','escaped-path')) {
    Invoke-ContractCase "evidence index rejects $mode" {
        param($f)
        $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
        $file = Join-Path $f.Artifacts 'evidence.txt'; [IO.File]::WriteAllText($file,'SYNTHETIC EVIDENCE')
        $path = Join-Path $f.Artifacts 'index.json'
        $expected = @(Get-SeedForgeEvidenceDigest $f.Root $file)
        Write-SeedForgeEvidenceIndex -Context $context -Path $path -Paths @($file) -ExpectedRecords $expected | Out-Null
        switch ($mode) {
            'changed-payload' { [IO.File]::AppendAllText($file,'changed') }
            'changed-index' { [IO.File]::AppendAllText($path,' ') }
            'missing-payload' { Move-Item -LiteralPath $file -Destination (Join-Path $f.Artifacts 'held.txt') }
            'duplicate-path' { Assert-ContractRejected { Write-SeedForgeEvidenceIndex -Context $context -Path $path -Paths @($file,$file) -ExpectedRecords $expected } @('duplicate'); return }
            'escaped-path' { Assert-ContractRejected { Write-SeedForgeEvidenceIndex -Context $context -Path $path -Paths @((Join-Path $f.Root 'sentinel.txt')) -ExpectedRecords $expected } @('escapes'); return }
        }
        Assert-ContractRejected { Assert-SeedForgeEvidenceIndex -Context $context -Path $path }
    }
}

Invoke-ContractCase 'snapshot validates all supplied child digest pairs and nested package logs' {
    param($f)
    $context = New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $pairs = @(@('Path','Sha256'),@('reportIndex','reportSha256'),@('trace','traceSha256'),@('archive','sha256'),@('ConsoleLog','ConsoleLogSha256'),@('Log','LogSha256'))
    $children = @()
    for($i=0;$i -lt $pairs.Count;++$i){
        $file=Join-Path $f.Artifacts "child-$i.txt"; [IO.File]::WriteAllText($file,"SYNTHETIC child $i")
        $digest=Get-SeedForgeEvidenceDigest $f.Root $file
        $child=[ordered]@{}; $child[$pairs[$i][0]]=$file; $child[$pairs[$i][1]]=$digest.sha256
        if($i -eq 0){$child.Length=$digest.size}
        $children += [pscustomobject]$child
    }
    $snapshot=@(Get-SeedForgeEvidenceSnapshot -Context $context -Evidence @{ gameplay=@{package=@{buildLogProof=$children}} })
    if($snapshot.Count -ne 6){throw 'Snapshot omitted nested child evidence.'}
    Assert-SeedForgeEvidenceRecords -Context $context -Records $snapshot
}
Invoke-ContractCase 'snapshot accepts a no-output audit step without inventing evidence' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    if(@(Get-SeedForgeEvidenceSnapshot -Context $context -Evidence $null).Count -ne 0){throw 'No-output step invented a record.'}
}
foreach($pair in @(@('Path','Sha256'),@('reportIndex','reportSha256'),@('trace','traceSha256'),@('archive','sha256'),@('ConsoleLog','ConsoleLogSha256'),@('Log','LogSha256'))){
    Invoke-ContractCase "snapshot rejects false child digest $($pair[1])" {
        param($f)
        $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
        $file=Join-Path $f.Artifacts 'child.txt';[IO.File]::WriteAllText($file,'SYNTHETIC child')
        $child=[ordered]@{};$child[$pair[0]]=$file;$child[$pair[1]]='0'*64
        Assert-ContractRejected { Get-SeedForgeEvidenceSnapshot -Context $context -Evidence $child } @('digest|hash|SHA')
    }
}
Invoke-ContractCase 'snapshot rejects child length mismatch' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'child.txt';[IO.File]::WriteAllText($file,'SYNTHETIC child')
    $digest=Get-SeedForgeEvidenceDigest $f.Root $file
    Assert-ContractRejected { Get-SeedForgeEvidenceSnapshot -Context $context -Evidence @{Path=$file;Sha256=$digest.sha256;Length=1} } @('size|length')
}
Invoke-ContractCase 'snapshot rejects conflicting Length and size aliases' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'child.txt';[IO.File]::WriteAllText($file,'SYNTHETIC child')
    $digest=Get-SeedForgeEvidenceDigest $f.Root $file
    Assert-ContractRejected { Get-SeedForgeEvidenceSnapshot -Context $context -Evidence @{Path=$file;Sha256=$digest.sha256;Length=$digest.size;size=1} } @('size|length')
}
Invoke-ContractCase 'snapshot rejects fractional expected size instead of coercing it' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'child.txt';[IO.File]::WriteAllText($file,'SYNTHETIC child')
    $digest=Get-SeedForgeEvidenceDigest $f.Root $file
    Assert-ContractRejected { Assert-SeedForgeEvidenceRecords -Context $context -Records @(@{path=$file;sha256=$digest.sha256;size=1.5}) } @('size|integer')
}
Invoke-ContractCase 'snapshot includes index self hash and keeps non-evidence executable metadata separate' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'index-fixture.txt';[IO.File]::WriteAllText($file,'SYNTHETIC index')
    $digest=Get-SeedForgeEvidenceDigest $f.Root $file
    $snapshot=@(Get-SeedForgeEvidenceSnapshot -Context $context -Evidence @{IndexPath=$file;Sha256=$digest.sha256;executable='C:\readonly-engine\UnrealEditor.exe';ProjectRoot=$f.Root})
    if($snapshot.Count -ne 1 -or $snapshot[0].sha256 -cne $digest.sha256){throw 'Index self hash was omitted.'}
    Assert-ContractRejected { Get-SeedForgeEvidenceSnapshot -Context $context -Evidence @{IndexPath=$file;Sha256='0'*64} } @('hash|digest')
}
Invoke-ContractCase 'omitted ExpectedRecords fails before any index is created' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'child.txt';[IO.File]::WriteAllText($file,'SYNTHETIC child')
    $path=Join-Path $f.Artifacts 'must-not-create.json'
    Assert-ContractRejected { Write-SeedForgeEvidenceIndex -Context $context -Path $path -Paths @($file) } @('ExpectedRecords')
    if(Test-Path -LiteralPath $path){throw 'Unbound index was created.'}
}
Invoke-ContractCase 'snapshot records cannot mutate or follow mutable child objects' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'child.txt';[IO.File]::WriteAllText($file,'SYNTHETIC child')
    $digest=Get-SeedForgeEvidenceDigest $f.Root $file
    $child=[pscustomobject]@{Path=$file;Sha256=$digest.sha256;Length=$digest.size}
    $snapshot=@(Get-SeedForgeEvidenceSnapshot -Context $context -Evidence @($child,$file,$child))
    if($snapshot.Count -ne 1){throw 'Snapshot did not coalesce consistent graph references.'}
    $child.Sha256='0'*64
    if($snapshot[0].sha256 -cne $digest.sha256){throw 'Snapshot retained a mutable child alias.'}
    $blocked=$false;try{$snapshot[0].sha256='1'*64}catch{$blocked=$true}
    if(-not $blocked){throw 'Snapshot record was mutable.'}
}
Invoke-ContractCase 'tamper between successful step digest and index seal is rejected' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'passed-log.txt';[IO.File]::WriteAllText($file,'SYNTHETIC passed log')
    $expected=@(Get-SeedForgeEvidenceDigest $f.Root $file)
    [IO.File]::AppendAllText($file,"`nLogFoo: Error: changed after passed step")
    $path=Join-Path $f.Artifacts 'must-not-seal.json'
    Assert-ContractRejected { Write-SeedForgeEvidenceIndex -Context $context -Path $path -Paths @($file) -ExpectedRecords $expected } @('digest|hash|size|changed')
    if(Test-Path -LiteralPath $path){throw 'Tampered payload was published before rejection.'}
}
Invoke-ContractCase 'tamper after machine-index check cannot be rebased into final index' {
    param($f)
    $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
    $file=Join-Path $f.Artifacts 'passed-report.txt';[IO.File]::WriteAllText($file,'SYNTHETIC passed report')
    $machinePath=Join-Path $f.Artifacts 'machine-index.json'
    $initial=@(Get-SeedForgeEvidenceDigest $f.Root $file)
    Write-SeedForgeEvidenceIndex -Context $context -Path $machinePath -Paths @($file) -ExpectedRecords $initial | Out-Null
    $machine=Assert-SeedForgeEvidenceIndex -Context $context -Path $machinePath
    $frozen=@($machine.files)+@(Get-SeedForgeEvidenceDigest $f.Root $machinePath)+@(Get-SeedForgeEvidenceDigest $f.Root "$machinePath.sha256")
    [IO.File]::AppendAllText($file,'CHANGED AFTER MACHINE CHECK')
    $finalPath=Join-Path $f.Artifacts 'must-not-finalize.json'
    Assert-ContractRejected { Write-SeedForgeEvidenceIndex -Context $context -Path $finalPath -Paths @($frozen | ForEach-Object path) -ExpectedRecords $frozen } @('digest|hash|size|changed')
    if(Test-Path -LiteralPath $finalPath){throw 'Final seal silently rebased a changed machine payload.'}
}
foreach($mode in @('missing-expectations','duplicate-expectations','conflicting-expectations','missing-expected-path','extra-expected-path','missing-file','false-hash')){
    Invoke-ContractCase "index frozen expectations reject $mode" {
        param($f)
        $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
        $file=Join-Path $f.Artifacts 'one.txt';[IO.File]::WriteAllText($file,'SYNTHETIC one')
        $other=Join-Path $f.Artifacts 'two.txt';[IO.File]::WriteAllText($other,'SYNTHETIC two')
        $expected=@(Get-SeedForgeEvidenceDigest $f.Root $file);$paths=@($file)
        switch($mode){
            'missing-expectations' {$expected=@()}
            'duplicate-expectations' {$expected=@($expected[0],$expected[0])}
            'conflicting-expectations' {$expected += [pscustomobject]@{path=$file;size=1;sha256='0'*64}}
            'missing-expected-path' {$paths += $other}
            'extra-expected-path' {$expected += Get-SeedForgeEvidenceDigest $f.Root $other}
            'missing-file' {Move-Item -LiteralPath $file -Destination (Join-Path $f.Artifacts 'held.txt')}
            'false-hash' {$expected[0].sha256='0'*64}
        }
        Assert-ContractRejected { Write-SeedForgeEvidenceIndex -Context $context -Path (Join-Path $f.Artifacts 'index.json') -Paths $paths -ExpectedRecords $expected }
    }
}
foreach($mode in @('outside','reparse','missing','missing-hash-path','cyclic','false-paired-claim')){
    Invoke-ContractCase "snapshot graph rejects $mode" {
        param($f)
        $context=New-SeedForgeVerificationContext -ProjectRoot $f.Root
        $file=Join-Path $f.Artifacts 'one.txt';[IO.File]::WriteAllText($file,'SYNTHETIC one')
        $digest=Get-SeedForgeEvidenceDigest $f.Root $file
        $value=@{Path=$file;Sha256=$digest.sha256}
        switch($mode){
            'outside' {$value.Path=Join-Path $f.Root 'sentinel.txt'}
            'reparse' {$dir=Join-Path $f.Artifacts 'linked';New-Item -ItemType Junction -Path $dir -Target $f.Artifacts|Out-Null;$value.Path=Join-Path $dir 'one.txt'}
            'missing' {$value.Path=Join-Path $f.Artifacts 'missing.txt'}
            'missing-hash-path' {$value=@{reportSha256=$digest.sha256}}
            'cyclic' {$value.self=$value}
            'false-paired-claim' {$value=@(@{Path=$file;Sha256=$digest.sha256},@{Path=$file;Sha256='0'*64})}
        }
        Assert-ContractRejected { Get-SeedForgeEvidenceSnapshot -Context $context -Evidence $value }
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

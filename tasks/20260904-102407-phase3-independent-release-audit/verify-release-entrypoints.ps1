[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$runRoot = Join-Path $projectRoot ('Artifacts/Reports/ReleaseEntrypoints/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runRoot | Out-Null
$entrypoints = @('Build','Test','Smoke','Report','CaptureDemo','CaptureInspector','CaptureGameplay','TestGameplay','TestInputSelfTest','TestRunFailure','PackagePlugin','PackageDemo','PackageGameplay','VerifyAll','VerifyPhase3','FinalizeRelease')
$results = [Collections.Generic.List[object]]::new()

function Invoke-FixtureGit([string]$Root, [string[]]$Arguments) {
    $output = @(& git -C $Root @Arguments 2>&1)
    if ($LASTEXITCODE -ne 0) { throw "Fixture git failed: $($output -join ' ')" }
    return ($output -join "`n").Trim()
}

foreach ($entrypoint in $entrypoints) {
    foreach ($case in @('staged','unstaged','untracked','invalid-revision','wrong-revision','git-failure')) {
        $caseRoot = Join-Path $runRoot ('c{0:D3}' -f $results.Count)
        $repo = Join-Path $caseRoot 'fixture'
        $scriptRoot = Join-Path $repo 'Scripts'
        New-Item -ItemType Directory -Path $scriptRoot | Out-Null
        # Script-only preflight fixtures, not a project/clone/worktree or UE integration copy.
        Copy-Item -Path (Join-Path $projectRoot 'Scripts/*.ps1') -Destination $scriptRoot
        [IO.File]::WriteAllText((Join-Path $repo '.gitignore'), "Artifacts/`n.user/`n.cache/`n")
        [IO.File]::WriteAllText((Join-Path $repo 'sentinel.txt'), "SYNTHETIC PREFLIGHT ONLY`n")
        & git init --object-format=sha1 --template= -q $repo
        if ($LASTEXITCODE -ne 0) { throw 'Fixture init failed.' }
        Invoke-FixtureGit $repo @('config','user.name','SeedForge Preflight Fixture') | Out-Null
        Invoke-FixtureGit $repo @('config','user.email','fixture@seedforge.invalid') | Out-Null
        Invoke-FixtureGit $repo @('config','commit.gpgsign','false') | Out-Null
        Invoke-FixtureGit $repo @('config','core.autocrlf','false') | Out-Null
        Invoke-FixtureGit $repo @('config','core.hooksPath',(Join-Path $repo '.git/disabled-hooks')) | Out-Null
        Invoke-FixtureGit $repo @('add','--all') | Out-Null
        Invoke-FixtureGit $repo @('commit','-q','-m','synthetic script preflight') | Out-Null
        $revision = Invoke-FixtureGit $repo @('rev-parse','HEAD')
        $expectedMessage = 'Verification source is not clean'
        switch ($case) {
            'staged' { [IO.File]::AppendAllText((Join-Path $repo 'sentinel.txt'),'changed'); Invoke-FixtureGit $repo @('add','sentinel.txt') | Out-Null }
            'unstaged' { [IO.File]::AppendAllText((Join-Path $repo 'sentinel.txt'),'changed') }
            'untracked' { [IO.File]::WriteAllText((Join-Path $repo 'untracked.txt'),'changed') }
            'invalid-revision' { $revision='abc'; $expectedMessage='complete 40-hex' }
            'wrong-revision' { $revision='0000000000000000000000000000000000000000'; $expectedMessage='differs from expected revision' }
            'git-failure' { [IO.File]::AppendAllText((Join-Path $repo '.git/config'),"`n[invalid syntax`n"); $expectedMessage='Git command' }
        }
        $arguments = @{ ExpectedRevision=$revision }
        if ($entrypoint -ne 'FinalizeRelease') { $arguments.EngineRoot = Join-Path $repo 'Artifacts/NO-ENGINE-PRESENT' }
        $message = ''; $passed = $false
        try { & (Join-Path $scriptRoot "$entrypoint.ps1") @arguments | Out-Null; $message='Entry point accepted invalid source.' }
        catch { $message=$_.Exception.Message; $passed=$message.Contains($expectedMessage) }
        $results.Add([pscustomobject]@{ entrypoint=$entrypoint; case=$case; passed=$passed; message=$message })
        Write-Host "$(if($passed){'PASS'}else{'FAIL'}) $entrypoint $case"
    }
}
$failed = @($results | Where-Object { -not $_.passed }).Count
$summary = [ordered]@{ fixtureKind='script-only preflight, no UE runtime present'; runRoot=$runRoot; total=$results.Count; passed=$results.Count-$failed; failed=$failed; cases=$results }
[IO.File]::WriteAllText((Join-Path $runRoot 'summary.json'),($summary | ConvertTo-Json -Depth 6))
Write-Host "RELEASE_ENTRYPOINTS total=$($results.Count) failed=$failed output=$runRoot"
if ($failed -ne 0) { exit 1 }

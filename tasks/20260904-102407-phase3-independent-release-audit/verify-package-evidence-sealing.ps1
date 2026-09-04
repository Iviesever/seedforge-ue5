[CmdletBinding()]
param([string]$SourceScript)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$projectRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if([string]::IsNullOrWhiteSpace($SourceScript)){$SourceScript=Join-Path $projectRoot 'Scripts/PackageGameplay.ps1'}
$runRoot=Join-Path $projectRoot ('Artifacts/Reports/PackageEvidenceSealing/'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runRoot | Out-Null
$results=New-Object 'Collections.Generic.List[object]'
function Invoke-FixtureGit([string]$Root,[string[]]$Arguments){$result=@(& git -C $Root @Arguments 2>&1);if($LASTEXITCODE -ne 0){throw "Synthetic Git fixture failed: $result"};return ($result -join "`n").Trim()}
$common=@'
function Artifact([string]$Name,[string]$Content='SYNTHETIC ONLY - NOT UE EVIDENCE') {
    $path=Join-Path (Split-Path -Parent $PSScriptRoot) "Artifacts/$Name"
    [IO.File]::WriteAllText($path,$Content); return $path
}
function Hash([string]$Path){return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Tamper([string]$Phase){
    $root=Split-Path -Parent $PSScriptRoot
    $mode=Get-Content -Raw -LiteralPath (Join-Path $root 'mode.txt')
    $target=switch($Phase){
        'input' {if($mode -eq 'package-summary'){'package.json'}}
        'gameplay' {switch($mode){'input-summary'{'input-summary.json'};'input-log'{'input.log'};'input-trace'{'input-trace.json'}}}
        'Grid' {switch($mode){'gameplay-summary'{'gameplay-summary.json'};'gameplay-log'{'gameplay.log'};'gameplay-capture'{'gameplay-capture.txt'}}}
        'Encounter' {if($mode -eq 'negative-observation'){'Grid-observation.json'}}
    }
    if($null -ne $target){[IO.File]::AppendAllText((Join-Path $root "Artifacts/$target"),"`n ")}
}
function Result([string]$Name,[string]$Revision){
    $trace=Artifact "$Name-trace.json" '{"fixture":"synthetic"}'
    $log=Artifact "$Name.log"
    $summary=Artifact "$Name-summary.json" '{"fixture":"synthetic"}'
    $capture=Artifact "$Name-capture.txt"
    return [pscustomobject]@{sourceRevision=$Revision;result='Passed';runLabel='packaged';seed='24301';layoutHash='1';encounterHash='2';trace=$trace;traceSha256=(Hash $trace);log=$log;logProof=@{Path=$log;Sha256=(Hash $log)};summaryPath=$summary;screenshots=@($capture);validatedScreenshots=@(@{Path=$capture;Sha256=(Hash $capture)})}
}
'@
$packageStub=@'
param($EngineRoot,$SmokeSeed,$SmokeTimeoutSeconds,$ExpectedRevision)
. (Join-Path $PSScriptRoot 'Fixture.ps1')
$root=Split-Path -Parent $PSScriptRoot
$archive=Artifact 'synthetic-package.zip'; $digest=Hash $archive
$null=Artifact 'synthetic-package.zip.sha256' "$digest  synthetic-package.zip`r`n"
$log=Artifact 'cook.log';$executable=Artifact 'synthetic-executable.txt';$capture=Artifact 'ordinary-capture.txt'
$manifest=Join-Path $root 'Artifacts/package.json'
$result=[pscustomobject]@{sourceRevision=$ExpectedRevision;version='0.3.0';result='Passed';archive=$archive;sha256=$digest;executable=$executable;packageDirectory=(Join-Path $root 'Artifacts/Package');smokeLog=$log;smokeCapture=$capture;buildLogProof=@(@{Path=$log;Sha256=(Hash $log)});ManifestPath=$manifest}
[IO.File]::WriteAllText($manifest,($result|ConvertTo-Json -Depth 6));return $result
'@
$inputStub=@'
param($EngineRoot,$Seed,$Executable,$RunLabel,$ExpectedRevision)
. (Join-Path $PSScriptRoot 'Fixture.ps1')
Tamper 'input'; return Result 'input' $ExpectedRevision
'@
$gameplayStub=@'
param($EngineRoot,$Seed,$TimeoutSeconds,$Executable,$RunLabel,$ExpectedRevision)
. (Join-Path $PSScriptRoot 'Fixture.ps1')
Tamper 'gameplay'; return Result 'gameplay' $ExpectedRevision
'@
$negativeStub=@'
param($EngineRoot,$Executable,$Case,$ExpectedRevision)
. (Join-Path $PSScriptRoot 'Fixture.ps1')
Tamper $Case
$observation=Artifact "$Case-observation.json" '{"fixture":"synthetic"}'
$trace=Artifact "$Case-trace.json" '{"fixture":"synthetic"}'
$log=Artifact "$Case.log"
return [pscustomobject]@{sourceRevision=$ExpectedRevision;result='ExpectedFailure';observation=$observation;trace=$trace;traceSha256=(Hash $trace);log=$log;logProof=@{Path=$log;Sha256=(Hash $log)}}
'@
foreach($mode in @('control','package-summary','input-summary','input-log','input-trace','gameplay-summary','gameplay-log','gameplay-capture','negative-observation')){
    $root=Join-Path $runRoot ('c{0:D2}' -f $results.Count)
    $scripts=Join-Path $root 'Scripts';$artifacts=Join-Path $root 'Artifacts'
    New-Item -ItemType Directory -Path $scripts,$artifacts,(Join-Path $artifacts 'Package') | Out-Null
    # Only two real scripts plus synthetic producer stubs: no UE project,
    # source, clone, worktree, binaries or second integration checkout.
    Copy-Item -LiteralPath (Join-Path $projectRoot 'Scripts/VerificationContract.ps1') -Destination $scripts
    Copy-Item -LiteralPath $SourceScript -Destination (Join-Path $scripts 'PackageGameplay.ps1')
    $stubs=@{'Fixture'=$common;'PackageDemo'=$packageStub;'TestInputSelfTest'=$inputStub;'TestGameplay'=$gameplayStub;'TestRunFailure'=$negativeStub}
    foreach($name in $stubs.Keys){[IO.File]::WriteAllText((Join-Path $scripts "$name.ps1"),$stubs[$name])}
    [IO.File]::WriteAllText((Join-Path $root '.gitignore'),"Artifacts/`n")
    [IO.File]::WriteAllText((Join-Path $root 'mode.txt'),$mode)
    Invoke-FixtureGit $root @('init','--object-format=sha1','--template=','-q') | Out-Null
    Invoke-FixtureGit $root @('config','user.name','SeedForge Synthetic Fixture') | Out-Null
    Invoke-FixtureGit $root @('config','user.email','fixture@seedforge.invalid') | Out-Null
    Invoke-FixtureGit $root @('config','commit.gpgsign','false') | Out-Null
    Invoke-FixtureGit $root @('config','core.autocrlf','false') | Out-Null
    Invoke-FixtureGit $root @('config','core.hooksPath',(Join-Path $root '.git/disabled-hooks')) | Out-Null
    Invoke-FixtureGit $root @('add','--all') | Out-Null
    Invoke-FixtureGit $root @('commit','-q','-m','synthetic package orchestration fixture') | Out-Null
    $revision=Invoke-FixtureGit $root @('rev-parse','HEAD')
    $accepted=$false;$message=''
    try {& (Join-Path $scripts 'PackageGameplay.ps1') -ExpectedRevision $revision -EngineRoot (Join-Path $artifacts 'NO-ENGINE') | Out-Null;$accepted=$true}
    catch {$message=$_.Exception.Message}
    $published=@(Get-ChildItem -LiteralPath (Join-Path $artifacts 'Package') -Filter 'gameplay-package-*.json').Count
    $passed=if($mode -eq 'control'){$accepted -and $published -eq 1}else{-not $accepted -and $published -eq 0 -and $message -match 'digest|hash|size|changed'}
    $results.Add([pscustomobject]@{mode=$mode;passed=$passed;accepted=$accepted;published=$published;message=$message})
    Write-Host "$(if($passed){'PASS'}else{'FAIL'}) $mode accepted=$accepted published=$published $message"
}
$failed=@($results|Where-Object {-not $_.passed}).Count
$summary=[ordered]@{SyntheticOnly=$true;purpose='Actual PackageGameplay orchestration and shared helper with synthetic child producers; no UE or release evidence';sourceScript=$SourceScript;sourceScriptSha256=(Get-FileHash -LiteralPath $SourceScript -Algorithm SHA256).Hash.ToLowerInvariant();total=$results.Count;passed=$results.Count-$failed;failed=$failed;cases=$results.ToArray()}
[IO.File]::WriteAllText((Join-Path $runRoot 'summary.json'),($summary|ConvertTo-Json -Depth 8))
Write-Host "PACKAGE_EVIDENCE_SEALING $($results.Count-$failed)/$($results.Count) $runRoot"
if($failed -ne 0){exit 1}

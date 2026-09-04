[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [string]$ExpectedRevision
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$revision = $verificationContext.ExpectedRevision
$stamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$runRoot = Join-Path $projectRoot "Artifacts/Reports/Phase3Verification/$stamp"
New-Item -ItemType Directory -Path $runRoot | Out-Null
$steps = [Collections.Generic.List[object]]::new()

function Invoke-Phase3Step([string]$Name,[scriptblock]$Action) {
    $started = [DateTimeOffset]::UtcNow
    Write-Host "Verifying: $Name"
    try {
        $result = Invoke-SeedForgeScriptStep -Context $verificationContext -Name $Name -Action $Action
        $frozenEvidence = if ($null -ne $result) { @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence $result) } else { @() }
        $steps.Add([pscustomobject]@{name=$Name; startedAtUtc=$started.ToString('o'); completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o'); result='Passed'; evidence=$result; frozenEvidence=$frozenEvidence})
        return $result
    }
    catch {
        $failure = [ordered]@{sourceRevision=$revision; result='Failed'; step=$Name; message=$_.Exception.Message; completedSteps=$steps.ToArray()}
        [IO.File]::WriteAllText((Join-Path $runRoot 'failure.json'),($failure | ConvertTo-Json -Depth 14))
        throw
    }
}
Invoke-Phase3Step 'Repository audit' { & (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean } | Out-Null
$build = Invoke-Phase3Step 'Editor Development build' { & (Join-Path $PSScriptRoot 'Build.ps1') -EngineRoot $EngineRoot -ExpectedRevision $revision }
$load = Invoke-Phase3Step 'Headless load smoke' { & (Join-Path $PSScriptRoot 'Smoke.ps1') -EngineRoot $EngineRoot -ExpectedRevision $revision }
$automation = Invoke-Phase3Step 'Complete Automation' { & (Join-Path $PSScriptRoot 'Test.ps1') -EngineRoot $EngineRoot -Filter SeedForge -TimeoutSeconds 900 -ExpectedRevision $revision }
$focused = @(foreach ($filter in @('SeedForge.Gameplay','SeedForge.GameplaySmoke','SeedForge.Model.Path','SeedForge.Model.Encounter','SeedForge.Model.RunState','SeedForge.Audit')) {
    Invoke-Phase3Step "Focused $filter" { & (Join-Path $PSScriptRoot 'Test.ps1') -EngineRoot $EngineRoot -Filter $filter -ExpectedRevision $revision }
})
$phase2 = Invoke-Phase3Step 'Canonical layout, diff and 10k benchmark' { & (Join-Path $PSScriptRoot 'Report.ps1') -EngineRoot $EngineRoot -SeedCount 10000 -Warmup 100 -ExpectedRevision $revision }
$editorInput = Invoke-Phase3Step 'Editor ordinary input, movement and restarts' { & (Join-Path $PSScriptRoot 'TestInputSelfTest.ps1') -EngineRoot $EngineRoot -Seed 24301 -RunLabel editor -ExpectedRevision $revision }
$editorGameplay = Invoke-Phase3Step 'Editor gameplay and rendered captures' { & (Join-Path $PSScriptRoot 'TestGameplay.ps1') -EngineRoot $EngineRoot -Seed 24301 -ExpectedRevision $revision }
$editorNegative = @(foreach ($case in @('Grid','Encounter','CapturePath','RenderUnavailable')) {
    Invoke-Phase3Step "Editor failure $case" { & (Join-Path $PSScriptRoot 'TestRunFailure.ps1') -EngineRoot $EngineRoot -Case $case -ExpectedRevision $revision }
})
$inspector = Invoke-Phase3Step 'Inspector materialization' { & (Join-Path $PSScriptRoot 'CaptureInspector.ps1') -EngineRoot $EngineRoot -Seed 24301 -ExpectedRevision $revision }
$plugin = Invoke-Phase3Step 'Actual BuildPlugin target matrix' { & (Join-Path $PSScriptRoot 'PackagePlugin.ps1') -EngineRoot $EngineRoot -ExpectedRevision $revision }
$gameplay = Invoke-Phase3Step 'Win64 build, ordinary input, gameplay, failures and captures' { & (Join-Path $PSScriptRoot 'PackageGameplay.ps1') -EngineRoot $EngineRoot -Seed 24301 -ExpectedRevision $revision }
$plugin = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $plugin.ManifestPath
$gameplay = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $gameplay.ManifestPath
Invoke-Phase3Step 'Final repository audit' { & (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean } | Out-Null

$version = (Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Plugins/SeedForge/SeedForge.uplugin') | ConvertFrom-Json).VersionName
if ($version -cne '0.3.0') { throw 'Verification requires version 0.3.0.' }
$summaryPath = Join-Path $runRoot 'summary.json'
$summary = [ordered]@{
    schema='seedforge.phase3-verification'; schemaVersion=1; sourceRevision=$revision; version=$version
    result='MachinePassed'; visualReview='Pending'; remoteReview='Pending'
    completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o')
    automation=$automation; focused=$focused; phase2=$phase2; editorInput=$editorInput; editorGameplay=$editorGameplay
    editorNegative=$editorNegative; inspector=$inspector; plugin=$plugin; gameplay=$gameplay; steps=$steps.ToArray()
    summaryPath=$summaryPath
}
Assert-SeedForgeScriptContext -Context $verificationContext
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 16))
$expectedRecords = @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence $summary)
$indexPath = Join-Path $runRoot 'evidence-index.json'
Write-SeedForgeEvidenceIndex -Context $verificationContext -Path $indexPath -Paths @($expectedRecords | ForEach-Object { $_.path }) -ExpectedRecords $expectedRecords | Out-Null
Assert-SeedForgeScriptContext -Context $verificationContext
Copy-Item -LiteralPath $summaryPath -Destination (Join-Path $projectRoot 'Artifacts/Reports/phase3-verification-last.json')
Assert-SeedForgeScriptContext -Context $verificationContext
$summary.evidenceIndex = $indexPath
Write-Host "Machine gates passed for $revision. Visual inspection and fresh remote review remain required."
Write-Host "Summary: $summaryPath"
return [pscustomobject]$summary

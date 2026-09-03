[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$reportRoot = Join-Path $projectRoot 'Artifacts\Reports'
New-Item -ItemType Directory -Force -Path $reportRoot | Out-Null

function Invoke-VerificationStep {
    param(
        [Parameter(Mandatory)]
        [string]$Name,
        [Parameter(Mandatory)]
        [scriptblock]$Action
    )

    $started = Get-Date
    Write-Host "===== $Name ====="
    & $Action
    $elapsed = (Get-Date) - $started
    Write-Host ("===== {0} PASSED in {1:n2}s =====" -f $Name, $elapsed.TotalSeconds)
}

Invoke-VerificationStep 'Repository audit' {
    & (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean
}
Invoke-VerificationStep 'UE 5.8 Editor build' {
    & (Join-Path $PSScriptRoot 'Build.ps1') -EngineRoot $EngineRoot
}
Invoke-VerificationStep 'Headless project/plugin smoke' {
    & (Join-Path $PSScriptRoot 'Smoke.ps1') -EngineRoot $EngineRoot
}
Invoke-VerificationStep 'Complete UE Automation suite' {
    & (Join-Path $PSScriptRoot 'Test.ps1') -EngineRoot $EngineRoot -Filter 'SeedForge' -TimeoutSeconds 900
}
Invoke-VerificationStep 'Editor-game runtime capture' {
    & (Join-Path $PSScriptRoot 'CaptureDemo.ps1') -EngineRoot $EngineRoot -Seed 24301
}
Invoke-VerificationStep 'Editor Inspector visual capture' {
    & (Join-Path $PSScriptRoot 'CaptureInspector.ps1') -EngineRoot $EngineRoot -Seed 24301
}
Invoke-VerificationStep 'Canonical documents, diff, and 10k benchmark report' {
    & (Join-Path $PSScriptRoot 'Report.ps1') -EngineRoot $EngineRoot -SeedCount 10000 -Warmup 100
}
Invoke-VerificationStep 'Independent plugin package' {
    & (Join-Path $PSScriptRoot 'PackagePlugin.ps1') -EngineRoot $EngineRoot
}
Invoke-VerificationStep 'Win64 package and packaged smoke' {
    & (Join-Path $PSScriptRoot 'PackageDemo.ps1') -EngineRoot $EngineRoot -SmokeSeed 24301
}

$revision = (git -C $projectRoot rev-parse HEAD).Trim()
$version = (Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin') | ConvertFrom-Json).VersionName
$phase2Summary = Get-ChildItem -File -Recurse -LiteralPath (Join-Path $projectRoot 'Artifacts\Reports\Phase2') -Filter 'report-summary.json' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $phase2Summary) {
    throw 'Phase 2 report summary is missing after Report.ps1.'
}
$phase2Report = Get-Content -Raw -LiteralPath $phase2Summary.FullName | ConvertFrom-Json
if ($phase2Report.sourceRevision -ne $revision) {
    throw "Phase 2 report summary does not prove current revision $revision."
}
$automationReport = Get-ChildItem -Directory -LiteralPath (Join-Path $projectRoot 'Artifacts\Reports') -Filter 'automation-*' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $automationReport) {
    throw 'UE Automation report is missing after Test.ps1.'
}
$editorCapture = Join-Path $projectRoot 'Artifacts\Media\SeedForge-24301.png'
$inspectorCapture = Join-Path $projectRoot 'Artifacts\Media\SeedForge-Inspector-24301.png'
$packagedCapture = Join-Path $projectRoot 'Artifacts\Media\SeedForge-Packaged-24301.png'
foreach ($capture in @($editorCapture, $inspectorCapture, $packagedCapture)) {
    if (-not (Test-Path -LiteralPath $capture)) {
        throw "Required verification capture is missing: '$capture'."
    }
}
$summary = [ordered]@{
    completedAt = (Get-Date).ToString('o')
    sourceRevision = $revision
    version = $version
    engine = 'Unreal Engine 5.8.0'
    platform = 'Win64'
    result = 'Passed'
    gates = @(
        'Repository audit',
        'Editor build',
        'Headless smoke',
        'UE Automation',
        'Editor runtime capture',
        'Editor Inspector visual capture',
        'Canonical JSON / Layout Diff / 10k benchmark report',
        'BuildPlugin',
        'BuildCookRun',
        'Packaged executable smoke'
    )
    phase2ReportSummary = $phase2Summary.FullName
    automationReport = $automationReport.FullName
    editorCapture = $editorCapture
    inspectorCapture = $inspectorCapture
    packagedCapture = $packagedCapture
}
$summaryPath = Join-Path $reportRoot 'verification-last.json'
[System.IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 5) + "`r`n")

Write-Host "All SeedForge verification gates passed for revision $revision."
Write-Host "Summary: $summaryPath"

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
Invoke-VerificationStep 'Independent plugin package' {
    & (Join-Path $PSScriptRoot 'PackagePlugin.ps1') -EngineRoot $EngineRoot
}
Invoke-VerificationStep 'Win64 package and packaged smoke' {
    & (Join-Path $PSScriptRoot 'PackageDemo.ps1') -EngineRoot $EngineRoot -SmokeSeed 24301
}

$revision = (git -C $projectRoot rev-parse HEAD).Trim()
$summary = [ordered]@{
    completedAt = (Get-Date).ToString('o')
    sourceRevision = $revision
    engine = 'Unreal Engine 5.8.0'
    platform = 'Win64'
    result = 'Passed'
    gates = @(
        'Repository audit',
        'Editor build',
        'Headless smoke',
        'UE Automation',
        'Editor runtime capture',
        'BuildPlugin',
        'BuildCookRun',
        'Packaged executable smoke'
    )
}
$summaryPath = Join-Path $reportRoot 'verification-last.json'
[System.IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 5) + "`r`n")

Write-Host "All SeedForge verification gates passed for revision $revision."
Write-Host "Summary: $summaryPath"


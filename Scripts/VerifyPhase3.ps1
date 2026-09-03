[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$reportRoot = Join-Path $projectRoot 'Artifacts\Reports'
New-Item -ItemType Directory -Force -Path $reportRoot | Out-Null

function Invoke-Phase3Step {
    param(
        [Parameter(Mandatory)]
        [string]$Name,
        [Parameter(Mandatory)]
        [scriptblock]$Action
    )

    $started = Get-Date
    Write-Host "===== $Name ====="
    & $Action
    Write-Host ("===== {0} PASSED in {1:n2}s =====" -f $Name, ((Get-Date) - $started).TotalSeconds)
}

Invoke-Phase3Step 'Repository audit' {
    & (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean
}
Invoke-Phase3Step 'UE 5.8 Editor build' {
    & (Join-Path $PSScriptRoot 'Build.ps1') -EngineRoot $EngineRoot
}
Invoke-Phase3Step 'Headless project/plugin smoke' {
    & (Join-Path $PSScriptRoot 'Smoke.ps1') -EngineRoot $EngineRoot
}
Invoke-Phase3Step 'Complete UE Automation suite' {
    & (Join-Path $PSScriptRoot 'Test.ps1') -EngineRoot $EngineRoot -Filter SeedForge -TimeoutSeconds 900
}
Invoke-Phase3Step 'Editor gameplay smoke and captures' {
    & (Join-Path $PSScriptRoot 'TestGameplay.ps1') -EngineRoot $EngineRoot -Seed 24301 -TimeoutSeconds 180
}
Invoke-Phase3Step 'Editor Inspector visual capture' {
    & (Join-Path $PSScriptRoot 'CaptureInspector.ps1') -EngineRoot $EngineRoot -Seed 24301
}
Invoke-Phase3Step 'Canonical layout/diff/benchmark evidence' {
    & (Join-Path $PSScriptRoot 'Report.ps1') -EngineRoot $EngineRoot -SeedCount 10000 -Warmup 100
}
Invoke-Phase3Step 'Independent Runtime plugin package' {
    & (Join-Path $PSScriptRoot 'PackagePlugin.ps1') -EngineRoot $EngineRoot
}
Invoke-Phase3Step 'Win64 package, ordinary launch, and gameplay smoke' {
    & (Join-Path $PSScriptRoot 'PackageGameplay.ps1') -EngineRoot $EngineRoot -Seed 24301 -TimeoutSeconds 300
}

$revision = (git -C $projectRoot rev-parse HEAD).Trim()
$version = (Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin') | ConvertFrom-Json).VersionName
$automationReport = Get-ChildItem -Directory -LiteralPath $reportRoot -Filter 'automation-*' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
$gameplayPackagePath = Join-Path $projectRoot 'Artifacts\Package\last-gameplay-package.json'
$pluginPackagePath = Join-Path $projectRoot 'Artifacts\Plugin\last-plugin-package.json'
if (-not $automationReport -or -not (Test-Path -LiteralPath $gameplayPackagePath) -or -not (Test-Path -LiteralPath $pluginPackagePath)) {
    throw 'Phase 3 verification is missing an Automation, gameplay package, or plugin package manifest.'
}
$automation = Get-Content -Raw -LiteralPath (Join-Path $automationReport.FullName 'index.json') | ConvertFrom-Json
$gameplayPackage = Get-Content -Raw -LiteralPath $gameplayPackagePath | ConvertFrom-Json
$pluginPackage = Get-Content -Raw -LiteralPath $pluginPackagePath | ConvertFrom-Json
if ($gameplayPackage.sourceRevision -ne $revision -or $pluginPackage.sourceRevision -ne $revision) {
    throw 'Phase 3 package manifests do not prove the current revision.'
}

$summary = [ordered]@{
    completedAt = (Get-Date).ToString('o')
    sourceRevision = $revision
    version = $version
    engine = 'Unreal Engine 5.8.0'
    platform = 'Win64'
    result = 'Passed'
    automation = [ordered]@{
        report = $automationReport.FullName
        passed = [int]$automation.succeeded
        warnings = [int]$automation.succeededWithWarnings
        failed = [int]$automation.failed
        notRun = [int]$automation.notRun
        inProcess = [int]$automation.inProcess
    }
    pluginPackage = $pluginPackagePath
    gameplayPackage = $gameplayPackagePath
    gameplayTrace = $gameplayPackage.gameplayTrace
    gameplayScreenshots = $gameplayPackage.gameplayScreenshots
    gates = @(
        'Repository audit',
        'Editor Development build',
        'Headless load smoke',
        'Complete SeedForge Automation',
        'Editor gameplay smoke',
        'Editor Inspector capture',
        'Canonical JSON / Layout Diff / 10k benchmark',
        'BuildPlugin Editor/Game Development/Game Shipping',
        'Win64 Build/Cook/Stage/Pak/Archive',
        'Ordinary packaged executable launch',
        'Packaged gameplay smoke / trace / captures'
    )
}
$summaryPath = Join-Path $reportRoot 'phase3-verification-last.json'
[IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 7) + "`r`n")

Write-Host "All SeedForge Phase 3 verification gates passed for revision $revision."
Write-Host "Summary: $summaryPath"

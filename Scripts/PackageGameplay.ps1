[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$packageManifestPath = Join-Path $projectRoot 'Artifacts\Package\last-package.json'
$gameplayManifestPath = Join-Path $projectRoot 'Artifacts\Package\last-gameplay-package.json'
$gameplayReportRoot = Join-Path $projectRoot 'Artifacts\Reports\Gameplay'

& (Join-Path $PSScriptRoot 'PackageDemo.ps1') `
    -EngineRoot $EngineRoot `
    -SmokeSeed $Seed `
    -SmokeTimeoutSeconds $TimeoutSeconds

if (-not (Test-Path -LiteralPath $packageManifestPath)) {
    throw "PackageDemo produced no manifest at '$packageManifestPath'."
}
$package = Get-Content -Raw -LiteralPath $packageManifestPath | ConvertFrom-Json
$revision = (git -C $projectRoot rev-parse HEAD).Trim()
if ($package.sourceRevision -ne $revision) {
    throw "Package manifest revision '$($package.sourceRevision)' does not match '$revision'."
}
if (-not (Test-Path -LiteralPath $package.executable)) {
    throw "Package manifest executable is missing: '$($package.executable)'."
}

$smokeStarted = Get-Date
& (Join-Path $PSScriptRoot 'TestGameplay.ps1') `
    -EngineRoot $EngineRoot `
    -Seed $Seed `
    -TimeoutSeconds $TimeoutSeconds `
    -Executable $package.executable `
    -RunLabel packaged

$gameplaySummary = Get-ChildItem -File -Recurse -LiteralPath $gameplayReportRoot -Filter 'summary.json' |
    Where-Object { $_.LastWriteTime -ge $smokeStarted } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $gameplaySummary) {
    throw 'Packaged gameplay smoke produced no fresh summary.'
}
$smoke = Get-Content -Raw -LiteralPath $gameplaySummary.FullName | ConvertFrom-Json
if ($smoke.runLabel -ne 'packaged' -or $smoke.result -ne 'Passed' -or $smoke.sourceRevision -ne $revision) {
    throw 'Fresh gameplay summary does not prove the packaged executable at the current revision.'
}

$manifest = [ordered]@{
    completedAt = (Get-Date).ToString('o')
    sourceRevision = $revision
    version = $package.version
    seed = $smoke.seed
    layoutHash = $smoke.layoutHash
    encounterHash = $smoke.encounterHash
    result = 'Passed'
    ordinaryLaunchLog = $package.smokeLog
    ordinaryLaunchCapture = $package.smokeCapture
    executable = $package.executable
    packageDirectory = $package.packageDirectory
    archive = $package.archive
    archiveSha256 = $package.sha256
    gameplayTrace = $smoke.trace
    gameplaySummary = $gameplaySummary.FullName
    gameplayScreenshots = $smoke.screenshots
    gameplayLog = $smoke.log
}
[IO.File]::WriteAllText($gameplayManifestPath, ($manifest | ConvertTo-Json -Depth 6) + "`r`n")

Write-Host 'SeedForge Win64 package, ordinary launch, and packaged gameplay smoke passed.'
Write-Host "Manifest: $gameplayManifestPath"
Write-Host "Executable: $($package.executable)"
Write-Host "Trace: $($smoke.trace)"
Write-Host "Screenshots: $($smoke.screenshots -join ', ')"

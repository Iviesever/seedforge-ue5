[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [string]$Filter = 'SeedForge',
    [int]$TimeoutSeconds = 600
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$reportRoot = Join-Path $projectRoot 'Artifacts\Reports'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $reportRoot, $cacheRoot, $userRoot | Out-Null

if (-not (Test-Path -LiteralPath $editor)) {
    throw "UnrealEditor-Cmd.exe was not found at '$editor'."
}

Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $cacheRoot
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logPath = Join-Path $logRoot "automation-$timestamp.log"
$reportPath = Join-Path $reportRoot "automation-$timestamp"
$arguments = @(
    $projectFile,
    "-ExecCmds=`"Automation RunTests $Filter`"",
    '-TestExit="Automation Test Queue Empty"',
    "-ReportExportPath=$reportPath",
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nullrhi',
    '-nosound',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    $process.Kill($true)
    throw "Automation timed out after $TimeoutSeconds seconds. See '$logPath'."
}

if ($process.ExitCode -ne 0) {
    throw "Automation failed with exit code $($process.ExitCode). See '$logPath' and '$reportPath'."
}

$reportIndex = Join-Path $reportPath 'index.json'
if (-not (Test-Path -LiteralPath $reportIndex)) {
    throw "Automation produced no machine-readable report. See '$logPath'."
}

$report = Get-Content -Raw -LiteralPath $reportIndex | ConvertFrom-Json
$executedCount = [int]$report.succeeded + [int]$report.succeededWithWarnings + [int]$report.failed
if ($executedCount -eq 0) {
    throw "Automation discovered no executable tests for '$Filter'. See '$logPath' and '$reportPath'."
}

if ([int]$report.failed -ne 0 -or [int]$report.notRun -ne 0 -or [int]$report.inProcess -ne 0) {
    throw "Automation did not pass cleanly: succeeded=$($report.succeeded), warnings=$($report.succeededWithWarnings), failed=$($report.failed), notRun=$($report.notRun), inProcess=$($report.inProcess). See '$logPath' and '$reportPath'."
}

if ([int]$report.succeededWithWarnings -ne 0) {
    throw "Automation passed with warnings; SeedForge requires clean tests. See '$logPath' and '$reportPath'."
}

Write-Host "Automation passed $($report.succeeded) test(s) for '$Filter'. Log: $logPath Report: $reportPath"

[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [string]$Filter = 'SeedForge',
    [int]$TimeoutSeconds = 600,
    [string]$ExpectedRevision,
    [switch]$AllowDirtyDiagnostic
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
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

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
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
    '-culture=en',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$arguments += @(Get-SeedForgeRuntimeArguments -ProjectRoot $projectRoot)
$startedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Automation Editor process' -Action {
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "Automation timed out after $TimeoutSeconds seconds. See '$logPath'."
    }
    if ($process.ExitCode -ne 0) {
        throw "Automation failed with exit code $($process.ExitCode). See '$logPath' and '$reportPath'."
    }
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

. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof = Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment
. (Join-Path $PSScriptRoot 'RuntimeStorageValidation.ps1')
$storageProof = Assert-SeedForgeRuntimeStorage -Path $logPath -ProjectRoot $projectRoot
Assert-SeedForgeScriptContext -Context $verificationContext
$summaryPath = Join-Path $reportPath 'verification-summary.json'
$summary = [pscustomobject]@{
    sourceRevision=$verificationContext.SourceRevision; result=$(if($AllowDirtyDiagnostic){'DiagnosticPassed'}else{'Passed'})
    startedAtUtc=$startedAtUtc.ToString('o'); completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o')
    filter=$Filter; exitCode=0; report=$reportPath; reportIndex=$reportIndex
    passed=[int]$report.succeeded; warnings=[int]$report.succeededWithWarnings; failed=[int]$report.failed
    notRun=[int]$report.notRun; inProcess=[int]$report.inProcess
    reportSha256=(Get-FileHash -LiteralPath $reportIndex -Algorithm SHA256).Hash.ToLowerInvariant()
    log=$logProof; storageProof=$storageProof; summaryPath=$summaryPath
}
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 7))
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "Automation passed $($report.succeeded) test(s) for '$Filter'. Log: $logPath Report: $reportPath"
return $summary

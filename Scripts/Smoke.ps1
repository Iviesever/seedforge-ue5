[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $cacheRoot, $userRoot | Out-Null

if (-not (Test-Path -LiteralPath $editor)) {
    throw "UnrealEditor-Cmd.exe was not found at '$editor'."
}

Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $cacheRoot
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logPath = Join-Path $logRoot "smoke-editor-$timestamp.log"
$arguments = @(
    $projectFile,
    '-run=CompileAllBlueprints',
    '-AllowListFile=Scripts/SmokeBlueprintAllowList.txt',
    '-ShowResultsOnly',
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
    throw "UnrealEditor-Cmd smoke timed out after $TimeoutSeconds seconds. See '$logPath'."
}

if ($process.ExitCode -ne 0) {
    throw "UnrealEditor-Cmd smoke failed with exit code $($process.ExitCode). See '$logPath'."
}

$fatalPattern = 'Fatal error:|Assertion failed:|LogPluginManager: Error|Missing or incompatible module'
if (Select-String -LiteralPath $logPath -Pattern $fatalPattern -Quiet) {
    throw "UnrealEditor-Cmd smoke log contains a fatal marker. See '$logPath'."
}

if (-not (Select-String -LiteralPath $logPath -Pattern 'Success - 0 error\(s\), 0 warning\(s\)' -Quiet)) {
    throw "UnrealEditor-Cmd smoke log is missing the commandlet success marker. See '$logPath'."
}

Write-Host "UnrealEditor-Cmd smoke passed. Log: $logPath"

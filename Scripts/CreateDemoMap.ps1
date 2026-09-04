[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [int]$TimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$pythonScript = Join-Path $PSScriptRoot 'CreateDemoMap.py'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $cacheRoot, $userRoot | Out-Null
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logPath = Join-Path $logRoot "create-demo-map-$timestamp.log"
$arguments = @(
    $projectFile,
    '-run=pythonscript',
    "-script=$pythonScript",
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
    throw "Demo map generation timed out after $TimeoutSeconds seconds. See '$logPath'."
}
if ($process.ExitCode -ne 0) {
    throw "Demo map generation failed with exit code $($process.ExitCode). See '$logPath'."
}
if (-not (Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_MAP_READY /Game/Maps/SeedForgeDemo' -Quiet)) {
    throw "Demo map generation log is missing its success marker. See '$logPath'."
}

$mapFile = Join-Path $projectRoot 'Content\Maps\SeedForgeDemo.umap'
if (-not (Test-Path -LiteralPath $mapFile)) {
    throw "Demo map was not created at '$mapFile'."
}

Write-Host "Demo map generated. Map: $mapFile Log: $logPath"

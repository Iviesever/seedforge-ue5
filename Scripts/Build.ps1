[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [ValidateSet('Development', 'DebugGame')]
    [string]$Configuration = 'Development'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $cacheRoot, $userRoot | Out-Null

if (-not (Test-Path -LiteralPath $buildScript)) {
    throw "Unreal Build.bat was not found at '$buildScript'."
}

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$consoleLogPath = Join-Path $logRoot "build-editor-$timestamp.log"
$ubtLogPath = Join-Path $logRoot "ubt-editor-$timestamp.log"
$arguments = @(
    'SeedForgeEditor',
    'Win64',
    $Configuration,
    "-Project=$projectFile",
    '-WaitMutex',
    '-NoHotReloadFromIDE',
    '-NoEngineChanges',
    '-UBADisableRemote',
    "-Log=$ubtLogPath"
)

& $buildScript @arguments 2>&1 | Tee-Object -FilePath $consoleLogPath
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "SeedForgeEditor build failed with exit code $exitCode. See '$consoleLogPath' and '$ubtLogPath'."
}

Write-Host "SeedForgeEditor build passed. Logs: $consoleLogPath and $ubtLogPath"

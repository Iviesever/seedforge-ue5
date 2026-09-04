[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$mediaRoot = Join-Path $projectRoot 'Artifacts\Media'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user\InspectorCapture'
$capturePath = Join-Path $mediaRoot "SeedForge-Inspector-$Seed.png"

New-Item -ItemType Directory -Force -Path $logRoot, $mediaRoot, $cacheRoot, $userRoot | Out-Null
if (-not (Test-Path -LiteralPath $editor)) {
    throw "UnrealEditor.exe was not found at '$editor'."
}

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
if (Test-Path -LiteralPath $capturePath) {
    Remove-Item -LiteralPath $capturePath
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logPath = Join-Path $logRoot "capture-inspector-$timestamp.log"
$arguments = @(
    $projectFile,
    '-RenderOffscreen',
    '-d3d11',
    '-windowed',
    '-ForceRes',
    '-ResX=1100',
    '-ResY=760',
    "-SeedForgeInspectorCapture=$capturePath",
    "-SeedForgeInspectorSeed=$Seed",
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nosound',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    $process.Kill($true)
    throw "Inspector capture timed out after $TimeoutSeconds seconds. See '$logPath'."
}
if ($process.ExitCode -ne 0) {
    throw "Inspector capture exited with code $($process.ExitCode). See '$logPath'."
}
if (-not (Select-String -LiteralPath $logPath -Pattern 'Inspector capture passed path=' -Quiet)) {
    throw "Inspector capture log has no success marker. See '$logPath'."
}
if (-not (Test-Path -LiteralPath $capturePath)) {
    throw "Inspector capture did not create '$capturePath'. See '$logPath'."
}
if ((Get-Item -LiteralPath $capturePath).Length -lt 5KB) {
    throw "Inspector capture is unexpectedly small: '$capturePath'."
}

Write-Host "SeedForge Inspector capture passed. Image: $capturePath Log: $logPath"

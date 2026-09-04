[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$mediaRoot = Join-Path $projectRoot 'Artifacts\Media'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $mediaRoot, $cacheRoot, $userRoot | Out-Null
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $cacheRoot

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logPath = Join-Path $logRoot "capture-demo-$timestamp.log"
$capturePath = Join-Path $mediaRoot "SeedForge-$Seed.png"
if (Test-Path -LiteralPath $capturePath) {
    Remove-Item -LiteralPath $capturePath
}

$arguments = @(
    $projectFile,
    '/Game/Maps/SeedForgeDemo',
    '-game',
    '-RenderOffscreen',
    '-windowed',
    '-ForceRes',
    '-ResX=1280',
    '-ResY=720',
    "-SeedForgeSeed=$Seed",
    "-SeedForgeCapturePath=$capturePath",
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
    throw "Demo capture timed out after $TimeoutSeconds seconds. See '$logPath'."
}
if ($process.ExitCode -ne 0) {
    throw "Demo capture failed with exit code $($process.ExitCode). See '$logPath'."
}
if (-not (Select-String -LiteralPath $logPath -Pattern 'Applied request=[1-9][0-9]* run=[1-9][0-9]* seed=[0-9]+ hash=[1-9][0-9]* floors=[1-9][0-9]* walls=[1-9][0-9]* gameplay=true\.$' -Quiet)) {
    throw "Demo capture log is missing the applied-layout marker. See '$logPath'."
}
if (-not (Test-Path -LiteralPath $capturePath)) {
    throw "Demo screenshot was not created at '$capturePath'. See '$logPath'."
}
if ((Get-Item -LiteralPath $capturePath).Length -lt 10KB) {
    throw "Demo screenshot is unexpectedly small. See '$capturePath' and '$logPath'."
}

Write-Host "Demo capture passed. Screenshot: $capturePath Log: $logPath"

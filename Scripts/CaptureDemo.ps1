[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 300,
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$mediaRoot = Join-Path $projectRoot 'Artifacts\Media'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $mediaRoot, $cacheRoot, $userRoot | Out-Null
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot

$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
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
    "-SeedForgeCaptureRoot=$mediaRoot",
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nosound',
    '-culture=en',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$arguments += @(Get-SeedForgeRuntimeArguments -ProjectRoot $projectRoot)
$captureStartedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Editor single-capture process' -Action {
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "Demo capture timed out after $TimeoutSeconds seconds. See '$logPath'."
    }
    if ($process.ExitCode -ne 0) {
        throw "Demo capture failed with exit code $($process.ExitCode). See '$logPath'."
    }
}
if (-not (Select-String -LiteralPath $logPath -Pattern 'Applied request=[1-9][0-9]* run=[1-9][0-9]* seed=[0-9]+ hash=[1-9][0-9]* floors=[1-9][0-9]* walls=[1-9][0-9]* gameplay=true\.$' -Quiet)) {
    throw "Demo capture log is missing the applied-layout marker. See '$logPath'."
}
. (Join-Path $PSScriptRoot 'PngValidation.ps1')
$pngProof = Assert-SeedForgePng -Path $capturePath -RunDirectory $mediaRoot -RequestedAtUtc $captureStartedAtUtc
if (-not (Select-String -LiteralPath $logPath -Pattern 'Gameplay capture completed token=[0-9a-f]{32} label=single .*bindings=0\.' -Quiet)) {
    throw 'Single capture has no clean native completion marker.'
}

. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof = Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment
. (Join-Path $PSScriptRoot 'RuntimeStorageValidation.ps1')
$storageProof = Assert-SeedForgeRuntimeStorage -Path $logPath -ProjectRoot $projectRoot
Assert-SeedForgeScriptContext -Context $verificationContext
$summaryPath = Join-Path $logRoot "capture-demo-$timestamp.json"
$summary = [pscustomobject]@{ sourceRevision=$verificationContext.SourceRevision; result='Passed'; exitCode=0; startedAtUtc=$captureStartedAtUtc.ToString('o'); completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o'); capture=$pngProof; log=$logProof; storageProof=$storageProof; summaryPath=$summaryPath }
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 7))
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "Demo capture passed. Screenshot: $capturePath Log: $logPath"
return $summary

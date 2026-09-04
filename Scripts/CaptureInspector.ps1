[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 180,
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

$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
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
    '-culture=en',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$startedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Inspector capture process' -Action {
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "Inspector capture timed out after $TimeoutSeconds seconds. See '$logPath'."
    }
    if ($process.ExitCode -ne 0) {
        throw "Inspector capture exited with code $($process.ExitCode). See '$logPath'."
    }
}
if (-not (Select-String -LiteralPath $logPath -Pattern 'Inspector capture passed path=' -Quiet)) {
    throw "Inspector capture log has no success marker. See '$logPath'."
}
. (Join-Path $PSScriptRoot 'LogValidation.ps1')
. (Join-Path $PSScriptRoot 'PngValidation.ps1')
$logProof = Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment
$receipts = @(Select-String -LiteralPath $logPath -Pattern "Inspector capture passed path='(?<path>[^']+)' width=(?<width>[0-9]+) height=(?<height>[0-9]+) bytes=(?<bytes>[0-9]+)\.$")
if ($receipts.Count -ne 1) { throw 'Inspector capture requires exactly one native pixel-save receipt.' }
$receipt = $receipts[0].Matches[0].Groups
if (-not [IO.Path]::GetFullPath($receipt['path'].Value).Equals([IO.Path]::GetFullPath($capturePath),[StringComparison]::OrdinalIgnoreCase)) { throw 'Inspector capture receipt path mismatch.' }
$width = [int]$receipt['width'].Value; $height = [int]$receipt['height'].Value
if ($width -lt 200 -or $height -lt 200 -or $width -gt 4096 -or $height -gt 4096) { throw 'Inspector widget capture dimensions are outside its supported bounds.' }
# This captures the Inspector widget, not the full requested window rectangle.
$pngProof = Assert-SeedForgePng -Path $capturePath -RunDirectory $mediaRoot -RequestedAtUtc $startedAtUtc -Width $width -Height $height -MinimumBytes 5120
if ($pngProof.Length -ne [long]$receipt['bytes'].Value) { throw 'Inspector saved bytes differ from its native receipt.' }
Assert-SeedForgeScriptContext -Context $verificationContext
$summaryPath = Join-Path $logRoot "capture-inspector-$timestamp.json"
$summary = [pscustomobject]@{ sourceRevision=$verificationContext.SourceRevision; result='Passed'; exitCode=0; startedAtUtc=$startedAtUtc.ToString('o'); completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o'); capture=$pngProof; log=$logProof; summaryPath=$summaryPath }
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 7))
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "SeedForge Inspector capture passed. Image: $capturePath Log: $logPath"
return $summary

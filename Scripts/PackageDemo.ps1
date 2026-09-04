[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$SmokeSeed = 24301,
    [int]$SmokeTimeoutSeconds = 300,
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$pluginFile = Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin'
$runUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$artifactRoot = Join-Path $projectRoot 'Artifacts'
$logRoot = Join-Path $artifactRoot 'Logs'
$packageRoot = Join-Path $artifactRoot 'Package'
$releaseRoot = Join-Path $artifactRoot 'Release'
$mediaRoot = Join-Path $artifactRoot 'Media'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$packageUserRoot = Join-Path $projectRoot '.user\PackagedDemo'

New-Item -ItemType Directory -Force -Path $logRoot, $packageRoot, $releaseRoot, $mediaRoot, $cacheRoot, $packageUserRoot | Out-Null
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot

if (-not (Test-Path -LiteralPath $runUat)) {
    throw "RunUAT.bat was not found at '$runUat'."
}
$version = (Get-Content -Raw -LiteralPath $pluginFile | ConvertFrom-Json).VersionName
if ([string]::IsNullOrWhiteSpace($version)) {
    throw "SeedForge.uplugin has no VersionName."
}

$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$packageDir = Join-Path $packageRoot "SeedForge-Win64-$timestamp"
$consoleLog = Join-Path $logRoot "package-demo-$timestamp.log"
$uatDiagnosticRoot = Join-Path $logRoot "uat-demo-$timestamp"
$uatEngineSavedRoot = Join-Path $uatDiagnosticRoot 'EngineSaved'
New-Item -ItemType Directory -Force -Path $uatDiagnosticRoot, $uatEngineSavedRoot | Out-Null
Set-Item -Path 'Env:uebp_LogFolder' -Value $uatDiagnosticRoot
Set-Item -Path 'Env:uebp_FinalLogFolder' -Value $uatDiagnosticRoot
Set-Item -Path 'Env:uebp_EngineSavedFolder' -Value $uatEngineSavedRoot
$arguments = @(
    'BuildCookRun',
    "-project=$projectFile",
    '-noP4',
    '-platform=Win64',
    '-clientconfig=Development',
    '-build',
    '-cook',
    '-map=/Game/Maps/SeedForgeDemo',
    '-stage',
    '-pak',
    '-archive',
    "-archivedirectory=$packageDir",
    '-unattended',
    '-utf8output',
    '-NoCodeSign'
    '-UbtArgs=-UBADisableRemote'
    '-AdditionalCookerOptions=-culture=en'
)

$startedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'BuildCookRun UAT process' -Action {
    & $runUat @arguments 2>&1 | Tee-Object -FilePath $consoleLog | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "BuildCookRun failed with exit code $LASTEXITCODE. See '$consoleLog'."
    }
}
. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$buildLogProof = @(Assert-SeedForgeLog -Path $consoleLog -AllowedWarnings UE58LocalEnvironment
    Get-ChildItem -LiteralPath $uatDiagnosticRoot -File -Recurse | Where-Object { $_.Extension -in @('.log','.txt') -and $_.Length -gt 0 } | ForEach-Object { Assert-SeedForgeLog -Path $_.FullName -AllowedWarnings UE58LocalEnvironment })

$executable = Join-Path $packageDir 'Windows/SeedForge.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "BuildCookRun succeeded without a SeedForge.exe under '$packageDir'."
}

$smokeLog = Join-Path $logRoot "smoke-packaged-$timestamp.log"
$mediaRoot = Join-Path $mediaRoot "PackagedSingle/$timestamp"
New-Item -ItemType Directory -Path $mediaRoot | Out-Null
$smokeCapture = Join-Path $mediaRoot "SeedForge-Packaged-$SmokeSeed.png"
$smokeArguments = @(
    '-RenderOffscreen',
    '-windowed',
    '-ForceRes',
    '-ResX=1280',
    '-ResY=720',
    "-SeedForgeSeed=$SmokeSeed",
    "-SeedForgeCapturePath=$smokeCapture",
    "-SeedForgeCaptureRoot=$mediaRoot",
    '-unattended',
    '-nosplash',
    '-nosound',
    '-culture=en',
    "-userdir=$packageUserRoot",
    "-abslog=$smokeLog"
)

$captureStartedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Ordinary packaged single-capture process' -Action {
    $process = Start-Process -FilePath $executable -ArgumentList $smokeArguments -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit($SmokeTimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "Packaged demo smoke timed out after $SmokeTimeoutSeconds seconds. See '$smokeLog'."
    }
    if ($process.ExitCode -ne 0) {
        throw "Packaged demo smoke failed with exit code $($process.ExitCode). See '$smokeLog'."
    }
}
$smokeLogProof = Assert-SeedForgeLog -Path $smokeLog -AllowedWarnings UE58LocalEnvironment
if (-not (Select-String -LiteralPath $smokeLog -Pattern 'Applied request=[1-9][0-9]* run=[1-9][0-9]* seed=[0-9]+ hash=[1-9][0-9]* floors=[1-9][0-9]* walls=[1-9][0-9]* gameplay=true\.$' -Quiet)) {
    throw "Packaged demo log is missing the applied-layout marker. See '$smokeLog'."
}
. (Join-Path $PSScriptRoot 'PngValidation.ps1')
$smokePngProof = Assert-SeedForgePng -Path $smokeCapture -RunDirectory $mediaRoot -RequestedAtUtc $captureStartedAtUtc
if (-not (Select-String -LiteralPath $smokeLog -Pattern 'Gameplay capture completed token=[0-9a-f]{32} label=single .*bindings=0\.' -Quiet)) {
    throw 'Packaged single capture has no clean native completion marker.'
}

$zipPath = Join-Path $releaseRoot "SeedForgeDemo-Win64-$version-$timestamp.zip"
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Win64 archive creation' -Action {
    Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
}
$hash = Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
$checksumPath = "$zipPath.sha256"
[System.IO.File]::WriteAllText($checksumPath, "$($hash.Hash.ToLowerInvariant())  $([System.IO.Path]::GetFileName($zipPath))`r`n")

$manifest = [ordered]@{
    createdAt = (Get-Date).ToString('o')
    sourceRevision = $verificationContext.ExpectedRevision
    version = $version
    packageDirectory = $packageDir
    executable = $executable
    smokeLog = $smokeLog
    smokeCapture = $smokeCapture
    smokePngProof = $smokePngProof
    archive = $zipPath
    sha256 = $hash.Hash.ToLowerInvariant()
    result = 'Passed'
    startedAtUtc = $startedAtUtc.ToString('o')
    buildLogProof = $buildLogProof
    smokeLogProof = $smokeLogProof
    consoleLog = $consoleLog
    diagnosticRoot = $uatDiagnosticRoot
}
$manifestPath = Join-Path $packageRoot "package-$timestamp.json"
Assert-SeedForgeScriptContext -Context $verificationContext
[System.IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 9) + "`r`n")
$verifiedManifest = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $manifestPath
Assert-SeedForgeScriptContext -Context $verificationContext
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $packageRoot 'last-package.json')
Assert-SeedForgeScriptContext -Context $verificationContext

Write-Host "SeedForge Win64 package and smoke passed."
Write-Host "Executable: $executable"
Write-Host "Screenshot: $smokeCapture"
Write-Host "Archive: $zipPath"
Write-Host "SHA256: $($hash.Hash.ToLowerInvariant())"
Write-Host "Manifest: $manifestPath"
return $verifiedManifest

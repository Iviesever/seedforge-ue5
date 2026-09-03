[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$SmokeSeed = 24301,
    [int]$SmokeTimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
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
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $cacheRoot

if (-not (Test-Path -LiteralPath $runUat)) {
    throw "RunUAT.bat was not found at '$runUat'."
}
$version = (Get-Content -Raw -LiteralPath $pluginFile | ConvertFrom-Json).VersionName
if ([string]::IsNullOrWhiteSpace($version)) {
    throw "SeedForge.uplugin has no VersionName."
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$packageDir = Join-Path $packageRoot "SeedForge-Win64-$timestamp"
$consoleLog = Join-Path $logRoot "package-demo-$timestamp.log"
$uatLog = Join-Path $logRoot "uat-package-demo-$timestamp.log"
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
    '-NoCodeSign',
    "-log=$uatLog"
)

& $runUat @arguments 2>&1 | Tee-Object -FilePath $consoleLog
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "BuildCookRun failed with exit code $exitCode. See '$consoleLog' and '$uatLog'."
}

$executable = Get-ChildItem -File -Recurse -LiteralPath $packageDir -Filter 'SeedForge.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if ([string]::IsNullOrWhiteSpace($executable)) {
    throw "BuildCookRun succeeded without a SeedForge.exe under '$packageDir'."
}

$smokeLog = Join-Path $logRoot "smoke-packaged-$timestamp.log"
$smokeCapture = Join-Path $mediaRoot "SeedForge-Packaged-$SmokeSeed.png"
if (Test-Path -LiteralPath $smokeCapture) {
    Remove-Item -LiteralPath $smokeCapture
}
$smokeArguments = @(
    '-RenderOffscreen',
    '-windowed',
    '-ForceRes',
    '-ResX=1280',
    '-ResY=720',
    "-SeedForgeSeed=$SmokeSeed",
    "-SeedForgeCapturePath=$smokeCapture",
    '-unattended',
    '-nosplash',
    '-nosound',
    "-userdir=$packageUserRoot",
    "-abslog=$smokeLog"
)

$process = Start-Process -FilePath $executable -ArgumentList $smokeArguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($SmokeTimeoutSeconds * 1000)) {
    $process.Kill($true)
    throw "Packaged demo smoke timed out after $SmokeTimeoutSeconds seconds. See '$smokeLog'."
}
if ($process.ExitCode -ne 0) {
    throw "Packaged demo smoke failed with exit code $($process.ExitCode). See '$smokeLog'."
}
if (-not (Select-String -LiteralPath $smokeLog -Pattern 'Applied request=.*hash=.*floors=.*walls=' -Quiet)) {
    throw "Packaged demo log is missing the applied-layout marker. See '$smokeLog'."
}
if (-not (Test-Path -LiteralPath $smokeCapture)) {
    throw "Packaged demo did not create '$smokeCapture'. See '$smokeLog'."
}
if ((Get-Item -LiteralPath $smokeCapture).Length -lt 10KB) {
    throw "Packaged demo screenshot is unexpectedly small: '$smokeCapture'."
}

$zipPath = Join-Path $releaseRoot "SeedForgeDemo-Win64-$version-$timestamp.zip"
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
$hash = Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
$checksumPath = "$zipPath.sha256"
[System.IO.File]::WriteAllText($checksumPath, "$($hash.Hash.ToLowerInvariant())  $([System.IO.Path]::GetFileName($zipPath))`r`n")

$manifest = [ordered]@{
    createdAt = (Get-Date).ToString('o')
    sourceRevision = (git -C $projectRoot rev-parse HEAD).Trim()
    version = $version
    packageDirectory = $packageDir
    executable = $executable
    smokeLog = $smokeLog
    smokeCapture = $smokeCapture
    archive = $zipPath
    sha256 = $hash.Hash.ToLowerInvariant()
}
$manifestPath = Join-Path $packageRoot 'last-package.json'
[System.IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 4) + "`r`n")

Write-Host "SeedForge Win64 package and smoke passed."
Write-Host "Executable: $executable"
Write-Host "Screenshot: $smokeCapture"
Write-Host "Archive: $zipPath"
Write-Host "SHA256: $($hash.Hash.ToLowerInvariant())"
Write-Host "Manifest: $manifestPath"

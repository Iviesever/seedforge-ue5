[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$pluginFile = Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin'
$runUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$artifactRoot = Join-Path $projectRoot 'Artifacts'
$logRoot = Join-Path $artifactRoot 'Logs'
$pluginRoot = Join-Path $artifactRoot 'Plugin'
$releaseRoot = Join-Path $artifactRoot 'Release'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'

New-Item -ItemType Directory -Force -Path $logRoot, $pluginRoot, $releaseRoot, $cacheRoot | Out-Null
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $cacheRoot

if (-not (Test-Path -LiteralPath $runUat)) {
    throw "RunUAT.bat was not found at '$runUat'."
}
if (-not (Test-Path -LiteralPath $pluginFile)) {
    throw "SeedForge.uplugin was not found at '$pluginFile'."
}
$version = (Get-Content -Raw -LiteralPath $pluginFile | ConvertFrom-Json).VersionName
if ([string]::IsNullOrWhiteSpace($version)) {
    throw "SeedForge.uplugin has no VersionName."
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$packageDir = Join-Path $pluginRoot "SeedForge-$timestamp"
$consoleLog = Join-Path $logRoot "package-plugin-$timestamp.log"

$arguments = @(
    'BuildPlugin',
    "-Plugin=$pluginFile",
    "-Package=$packageDir",
    '-TargetPlatforms=Win64',
    '-Rocket'
)

& $runUat @arguments 2>&1 | Tee-Object -FilePath $consoleLog
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "BuildPlugin failed with exit code $exitCode. See '$consoleLog'."
}

$packagedDescriptor = Join-Path $packageDir 'SeedForge.uplugin'
if (-not (Test-Path -LiteralPath $packagedDescriptor)) {
    throw "BuildPlugin succeeded without the expected descriptor '$packagedDescriptor'."
}

$zipPath = Join-Path $releaseRoot "SeedForgePlugin-$version-$timestamp.zip"
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
$hash = Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
$checksumPath = "$zipPath.sha256"
[System.IO.File]::WriteAllText($checksumPath, "$($hash.Hash.ToLowerInvariant())  $([System.IO.Path]::GetFileName($zipPath))`r`n")

$manifest = [ordered]@{
    createdAt = (Get-Date).ToString('o')
    sourceRevision = (git -C $projectRoot rev-parse HEAD).Trim()
    version = $version
    packageDirectory = $packageDir
    archive = $zipPath
    sha256 = $hash.Hash.ToLowerInvariant()
    verifiedTargets = @(
        'UnrealEditor Win64 Development'
        'UnrealGame Win64 Development'
        'UnrealGame Win64 Shipping'
    )
}
$manifestPath = Join-Path $pluginRoot 'last-plugin-package.json'
[System.IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 5) + "`r`n")

Write-Host "SeedForge plugin package passed."
Write-Host "Package: $packageDir"
Write-Host "Archive: $zipPath"
Write-Host "SHA256: $($hash.Hash.ToLowerInvariant())"
Write-Host "Manifest: $manifestPath"
Write-Host "Log: $consoleLog"

function Initialize-SeedForgeBuildEnvironment {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ProjectRoot)

    $resolvedProjectRoot = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedProjectRoot 'SeedForge.uproject') -PathType Leaf)) {
        throw 'Build environment requires the SeedForge project root.'
    }
    $ubaStorageRoot = Join-Path $resolvedProjectRoot '.cache\UnrealBuildAccelerator'
    $derivedDataRoot = Join-Path $resolvedProjectRoot '.cache\DerivedDataCache'
    $temporaryRoot = Join-Path $resolvedProjectRoot '.cache\Temp'
    New-Item -ItemType Directory -Force -Path $ubaStorageRoot, $derivedDataRoot, $temporaryRoot | Out-Null
    [Environment]::SetEnvironmentVariable('UBA_ROOT', $ubaStorageRoot, 'Process')
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $derivedDataRoot, 'Process')
    [Environment]::SetEnvironmentVariable('TEMP', $temporaryRoot, 'Process')
    [Environment]::SetEnvironmentVariable('TMP', $temporaryRoot, 'Process')
}

function Get-SeedForgeRuntimeArguments {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ProjectRoot)
    $root = [IO.Path]::GetFullPath($ProjectRoot)
    if (-not (Test-Path -LiteralPath (Join-Path $root 'SeedForge.uproject') -PathType Leaf)) {
        throw 'Runtime arguments require the SeedForge project root.'
    }
    return @('-DDC=SeedForgeLocal','-DDC-NoDefaultGraph',('-LocalDataCachePath='+(Join-Path $root '.cache/DerivedDataCache')))
}

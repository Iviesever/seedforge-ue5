function Initialize-SeedForgeBuildEnvironment {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ProjectRoot)

    $resolvedProjectRoot = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
    if (-not (Test-Path -LiteralPath (Join-Path $resolvedProjectRoot 'SeedForge.uproject') -PathType Leaf)) {
        throw 'Build environment requires the SeedForge project root.'
    }
    $ubaStorageRoot = Join-Path $resolvedProjectRoot '.cache\UnrealBuildAccelerator'
    $derivedDataRoot = Join-Path $resolvedProjectRoot '.cache\DerivedDataCache'
    New-Item -ItemType Directory -Force -Path $ubaStorageRoot, $derivedDataRoot | Out-Null
    [Environment]::SetEnvironmentVariable('UBA_ROOT', $ubaStorageRoot, 'Process')
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $derivedDataRoot, 'Process')
}

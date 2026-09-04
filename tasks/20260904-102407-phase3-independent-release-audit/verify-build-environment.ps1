[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts\BuildEnvironment.ps1')
$priorUbaRoot = [Environment]::GetEnvironmentVariable('UBA_ROOT', 'Process')
$priorExtraArgs = [Environment]::GetEnvironmentVariable('UBT_EXTRA_ARGS', 'Process')
$priorDdc = [Environment]::GetEnvironmentVariable('UE-LocalDataCachePath', 'Process')
$failures = [Collections.Generic.List[string]]::new()
try {
    [Environment]::SetEnvironmentVariable('UBA_ROOT', $null, 'Process')
    [Environment]::SetEnvironmentVariable('UBT_EXTRA_ARGS', '-NoHotReloadFromIDE', 'Process')
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $null, 'Process')
    Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
    Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
    $actualRoot = [Environment]::GetEnvironmentVariable('UBA_ROOT', 'Process')
    $actualArgs = [Environment]::GetEnvironmentVariable('UBT_EXTRA_ARGS', 'Process')
    $actualDdc = [Environment]::GetEnvironmentVariable('UE-LocalDataCachePath', 'Process')
    if ($actualRoot -ne (Join-Path $projectRoot '.cache\UnrealBuildAccelerator')) { $failures.Add('UBA_ROOT is not project-local') }
    if ($actualDdc -ne (Join-Path $projectRoot '.cache\DerivedDataCache')) { $failures.Add('DDC is not project-local') }
    if ($actualArgs -ne '-NoHotReloadFromIDE') { $failures.Add('UBT_EXTRA_ARGS must remain unchanged for metadata/tool modes') }
    $child = & (Get-Process -Id $PID).Path -NoProfile -Command '[pscustomobject]@{root=$env:UBA_ROOT;args=$env:UBT_EXTRA_ARGS;ddc=[Environment]::GetEnvironmentVariable("UE-LocalDataCachePath")} | ConvertTo-Json -Compress'
    if ($LASTEXITCODE -ne 0) { throw 'Environment child probe failed' }
    $inherited = $child | ConvertFrom-Json
    if ($inherited.root -ne (Join-Path $projectRoot '.cache\UnrealBuildAccelerator') -or $inherited.args -ne '-NoHotReloadFromIDE') { $failures.Add('Child process did not inherit confined storage and unchanged arguments') }
    if ($inherited.ddc -ne (Join-Path $projectRoot '.cache\DerivedDataCache')) { $failures.Add('Child process did not inherit DDC root') }
    $badRootRejected = $false
    try { Initialize-SeedForgeBuildEnvironment -ProjectRoot (Join-Path $projectRoot 'Artifacts\NotAProject') }
    catch { $badRootRejected = $true }
    if (-not $badRootRejected) { $failures.Add('Non-project root was not rejected') }
    foreach ($opaqueArguments in @('-UBADisableRemote=true', '"-UBADisableRemote=false"', '-Custom="text -UBADisableRemote text"')) {
        [Environment]::SetEnvironmentVariable('UBT_EXTRA_ARGS', $opaqueArguments, 'Process')
        try {
            Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
            if ([Environment]::GetEnvironmentVariable('UBT_EXTRA_ARGS', 'Process') -ne $opaqueArguments) { $failures.Add('Opaque caller arguments were rewritten') }
        }
        catch { $failures.Add('Opaque caller arguments were interpreted by cache setup') }
    }
    [Environment]::SetEnvironmentVariable('UBT_EXTRA_ARGS', $actualArgs, 'Process')
    $launchers = @('Build','Test','Smoke','Report','CreateDemoMap','CaptureDemo','CaptureInspector','PackageDemo','PackagePlugin','TestGameplay','TestRunFailure')
    foreach ($launcher in $launchers) {
        $source = Get-Content -LiteralPath (Join-Path $projectRoot "Scripts\$launcher.ps1") -Raw
        if ($source -notmatch 'BuildEnvironment\.ps1' -or $source -notmatch 'Initialize-SeedForgeBuildEnvironment -ProjectRoot \$projectRoot') {
            $failures.Add("$launcher does not initialize the shared build environment")
        }
    }
    $buildSource = Get-Content -LiteralPath (Join-Path $projectRoot 'Scripts\Build.ps1') -Raw
    $packageSource = Get-Content -LiteralPath (Join-Path $projectRoot 'Scripts\PackageDemo.ps1') -Raw
    if ($buildSource -notmatch "'-UBADisableRemote'") { $failures.Add('Build-mode remote flag is missing') }
    if ($packageSource -notmatch "'-UbtArgs=-UBADisableRemote'") { $failures.Add('BuildCookRun scoped UBT flag is missing') }
    if ($failures.Count -ne 0) { throw ($failures -join [Environment]::NewLine) }
    Write-Host 'Build environment checks passed: project roots, opaque flags preserved, child inheritance, 11 launchers, scoped build flags.'
}
finally {
    [Environment]::SetEnvironmentVariable('UBA_ROOT', $priorUbaRoot, 'Process')
    [Environment]::SetEnvironmentVariable('UBT_EXTRA_ARGS', $priorExtraArgs, 'Process')
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $priorDdc, 'Process')
}

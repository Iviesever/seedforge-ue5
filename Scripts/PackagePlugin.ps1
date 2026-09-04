[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$pluginFile = Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin'
$runUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$artifactRoot = Join-Path $projectRoot 'Artifacts'
$logRoot = Join-Path $artifactRoot 'Logs'
$pluginRoot = Join-Path $artifactRoot 'Plugin'
$releaseRoot = Join-Path $artifactRoot 'Release'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'

New-Item -ItemType Directory -Force -Path $logRoot, $pluginRoot, $releaseRoot, $cacheRoot | Out-Null
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot

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

$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$packageDir = Join-Path $pluginRoot "SeedForge-$timestamp"
$consoleLog = Join-Path $logRoot "package-plugin-$timestamp.log"
$uatDiagnosticRoot = Join-Path $logRoot "uat-plugin-$timestamp"
$uatEngineSavedRoot = Join-Path $uatDiagnosticRoot 'EngineSaved'
New-Item -ItemType Directory -Force -Path $uatDiagnosticRoot, $uatEngineSavedRoot | Out-Null
Set-Item -Path 'Env:uebp_LogFolder' -Value $uatDiagnosticRoot
Set-Item -Path 'Env:uebp_FinalLogFolder' -Value $uatDiagnosticRoot
Set-Item -Path 'Env:uebp_EngineSavedFolder' -Value $uatEngineSavedRoot

$arguments = @(
    'BuildPlugin',
    "-ScriptDir=$(Join-Path $PSScriptRoot 'Automation')",
    "-ScriptsForProject=$(Join-Path $projectRoot 'SeedForge.uproject')",
    "-Plugin=$pluginFile",
    "-Package=$packageDir",
    '-TargetPlatforms=Win64',
    '-Rocket'
)

$startedAtUtc = [DateTimeOffset]::UtcNow
$automationRoot = Join-Path $PSScriptRoot 'Automation'
$automationProjects = @(Get-ChildItem -LiteralPath $automationRoot -Recurse -File -Filter '*.Automation.csproj')
if ($automationProjects.Count -ne 1 -or $automationProjects[0].Name -cne 'SeedForgeBuild.Automation.csproj') {
    throw 'Read-only Engine confinement requires exactly one external Automation project.'
}
[xml]$automationProjectXml = Get-Content -Raw -LiteralPath $automationProjects[0].FullName
if ($automationProjectXml.SelectNodes('//*[local-name()="ProjectReference"]').Count -ne 0) {
    throw 'Automation adapter may use installed binary references only, not ProjectReferences.'
}
$readOnlyPaths = @(
    (Join-Path $EngineRoot 'Engine/Intermediate/Build/EpicGames.ScriptBuild.props'),
    (Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'UnrealEngine/Intermediate/Build/UnrealBuildTool.Env.BuildConfiguration.xml')
)
function Read-InfrastructureState {
    @(foreach ($path in $readOnlyPaths) {
        if (Test-Path -LiteralPath $path) {
            $file = Get-Item -LiteralPath $path
            [pscustomobject]@{path=$path;exists=$true;size=$file.Length;lastWriteUtc=$file.LastWriteTimeUtc.ToString('o');sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
        }
        else { [pscustomobject]@{path=$path;exists=$false} }
    })
}
$infrastructureBefore = Read-InfrastructureState
$infrastructureReport = Join-Path $uatDiagnosticRoot 'read-only-infrastructure.json'
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Stock BuildPlugin UAT process' -Action {
    $automationCache = Join-Path $projectRoot '.cache/AutomationExtension'
    $settings = @{
        SEEDFORGE_ENGINE_ROOT=$EngineRoot; DOTNET_CLI_HOME=Join-Path $automationCache 'cli'
        NUGET_PACKAGES=Join-Path $automationCache 'nuget/packages'; NUGET_HTTP_CACHE_PATH=Join-Path $automationCache 'nuget/http'
        NUGET_SCRATCH=Join-Path $automationCache 'nuget/scratch'; TEMP=Join-Path $automationCache 'temp'; TMP=Join-Path $automationCache 'temp'
        DOTNET_CLI_TELEMETRY_OPTOUT='1'; DOTNET_SKIP_FIRST_TIME_EXPERIENCE='1'; DOTNET_CLI_WORKLOAD_UPDATE_NOTIFY_DISABLE='1'
        MSBUILDDISABLENODEREUSE='1'
    }
    $prior = @{}
    $uatActionError = $null
    $infrastructureError = $null
    foreach ($key in $settings.Keys) { $prior[$key]=[Environment]::GetEnvironmentVariable($key,'Process') }
    try {
        foreach ($key in $settings.Keys) { [Environment]::SetEnvironmentVariable($key,$settings[$key],'Process') }
        New-Item -ItemType Directory -Force -Path $settings.DOTNET_CLI_HOME,$settings.NUGET_PACKAGES,$settings.NUGET_HTTP_CACHE_PATH,$settings.NUGET_SCRATCH,$settings.TEMP | Out-Null
        & $runUat @arguments 2>&1 | Tee-Object -FilePath $consoleLog | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "BuildPlugin failed with exit code $LASTEXITCODE. See '$consoleLog'."
        }
    }
    catch { $uatActionError = $_ }
    finally {
        foreach ($key in $prior.Keys) { [Environment]::SetEnvironmentVariable($key,$prior[$key],'Process') }
        try {
            $infrastructureAfter = Read-InfrastructureState
            [IO.File]::WriteAllText($infrastructureReport,([ordered]@{before=$infrastructureBefore;after=$infrastructureAfter} | ConvertTo-Json -Depth 6))
            if (($infrastructureBefore | ConvertTo-Json -Compress) -cne ($infrastructureAfter | ConvertTo-Json -Compress)) {
                throw "Read-only Engine/global build metadata changed; see $infrastructureReport."
            }
        }
        catch { $infrastructureError = $_ }
    }
    if ($null -ne $uatActionError -and $null -ne $infrastructureError) {
        throw [AggregateException]::new("BuildPlugin and infrastructure guard failed: $($uatActionError.Exception.Message); $($infrastructureError.Exception.Message)",[Exception[]]@($uatActionError.Exception,$infrastructureError.Exception))
    }
    if ($null -ne $infrastructureError) { throw $infrastructureError }
    if ($null -ne $uatActionError) { throw $uatActionError }
}
. (Join-Path $PSScriptRoot 'BuildPluginValidation.ps1')
$targetProof = @(Get-SeedForgeBuildPluginTargetProof -ConsoleLog $consoleLog -DiagnosticRoot $uatDiagnosticRoot -PackageDirectory $packageDir -StartedAtUtc $startedAtUtc)
if (@(Select-String -LiteralPath $consoleLog -Pattern '^SeedForge BuildPlugin adapter: (UnrealEditor Win64 Development|UnrealGame Win64 Development|UnrealGame Win64 Shipping); x64; direct remote-disable\.$').Count -ne 3) {
    throw 'Stock BuildPlugin did not confirm all three local-only adapter invocations.'
}

$packagedDescriptor = Join-Path $packageDir 'SeedForge.uplugin'
if (-not (Test-Path -LiteralPath $packagedDescriptor)) {
    throw "BuildPlugin succeeded without the expected descriptor '$packagedDescriptor'."
}

$zipPath = Join-Path $releaseRoot "SeedForgePlugin-$version-$timestamp.zip"
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Plugin archive creation' -Action {
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
    archive = $zipPath
    sha256 = $hash.Hash.ToLowerInvariant()
    startedAtUtc = $startedAtUtc.ToString('o')
    verifiedTargets = @($targetProof | ForEach-Object { $_.Label })
    targetProof = $targetProof
    consoleLog = $consoleLog
    diagnosticRoot = $uatDiagnosticRoot
    infrastructureGuardReport = $infrastructureReport
    result = 'Passed'
}
$manifestPath = Join-Path $pluginRoot "plugin-package-$timestamp.json"
Assert-SeedForgeScriptContext -Context $verificationContext
[System.IO.File]::WriteAllText(
    $manifestPath,
    ($manifest | ConvertTo-Json -Depth 9) + "`r`n")
$verifiedManifest = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $manifestPath
Assert-SeedForgeScriptContext -Context $verificationContext
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $pluginRoot 'last-plugin-package.json')
Assert-SeedForgeScriptContext -Context $verificationContext

Write-Host "SeedForge plugin package passed."
Write-Host "Package: $packageDir"
Write-Host "Archive: $zipPath"
Write-Host "SHA256: $($hash.Hash.ToLowerInvariant())"
Write-Host "Manifest: $manifestPath"
Write-Host "Log: $consoleLog"
return $verifiedManifest

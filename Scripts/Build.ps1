[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [ValidateSet('Development', 'DebugGame')]
    [string]$Configuration = 'Development',
    [string]$ExpectedRevision,
    [switch]$AllowDirtyDiagnostic
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $cacheRoot, $userRoot | Out-Null

if (-not (Test-Path -LiteralPath $buildScript)) {
    throw "Unreal Build.bat was not found at '$buildScript'."
}

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$consoleLogPath = Join-Path $logRoot "build-editor-$timestamp.log"
$ubtLogPath = Join-Path $logRoot "ubt-editor-$timestamp.log"
$arguments = @(
    'SeedForgeEditor',
    'Win64',
    $Configuration,
    "-Project=$projectFile",
    '-WaitMutex',
    '-NoHotReloadFromIDE',
    '-NoEngineChanges',
    '-NoUBTMakefiles',
    '-UBADisableRemote',
    "-Log=$ubtLogPath"
)

$startedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Editor UBT process' -Action {
    & $buildScript @arguments 2>&1 | Tee-Object -FilePath $consoleLogPath | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "SeedForgeEditor build failed with exit code $LASTEXITCODE. See '$consoleLogPath' and '$ubtLogPath'."
    }
}
. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof = @(Assert-SeedForgeLog -Path $consoleLogPath; Assert-SeedForgeLog -Path $ubtLogPath)
Assert-SeedForgeScriptContext -Context $verificationContext
$summaryPath = Join-Path $logRoot "build-editor-$timestamp.json"
$summary = [pscustomobject]@{
    sourceRevision=$verificationContext.SourceRevision; result=$(if($AllowDirtyDiagnostic){'DiagnosticPassed'}else{'Passed'})
    startedAtUtc=$startedAtUtc.ToString('o'); completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o')
    configuration=$Configuration; exitCode=0; logs=$logProof; summaryPath=$summaryPath
}
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 7))
Assert-SeedForgeScriptContext -Context $verificationContext

Write-Host "SeedForgeEditor build passed. Logs: $consoleLogPath and $ubtLogPath"
return $summary

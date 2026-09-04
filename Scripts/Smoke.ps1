[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [int]$TimeoutSeconds = 180,
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$logRoot = Join-Path $projectRoot 'Artifacts\Logs'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'

New-Item -ItemType Directory -Force -Path $logRoot, $cacheRoot, $userRoot | Out-Null

if (-not (Test-Path -LiteralPath $editor)) {
    throw "UnrealEditor-Cmd.exe was not found at '$editor'."
}

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$logPath = Join-Path $logRoot "smoke-editor-$timestamp.log"
$arguments = @(
    $projectFile,
    '-run=CompileAllBlueprints',
    '-AllowListFile=Scripts/SmokeBlueprintAllowList.txt',
    '-ShowResultsOnly',
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nullrhi',
    '-nosound',
    '-culture=en',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$startedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Headless smoke process' -Action {
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "UnrealEditor-Cmd smoke timed out after $TimeoutSeconds seconds. See '$logPath'."
    }
    if ($process.ExitCode -ne 0) {
        throw "UnrealEditor-Cmd smoke failed with exit code $($process.ExitCode). See '$logPath'."
    }
}

$fatalPattern = 'Fatal error:|Assertion failed:|LogPluginManager: Error|Missing or incompatible module'
if (Select-String -LiteralPath $logPath -Pattern $fatalPattern -Quiet) {
    throw "UnrealEditor-Cmd smoke log contains a fatal marker. See '$logPath'."
}

if (-not (Select-String -LiteralPath $logPath -Pattern 'Success - 0 error\(s\), 0 warning\(s\)' -Quiet)) {
    throw "UnrealEditor-Cmd smoke log is missing the commandlet success marker. See '$logPath'."
}

. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof = Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment
Assert-SeedForgeScriptContext -Context $verificationContext
$summaryPath = Join-Path $logRoot "smoke-editor-$timestamp.json"
$summary = [pscustomobject]@{ sourceRevision=$verificationContext.SourceRevision; result='Passed'; exitCode=0; startedAtUtc=$startedAtUtc.ToString('o'); completedAtUtc=[DateTimeOffset]::UtcNow.ToString('o'); log=$logProof; summaryPath=$summaryPath }
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 7))
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "UnrealEditor-Cmd smoke passed. Log: $logPath"
return $summary

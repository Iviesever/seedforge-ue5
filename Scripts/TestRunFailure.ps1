[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [ValidateSet('Grid', 'Encounter')][string]$Case = 'Grid',
    [int]$TimeoutSeconds = 45,
    [string]$Executable
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$runRoot = Join-Path $projectRoot "Artifacts\Reports\RunFailure\$stamp-$Case"
$userRoot = Join-Path $projectRoot '.user\RunFailure'
New-Item -ItemType Directory -Force -Path $runRoot, $userRoot | Out-Null
$tracePath = Join-Path $runRoot 'failure-trace.json'
$logPath = Join-Path $runRoot 'runtime.log'
$headBefore = (git -C $projectRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve test HEAD.' }
$dirtyBefore = @(git -C $projectRoot status --porcelain=v1 --untracked-files=all).Count -ne 0
$prefix = @()
if ([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
    $prefix = @((Join-Path $projectRoot 'SeedForge.uproject'), '/Game/Maps/SeedForgeDemo', '-game')
}
$badArgument = if ($Case -eq 'Grid') { '-SeedForgeGridWidth=0' } else { '-SeedForgeRoomCount=1' }
$expectedCode = if ($Case -eq 'Grid') { 'GenerationFailed' } else { 'EncounterFailed' }
$arguments = $prefix + @('-nullrhi', '-unattended', '-nosplash', '-nosound', '-nop4',
    '-SeedForgeGameplaySmoke', '-SeedForgeSeed=24301', $badArgument,
    "-SeedForgeGitSha=$headBefore", "-SeedForgeGameplayTrace=$tracePath",
    "-SeedForgeGameplayCaptureDir=$runRoot", "-userdir=$userRoot", "-abslog=$logPath")
$started = Get-Date
$process = Start-Process -FilePath $Executable -ArgumentList $arguments -PassThru -WindowStyle Hidden
$timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
if ($timedOut) { $process.Kill($true); $process.WaitForExit() }
$ended = Get-Date
$headAfter = (git -C $projectRoot rev-parse HEAD).Trim()
$observation = [ordered]@{
    case = $Case; startedAt = $started.ToString('o'); endedAt = $ended.ToString('o')
    headBefore = $headBefore; headAfter = $headAfter; sourceTreeDirty = $dirtyBefore
    exitCode = $process.ExitCode; outerTimeout = $timedOut
    traceExists = Test-Path -LiteralPath $tracePath; trace = $tracePath; log = $logPath
}
[IO.File]::WriteAllText((Join-Path $runRoot 'observation.json'), ($observation | ConvertTo-Json) + [Environment]::NewLine)
Write-Host "Failure observation: $runRoot"
if ($timedOut) { throw 'Pre-Playing failure was abandoned: runtime did not request its own exit.' }
if ($process.ExitCode -ne 2) { throw "Expected failure exit 2, got $($process.ExitCode)." }
if (-not (Test-Path -LiteralPath $tracePath)) { throw 'No failed JSON trace was written.' }
$trace = Get-Content -Raw -LiteralPath $tracePath | ConvertFrom-Json
if ($trace.result -ne 'Failed' -or $trace.failureCode -ne $expectedCode) { throw 'Incorrect failure trace result/code.' }
if ($trace.gitSha -ne $headBefore -or $trace.seed -ne '24301') { throw 'Failure trace identity mismatch.' }
if (@($trace.stateTransitions)[-1] -ne 'Failed') { throw 'Failure trace never entered Failed.' }
if ($headBefore -ne $headAfter) { throw 'Test HEAD changed during process execution.' }
if (Select-String -LiteralPath $logPath -Pattern 'Gameplay ready|SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS' -Quiet) { throw 'Failed run incorrectly reported readiness/success.' }
$failures = @(Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_GAMEPLAY_SMOKE_FAILURE code=')
if ($failures.Count -ne 1) { throw "Expected one smoke failure completion, found $($failures.Count)." }
Write-Host "Run failure contract passed: $Case, runtime exit 2, failed trace reparsed, no outer timeout."

[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [ValidateSet('Grid', 'Encounter', 'CapturePath', 'RenderUnavailable')][string]$Case = 'Grid',
    [int]$TimeoutSeconds = 45,
    [string]$Executable,
    [string]$ExpectedRevision,
    [switch]$AllowDirtyDiagnostic
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$runRoot = Join-Path $projectRoot "Artifacts\Reports\RunFailure\$stamp-$Case"
$userRoot = Join-Path $projectRoot '.user\RunFailure'
New-Item -ItemType Directory -Force -Path $runRoot, $userRoot | Out-Null
$tracePath = Join-Path $runRoot 'failure-trace.json'
$logPath = Join-Path $runRoot 'runtime.log'
$headBefore = $verificationContext.ExpectedRevision
$dirtyBefore = -not [string]::IsNullOrEmpty((Invoke-SeedForgeContractGit $projectRoot @('status','--porcelain=v1','--untracked-files=all')))
$traceRevision = $verificationContext.SourceRevision
$prefix = @()
if ([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
    $prefix = @((Join-Path $projectRoot 'SeedForge.uproject'), '/Game/Maps/SeedForgeDemo', '-game')
}
$expectedCode = switch ($Case) { 'Grid' { 'GenerationFailed' }; 'Encounter' { 'EncounterFailed' }; default { 'CaptureFailed' } }
$captureDirectory = $runRoot
if ($Case -eq 'CapturePath') {
    $captureDirectory = Join-Path $runRoot 'blocked-capture-directory'
    [IO.File]::WriteAllText($captureDirectory, 'Synthetic negative fixture: this file cannot be a capture directory.')
}
$arguments = $prefix + @('-unattended', '-nosplash', '-nosound', '-nop4', '-culture=en',
    '-SeedForgeGameplaySmoke', '-SeedForgeSeed=24301',
    "-SeedForgeGitSha=$traceRevision", "-SeedForgeGameplayTrace=$tracePath",
    "-SeedForgeGameplayCaptureDir=$captureDirectory", "-SeedForgeCaptureRoot=$runRoot", "-userdir=$userRoot", "-abslog=$logPath")
if ($Case -eq 'CapturePath') { $arguments += @('-RenderOffscreen', '-windowed', '-ForceRes', '-ResX=1280', '-ResY=720') }
else { $arguments += '-nullrhi' }
if ($Case -eq 'Grid') { $arguments += '-SeedForgeGridWidth=0' }
if ($Case -eq 'Encounter') { $arguments += '-SeedForgeRoomCount=1' }
$arguments += @(Get-SeedForgeRuntimeArguments -ProjectRoot $projectRoot)
$started = Get-Date
$outcome = Invoke-SeedForgeScriptStep -Context $verificationContext -Name "Expected failure $Case process" -Action {
    $process = Start-Process -FilePath $Executable -ArgumentList $arguments -PassThru -WindowStyle Hidden
    $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
    if ($timedOut) { $process.Kill($true); $process.WaitForExit() }
    [pscustomobject]@{ ExitCode=$process.ExitCode; TimedOut=$timedOut }
}
$timedOut = $outcome.TimedOut
$ended = Get-Date
$headAfter = Invoke-SeedForgeContractGit $projectRoot @('rev-parse','--verify','HEAD')
$observation = [ordered]@{
    case = $Case; startedAt = $started.ToString('o'); endedAt = $ended.ToString('o')
    headBefore = $headBefore; headAfter = $headAfter; sourceTreeDirty = $dirtyBefore; traceRevision = $traceRevision
    exitCode = $outcome.ExitCode; outerTimeout = $timedOut
    traceExists = Test-Path -LiteralPath $tracePath; trace = $tracePath; log = $logPath
}
[IO.File]::WriteAllText((Join-Path $runRoot 'observation.json'), ($observation | ConvertTo-Json) + [Environment]::NewLine)
Write-Host "Failure observation: $runRoot"
if ($timedOut) { throw 'Pre-Playing failure was abandoned: runtime did not request its own exit.' }
if ($outcome.ExitCode -ne 2) { throw "Expected failure exit 2, got $($outcome.ExitCode)." }
if (-not (Test-Path -LiteralPath $tracePath)) { throw 'No failed JSON trace was written.' }
$trace = Get-Content -Raw -LiteralPath $tracePath | ConvertFrom-Json
if ($trace.result -ne 'Failed' -or $trace.failureCode -ne $expectedCode) { throw 'Incorrect failure trace result/code.' }
if ($trace.gitSha -ne $traceRevision -or $trace.seed -ne '24301') { throw 'Failure trace identity mismatch.' }
if (@($trace.stateTransitions)[-1] -ne 'Failed') { throw 'Failure trace never entered Failed.' }
if ($headBefore -ne $headAfter) { throw 'Test HEAD changed during process execution.' }
if (Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS' -Quiet) { throw 'Failed run incorrectly reported success.' }
if ($Case -in @('Grid', 'Encounter') -and (Select-String -LiteralPath $logPath -Pattern 'Gameplay ready' -Quiet)) { throw 'Pre-playing failure incorrectly reported readiness.' }
if ($Case -in @('CapturePath', 'RenderUnavailable')) {
    if (($trace.stateTransitions -join ',') -ne 'Generating,Playing,Failed') { throw 'Capture failure did not follow the production Playing-to-Failed path.' }
    if (@($trace.captures).Count -ne 0 -or @($trace.screenshots).Count -ne 0) { throw 'Failed first capture published a successful receipt/path.' }
    if ($trace.appliedRequestId -ne '0' -or $trace.layoutHash -ne '0' -or $trace.encounterHash -ne '0') { throw 'Capture failure retained a stale applied identity.' }
}
$failures = @(Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_GAMEPLAY_SMOKE_FAILURE code=')
if ($failures.Count -ne 1) { throw "Expected one smoke failure completion, found $($failures.Count)." }
. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof = Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment -ExpectedFailure $Case
. (Join-Path $PSScriptRoot 'RuntimeStorageValidation.ps1')
$storageProof = Assert-SeedForgeRuntimeStorage -Path $logPath -ProjectRoot $projectRoot -RequireDdc:($prefix.Count -gt 0)
if ($trace.failureMessage -cne $logProof.ExpectedFailureMessage) { throw 'Negative trace does not match its exact expected diagnostic.' }
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "Run failure contract passed: $Case, runtime exit 2, failed trace reparsed, no outer timeout."
return [pscustomobject]@{ sourceRevision=$traceRevision; result='ExpectedFailure'; observation=(Join-Path $runRoot 'observation.json'); trace=$tracePath; traceSha256=(Get-FileHash -LiteralPath $tracePath -Algorithm SHA256).Hash.ToLowerInvariant(); log=$logPath; logProof=$logProof;storageProof=$storageProof; exitCode=$outcome.ExitCode }

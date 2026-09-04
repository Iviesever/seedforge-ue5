[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 180,
    [string]$Executable,
    [ValidateSet('editor', 'packaged')]
    [string]$RunLabel = 'editor',
    [switch]$AllowDirtyDiagnostic,
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$artifactRoot = Join-Path $projectRoot 'Artifacts'
$logRoot = Join-Path $artifactRoot 'Logs'
$reportRoot = Join-Path $artifactRoot 'Reports\Gameplay'
$mediaRoot = Join-Path $artifactRoot 'Media\Gameplay'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot ".user\GameplaySmoke-$RunLabel"
$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N')
$runReportRoot = Join-Path $reportRoot $timestamp
$runMediaRoot = Join-Path $mediaRoot $timestamp
$tracePath = Join-Path $runReportRoot 'gameplay-smoke.json'
$summaryPath = Join-Path $runReportRoot 'summary.json'
$logPath = Join-Path $logRoot "gameplay-smoke-$RunLabel-$timestamp.log"

New-Item -ItemType Directory -Force -Path $logRoot, $runReportRoot, $runMediaRoot, $cacheRoot, $userRoot | Out-Null
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot

if ([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable = $editor
    $launchPrefixArguments = @($projectFile, '/Game/Maps/SeedForgeDemo')
}
else {
    $Executable = [IO.Path]::GetFullPath($Executable)
    $launchPrefixArguments = @()
}
if (-not (Test-Path -LiteralPath $Executable)) {
    throw "Gameplay executable was not found at '$Executable'."
}
$revision = $verificationContext.ExpectedRevision
$sourceTreeDirty = -not [string]::IsNullOrEmpty((Invoke-SeedForgeContractGit $projectRoot @('status','--porcelain=v1','--untracked-files=all')))
$traceRevision = $verificationContext.SourceRevision

$arguments = @($launchPrefixArguments) + @(
    '-game',
    '-RenderOffscreen',
    '-windowed',
    '-ForceRes',
    '-ResX=1280',
    '-ResY=720',
    '-SeedForgeGameplaySmoke',
    "-SeedForgeSeed=$Seed",
    "-SeedForgeGitSha=$traceRevision",
    "-SeedForgeGameplayTrace=$tracePath",
    "-SeedForgeGameplayCaptureDir=$runMediaRoot",
    "-SeedForgeCaptureRoot=$runMediaRoot",
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nosound',
    '-culture=en',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$arguments += @(Get-SeedForgeRuntimeArguments -ProjectRoot $projectRoot)
$processStartedAtUtc = [DateTimeOffset]::UtcNow
Invoke-SeedForgeScriptStep -Context $verificationContext -Name "$RunLabel gameplay smoke process" -Action {
    $process = Start-Process -FilePath $Executable -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill($true)
        throw "$RunLabel gameplay smoke timed out after $TimeoutSeconds seconds. See '$logPath'."
    }
    if ($process.ExitCode -ne 0) {
        throw "$RunLabel gameplay smoke failed with exit code $($process.ExitCode). See '$logPath'."
    }
}
$processEndedAtUtc = [DateTimeOffset]::UtcNow
if (-not (Test-Path -LiteralPath $tracePath)) {
    throw "$RunLabel gameplay smoke produced no JSON trace at '$tracePath'."
}
if (@(Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS').Count -ne 1) {
    throw "$RunLabel gameplay smoke log must have exactly one success marker. See '$logPath'."
}

$traceText = Get-Content -Raw -LiteralPath $tracePath
$trace = if ((Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) {
    $traceText | ConvertFrom-Json -DateKind String
} else { $traceText | ConvertFrom-Json }
if ($trace.schema -ne 'seedforge.gameplay-smoke' -or [int]$trace.schemaVersion -ne 1) {
    throw 'Gameplay smoke trace has an unsupported schema.'
}
if ($trace.result -ne 'Passed' -or $trace.failureCode -ne '' -or $trace.failureMessage -ne '') {
    throw "Gameplay smoke trace reports failure: $($trace.failureCode) $($trace.failureMessage)"
}
if ($trace.gitSha -ne $traceRevision) {
    throw "Gameplay smoke trace revision '$($trace.gitSha)' does not match '$traceRevision'."
}
if ($trace.seed -ne $Seed.ToString([Globalization.CultureInfo]::InvariantCulture)) {
    throw "Gameplay smoke trace seed '$($trace.seed)' does not match '$Seed'."
}
if ($trace.layoutHash -notmatch '^[1-9][0-9]*$' -or $trace.encounterHash -notmatch '^[1-9][0-9]*$') {
    throw 'Gameplay smoke trace is missing exact non-zero layout/encounter hash strings.'
}
if (([int]$trace.actorCounts.players) -ne 1 -or
    ([int]$trace.actorCounts.dataCores) -ne 3 -or
    ([int]$trace.actorCounts.enemies) -ne 5 -or
    ([int]$trace.actorCounts.exits) -ne 1) {
    throw 'Gameplay smoke trace actor counts are not 1 player / 3 Cores / 5 enemies / 1 exit.'
}
if (($trace.stateTransitions -join ',') -ne 'Generating,Playing,Won') {
    throw "Gameplay smoke trace has an unexpected state path: $($trace.stateTransitions -join ',')"
}
$actions = @($trace.actions)
if (@($actions | Where-Object { $_ -like 'Attack:Enemy:*' }).Count -ne 1 -or
    @($actions | Where-Object { $_ -like 'Kill:Enemy:*' }).Count -ne 1 -or
    @($actions | Where-Object { $_ -like 'Collect:Core:*' }).Count -ne 3 -or
    'ExitUnlocked' -notin $actions -or
    'ReachExit' -notin $actions) {
    throw "Gameplay smoke trace does not prove attack/kill/3 Core/unlock/exit actions: $($actions -join ',')"
}

. (Join-Path $PSScriptRoot 'GameplayPathValidation.ps1')
$pathProof = Assert-SeedForgeGameplayPath -Trace $trace
. (Join-Path $PSScriptRoot 'CaptureValidation.ps1')
$validatedScreenshots = @(Assert-SeedForgeGameplayCaptures -Trace $trace -RunDirectory $runMediaRoot `
    -ProcessStartedAtUtc $processStartedAtUtc -ProcessEndedAtUtc $processEndedAtUtc)

. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof = Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment
. (Join-Path $PSScriptRoot 'RuntimeStorageValidation.ps1')
$storageProof = Assert-SeedForgeRuntimeStorage -Path $logPath -ProjectRoot $projectRoot -RequireDdc:($RunLabel -eq 'editor')
Assert-SeedForgeScriptContext -Context $verificationContext

$summary = [ordered]@{
    completedAt = (Get-Date).ToString('o')
    runLabel = $RunLabel
    executable = $Executable
    sourceRevision = $traceRevision
    baseRevision = $revision
    sourceTreeDirty = $sourceTreeDirty
    verificationKind = $(if ($AllowDirtyDiagnostic) { 'diagnostic' } else { 'clean-revision' })
    processStartedAtUtc = $processStartedAtUtc.ToString('o')
    processEndedAtUtc = $processEndedAtUtc.ToString('o')
    seed = $trace.seed
    layoutHash = $trace.layoutHash
    encounterHash = $trace.encounterHash
    result = $(if($AllowDirtyDiagnostic){'DiagnosticPassed'}else{'Passed'})
    actorCounts = $trace.actorCounts
    stateTransitions = $trace.stateTransitions
    actions = $trace.actions
    trace = $tracePath
    screenshots = $trace.screenshots
    captures = $trace.captures
    validatedScreenshots = $validatedScreenshots
    pathProof = $pathProof
    visualReview = 'not-assessed-by-script'
    log = $logPath
    logProof = $logProof
    storageProof = $storageProof
    traceSha256 = (Get-FileHash -LiteralPath $tracePath -Algorithm SHA256).Hash.ToLowerInvariant()
    summaryPath = $summaryPath
    allowedWarningCount = $logProof.AllowedWarningCount
    unexpectedWarningCount = 0
    errorCount = 0
}
[IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 6) + "`r`n")
Assert-SeedForgeScriptContext -Context $verificationContext

Write-Host "$RunLabel gameplay smoke passed for seed $Seed."
Write-Host "Trace: $tracePath"
Write-Host "Summary: $summaryPath"
Write-Host "Screenshots: $runMediaRoot"
Write-Host "Log: $logPath"
return [pscustomobject]$summary

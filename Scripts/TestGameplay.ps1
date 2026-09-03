[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 180,
    [string]$Executable,
    [ValidateSet('editor', 'packaged')]
    [string]$RunLabel = 'editor'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$artifactRoot = Join-Path $projectRoot 'Artifacts'
$logRoot = Join-Path $artifactRoot 'Logs'
$reportRoot = Join-Path $artifactRoot 'Reports\Gameplay'
$mediaRoot = Join-Path $artifactRoot 'Media\Gameplay'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot ".user\GameplaySmoke-$RunLabel"
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$runReportRoot = Join-Path $reportRoot $timestamp
$runMediaRoot = Join-Path $mediaRoot $timestamp
$tracePath = Join-Path $runReportRoot 'gameplay-smoke.json'
$summaryPath = Join-Path $runReportRoot 'summary.json'
$logPath = Join-Path $logRoot "gameplay-smoke-$RunLabel-$timestamp.log"

New-Item -ItemType Directory -Force -Path $logRoot, $runReportRoot, $runMediaRoot, $cacheRoot, $userRoot | Out-Null
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $cacheRoot

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
$revision = (git -C $projectRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') {
    throw 'Unable to resolve the exact source revision for gameplay smoke.'
}

$arguments = @($launchPrefixArguments) + @(
    '-game',
    '-RenderOffscreen',
    '-windowed',
    '-ForceRes',
    '-ResX=1280',
    '-ResY=720',
    '-SeedForgeGameplaySmoke',
    "-SeedForgeSeed=$Seed",
    "-SeedForgeGitSha=$revision",
    "-SeedForgeGameplayTrace=$tracePath",
    "-SeedForgeGameplayCaptureDir=$runMediaRoot",
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nosound',
    "-userdir=$userRoot",
    "-abslog=$logPath"
)

$process = Start-Process -FilePath $Executable -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    $process.Kill($true)
    throw "$RunLabel gameplay smoke timed out after $TimeoutSeconds seconds. See '$logPath'."
}
if ($process.ExitCode -ne 0) {
    throw "$RunLabel gameplay smoke failed with exit code $($process.ExitCode). See '$logPath'."
}
if (-not (Test-Path -LiteralPath $tracePath)) {
    throw "$RunLabel gameplay smoke produced no JSON trace at '$tracePath'."
}
if (-not (Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS' -Quiet)) {
    throw "$RunLabel gameplay smoke log has no success marker. See '$logPath'."
}

$trace = Get-Content -Raw -LiteralPath $tracePath | ConvertFrom-Json
if ($trace.schema -ne 'seedforge.gameplay-smoke' -or [int]$trace.schemaVersion -ne 1) {
    throw 'Gameplay smoke trace has an unsupported schema.'
}
if ($trace.result -ne 'Passed' -or $trace.failureCode -ne '' -or $trace.failureMessage -ne '') {
    throw "Gameplay smoke trace reports failure: $($trace.failureCode) $($trace.failureMessage)"
}
if ($trace.gitSha -ne $revision) {
    throw "Gameplay smoke trace revision '$($trace.gitSha)' does not match '$revision'."
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

$projectPrefix = [IO.Path]::GetFullPath($projectRoot).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
$screenshots = @($trace.screenshots)
if ($screenshots.Count -lt 3) {
    throw "Gameplay smoke created only $($screenshots.Count) screenshots; three are required."
}
foreach ($screenshot in $screenshots) {
    $fullScreenshot = [IO.Path]::GetFullPath([string]$screenshot)
    if (-not $fullScreenshot.StartsWith($projectPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Gameplay screenshot escaped the project boundary: '$fullScreenshot'."
    }
    if (-not (Test-Path -LiteralPath $fullScreenshot) -or (Get-Item -LiteralPath $fullScreenshot).Length -lt 10KB) {
        throw "Gameplay screenshot is missing or too small: '$fullScreenshot'."
    }
}

$errorLines = @(Select-String -LiteralPath $logPath -Pattern ': Error:|Fatal error|ensure condition failed|LogSeedForge: Error')
if ($errorLines.Count -ne 0) {
    throw "Gameplay smoke log contains error/fatal markers:`n$($errorLines -join "`n")"
}
$allowedWarningPatterns = @(
    'LogRHI: Warning: Out of date driver found',
    'LogRHI: Warning: Running with bad GPU drivers',
    'LogD3D12RHI: Warning: Batched command list execution is disabled',
    "LogConsoleManager: Warning: Console variable 'r.MotionVectorSimulation' used in the render thread",
    'LogEditorDataStorageUI: Warning: Unable to register widget factory'
)
$unexpectedWarnings = @()
foreach ($warning in @(Select-String -LiteralPath $logPath -Pattern ': Warning:')) {
    $allowed = $false
    foreach ($pattern in $allowedWarningPatterns) {
        if ($warning.Line.Contains($pattern)) {
            $allowed = $true
            break
        }
    }
    if (-not $allowed) {
        $unexpectedWarnings += $warning
    }
}
if ($unexpectedWarnings.Count -ne 0) {
    throw "Gameplay smoke log contains unexpected warnings:`n$($unexpectedWarnings -join "`n")"
}

$summary = [ordered]@{
    completedAt = (Get-Date).ToString('o')
    runLabel = $RunLabel
    executable = $Executable
    sourceRevision = $revision
    seed = $trace.seed
    layoutHash = $trace.layoutHash
    encounterHash = $trace.encounterHash
    result = 'Passed'
    actorCounts = $trace.actorCounts
    stateTransitions = $trace.stateTransitions
    actions = $trace.actions
    trace = $tracePath
    screenshots = $trace.screenshots
    log = $logPath
    allowedWarningCount = @(Select-String -LiteralPath $logPath -Pattern ': Warning:').Count
    unexpectedWarningCount = 0
    errorCount = 0
}
[IO.File]::WriteAllText($summaryPath, ($summary | ConvertTo-Json -Depth 6) + "`r`n")

Write-Host "$RunLabel gameplay smoke passed for seed $Seed."
Write-Host "Trace: $tracePath"
Write-Host "Summary: $summaryPath"
Write-Host "Screenshots: $runMediaRoot"
Write-Host "Log: $logPath"

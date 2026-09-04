[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$LeftSeed = 24301,
    [UInt64]$RightSeed = 12648430,
    [UInt64]$SeedStart = 0,
    [ValidateRange(1, 1000000)]
    [int]$SeedCount = 10000,
    [ValidateRange(0, 1000000)]
    [int]$Warmup = 100,
    [int]$TimeoutSeconds = 900,
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$projectFile = Join-Path $projectRoot 'SeedForge.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$reportRoot = Join-Path $projectRoot 'Artifacts\Reports\Phase2'
$cacheRoot = Join-Path $projectRoot '.cache\DerivedDataCache'
$userRoot = Join-Path $projectRoot '.user'
$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$runRoot = Join-Path $reportRoot $timestamp
$logRoot = Join-Path $runRoot 'logs'

New-Item -ItemType Directory -Force -Path $runRoot, $logRoot, $cacheRoot, $userRoot | Out-Null
if (-not (Test-Path -LiteralPath $editor)) {
    throw "UnrealEditor-Cmd.exe was not found at '$editor'."
}

. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot

function Convert-ToInvariantString {
    param([Parameter(Mandatory)]$Value)
    return $Value.ToString([System.Globalization.CultureInfo]::InvariantCulture)
}

function Invoke-SeedForgeReport {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string[]]$CommandArguments
    )

    $logPath = Join-Path $logRoot "$Name.log"
    $arguments = @(
        $projectFile,
        '-run=SeedForgeReport',
        '-unattended',
        '-nop4',
        '-nosplash',
        '-nullrhi',
        '-nosound',
        '-culture=en',
        '-UTF8Output',
        "-userdir=$userRoot",
        "-abslog=$logPath"
    ) + $CommandArguments

    $arguments += @(Get-SeedForgeRuntimeArguments -ProjectRoot $projectRoot)
    Invoke-SeedForgeScriptStep -Context $verificationContext -Name "Phase2 $Name process" -Action {
        $process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $process.Kill($true)
            throw "SeedForge report command '$Name' timed out after $TimeoutSeconds seconds. See '$logPath'."
        }
        if ($process.ExitCode -ne 0) {
            throw "SeedForge report command '$Name' failed with exit code $($process.ExitCode). See '$logPath'."
        }
    }
    . (Join-Path $PSScriptRoot 'LogValidation.ps1')
    Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment | Out-Null
    . (Join-Path $PSScriptRoot 'RuntimeStorageValidation.ps1')
    $storageProofs.Add((Assert-SeedForgeRuntimeStorage -Path $logPath -ProjectRoot $projectRoot))
    return $logPath
}

$leftPath = Join-Path $runRoot "layout-$LeftSeed.json"
$rightPath = Join-Path $runRoot "layout-$RightSeed.json"
$diffPath = Join-Path $runRoot "diff-$LeftSeed-$RightSeed.json"
$benchmarkPath = Join-Path $runRoot "benchmark-$SeedCount.json"

$leftSeedText = Convert-ToInvariantString $LeftSeed
$rightSeedText = Convert-ToInvariantString $RightSeed
$seedStartText = Convert-ToInvariantString $SeedStart
$seedCountText = Convert-ToInvariantString $SeedCount
$warmupText = Convert-ToInvariantString $Warmup

$logs = [ordered]@{}
$storageProofs = New-Object 'Collections.Generic.List[object]'
$logs.generateLeft = Invoke-SeedForgeReport -Name 'generate-left' -CommandArguments @(
    '-Mode=Generate',
    "-Seed=$leftSeedText",
    "-Output=$leftPath"
)
$logs.generateRight = Invoke-SeedForgeReport -Name 'generate-right' -CommandArguments @(
    '-Mode=Generate',
    "-Seed=$rightSeedText",
    "-Output=$rightPath"
)
$logs.diff = Invoke-SeedForgeReport -Name 'diff' -CommandArguments @(
    '-Mode=Diff',
    "-Left=$leftPath",
    "-Right=$rightPath",
    "-Output=$diffPath"
)
$logs.benchmark = Invoke-SeedForgeReport -Name 'benchmark' -CommandArguments @(
    '-Mode=Benchmark',
    "-SeedStart=$seedStartText",
    "-SeedCount=$seedCountText",
    "-Warmup=$warmupText",
    "-Output=$benchmarkPath"
)

$left = Get-Content -Raw -LiteralPath $leftPath | ConvertFrom-Json
$right = Get-Content -Raw -LiteralPath $rightPath | ConvertFrom-Json
$diff = Get-Content -Raw -LiteralPath $diffPath | ConvertFrom-Json
$benchmark = Get-Content -Raw -LiteralPath $benchmarkPath | ConvertFrom-Json

if ($left.schema -ne 'seedforge.layout' -or [string]$left.seed -ne $leftSeedText) {
    throw "Left canonical document has unexpected schema or seed."
}
if ($right.schema -ne 'seedforge.layout' -or [string]$right.seed -ne $rightSeedText) {
    throw "Right canonical document has unexpected schema or seed."
}
if ($diff.schema -ne 'seedforge.layout-diff' -or
    [string]$diff.leftSeed -ne $leftSeedText -or
    [string]$diff.rightSeed -ne $rightSeedText) {
    throw "Structural diff has unexpected schema or identities."
}
if ($benchmark.schema -ne 'seedforge.benchmark' -or
    [int]$benchmark.requestedSampleCount -ne $SeedCount -or
    [int]$benchmark.attemptedSampleCount -ne $SeedCount -or
    [int]$benchmark.successCount -ne $SeedCount -or
    [int]$benchmark.failureCount -ne 0 -or
    [string]$benchmark.aggregateHash -eq '0') {
    throw "Benchmark report failed its count/hash contract."
}
if ([double]$benchmark.timingsMilliseconds.min -gt [double]$benchmark.timingsMilliseconds.median -or
    [double]$benchmark.timingsMilliseconds.median -gt [double]$benchmark.timingsMilliseconds.p95 -or
    [double]$benchmark.timingsMilliseconds.p95 -gt [double]$benchmark.timingsMilliseconds.max) {
    throw "Benchmark timing distribution is not ordered."
}

$summaryPath = Join-Path $runRoot 'report-summary.json'
$sourceRevision = $verificationContext.ExpectedRevision
Assert-SeedForgeScriptContext -Context $verificationContext
$summary = [ordered]@{
    schema = 'seedforge.phase2-report-run'
    schemaVersion = 1
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
    sourceRevision = $sourceRevision
    leftDocument = $leftPath
    rightDocument = $rightPath
    diff = $diffPath
    benchmark = $benchmarkPath
    logs = $logs
    storageProofs = $storageProofs.ToArray()
    benchmarkSamples = $SeedCount
    benchmarkFailures = 0
    benchmarkAggregateHash = [string]$benchmark.aggregateHash
    summaryPath = $summaryPath
    result = 'Passed'
    fileHashes = @(@($leftPath,$rightPath,$diffPath,$benchmarkPath) + @($logs.Values) | ForEach-Object { [pscustomobject]@{ path=$_; sha256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant() } })
}
[IO.File]::WriteAllText($summaryPath,($summary | ConvertTo-Json -Depth 5) + [Environment]::NewLine)
Assert-SeedForgeScriptContext -Context $verificationContext

Write-Host "Phase 2 report integration passed. Run: $runRoot Summary: $summaryPath"
return [pscustomobject]$summary

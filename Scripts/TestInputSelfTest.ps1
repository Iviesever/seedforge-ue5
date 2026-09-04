[CmdletBinding()]
param(
    [string]$EngineRoot='D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed=24301,
    [ValidateRange(1,600)][int]$TimeoutSeconds=120,
    [string]$Executable,
    [ValidateSet('editor','packaged')][string]$RunLabel='editor',
    [string]$ExpectedRevision,
    [switch]$AllowDirtyDiagnostic
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$projectRoot=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext=New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
. (Join-Path $PSScriptRoot 'BuildEnvironment.ps1')
Initialize-SeedForgeBuildEnvironment -ProjectRoot $projectRoot
$stamp=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N')
$runRoot=Join-Path $projectRoot "Artifacts/Reports/InputSelfTest/$stamp"
$userRoot=Join-Path $projectRoot ".user/InputSelfTest-$RunLabel"
New-Item -ItemType Directory -Force -Path $runRoot,$userRoot | Out-Null
$tracePath=Join-Path $runRoot 'input-selftest.json'
$logPath=Join-Path $runRoot 'runtime.log'
$summaryPath=Join-Path $runRoot 'summary.json'
$prefix=@()
if([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
    $prefix=@((Join-Path $projectRoot 'SeedForge.uproject'),'/Game/Maps/SeedForgeDemo','-game')
}
if(-not(Test-Path -LiteralPath $Executable -PathType Leaf)){throw 'Input self-test executable is missing.'}
$arguments=$prefix+@('-RenderOffscreen','-windowed','-ForceRes','-ResX=1280','-ResY=720',
    '-unattended','-nosplash','-nosound','-nop4','-culture=en','-SeedForgeInputSelfTest',
    "-SeedForgeSeed=$Seed","-SeedForgeInputTrace=$tracePath","-SeedForgeGitSha=$($verificationContext.SourceRevision)",
    "-userdir=$userRoot","-abslog=$logPath")
$started=[DateTimeOffset]::UtcNow
$outcome=Invoke-SeedForgeScriptStep -Context $verificationContext -Name "$RunLabel ordinary input process" -Action {
    $process=Start-Process -FilePath $Executable -ArgumentList $arguments -PassThru -WindowStyle Hidden
    $timedOut=-not $process.WaitForExit($TimeoutSeconds*1000)
    if($timedOut){$process.Kill($true);$process.WaitForExit()}
    [pscustomobject]@{exitCode=$process.ExitCode;outerTimeout=$timedOut}
}
$ended=[DateTimeOffset]::UtcNow
$observation=[ordered]@{sourceRevision=$verificationContext.SourceRevision;runLabel=$RunLabel;executable=$Executable;exitCode=$outcome.exitCode;outerTimeout=$outcome.outerTimeout;processStartedAtUtc=$started.ToString('o');processEndedAtUtc=$ended.ToString('o');trace=$tracePath;log=$logPath}
[IO.File]::WriteAllText((Join-Path $runRoot 'process-observation.json'),($observation|ConvertTo-Json))
if($outcome.outerTimeout){throw "Ordinary input process timed out without completion: $runRoot"}
if($outcome.exitCode -ne 0){throw "Ordinary input process failed with exit $($outcome.exitCode): $runRoot"}
if(@(Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_INPUT_SELFTEST_SUCCESS').Count -ne 1 -or
    (Select-String -LiteralPath $logPath -Pattern 'SEEDFORGE_INPUT_SELFTEST_FAILURE|SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS' -Quiet)) {
    throw 'Ordinary input process has no unique success or started a conflicting smoke driver.'
}
$traceText=Get-Content -Raw -LiteralPath $tracePath
$trace=if((Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')){$traceText|ConvertFrom-Json -DateKind String}else{$traceText|ConvertFrom-Json}
. (Join-Path $PSScriptRoot 'InputSelfTestValidation.ps1')
$inputProof=Assert-SeedForgeInputSelfTest -Trace $trace -SourceIdentity $verificationContext.SourceRevision -Seed $Seed -ProcessStartedAtUtc $started -ProcessEndedAtUtc $ended
. (Join-Path $PSScriptRoot 'LogValidation.ps1')
$logProof=Assert-SeedForgeLog -Path $logPath -AllowedWarnings UE58LocalEnvironment
Assert-SeedForgeScriptContext -Context $verificationContext
$summary=[ordered]@{sourceRevision=$verificationContext.SourceRevision;result=$(if($AllowDirtyDiagnostic){'DiagnosticPassed'}else{'Passed'});runLabel=$RunLabel;seed=$Seed.ToString([Globalization.CultureInfo]::InvariantCulture);executable=$Executable;trace=$tracePath;traceSha256=(Get-FileHash -LiteralPath $tracePath -Algorithm SHA256).Hash.ToLowerInvariant();log=$logPath;logProof=$logProof;inputProof=$inputProof;summaryPath=$summaryPath;processStartedAtUtc=$started.ToString('o');processEndedAtUtc=$ended.ToString('o')}
[IO.File]::WriteAllText($summaryPath,($summary|ConvertTo-Json -Depth 12))
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "Ordinary $RunLabel input, movement and restart self-test passed: $summaryPath"
return [pscustomobject]$summary

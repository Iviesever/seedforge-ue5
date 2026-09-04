[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 300,
    [string]$ExpectedRevision
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$revision = $verificationContext.ExpectedRevision
$childSnapshots = [Collections.Generic.List[object]]::new()
function Invoke-PackageEvidenceStep([string]$Name,[scriptblock]$Action) {
    $result = Invoke-SeedForgeScriptStep -Context $verificationContext -Name $Name -Action $Action
    # Freeze before any later child can change an earlier raw summary/log.
    $records = @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence $result)
    $childSnapshots.Add([pscustomobject]@{name=$Name;frozenEvidence=$records})
    return $result
}

$package = Invoke-PackageEvidenceStep -Name 'Win64 build and ordinary capture' -Action {
    & (Join-Path $PSScriptRoot 'PackageDemo.ps1') -EngineRoot $EngineRoot -SmokeSeed $Seed -SmokeTimeoutSeconds $TimeoutSeconds -ExpectedRevision $revision
}
$package = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $package.ManifestPath
if (-not (Test-Path -LiteralPath $package.executable -PathType Leaf)) { throw 'Fresh package has no executable.' }

# Mandatory ordinary input evidence: no gameplay-smoke state driver in this process.
$inputResult = Invoke-PackageEvidenceStep -Name 'Ordinary packaged input and restart self-test' -Action {
    & (Join-Path $PSScriptRoot 'TestInputSelfTest.ps1') -EngineRoot $EngineRoot -Seed $Seed -Executable $package.executable -RunLabel packaged -ExpectedRevision $revision
}
$smoke = Invoke-PackageEvidenceStep -Name 'Packaged gameplay smoke and captures' -Action {
    & (Join-Path $PSScriptRoot 'TestGameplay.ps1') -EngineRoot $EngineRoot -Seed $Seed -TimeoutSeconds $TimeoutSeconds -Executable $package.executable -RunLabel packaged -ExpectedRevision $revision
}
foreach ($result in @($inputResult,$smoke)) {
    if ($result.result -cne 'Passed' -or $result.sourceRevision -cne $revision -or $result.runLabel -cne 'packaged') {
        throw 'Packaged runtime summary does not prove the exact clean source revision.'
    }
}
$negative = @(foreach ($case in @('Grid','Encounter','CapturePath','RenderUnavailable')) {
    Invoke-PackageEvidenceStep -Name "Packaged failure $case" -Action {
        & (Join-Path $PSScriptRoot 'TestRunFailure.ps1') -EngineRoot $EngineRoot -Executable $package.executable -Case $case -ExpectedRevision $revision
    }
})
Assert-SeedForgeScriptContext -Context $verificationContext
$package = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $package.ManifestPath
$frozenChildren = @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence $childSnapshots.ToArray())
Assert-SeedForgeEvidenceRecords -Context $verificationContext -Records $frozenChildren | Out-Null
$timestamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$manifestPath = Join-Path $projectRoot "Artifacts/Package/gameplay-package-$timestamp.json"
$manifest = [ordered]@{
    completedAt=[DateTimeOffset]::UtcNow.ToString('o'); sourceRevision=$revision; version=$package.version; result='Passed'
    seed=$smoke.seed; layoutHash=$smoke.layoutHash; encounterHash=$smoke.encounterHash
    ordinaryLaunchLog=$package.smokeLog; ordinaryLaunchCapture=$package.smokeCapture
    executable=$package.executable; packageDirectory=$package.packageDirectory
    archive=$package.archive; sha256=$package.sha256; packageManifest=$package.ManifestPath
    packageProof=$package
    gameplayTrace=$smoke.trace; gameplaySummary=$smoke.summaryPath; gameplayScreenshots=$smoke.screenshots; gameplayLog=$smoke.log
    inputSummary=$inputResult.summaryPath; inputTrace=$inputResult.trace; inputLog=$inputResult.log
    inputProof=$inputResult; gameplayProof=$smoke; frozenEvidence=$frozenChildren
    negativeRuns=$negative
}
[IO.File]::WriteAllText($manifestPath,($manifest | ConvertTo-Json -Depth 12))
$verifiedManifest = Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $manifestPath
Assert-SeedForgeScriptContext -Context $verificationContext
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $projectRoot 'Artifacts/Package/last-gameplay-package.json')
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "Win64 build, ordinary input/restarts, positive/negative gameplay and captures passed: $manifestPath"
return $verifiedManifest

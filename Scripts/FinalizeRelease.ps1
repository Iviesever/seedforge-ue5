[CmdletBinding()]
param(
    [string]$ExpectedRevision,
    [string]$VisualReviewPath,
    [string]$RemoteReviewPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
$revision = $verificationContext.ExpectedRevision
& (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean
$pointerPath = Join-Path $projectRoot 'Artifacts/Reports/phase3-verification-last.json'
$pointer = Get-Content -Raw -LiteralPath $pointerPath | ConvertFrom-Json
$summaryFile = Get-SeedForgeArtifactFile $projectRoot $pointer.summaryPath 'machine summary'
$machine = Get-Content -Raw -LiteralPath $summaryFile.FullName | ConvertFrom-Json
if ($machine.schema -cne 'seedforge.phase3-verification' -or $machine.result -cne 'MachinePassed' -or $machine.sourceRevision -cne $revision -or $machine.version -cne '0.3.0') {
    throw 'Machine verification does not prove the exact 0.3.0 candidate.'
}
$machineIndexPath = Join-Path $summaryFile.DirectoryName 'evidence-index.json'
$machineIndex = Assert-SeedForgeEvidenceIndex -Context $verificationContext -Path $machineIndexPath
$machineFrozen = @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence @($machineIndex,($machineIndexPath+'.sha256')))
if (@($machineIndex.files | Where-Object { $_.path -ieq $summaryFile.FullName }).Count -ne 1) { throw 'Machine summary is absent from its independent evidence index.' }
Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $machine.plugin.ManifestPath | Out-Null
Assert-SeedForgeArtifactManifest -Context $verificationContext -ManifestPath $machine.gameplay.ManifestPath | Out-Null

# These records are created only after actual original-resolution visual review
# and a fresh remote fetch/API review by the primary agent, never by this script.
if ([string]::IsNullOrWhiteSpace($VisualReviewPath) -or [string]::IsNullOrWhiteSpace($RemoteReviewPath)) {
    throw 'Finalization requires explicit visual and remote review records; machine gates alone are insufficient.'
}
$visualFile = Get-SeedForgeArtifactFile $projectRoot $VisualReviewPath 'visual review'
$remoteFile = Get-SeedForgeArtifactFile $projectRoot $RemoteReviewPath 'remote review'
$reviewFrozen = @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence @($visualFile.FullName,$remoteFile.FullName))
$visual = Get-Content -Raw -LiteralPath $visualFile.FullName | ConvertFrom-Json
$remote = Get-Content -Raw -LiteralPath $remoteFile.FullName | ConvertFrom-Json
Assert-SeedForgeEvidenceRecords -Context $verificationContext -Records $reviewFrozen | Out-Null
foreach ($review in @($visual,$remote)) {
    if ($review.sourceRevision -cne $revision -or $review.result -cne 'Passed' -or $review.verificationSummary -ine $summaryFile.FullName) {
        throw 'Review record does not identify this exact candidate and machine run.'
    }
}
if ($visual.schema -cne 'seedforge.visual-review' -or $remote.schema -cne 'seedforge.remote-review' -or $remote.prNumber -ne 1 -or $remote.headRevision -cne $revision -or $remote.behind -ne 0) {
    throw 'Visual/remote review schema, PR, head or base freshness contract failed.'
}
$requiredImages = @(@($machine.editorGameplay.screenshots) + @($machine.gameplay.gameplayScreenshots) | ForEach-Object { [IO.Path]::GetFullPath($_) })
if ($requiredImages.Count -ne 6 -or @($visual.images).Count -ne 6) { throw 'Exactly six Editor/packaged gameplay image reviews are required.' }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($entry in $visual.images) {
    $digest = Get-SeedForgeEvidenceDigest $projectRoot $entry.path
    if (-not $seen.Add($digest.path) -or $requiredImages -inotcontains $digest.path) {
        throw 'Visual review contains a duplicate or unexpected screenshot.'
    }
    if ($entry.result -cne 'Passed' -or $entry.sha256 -cne $digest.sha256 -or $entry.width -ne 1280 -or $entry.height -ne 720) {
        throw 'Reviewed screenshot bytes/resolution/result do not match current evidence.'
    }
}
$stamp = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N')
$finalRoot = Join-Path $projectRoot "Artifacts/Final/SeedForge-0.3.0-$stamp"
New-Item -ItemType Directory -Path $finalRoot | Out-Null
$sourceArchive = Join-Path $finalRoot "SeedForge-Source-$($revision.Substring(0,10)).zip"
Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Verified source archive' -Action {
    & git -C $projectRoot archive --format=zip "--output=$sourceArchive" $revision
    if ($LASTEXITCODE -ne 0) { throw 'Source archive failed.' }
}
$finalPath = Join-Path $finalRoot 'release-readiness.json'
$final = [ordered]@{
    schema='seedforge.release-readiness'; schemaVersion=1; version='0.3.0'; sourceRevision=$revision; result='Passed'
    machineSummary=$summaryFile.FullName; machineEvidenceIndex=$machineIndexPath
    visualReview=$visualFile.FullName; remoteReview=$remoteFile.FullName
    sourceArchive=$sourceArchive; distribution='GitHub default source archives only; no binary assets uploaded'
    published=$false; summaryPath=$finalPath
}
Assert-SeedForgeScriptContext -Context $verificationContext
[IO.File]::WriteAllText($finalPath,($final | ConvertTo-Json -Depth 7))
$finalExpected = @(Get-SeedForgeEvidenceSnapshot -Context $verificationContext -Evidence @(
    $machineFrozen,$reviewFrozen,$finalPath,$sourceArchive))
$finalIndexPath = Join-Path $finalRoot 'evidence-index.json'
Write-SeedForgeEvidenceIndex -Context $verificationContext -Path $finalIndexPath -Paths @($finalExpected | ForEach-Object { $_.path }) -ExpectedRecords $finalExpected | Out-Null
Assert-SeedForgeScriptContext -Context $verificationContext
Write-Host "Release readiness verified locally for $revision. This script does not merge, tag, upload or publish."
return [pscustomobject]$final

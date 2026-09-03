[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$artifactRoot = Join-Path $projectRoot 'Artifacts'
$releaseRoot = Join-Path $artifactRoot 'Release'
$finalRoot = Join-Path $artifactRoot 'Final'
$verificationPath = Join-Path $artifactRoot 'Reports\verification-last.json'
$packageManifestPath = Join-Path $artifactRoot 'Package\last-package.json'
$pluginManifestPath = Join-Path $artifactRoot 'Plugin\last-plugin-package.json'
$pluginDescriptorPath = Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin'

& (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean

foreach ($required in @($verificationPath, $packageManifestPath, $pluginManifestPath, $pluginDescriptorPath)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required finalization input is missing: '$required'."
    }
}

$revision = (git -C $projectRoot rev-parse HEAD).Trim()
$shortRevision = (git -C $projectRoot rev-parse --short=10 HEAD).Trim()
$version = (Get-Content -Raw -LiteralPath $pluginDescriptorPath | ConvertFrom-Json).VersionName
$verification = Get-Content -Raw -LiteralPath $verificationPath | ConvertFrom-Json
$packageManifest = Get-Content -Raw -LiteralPath $packageManifestPath | ConvertFrom-Json
$pluginManifest = Get-Content -Raw -LiteralPath $pluginManifestPath | ConvertFrom-Json

if ($verification.result -ne 'Passed' -or
    $verification.sourceRevision -ne $revision -or
    $verification.version -ne $version) {
    throw "Verification summary does not prove version $version at revision $revision."
}
if ($packageManifest.sourceRevision -ne $revision -or $packageManifest.version -ne $version) {
    throw "Win64 package manifest does not prove version $version at revision $revision."
}
if ($pluginManifest.sourceRevision -ne $revision -or $pluginManifest.version -ne $version) {
    throw "Plugin package manifest does not prove version $version at revision $revision."
}

$phase2SummaryPath = [string]$verification.phase2ReportSummary
if (-not (Test-Path -LiteralPath $phase2SummaryPath)) {
    throw "Phase 2 report summary is missing: '$phase2SummaryPath'."
}
$phase2Summary = Get-Content -Raw -LiteralPath $phase2SummaryPath | ConvertFrom-Json
if ($phase2Summary.sourceRevision -ne $revision) {
    throw "Phase 2 report summary does not prove revision $revision."
}

function Resolve-ArtifactChild {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$Label
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootPrefix = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    if (-not $fullPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label escapes its required artifact root: '$fullPath'."
    }
    if (-not (Test-Path -LiteralPath $fullPath)) {
        throw "$Label is missing: '$fullPath'."
    }
    return $fullPath
}

$pluginArchivePath = Resolve-ArtifactChild -Path ([string]$pluginManifest.archive) -Root $releaseRoot -Label 'Plugin archive'
$demoArchivePath = Resolve-ArtifactChild -Path ([string]$packageManifest.archive) -Root $releaseRoot -Label 'Demo archive'
$phase2RunRoot = Resolve-ArtifactChild -Path (Split-Path -Parent $phase2SummaryPath) -Root (Join-Path $artifactRoot 'Reports\Phase2') -Label 'Phase 2 report run'
$automationReportPath = Resolve-ArtifactChild -Path ([string]$verification.automationReport) -Root (Join-Path $artifactRoot 'Reports') -Label 'Automation report'
$editorCapture = Resolve-ArtifactChild -Path ([string]$verification.editorCapture) -Root (Join-Path $artifactRoot 'Media') -Label 'Editor capture'
$inspectorCapture = Resolve-ArtifactChild -Path ([string]$verification.inspectorCapture) -Root (Join-Path $artifactRoot 'Media') -Label 'Inspector capture'
$packagedCapture = Resolve-ArtifactChild -Path ([string]$verification.packagedCapture) -Root (Join-Path $artifactRoot 'Media') -Label 'Packaged capture'

foreach ($archivePath in @($pluginArchivePath, $demoArchivePath)) {
    $checksumPath = "$archivePath.sha256"
    if (-not (Test-Path -LiteralPath $checksumPath)) {
        throw "Checksum is missing for '$archivePath'."
    }
    $expected = ((Get-Content -Raw -LiteralPath $checksumPath).Trim() -split '\s+')[0]
    $actual = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected.ToLowerInvariant()) {
        throw "Checksum mismatch for '$archivePath'."
    }
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$deliveryRoot = Join-Path $finalRoot "SeedForge-$version-$shortRevision-$timestamp"
$deliveryArtifacts = Join-Path $deliveryRoot 'artifacts'
$deliveryEvidence = Join-Path $deliveryRoot 'evidence'
$deliveryDocs = Join-Path $deliveryRoot 'docs'
$phase1Evidence = Join-Path $deliveryEvidence 'phase1-task'
$phase2Evidence = Join-Path $deliveryEvidence 'phase2-task'
New-Item -ItemType Directory -Force -Path $deliveryArtifacts, $deliveryEvidence, $deliveryDocs, $phase1Evidence, $phase2Evidence | Out-Null

Copy-Item -LiteralPath $pluginArchivePath, "$pluginArchivePath.sha256", $demoArchivePath, "$demoArchivePath.sha256" -Destination $deliveryArtifacts
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md'), (Join-Path $projectRoot 'LICENSE') -Destination $deliveryRoot
Copy-Item -Recurse -Path (Join-Path $projectRoot 'docs\*') -Destination $deliveryDocs
Copy-Item -LiteralPath $verificationPath, $packageManifestPath, $pluginManifestPath -Destination $deliveryEvidence
Copy-Item -Recurse -LiteralPath $phase2RunRoot -Destination (Join-Path $deliveryEvidence 'phase2-report')
Copy-Item -Recurse -LiteralPath $automationReportPath -Destination (Join-Path $deliveryEvidence 'automation-report')

Copy-Item -LiteralPath (Join-Path $projectRoot 'tasks\20260903-113501-ue58-agentic-sprint\issue.md'), (Join-Path $projectRoot 'tasks\20260903-113501-ue58-agentic-sprint\implementation_plan.md'), (Join-Path $projectRoot 'tasks\20260903-113501-ue58-agentic-sprint\evidence.md') -Destination $phase1Evidence
Copy-Item -Path (Join-Path $projectRoot 'tasks\20260903-143421-phase2-observability\*.md') -Destination $phase2Evidence

$logPatterns = @(
    'build-editor-*.log',
    'smoke-editor-*.log',
    'automation-*.log',
    'capture-demo-*.log',
    'capture-inspector-*.log',
    'package-plugin-*.log',
    'uat-package-plugin-*.log',
    'package-demo-*.log',
    'uat-package-demo-*.log',
    'smoke-packaged-*.log'
)
foreach ($pattern in $logPatterns) {
    $latestLog = Get-ChildItem -File -LiteralPath (Join-Path $artifactRoot 'Logs') -Filter $pattern |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $latestLog) {
        throw "Required final log is missing for pattern '$pattern'."
    }
    Copy-Item -LiteralPath $latestLog.FullName -Destination $deliveryEvidence
}

Copy-Item -LiteralPath $editorCapture, $inspectorCapture, $packagedCapture -Destination $deliveryEvidence

$sourceArchive = Join-Path $deliveryArtifacts "SeedForge-Source-$shortRevision.zip"
git -C $projectRoot archive --format=zip --output=$sourceArchive HEAD
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $sourceArchive)) {
    throw 'git archive failed to create the source package.'
}

$manifestEntries = @(
    foreach ($file in Get-ChildItem -File -Recurse -LiteralPath $deliveryRoot | Sort-Object FullName) {
        [ordered]@{
            path = [System.IO.Path]::GetRelativePath($deliveryRoot, $file.FullName).Replace('\', '/')
            size = $file.Length
            sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
)
$deliveryManifest = [ordered]@{
    project = 'SeedForge'
    version = $version
    createdAt = (Get-Date).ToString('o')
    sourceRevision = $revision
    engine = 'Unreal Engine 5.8.0'
    platform = 'Win64'
    verification = 'Passed'
    manifestScope = 'Every payload file; the manifest itself is protected by the adjacent SHA-256 file.'
    files = $manifestEntries
}
$deliveryManifestPath = Join-Path $deliveryRoot 'DELIVERY_MANIFEST.json'
[System.IO.File]::WriteAllText(
    $deliveryManifestPath,
    ($deliveryManifest | ConvertTo-Json -Depth 7) + "`r`n")
$manifestHash = (Get-FileHash -LiteralPath $deliveryManifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText(
    "$deliveryManifestPath.sha256",
    "$manifestHash  DELIVERY_MANIFEST.json`r`n")

& (Join-Path $PSScriptRoot 'AuditDelivery.ps1') -DeliveryRoot $deliveryRoot -ExpectedRevision $revision -ExpectedVersion $version

$latestPointer = Join-Path $finalRoot 'LATEST.txt'
[System.IO.File]::WriteAllText($latestPointer, "$deliveryRoot`r`n")

Write-Host "Final SeedForge delivery assembled and independently rehashed."
Write-Host "Delivery: $deliveryRoot"
Write-Host "Manifest: $deliveryManifestPath"

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

& (Join-Path $PSScriptRoot 'AuditRepository.ps1') -RequireClean

if (-not (Test-Path -LiteralPath $verificationPath)) {
    throw "Final verification summary is missing: '$verificationPath'."
}
if (-not (Test-Path -LiteralPath $packageManifestPath)) {
    throw "Final package manifest is missing: '$packageManifestPath'."
}

$revision = (git -C $projectRoot rev-parse HEAD).Trim()
$shortRevision = (git -C $projectRoot rev-parse --short=10 HEAD).Trim()
$verification = Get-Content -Raw -LiteralPath $verificationPath | ConvertFrom-Json
$packageManifest = Get-Content -Raw -LiteralPath $packageManifestPath | ConvertFrom-Json
if ($verification.result -ne 'Passed' -or $verification.sourceRevision -ne $revision) {
    throw "Verification summary does not prove current revision $revision."
}
if ($packageManifest.sourceRevision -ne $revision) {
    throw "Package manifest does not prove current revision $revision."
}

$pluginArchive = Get-ChildItem -File -LiteralPath $releaseRoot -Filter 'SeedForgePlugin-*.zip' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
$demoArchive = Get-ChildItem -File -LiteralPath $releaseRoot -Filter 'SeedForgeDemo-Win64-*.zip' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $pluginArchive -or -not $demoArchive) {
    throw 'Final plugin or demo archive is missing.'
}

foreach ($archive in @($pluginArchive, $demoArchive)) {
    $checksumPath = "$($archive.FullName).sha256"
    if (-not (Test-Path -LiteralPath $checksumPath)) {
        throw "Checksum is missing for '$($archive.FullName)'."
    }
    $expected = ((Get-Content -Raw -LiteralPath $checksumPath).Trim() -split '\s+')[0]
    $actual = (Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected.ToLowerInvariant()) {
        throw "Checksum mismatch for '$($archive.FullName)'."
    }
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$deliveryRoot = Join-Path $finalRoot "SeedForge-0.1.0-$shortRevision-$timestamp"
$deliveryArtifacts = Join-Path $deliveryRoot 'artifacts'
$deliveryEvidence = Join-Path $deliveryRoot 'evidence'
$deliveryDocs = Join-Path $deliveryRoot 'docs'
New-Item -ItemType Directory -Force -Path $deliveryArtifacts, $deliveryEvidence, $deliveryDocs | Out-Null

Copy-Item -LiteralPath $pluginArchive.FullName, "$($pluginArchive.FullName).sha256", $demoArchive.FullName, "$($demoArchive.FullName).sha256" -Destination $deliveryArtifacts
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md'), (Join-Path $projectRoot 'LICENSE') -Destination $deliveryRoot
Copy-Item -Recurse -Path (Join-Path $projectRoot 'docs\*') -Destination $deliveryDocs
Copy-Item -LiteralPath $verificationPath, $packageManifestPath -Destination $deliveryEvidence
Copy-Item -LiteralPath (Join-Path $projectRoot 'tasks\20260903-113501-ue58-agentic-sprint\issue.md'), (Join-Path $projectRoot 'tasks\20260903-113501-ue58-agentic-sprint\implementation_plan.md'), (Join-Path $projectRoot 'tasks\20260903-113501-ue58-agentic-sprint\evidence.md') -Destination $deliveryEvidence

$latestAutomation = Get-ChildItem -Directory -LiteralPath (Join-Path $artifactRoot 'Reports') -Filter 'automation-*' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if ($latestAutomation) {
    Copy-Item -Recurse -LiteralPath $latestAutomation.FullName -Destination (Join-Path $deliveryEvidence 'automation-report')
}

$logPatterns = @(
    'build-editor-*.log',
    'smoke-editor-*.log',
    'automation-*.log',
    'capture-demo-*.log',
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
    if ($latestLog) {
        Copy-Item -LiteralPath $latestLog.FullName -Destination $deliveryEvidence
    }
}

$editorCapture = Join-Path $artifactRoot 'Media\SeedForge-24301.png'
$packagedCapture = Join-Path $artifactRoot 'Media\SeedForge-Packaged-24301.png'
Copy-Item -LiteralPath $editorCapture, $packagedCapture -Destination $deliveryEvidence

$sourceArchive = Join-Path $deliveryArtifacts "SeedForge-Source-$shortRevision.zip"
git -C $projectRoot archive --format=zip --output=$sourceArchive HEAD
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $sourceArchive)) {
    throw 'git archive failed to create the source package.'
}

$manifestEntries = @()
foreach ($file in Get-ChildItem -File -Recurse -LiteralPath $deliveryRoot) {
    $manifestEntries += [ordered]@{
        path = [System.IO.Path]::GetRelativePath($deliveryRoot, $file.FullName).Replace('\', '/')
        size = $file.Length
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$deliveryManifest = [ordered]@{
    project = 'SeedForge'
    version = '0.1.0'
    createdAt = (Get-Date).ToString('o')
    sourceRevision = $revision
    engine = 'Unreal Engine 5.8.0'
    verification = 'Passed'
    files = $manifestEntries
}
$deliveryManifestPath = Join-Path $deliveryRoot 'DELIVERY_MANIFEST.json'
[System.IO.File]::WriteAllText(
    $deliveryManifestPath,
    ($deliveryManifest | ConvertTo-Json -Depth 6) + "`r`n")

$latestPointer = Join-Path $finalRoot 'LATEST.txt'
[System.IO.File]::WriteAllText($latestPointer, "$deliveryRoot`r`n")

Write-Host "Final SeedForge delivery assembled and checksummed."
Write-Host "Delivery: $deliveryRoot"
Write-Host "Manifest: $deliveryManifestPath"

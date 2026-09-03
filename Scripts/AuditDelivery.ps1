[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$DeliveryRoot,
    [string]$ExpectedRevision,
    [string]$ExpectedVersion
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$resolvedRoot = (Resolve-Path -LiteralPath $DeliveryRoot).Path
$manifestPath = Join-Path $resolvedRoot 'DELIVERY_MANIFEST.json'
$manifestChecksumPath = "$manifestPath.sha256"
if (-not (Test-Path -LiteralPath $manifestPath) -or
    -not (Test-Path -LiteralPath $manifestChecksumPath)) {
    throw "Delivery manifest or its checksum is missing under '$resolvedRoot'."
}

$expectedManifestHash = ((Get-Content -Raw -LiteralPath $manifestChecksumPath).Trim() -split '\s+')[0].ToLowerInvariant()
$actualManifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actualManifestHash -ne $expectedManifestHash) {
    throw "Delivery manifest checksum mismatch: expected $expectedManifestHash, actual $actualManifestHash."
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
if ($ExpectedRevision -and $manifest.sourceRevision -ne $ExpectedRevision) {
    throw "Delivery revision '$($manifest.sourceRevision)' does not match '$ExpectedRevision'."
}
if ($ExpectedVersion -and $manifest.version -ne $ExpectedVersion) {
    throw "Delivery version '$($manifest.version)' does not match '$ExpectedVersion'."
}

$controlNames = @('DELIVERY_MANIFEST.json', 'DELIVERY_MANIFEST.json.sha256')
$actualFiles = @(Get-ChildItem -File -Recurse -LiteralPath $resolvedRoot | Where-Object {
    $_.Name -notin $controlNames
})
$manifestEntries = @($manifest.files)
if ($actualFiles.Count -ne $manifestEntries.Count) {
    throw "Delivery payload count mismatch: manifest=$($manifestEntries.Count), actual=$($actualFiles.Count)."
}

$seen = @{}
foreach ($entry in $manifestEntries) {
    $relativePath = [string]$entry.path
    if ($seen.ContainsKey($relativePath)) {
        throw "Duplicate delivery manifest path '$relativePath'."
    }
    $seen[$relativePath] = $true

    $candidate = Join-Path $resolvedRoot ($relativePath.Replace('/', '\'))
    $fullCandidate = [System.IO.Path]::GetFullPath($candidate)
    $rootPrefix = $resolvedRoot.TrimEnd('\') + '\'
    if (-not $fullCandidate.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Manifest path escapes the delivery root: '$relativePath'."
    }
    if (-not (Test-Path -LiteralPath $fullCandidate -PathType Leaf)) {
        throw "Manifest payload is missing: '$relativePath'."
    }

    $file = Get-Item -LiteralPath $fullCandidate
    if ([Int64]$entry.size -ne $file.Length) {
        throw "Size mismatch for '$relativePath'."
    }
    $actualHash = (Get-FileHash -LiteralPath $fullCandidate -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne ([string]$entry.sha256).ToLowerInvariant()) {
        throw "SHA-256 mismatch for '$relativePath'."
    }
}

foreach ($file in $actualFiles) {
    $relativePath = [System.IO.Path]::GetRelativePath($resolvedRoot, $file.FullName).Replace('\', '/')
    if (-not $seen.ContainsKey($relativePath)) {
        throw "Unmanifested delivery payload '$relativePath'."
    }
}

Write-Host "Delivery audit passed: $($actualFiles.Count) payload files, revision $($manifest.sourceRevision), version $($manifest.version)."

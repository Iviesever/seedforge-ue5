[CmdletBinding()]
param(
    [switch]$RequireClean
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
    'SeedForge.uproject',
    'Plugins\SeedForge\SeedForge.uplugin',
    'README.md',
    'LICENSE',
    'docs\ARCHITECTURE.md',
    'docs\DEVELOPMENT.md',
    'docs\ACCEPTANCE_MATRIX.md',
    'docs\AI_ASSISTANCE.md',
    'docs\CODE_WALKTHROUGH.md',
    'docs\INTERVIEW_GUIDE.md',
    'docs\KNOWN_LIMITATIONS.md',
    'docs\ROLLBACK.md'
)

foreach ($relativePath in $requiredFiles) {
    $path = Join-Path $projectRoot $relativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required repository file is missing: $relativePath"
    }
}

Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'SeedForge.uproject') | ConvertFrom-Json | Out-Null
Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Plugins\SeedForge\SeedForge.uplugin') | ConvertFrom-Json | Out-Null

git -C $projectRoot diff --check
if ($LASTEXITCODE -ne 0) {
    throw 'git diff --check failed.'
}

$trackedFiles = @(git -C $projectRoot ls-files)
$forbiddenTracked = @($trackedFiles | Where-Object {
    $_ -match '(^|/)(Artifacts|Binaries|Intermediate|Saved|DerivedDataCache|\.cache|\.user)(/|$)'
})
if ($forbiddenTracked.Count -ne 0) {
    throw "Generated paths are tracked:`n$($forbiddenTracked -join "`n")"
}

$sourceFiles = @(
    Get-ChildItem -File -Recurse -LiteralPath (Join-Path $projectRoot 'Source'), (Join-Path $projectRoot 'Plugins\SeedForge\Source') |
        Where-Object { $_.Extension -in '.h', '.cpp', '.cs' }
)
$unfinished = @($sourceFiles | Select-String -Pattern '\bTODO\b|\bTBD\b|NotImplemented')
if ($unfinished.Count -ne 0) {
    throw "Unfinished marker found:`n$($unfinished -join "`n")"
}

if ($RequireClean) {
    $status = @(git -C $projectRoot status --porcelain)
    if ($status.Count -ne 0) {
        throw "A clean worktree is required:`n$($status -join "`n")"
    }
}

Write-Host "Repository audit passed: $($trackedFiles.Count) tracked files, $($sourceFiles.Count) source/build-rule files."


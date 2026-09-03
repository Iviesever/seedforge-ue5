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
    'docs\FINAL_HANDOFF.md',
    'docs\ROLLBACK.md',
    'docs\RELEASE_NOTES.md',
    'docs\LAYOUT_FORMAT.md',
    'docs\BENCHMARKING.md',
    'docs\PHASE3_ARCHITECTURE.md',
    'docs\GAMEPLAY_LOOP.md',
    'docs\PHASE3_CODE_WALKTHROUGH.md',
    'docs\PHASE3_INTERVIEW_GUIDE.md',
    'docs\LIVE_CHANGE_DRILLS.md',
    'docs\PHASE3_ACCEPTANCE_MATRIX.md',
    'docs\PHASE3_EVIDENCE_JOURNAL.md',
    'docs\PHASE3_CANDIDATE_RELEASE_NOTES.md',
    'docs\images\phase3-start-24301.png',
    'docs\images\phase3-combat-24301.png',
    'docs\images\phase3-win-24301.png'
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

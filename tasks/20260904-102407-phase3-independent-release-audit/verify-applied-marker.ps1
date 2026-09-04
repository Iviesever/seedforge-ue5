[CmdletBinding()]
param([string]$LogPath)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$validMarker = 'LogSeedForge: Applied request=3 run=2 seed=24301 hash=7425849530159566348 floors=587 walls=436 gameplay=true.'
$invalidMarkers = @(
    $validMarker.Replace(' run=2', ''),
    $validMarker.Replace('request=3', 'request=0'),
    $validMarker.Replace('run=2', 'run=0'),
    $validMarker.Replace(' gameplay=true', ''),
    $validMarker.Replace('hash=7425849530159566348', 'hash=0'),
    $validMarker.Replace('floors=587', 'floors=0'),
    ($validMarker + ' run=99')
)
foreach ($markerScript in @('Scripts\CaptureDemo.ps1', 'Scripts\PackageDemo.ps1')) {
    $parseTokens = $null
    $parseErrors = $null
    $parsed = [System.Management.Automation.Language.Parser]::ParseFile(
        (Join-Path $projectRoot $markerScript), [ref]$parseTokens, [ref]$parseErrors)
    if ($parseErrors.Count -ne 0) { throw "Script parse failed: $markerScript" }
    $patterns = @($parsed.FindAll({
        param($node)
        $node -is [System.Management.Automation.Language.StringConstantExpressionAst] -and
            $node.Value.StartsWith('Applied request=')
    }, $true))
    if ($patterns.Count -ne 1) { throw "Expected one applied marker pattern: $markerScript" }
    if ($validMarker -notmatch $patterns[0].Value) { throw "$markerScript rejected correct marker" }
    foreach ($invalidMarker in $invalidMarkers) {
        if ($invalidMarker -match $patterns[0].Value) { throw "$markerScript accepted malformed marker: $invalidMarker" }
    }
    if ($LogPath -and -not (Select-String -LiteralPath $LogPath -Pattern $patterns[0].Value -Quiet)) {
        throw "$markerScript did not match the real process log: $LogPath"
    }
    [pscustomobject]@{
        script = $markerScript
        validAccepted = 1
        invalidRejected = $invalidMarkers.Count
        parseErrors = $parseErrors.Count
        realLog = $LogPath
    } | ConvertTo-Json -Compress
}

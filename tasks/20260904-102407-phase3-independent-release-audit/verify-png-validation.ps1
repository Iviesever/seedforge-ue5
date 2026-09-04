[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/PngValidation.ps1')

$fixtureId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N')
$fixtureRoot = Join-Path $projectRoot "Artifacts/Reports/PngValidation/$fixtureId"
$runDirectory = Join-Path $fixtureRoot 'run'
$siblingDirectory = Join-Path $fixtureRoot 'run-sibling'
New-Item -ItemType Directory -Path $runDirectory, $siblingDirectory | Out-Null
$sourcePath = Join-Path $projectRoot 'Artifacts/Media/Gameplay/20260904-120630/SeedForge-Gameplay-start-24301.png'
$sourceBytes = [IO.File]::ReadAllBytes($sourcePath)
$requestedAt = [DateTimeOffset]::UtcNow
$results = [Collections.Generic.List[object]]::new()

function New-Fixture {
    param([string]$Name, [byte[]]$Bytes = $sourceBytes, [string]$Directory = $runDirectory)
    $fixturePath = Join-Path $Directory $Name
    [IO.File]::WriteAllBytes($fixturePath, $Bytes)
    $freshUtc = [DateTime]::UtcNow
    [IO.File]::SetCreationTimeUtc($fixturePath, $freshUtc)
    [IO.File]::SetLastWriteTimeUtc($fixturePath, $freshUtc)
    return $fixturePath
}

function Invoke-Case {
    param([string]$Name, [scriptblock]$Check)
    try {
        $detail = @(& $Check) -join [Environment]::NewLine
        $results.Add([pscustomobject]@{ name = $Name; passed = $true; detail = $detail })
        Write-Host "PASS $Name"
    }
    catch {
        $results.Add([pscustomobject]@{ name = $Name; passed = $false; detail = $_.Exception.Message })
        Write-Host "FAIL $Name : $($_.Exception.Message)"
    }
}

function Assert-Rejected {
    param([scriptblock]$Operation, [string]$ReasonPattern = '')
    $rejected = $false
    $reason = ''
    try { & $Operation | Out-Null } catch { $rejected = $true; $reason = $_.Exception.Message }
    if (-not $rejected) { throw 'Invalid PNG input was accepted.' }
    if ($ReasonPattern -and $reason -notmatch $ReasonPattern) { throw "Wrong rejection reason: $reason" }
    return $reason
}

function Assert-That {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Set-FixtureChunkCrc {
    param([byte[]]$Bytes, [int]$Offset)
    $length = [long]$Bytes[$Offset] * 16777216 + [long]$Bytes[$Offset + 1] * 65536 + [long]$Bytes[$Offset + 2] * 256 + $Bytes[$Offset + 3]
    [uint32]$crc = 4294967295
    for ($index = $Offset + 4; $index -lt $Offset + 8 + $length; ++$index) {
        $crc = $crc -bxor $Bytes[$index]
        for ($bit = 0; $bit -lt 8; ++$bit) {
            if (($crc -band 1) -ne 0) { $crc = ($crc -shr 1) -bxor [uint32]3988292384 }
            else { $crc = $crc -shr 1 }
        }
    }
    $crc = $crc -bxor [uint32]4294967295
    $crcOffset = $Offset + 8 + $length
    for ($index = 0; $index -lt 4; ++$index) {
        $Bytes[$crcOffset + $index] = [byte](($crc -shr (24 - 8 * $index)) -band 255)
    }
}

$validPath = New-Fixture 'valid.png'
Invoke-Case 'fresh PNG returns verified metadata' {
    $record = Assert-SeedForgePng -Path $validPath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt
    Assert-That ($record.Path -ceq [IO.Path]::GetFullPath($validPath)) 'Path was not normalized.'
    Assert-That ($record.Width -eq 1280 -and $record.Height -eq 720) 'Wrong decoded dimensions.'
    Assert-That ($record.Length -eq $sourceBytes.LongLength) 'Wrong byte length.'
    Assert-That ($record.Sha256 -ceq (Get-FileHash -LiteralPath $validPath -Algorithm SHA256).Hash.ToLowerInvariant()) 'Wrong SHA256.'
    Assert-That ($record.CreationTimeUtc -ge $requestedAt.UtcDateTime) 'Wrong creation timestamp.'
    Assert-That ($record.LastWriteTimeUtc -ge $requestedAt.UtcDateTime) 'Wrong write timestamp.'
}
Invoke-Case 'normalizes dot segments inside run' {
    $record = Assert-SeedForgePng -Path (Join-Path $runDirectory './valid.png') -RunDirectory ($runDirectory + '/') -RequestedAtUtc $requestedAt
    Assert-That ($record.Path -ceq $validPath) 'Equivalent path was not normalized.'
}
Invoke-Case 'releases file handles after success' {
    Assert-SeedForgePng -Path $validPath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt | Out-Null
    $exclusive = [IO.File]::Open($validPath, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    $exclusive.Dispose()
}
Invoke-Case 'missing file rejected' {
    Assert-Rejected { Assert-SeedForgePng -Path (Join-Path $runDirectory 'missing.png') -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'directory rejected' {
    $directory = Join-Path $runDirectory 'directory.png'
    New-Item -ItemType Directory -Path $directory | Out-Null
    Assert-Rejected { Assert-SeedForgePng -Path $directory -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
$outsidePath = New-Fixture -Name 'outside.png' -Directory $fixtureRoot
$siblingPath = New-Fixture -Name 'sibling.png' -Directory $siblingDirectory
Invoke-Case 'outside current run rejected' {
    Assert-Rejected { Assert-SeedForgePng -Path $outsidePath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'sibling-prefix directory rejected' {
    Assert-Rejected { Assert-SeedForgePng -Path $siblingPath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'dot-dot escape rejected' {
    Assert-Rejected { Assert-SeedForgePng -Path (Join-Path $runDirectory '../outside.png') -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'wrong extension rejected' {
    $path = New-Fixture 'wrong.jpg'
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'stale creation rejected independently' {
    $path = New-Fixture 'stale-created.png'
    [IO.File]::SetCreationTimeUtc($path, $requestedAt.UtcDateTime.AddMinutes(-1))
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'stale last-write rejected independently' {
    $path = New-Fixture 'stale-written.png'
    [IO.File]::SetLastWriteTimeUtc($path, $requestedAt.UtcDateTime.AddMinutes(-1))
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'minimum byte size enforced' {
    Assert-Rejected { Assert-SeedForgePng -Path $validPath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt -MinimumBytes ($sourceBytes.LongLength + 1) }
}
Invoke-Case 'wrong requested width rejected' {
    Assert-Rejected { Assert-SeedForgePng -Path $validPath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt -Width 640 }
}
Invoke-Case 'wrong requested height rejected' {
    Assert-Rejected { Assert-SeedForgePng -Path $validPath -RunDirectory $runDirectory -RequestedAtUtc $requestedAt -Height 480 }
}
Invoke-Case 'wrong signature rejected' {
    $bytes = [byte[]]$sourceBytes.Clone()
    $bytes[0] = 0
    $path = New-Fixture -Name 'bad-signature.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'first chunk must be IHDR' {
    $bytes = [byte[]]$sourceBytes.Clone()
    $bytes[12] = 88
    $path = New-Fixture -Name 'bad-ihdr.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'truncated header rejected' {
    $path = New-Fixture -Name 'short-header.png' -Bytes ([byte[]]$sourceBytes[0..23])
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt -MinimumBytes 1 }
}
Invoke-Case 'large truncated image rejected' {
    $path = New-Fixture -Name 'truncated-data.png' -Bytes ([byte[]]$sourceBytes[0..([int]($sourceBytes.Length / 2))])
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'missing IEND rejected' {
    $path = New-Fixture -Name 'missing-iend.png' -Bytes ([byte[]]$sourceBytes[0..($sourceBytes.Length - 13)])
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'corrupt pixel data rejected' {
    $bytes = [byte[]]$sourceBytes.Clone()
    for ($offset = 8; $offset -lt $bytes.Length - 12;) {
        $length = [long]$bytes[$offset] * 16777216 + [long]$bytes[$offset + 1] * 65536 + [long]$bytes[$offset + 2] * 256 + $bytes[$offset + 3]
        if ([Text.Encoding]::ASCII.GetString($bytes, $offset + 4, 4) -eq 'IDAT') {
            $bytes[$offset + 8] = 0
            $bytes[$offset + 9] = 0
            break
        }
        $offset += 12 + $length
    }
    $path = New-Fixture -Name 'corrupt-data.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'invalid zlib header rejected even with valid chunk CRC' {
    $bytes = [IO.File]::ReadAllBytes((Join-Path $runDirectory 'corrupt-data.png'))
    Set-FixtureChunkCrc -Bytes $bytes -Offset 33
    $path = New-Fixture -Name 'invalid-zlib-valid-crc.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt } -ReasonPattern 'zlib header'
}
Invoke-Case 'invalid DEFLATE block rejected even with valid chunk CRC' {
    $bytes = [byte[]]$sourceBytes.Clone()
    # The baseline first IDAT starts at 33; after its zlib header, BTYPE=3 is reserved.
    Assert-That ([Text.Encoding]::ASCII.GetString($bytes, 37, 4) -eq 'IDAT') 'Fixture baseline changed its first chunk.'
    $bytes[43] = 7
    Set-FixtureChunkCrc -Bytes $bytes -Offset 33
    $path = New-Fixture -Name 'invalid-deflate-valid-crc.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'bad Adler checksum rejected even with valid chunk CRC' {
    $bytes = [byte[]]$sourceBytes.Clone()
    $lastIdat = 0
    $lastLength = 0
    for ($offset = 8; $offset -lt $bytes.Length - 12;) {
        $length = [long]$bytes[$offset] * 16777216 + [long]$bytes[$offset + 1] * 65536 + [long]$bytes[$offset + 2] * 256 + $bytes[$offset + 3]
        if ([Text.Encoding]::ASCII.GetString($bytes, $offset + 4, 4) -eq 'IDAT') { $lastIdat = $offset; $lastLength = $length }
        $offset += 12 + $length
    }
    $bytes[$lastIdat + 7 + $lastLength] = $bytes[$lastIdat + 7 + $lastLength] -bxor 1
    Set-FixtureChunkCrc -Bytes $bytes -Offset $lastIdat
    $path = New-Fixture -Name 'bad-adler-valid-crc.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt } -ReasonPattern 'Adler checksum'
}
Invoke-Case 'decoded byte count cannot exceed IHDR' {
    $bytes = [byte[]]$sourceBytes.Clone()
    $bytes[20] = 0; $bytes[21] = 0; $bytes[22] = 0; $bytes[23] = 1
    Set-FixtureChunkCrc -Bytes $bytes -Offset 8
    $path = New-Fixture -Name 'too-many-decoded-rows.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt -Height 1 } -ReasonPattern 'decoded data exceeds'
}
Invoke-Case 'decoded byte count cannot fall short of IHDR' {
    $bytes = [byte[]]$sourceBytes.Clone()
    $bytes[22] = 2; $bytes[23] = 209
    Set-FixtureChunkCrc -Bytes $bytes -Offset 8
    $path = New-Fixture -Name 'too-few-decoded-rows.png' -Bytes $bytes
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt -Height 721 } -ReasonPattern 'decoded data is truncated'
}
Invoke-Case 'releases file handles after rejection' {
    $path = Join-Path $runDirectory 'corrupt-data.png'
    Assert-Rejected { Assert-SeedForgePng -Path $path -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
    $exclusive = [IO.File]::Open($path, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    $exclusive.Dispose()
}
Invoke-Case 'reparse child traversal rejected' {
    $link = Join-Path $runDirectory 'linked'
    New-Item -ItemType Junction -Path $link -Target $siblingDirectory | Out-Null
    Assert-Rejected { Assert-SeedForgePng -Path (Join-Path $link 'sibling.png') -RunDirectory $runDirectory -RequestedAtUtc $requestedAt }
}
Invoke-Case 'reparse run directory rejected' {
    $link = Join-Path $fixtureRoot 'linked-run'
    New-Item -ItemType Junction -Path $link -Target $runDirectory | Out-Null
    Assert-Rejected { Assert-SeedForgePng -Path (Join-Path $link 'valid.png') -RunDirectory $link -RequestedAtUtc $requestedAt }
}

$passed = @($results | Where-Object { $_.passed }).Count
$failed = $results.Count - $passed
$summary = [ordered]@{
    purpose = 'Synthetic PNG validator acceptance fixtures; NOT release or rendered-state evidence.'
    fixtureDirectory = $fixtureRoot
    sourceBytesOnly = $sourcePath
    requestedAtUtc = $requestedAt.ToString('o')
    powershellVersion = $PSVersionTable.PSVersion.ToString()
    total = $results.Count
    passed = $passed
    failed = $failed
    results = @($results.ToArray())
}
[IO.File]::WriteAllText((Join-Path $fixtureRoot 'summary.json'), ($summary | ConvertTo-Json -Depth 6) + [Environment]::NewLine)
Write-Host "PNG_VALIDATION total=$($results.Count) passed=$passed failed=$failed fixtures=$fixtureRoot"
if ($failed -gt 0) { exit 1 }

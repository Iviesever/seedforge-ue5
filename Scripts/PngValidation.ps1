function Assert-SeedForgePng {
    <#
    .SYNOPSIS
    Verifies one fresh PNG owned by the supplied run directory.
    .DESCRIPTION
    Uses Windows System.Drawing to decode all pixels, not just image metadata.
    Validation is read-only; the caller owns capture cardinality and provenance.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$Path,
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$RunDirectory,
        [Parameter(Mandatory = $true)][DateTimeOffset]$RequestedAtUtc,
        [ValidateRange(1, 2147483647)][int]$Width = 1280,
        [ValidateRange(1, 2147483647)][int]$Height = 720,
        [ValidateRange(1, 9223372036854775807)][long]$MinimumBytes = 10240
    )

    # Function-local only: decoder errors must terminate even for permissive callers.
    $ErrorActionPreference = 'Stop'
    $fullPath = [IO.Path]::GetFullPath($Path)
    $runPath = [IO.Path]::GetFullPath($RunDirectory)
    $runPrefix = $runPath.TrimEnd([char[]]@('\', '/')) + [IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($runPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "PNG path is outside the current run directory: '$fullPath'."
    }
    if ([IO.Path]::GetExtension($fullPath) -ine '.png') {
        throw "PNG path must have a .png extension: '$fullPath'."
    }

    # Check from the volume root downward, before traversing each next component.
    # Checking only the leaf would miss directory junctions and symbolic links.
    $ancestors = [Collections.Generic.List[string]]::new()
    $currentPath = $fullPath
    while (-not [string]::IsNullOrEmpty($currentPath)) {
        $ancestors.Add($currentPath)
        $currentPath = [IO.Path]::GetDirectoryName($currentPath)
    }
    $ancestors.Reverse()
    $file = $null
    foreach ($ancestor in $ancestors) {
        $file = Get-Item -LiteralPath $ancestor -Force -ErrorAction Stop
        if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "PNG path traverses a reparse point: '$ancestor'."
        }
    }
    if ($file -isnot [IO.FileInfo]) {
        throw "PNG path must identify a file: '$fullPath'."
    }
    if ($file.CreationTimeUtc -lt $RequestedAtUtc.UtcDateTime -or $file.LastWriteTimeUtc -lt $RequestedAtUtc.UtcDateTime) {
        throw "PNG file predates the capture request: '$fullPath'."
    }
    if ($file.Length -lt $MinimumBytes) {
        throw "PNG file has $($file.Length) bytes; at least $MinimumBytes are required: '$fullPath'."
    }

    $stream = $null
    $image = $null
    $bitmap = $null
    $graphics = $null
    $sha = $null
    $compressed = $null
    $deflateSource = $null
    $deflater = $null
    try {
        # Deny writes/deletion while checking structure, decoding and hashing.
        $stream = [IO.File]::Open($fullPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
        $header = New-Object byte[] 33
        if ($stream.Read($header, 0, $header.Length) -ne $header.Length) {
            throw "PNG has a truncated signature or IHDR: '$fullPath'."
        }
        $signature = [byte[]]@(137, 80, 78, 71, 13, 10, 26, 10)
        for ($index = 0; $index -lt $signature.Length; ++$index) {
            if ($header[$index] -ne $signature[$index]) { throw "PNG signature is invalid: '$fullPath'." }
        }
        if ($header[8] -ne 0 -or $header[9] -ne 0 -or $header[10] -ne 0 -or $header[11] -ne 13 -or
            [Text.Encoding]::ASCII.GetString($header, 12, 4) -cne 'IHDR') {
            throw "PNG must start with a 13-byte IHDR: '$fullPath'."
        }
        $pngWidth = [long]$header[16] * 16777216 + [long]$header[17] * 65536 + [long]$header[18] * 256 + $header[19]
        $pngHeight = [long]$header[20] * 16777216 + [long]$header[21] * 65536 + [long]$header[22] * 256 + $header[23]
        if ($pngWidth -ne $Width -or $pngHeight -ne $Height) {
            throw "PNG dimensions ${pngWidth}x${pngHeight} do not match ${Width}x${Height}: '$fullPath'."
        }

        # GDI+ tolerates corrupt compressed data. Verify every chunk CRC and the
        # zlib stream ourselves, in addition to forcing an actual image decode.
        $crcTable = New-Object uint32[] 256
        for ($index = 0; $index -lt 256; ++$index) {
            [uint32]$entry = $index
            for ($bit = 0; $bit -lt 8; ++$bit) {
                if (($entry -band 1) -ne 0) { $entry = ($entry -shr 1) -bxor [uint32]3988292384 }
                else { $entry = $entry -shr 1 }
            }
            $crcTable[$index] = $entry
        }
        $stream.Position = 8
        $compressed = [IO.MemoryStream]::new()
        $hasPixels = $false
        $hasEnd = $false
        $chunkIndex = 0
        $chunkHeader = New-Object byte[] 8
        $crcBytes = New-Object byte[] 4
        $buffer = New-Object byte[] 65536
        while ($stream.Position -lt $stream.Length) {
            if ($stream.Read($chunkHeader, 0, 8) -ne 8) { throw "PNG chunk header is truncated: '$fullPath'." }
            $chunkLength = [long]$chunkHeader[0] * 16777216 + [long]$chunkHeader[1] * 65536 + [long]$chunkHeader[2] * 256 + $chunkHeader[3]
            $chunkType = [Text.Encoding]::ASCII.GetString($chunkHeader, 4, 4)
            if ($chunkLength -gt [int]::MaxValue -or $chunkLength + 4 -gt $stream.Length - $stream.Position) {
                throw "PNG chunk is truncated or has an invalid length: '$fullPath'."
            }
            if ($chunkType -ceq 'IHDR' -and $chunkIndex -ne 0) { throw "PNG contains a duplicate IHDR: '$fullPath'." }
            if ($chunkType -ceq 'IDAT' -and $chunkLength -gt 0) { $hasPixels = $true }
            [uint32]$crc = 4294967295
            for ($index = 4; $index -lt 8; ++$index) {
                $crc = ($crc -shr 8) -bxor $crcTable[($crc -bxor $chunkHeader[$index]) -band 255]
            }
            $remaining = $chunkLength
            while ($remaining -gt 0) {
                $read = $stream.Read($buffer, 0, [int][Math]::Min($remaining, $buffer.Length))
                if ($read -eq 0) { throw "PNG chunk data is truncated: '$fullPath'." }
                for ($index = 0; $index -lt $read; ++$index) {
                    $crc = ($crc -shr 8) -bxor $crcTable[($crc -bxor $buffer[$index]) -band 255]
                }
                if ($chunkType -ceq 'IDAT') { $compressed.Write($buffer, 0, $read) }
                $remaining -= $read
            }
            if ($stream.Read($crcBytes, 0, 4) -ne 4) { throw "PNG chunk CRC is truncated: '$fullPath'." }
            $expectedCrc = [long]$crcBytes[0] * 16777216 + [long]$crcBytes[1] * 65536 + [long]$crcBytes[2] * 256 + $crcBytes[3]
            if (($crc -bxor [uint32]4294967295) -ne $expectedCrc) {
                throw "PNG $chunkType chunk CRC is invalid: '$fullPath'."
            }
            ++$chunkIndex
            if ($chunkType -ceq 'IEND') {
                if ($chunkLength -ne 0 -or $stream.Position -ne $stream.Length) {
                    throw "PNG IEND is malformed or followed by trailing data: '$fullPath'."
                }
                $hasEnd = $true
                break
            }
        }
        if (-not $hasPixels -or -not $hasEnd) { throw "PNG is missing IDAT or IEND: '$fullPath'." }

        $bitDepth = [int]$header[24]
        $colorType = [int]$header[25]
        $channelCounts = @{ 0 = 1; 2 = 3; 3 = 1; 4 = 2; 6 = 4 }
        $legalDepths = @{ 0 = @(1, 2, 4, 8, 16); 2 = @(8, 16); 3 = @(1, 2, 4, 8); 4 = @(8, 16); 6 = @(8, 16) }
        if (-not $channelCounts.ContainsKey($colorType) -or $bitDepth -notin $legalDepths[$colorType] -or
            $header[26] -ne 0 -or $header[27] -ne 0 -or $header[28] -gt 1) {
            throw "PNG IHDR encoding fields are invalid: '$fullPath'."
        }
        $bitsPerPixel = $channelCounts[$colorType] * $bitDepth
        $passes = ,@(0, 0, 1, 1)
        if ($header[28] -eq 1) {
            $passes = @(@(0, 0, 8, 8), @(4, 0, 8, 8), @(0, 4, 4, 8), @(2, 0, 4, 4), @(0, 2, 2, 4), @(1, 0, 2, 2), @(0, 1, 1, 2))
        }
        $scanlines = [Collections.Generic.List[object]]::new()
        [long]$expectedDecodedLength = 0
        foreach ($pass in $passes) {
            $passWidth = [long][Math]::Ceiling([Math]::Max(0, $Width - $pass[0]) / [double]$pass[2])
            $passHeight = [long][Math]::Ceiling([Math]::Max(0, $Height - $pass[1]) / [double]$pass[3])
            if ($passWidth -gt 0 -and $passHeight -gt 0) {
                $rowBytes = [long][Math]::Ceiling($passWidth * $bitsPerPixel / 8.0) + 1
                $scanlines.Add([pscustomobject]@{ rowBytes = $rowBytes; rows = $passHeight })
                $expectedDecodedLength += $rowBytes * $passHeight
            }
        }

        $compressedBytes = $compressed.GetBuffer()
        $compressedLength = $compressed.Length
        if ($compressedLength -lt 6 -or ($compressedBytes[0] -band 15) -ne 8 -or
            ($compressedBytes[0] -shr 4) -gt 7 -or ($compressedBytes[1] -band 32) -ne 0 -or
            ($compressedBytes[0] * 256 + $compressedBytes[1]) % 31 -ne 0) {
            throw "PNG zlib header is invalid: '$fullPath'."
        }
        $trailer = [int]$compressedLength - 4
        $expectedAdler = [long]$compressedBytes[$trailer] * 16777216 + [long]$compressedBytes[$trailer + 1] * 65536 + [long]$compressedBytes[$trailer + 2] * 256 + $compressedBytes[$trailer + 3]
        $deflateSource = [IO.MemoryStream]::new($compressedBytes, 2, [int]$compressedLength - 6, $false)
        $deflater = [IO.Compression.DeflateStream]::new($deflateSource, [IO.Compression.CompressionMode]::Decompress)
        [long]$decodedLength = 0
        [long]$adler1 = 1
        [long]$adler2 = 0
        [long]$nextFilter = 0
        $passIndex = 0
        $rowsLeft = $scanlines[0].rows
        while (($read = $deflater.Read($buffer, 0, $buffer.Length)) -gt 0) {
            if ($decodedLength + $read -gt $expectedDecodedLength) {
                throw "PNG decoded data exceeds its IHDR dimensions: '$fullPath'."
            }
            while ($nextFilter -lt $decodedLength + $read) {
                if ($buffer[[int]($nextFilter - $decodedLength)] -gt 4) {
                    throw "PNG scanline filter is invalid: '$fullPath'."
                }
                $nextFilter += $scanlines[$passIndex].rowBytes
                --$rowsLeft
                if ($rowsLeft -eq 0 -and $passIndex + 1 -lt $scanlines.Count) {
                    ++$passIndex
                    $rowsLeft = $scanlines[$passIndex].rows
                }
            }
            for ($index = 0; $index -lt $read; ++$index) {
                $adler1 += $buffer[$index]
                $adler2 += $adler1
            }
            $adler1 %= 65521
            $adler2 %= 65521
            $decodedLength += $read
        }
        if ($decodedLength -ne $expectedDecodedLength) { throw "PNG decoded data is truncated: '$fullPath'." }
        if ($adler2 * 65536 + $adler1 -ne $expectedAdler) { throw "PNG zlib Adler checksum is invalid: '$fullPath'." }

        Add-Type -AssemblyName System.Drawing -ErrorAction Stop
        $stream.Position = 0
        $image = [Drawing.Image]::FromStream($stream, $false, $true)
        if ($image.RawFormat.Guid -ne [Drawing.Imaging.ImageFormat]::Png.Guid -or
            $image.Width -ne $Width -or $image.Height -ne $Height) {
            throw "Decoded image is not the required PNG: '$fullPath'."
        }
        # Drawing every source pixel forces decoding, including lazy image data.
        $bitmap = [Drawing.Bitmap]::new($Width, $Height, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        $graphics.DrawImage($image, [Drawing.Rectangle]::new(0, 0, $Width, $Height), 0, 0, $Width, $Height, [Drawing.GraphicsUnit]::Pixel)
        $graphics.Flush([Drawing.Drawing2D.FlushIntention]::Sync)

        $stream.Position = 0
        $sha = [Security.Cryptography.SHA256]::Create()
        $digest = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
        $file.Refresh()
        if ($file.CreationTimeUtc -lt $RequestedAtUtc.UtcDateTime -or $file.LastWriteTimeUtc -lt $RequestedAtUtc.UtcDateTime -or
            $file.Length -ne $stream.Length -or $stream.Length -lt $MinimumBytes) {
            throw "PNG metadata changed or became stale during validation: '$fullPath'."
        }
        [pscustomobject]@{
            Path = $fullPath
            Width = $image.Width
            Height = $image.Height
            Length = $stream.Length
            Sha256 = $digest
            CreationTimeUtc = $file.CreationTimeUtc
            LastWriteTimeUtc = $file.LastWriteTimeUtc
        }
    }
    finally {
        if ($deflater) { $deflater.Dispose() }
        if ($deflateSource) { $deflateSource.Dispose() }
        if ($compressed) { $compressed.Dispose() }
        if ($sha) { $sha.Dispose() }
        if ($graphics) { $graphics.Dispose() }
        if ($bitmap) { $bitmap.Dispose() }
        if ($image) { $image.Dispose() }
        if ($stream) { $stream.Dispose() }
    }
}

. (Join-Path $PSScriptRoot 'PngValidation.ps1')

function Assert-SeedForgeGameplayCaptures {
    <#
    .SYNOPSIS
    Verifies the ordered start/combat/win receipt set and its three owned PNGs.
    .DESCRIPTION
    Trace must preserve JSON timestamp strings (use ConvertFrom-Json -DateKind
    String where available). This validates evidence consistency, not rendering.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateNotNull()]$Trace,
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$RunDirectory,
        [Parameter(Mandatory = $true)][DateTimeOffset]$ProcessStartedAtUtc,
        [Parameter(Mandatory = $true)][DateTimeOffset]$ProcessEndedAtUtc
    )
    $ErrorActionPreference = 'Stop'

    function Read-CaptureField($Object, [string]$Name) {
        if ($null -eq $Object) { throw "Capture evidence is missing object for '$Name'." }
        if ($Object -is [Collections.IDictionary]) {
            if (-not $Object.Contains($Name)) { throw "Capture evidence is missing '$Name'." }
            return ,$Object[$Name]
        }
        $property = $Object.PSObject.Properties[$Name]
        if ($null -eq $property) { throw "Capture evidence is missing '$Name'." }
        return ,$property.Value
    }

    function Read-CaptureUInt64($Value, [string]$Name, [bool]$Positive = $false) {
        $pattern = '^(0|[1-9][0-9]*)$'
        if ($Positive) { $pattern = '^[1-9][0-9]*$' }
        [uint64]$number = 0
        if ($Value -isnot [string] -or $Value -cnotmatch $pattern -or
            -not [uint64]::TryParse($Value, [Globalization.NumberStyles]::None,
                [Globalization.CultureInfo]::InvariantCulture, [ref]$number)) {
            throw "Capture '$Name' must be a canonical $(if ($Positive) { 'positive ' })uint64 decimal string."
        }
        return $number
    }

    function Read-CaptureUtc($Value, [string]$Name) {
        $pattern = '^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{1,7})?Z$'
        [DateTimeOffset]$parsed = [DateTimeOffset]::MinValue
        $formats = [string[]]@("yyyy-MM-dd'T'HH:mm:ss'Z'", "yyyy-MM-dd'T'HH:mm:ss.FFFFFFF'Z'")
        $styles = [Globalization.DateTimeStyles]::AssumeUniversal -bor [Globalization.DateTimeStyles]::AdjustToUniversal
        if ($Value -isnot [string] -or $Value -cnotmatch $pattern -or
            -not [DateTimeOffset]::TryParseExact($Value, $formats, [Globalization.CultureInfo]::InvariantCulture, $styles, [ref]$parsed)) {
            throw "Capture '$Name' must be a valid UTC ISO timestamp ending in Z."
        }
        return $parsed
    }

    function Read-CaptureInteger($Value, [string]$Name) {
        $numericCodes = @('Byte', 'SByte', 'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64', 'Single', 'Double', 'Decimal')
        if ($null -eq $Value -or [Type]::GetTypeCode($Value.GetType()).ToString() -notin $numericCodes) {
            throw "Capture '$Name' must be a numeric integer, not a string or boolean."
        }
        [decimal]$number = $Value
        if ([decimal]::Truncate($number) -ne $number) { throw "Capture '$Name' must be a numeric integer." }
        return $number
    }

    if ($ProcessStartedAtUtc -gt $ProcessEndedAtUtc) { throw 'Capture process interval is reversed.' }
    $run = Read-CaptureUInt64 (Read-CaptureField $Trace 'runGeneration') 'runGeneration' $true
    $source = Read-CaptureUInt64 (Read-CaptureField $Trace 'appliedRequestId') 'appliedRequestId' $true
    $captures = Read-CaptureField $Trace 'captures'
    $screenshots = Read-CaptureField $Trace 'screenshots'
    if ($captures -isnot [Array] -or $captures.Count -ne 3 -or $screenshots -isnot [Array] -or $screenshots.Count -ne 3) {
        throw 'Capture evidence requires exactly three captures and three screenshots.'
    }

    $runPath = [IO.Path]::GetFullPath($RunDirectory)
    $runComparable = $runPath.TrimEnd([char[]]@('\', '/'))
    $paths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $tokens = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $expectedLabels = @('start', 'combat', 'win')
    $pending = [Collections.Generic.List[object]]::new()
    $previousFrame = [uint64]0
    $previousTime = $ProcessStartedAtUtc
    for ($index = 0; $index -lt 3; ++$index) {
        $receipt = $captures[$index]
        $label = Read-CaptureField $receipt 'label'
        if ($label -isnot [string] -or $label -cne $expectedLabels[$index]) {
            throw 'Capture labels must occur exactly in start, combat, win order.'
        }
        $token = Read-CaptureField $receipt 'token'
        if ($token -isnot [string] -or $token -cnotmatch '^[0-9a-f]{32}$' -or
            $token -ceq '00000000000000000000000000000000' -or -not $tokens.Add($token)) {
            throw "Capture '$label' token must be a distinct nonzero lowercase 32-digit GUID."
        }
        $receiptRun = Read-CaptureUInt64 (Read-CaptureField $receipt 'runGeneration') "$label.runGeneration" $true
        $receiptSource = Read-CaptureUInt64 (Read-CaptureField $receipt 'sourceRequestId') "$label.sourceRequestId" $true
        if ($receiptRun -ne $run -or $receiptSource -ne $source) { throw "Capture '$label' run or source request differs from trace identity." }

        $rawPath = Read-CaptureField $receipt 'path'
        if ($rawPath -isnot [string] -or [string]::IsNullOrWhiteSpace($rawPath) -or
            $screenshots[$index] -isnot [string] -or [string]::IsNullOrWhiteSpace($screenshots[$index])) {
            throw "Capture '$label' and screenshot paths must be nonempty strings."
        }
        $path = [IO.Path]::GetFullPath($rawPath)
        $screenshot = [IO.Path]::GetFullPath($screenshots[$index])
        if (-not [IO.Path]::GetDirectoryName($path).TrimEnd([char[]]@('\', '/')).Equals($runComparable, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Capture '$label' must be directly inside the current run directory."
        }
        if (-not $path.Equals($screenshot, [StringComparison]::OrdinalIgnoreCase)) { throw "Capture '$label' does not match its ordered screenshot path." }
        if (-not $paths.Add($path)) { throw 'Capture paths must be distinct after normalization.' }
        if (-not [IO.Path]::GetFileName($path).EndsWith("-$token.png", [StringComparison]::Ordinal)) {
            throw "Capture '$label' filename must end in its native -<token>.png suffix."
        }

        $frames = @(foreach ($field in @('requestedFrame', 'renderedFrame', 'capturedFrame', 'completedFrame')) {
            Read-CaptureUInt64 (Read-CaptureField $receipt $field) "$label.$field"
        })
        if ($frames[0] -gt $frames[1] -or $frames[1] -gt $frames[2] -or $frames[2] -gt $frames[3] -or $frames[0] -lt $previousFrame) {
            throw "Capture '$label' frame order or cross-capture ordering is invalid."
        }
        $requested = Read-CaptureUtc (Read-CaptureField $receipt 'requestedAtUtc') "$label.requestedAtUtc"
        $completed = Read-CaptureUtc (Read-CaptureField $receipt 'completedAtUtc') "$label.completedAtUtc"
        if ($requested -lt $ProcessStartedAtUtc -or $requested -gt $completed -or $completed -gt $ProcessEndedAtUtc -or $requested -lt $previousTime) {
            throw "Capture '$label' UTC interval is outside its process or out of order."
        }
        $success = Read-CaptureField $receipt 'success'
        if ($success -isnot [bool] -or -not $success) { throw "Capture '$label' success must be boolean true." }
        if ((Read-CaptureInteger (Read-CaptureField $receipt 'remainingDelegateBindings') "$label.remainingDelegateBindings") -ne 0) {
            throw "Capture '$label' still has delegate bindings."
        }
        if ((Read-CaptureInteger (Read-CaptureField $receipt 'width') "$label.width") -ne 1280 -or
            (Read-CaptureInteger (Read-CaptureField $receipt 'height') "$label.height") -ne 720) {
            throw "Capture '$label' dimensions must be numeric 1280x720."
        }
        $fileBytes = Read-CaptureInteger (Read-CaptureField $receipt 'fileBytes') "$label.fileBytes"
        if ($fileBytes -le 0) { throw "Capture '$label' fileBytes must be positive." }
        $pending.Add([pscustomobject]@{ Path = $path; Token = $token; Label = $label; RequestedAtUtc = $requested; FileBytes = $fileBytes })
        $previousFrame = $frames[3]
        $previousTime = $completed
    }

    # Reject directory links before enumerating the flat current-run image set.
    $ancestors = [Collections.Generic.List[string]]::new()
    $ancestorPath = $runPath
    while (-not [string]::IsNullOrEmpty($ancestorPath)) {
        $ancestors.Add($ancestorPath)
        $ancestorPath = [IO.Path]::GetDirectoryName($ancestorPath)
    }
    $ancestors.Reverse()
    $directory = $null
    foreach ($ancestor in $ancestors) {
        $directory = Get-Item -LiteralPath $ancestor -Force -ErrorAction Stop
        if (($directory.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Capture directory traverses a reparse point: '$ancestor'." }
    }
    if ($directory -isnot [IO.DirectoryInfo]) { throw 'Capture run directory must be an existing directory.' }
    $actualPngs = @(Get-ChildItem -LiteralPath $runPath -File -Force -ErrorAction Stop | Where-Object { $_.Extension -ieq '.png' })
    if ($actualPngs.Count -ne 3) { throw 'Current capture directory must contain exactly three actual PNG files.' }
    foreach ($pngFile in $actualPngs) {
        if (-not $paths.Contains([IO.Path]::GetFullPath($pngFile.FullName))) { throw 'Current directory PNGs do not match the receipt path set.' }
    }

    $validated = [Collections.Generic.List[object]]::new()
    foreach ($receipt in $pending) {
        $png = Assert-SeedForgePng -Path $receipt.Path -RunDirectory $runPath -RequestedAtUtc $receipt.RequestedAtUtc -Width 1280 -Height 720 -MinimumBytes 10240
        if ($receipt.FileBytes -ne $png.Length) { throw "Capture '$($receipt.Label)' fileBytes differs from its validated PNG length." }
        $validated.Add([pscustomobject]@{
            Path = $png.Path; Width = $png.Width; Height = $png.Height; Length = $png.Length
            Sha256 = $png.Sha256; CreationTimeUtc = $png.CreationTimeUtc; LastWriteTimeUtc = $png.LastWriteTimeUtc
            Token = $receipt.Token; Label = $receipt.Label
        })
    }
    # Publish no partial results if a later receipt or PNG fails validation.
    return $validated.ToArray()
}

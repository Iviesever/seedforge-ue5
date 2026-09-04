[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/CaptureValidation.ps1')
$fixtureId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N')
$fixtureRoot = Join-Path $projectRoot "Artifacts/Reports/CaptureValidation/$fixtureId"
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$sourcePath = Join-Path $projectRoot 'Artifacts/Media/Gameplay/20260904-120630/SeedForge-Gameplay-start-24301.png'
$sourceBytes = [IO.File]::ReadAllBytes($sourcePath)
$results = [Collections.Generic.List[object]]::new()
$jsonOptions = @{}
if ((Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) { $jsonOptions.DateKind = 'String' }

function Format-Utc([DateTimeOffset]$Value) {
    return $Value.UtcDateTime.ToString("yyyy-MM-dd'T'HH:mm:ss.fffffff'Z'", [Globalization.CultureInfo]::InvariantCulture)
}

function New-CaptureFixture([string]$Name) {
    $caseDirectory = Join-Path $fixtureRoot ($Name -replace '[^a-zA-Z0-9-]', '-')
    $images = Join-Path $caseDirectory 'images'
    New-Item -ItemType Directory -Path $images | Out-Null
    # A synthetic process envelope leaves room for one-tick negative mutations;
    # successive UtcNow calls may legitimately return equal timestamps.
    $started = [DateTimeOffset]::UtcNow.AddSeconds(-1)
    $receipts = [Collections.Generic.List[object]]::new()
    $labels = @('start', 'combat', 'win')
    for ($index = 0; $index -lt 3; ++$index) {
        $token = [Guid]::NewGuid().ToString('N')
        $path = Join-Path $images "synthetic-$($labels[$index])-$token.png"
        $requested = [DateTimeOffset]::UtcNow
        [IO.File]::WriteAllBytes($path, $sourceBytes)
        $written = [DateTime]::UtcNow
        [IO.File]::SetCreationTimeUtc($path, $written)
        [IO.File]::SetLastWriteTimeUtc($path, $written)
        $completed = [DateTimeOffset]::UtcNow
        $frame = 10 * $index + 1
        $receipts.Add([ordered]@{
            token = $token; label = $labels[$index]; path = $path
            runGeneration = '7'; sourceRequestId = '11'
            requestedAtUtc = Format-Utc $requested; completedAtUtc = Format-Utc $completed
            requestedFrame = "$frame"; renderedFrame = "$($frame + 1)"
            capturedFrame = "$($frame + 2)"; completedFrame = "$($frame + 3)"
            width = 1280; height = 720; fileBytes = $sourceBytes.LongLength
            success = $true; remainingDelegateBindings = 0
        })
    }
    $trace = [ordered]@{
        runGeneration = '7'; appliedRequestId = '11'
        captures = @($receipts.ToArray())
        screenshots = @($receipts | ForEach-Object { $_.path })
    } | ConvertTo-Json -Depth 8 | ConvertFrom-Json @jsonOptions
    return [pscustomobject]@{
        Trace = $trace; RunDirectory = $images; CaseDirectory = $caseDirectory
        Started = $started; Ended = [DateTimeOffset]::UtcNow
    }
}

function Invoke-CaptureCase {
    param([string]$Name, [bool]$Reject, [scriptblock]$Mutate = {}, [scriptblock]$Check = {})
    $context = New-CaptureFixture $Name
    try {
        & $Mutate $context
        [IO.File]::WriteAllText((Join-Path $context.CaseDirectory 'trace.json'), ($context.Trace | ConvertTo-Json -Depth 8))
        $inputRecord = [ordered]@{
            purpose = 'Synthetic capture-validator input, not a production process record.'
            runDirectory = $context.RunDirectory
            processStartedAtUtc = Format-Utc $context.Started
            processEndedAtUtc = Format-Utc $context.Ended
        }
        [IO.File]::WriteAllText((Join-Path $context.CaseDirectory 'validation-input.json'), ($inputRecord | ConvertTo-Json))
        $rejected = $false
        $reason = ''
        $records = @()
        try {
            $records = @(Assert-SeedForgeGameplayCaptures -Trace $context.Trace -RunDirectory $context.RunDirectory -ProcessStartedAtUtc $context.Started -ProcessEndedAtUtc $context.Ended)
        }
        catch { $rejected = $true; $reason = $_.Exception.Message }
        if ($Reject -and -not $rejected) { throw 'Invalid capture set was accepted.' }
        if (-not $Reject) {
            if ($rejected) { throw "Valid capture set was rejected: $reason" }
            if ($records.Count -ne 3) { throw 'Valid set did not return three PNG records.' }
            for ($index = 0; $index -lt 3; ++$index) {
                if ($records[$index].Path -ine [IO.Path]::GetFullPath($context.Trace.captures[$index].path) -or
                    $records[$index].Token -cne $context.Trace.captures[$index].token -or
                    $records[$index].Label -cne $context.Trace.captures[$index].label -or
                    $records[$index].Length -ne $sourceBytes.LongLength -or
                    $records[$index].Sha256 -cne (Get-FileHash -LiteralPath $records[$index].Path -Algorithm SHA256).Hash.ToLowerInvariant()) {
                    throw 'Returned PNG metadata or receipt enrichment is incorrect.'
                }
            }
            & $Check $context $records
        }
        $results.Add([pscustomobject]@{ name = $Name; passed = $true; detail = $reason; fixture = $context.CaseDirectory })
        Write-Host "PASS $Name"
    }
    catch {
        $results.Add([pscustomobject]@{ name = $Name; passed = $false; detail = $_.Exception.Message; fixture = $context.CaseDirectory })
        Write-Host "FAIL $Name : $($_.Exception.Message)"
    }
}

Invoke-CaptureCase 'valid ordered captures and identical PNG hashes' $false
Invoke-CaptureCase 'uint64 maximum IDs and frames preserve precision' $false {
    param($c)
    $c.Trace.runGeneration = '18446744073709551615'; $c.Trace.appliedRequestId = '18446744073709551615'
    foreach ($r in $c.Trace.captures) {
        $r.runGeneration = $c.Trace.runGeneration; $r.sourceRequestId = $c.Trace.appliedRequestId
        foreach ($field in @('requestedFrame', 'renderedFrame', 'capturedFrame', 'completedFrame')) { $r.$field = '18446744073709551615' }
    }
}
Invoke-CaptureCase 'zero and equal frame boundaries are valid' $false {
    param($c)
    foreach ($r in $c.Trace.captures) {
        foreach ($field in @('requestedFrame', 'renderedFrame', 'capturedFrame', 'completedFrame')) { $r.$field = '0' }
    }
}
Invoke-CaptureCase 'numeric integral fixed fields are valid' $false {
    param($c)
    foreach ($r in $c.Trace.captures) { $r.width = 1280.0; $r.height = 720.0; $r.remainingDelegateBindings = 0.0; $r.fileBytes = [double]$sourceBytes.LongLength }
}
Invoke-CaptureCase 'native millisecond UTC format is valid' $false {
    param($c)
    foreach ($r in $c.Trace.captures) {
        foreach ($field in @('requestedAtUtc', 'completedAtUtc')) {
            $utc = [DateTimeOffset]::Parse($r.$field, [Globalization.CultureInfo]::InvariantCulture)
            $r.$field = $utc.UtcDateTime.ToString("yyyy-MM-dd'T'HH:mm:ss.fff'Z'", [Globalization.CultureInfo]::InvariantCulture)
        }
    }
}
Invoke-CaptureCase 'whole-second UTC and equal time boundaries are valid' $false {
    param($c)
    $utc = [DateTimeOffset]::Parse($c.Trace.captures[0].requestedAtUtc, [Globalization.CultureInfo]::InvariantCulture)
    $stamp = $utc.UtcDateTime.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", [Globalization.CultureInfo]::InvariantCulture)
    $c.Started = [DateTimeOffset]::Parse($stamp, [Globalization.CultureInfo]::InvariantCulture).AddSeconds(-1)
    foreach ($r in $c.Trace.captures) { $r.requestedAtUtc = $stamp; $r.completedAtUtc = $stamp }
}
foreach ($field in @('runGeneration', 'appliedRequestId')) {
    foreach ($bad in @('0', '01', '-1', '18446744073709551616', '1e2', 7)) {
        $caseName = "top $field rejects $bad type $($bad.GetType().Name)"
        Invoke-CaptureCase $caseName $true { param($c) $c.Trace.$field = $bad }
    }
}
Invoke-CaptureCase 'receipt run must equal top run' $true { param($c) $c.Trace.captures[0].runGeneration = '8' }
Invoke-CaptureCase 'receipt source must equal top applied request' $true { param($c) $c.Trace.captures[0].sourceRequestId = '12' }
Invoke-CaptureCase 'receipt ID numeric coercion rejected' $true { param($c) $c.Trace.captures[0].sourceRequestId = 11 }
foreach ($bad in @('-1', '01', '18446744073709551616', '2.5', 1)) {
    Invoke-CaptureCase "frame rejects $bad type $($bad.GetType().Name)" $true { param($c) $c.Trace.captures[0].requestedFrame = $bad }
}
foreach ($field in @('requestedFrame', 'renderedFrame', 'capturedFrame', 'completedFrame', 'requestedAtUtc', 'completedAtUtc', 'success', 'remainingDelegateBindings')) {
    Invoke-CaptureCase "missing receipt $field rejected" $true { param($c) $c.Trace.captures[0].PSObject.Properties.Remove($field) }
}
Invoke-CaptureCase 'render cannot precede request' $true { param($c) $c.Trace.captures[0].renderedFrame = '0' }
Invoke-CaptureCase 'pixels cannot precede render' $true { param($c) $c.Trace.captures[0].capturedFrame = '1' }
Invoke-CaptureCase 'completion cannot precede pixels' $true { param($c) $c.Trace.captures[0].completedFrame = '2' }
Invoke-CaptureCase 'later request frame cannot precede prior completion' $true { param($c) $c.Trace.captures[1].requestedFrame = '3' }
foreach ($bad in @('2026-09-04T00:00:00+00:00', '2026-09-04T00:00:00', '2026-02-30T00:00:00Z', '2026-09-04T00:00:00z', '09/04/2026 00:00:00Z', 123)) {
    Invoke-CaptureCase "invalid UTC timestamp $bad" $true { param($c) $c.Trace.captures[0].requestedAtUtc = $bad }
}
Invoke-CaptureCase 'request before process start rejected' $true { param($c) $c.Trace.captures[0].requestedAtUtc = Format-Utc ($c.Started.AddSeconds(-1)) }
Invoke-CaptureCase 'completion after process end rejected' $true { param($c) $c.Trace.captures[2].completedAtUtc = Format-Utc ($c.Ended.AddSeconds(1)) }
Invoke-CaptureCase 'completion before its request rejected' $true {
    param($c)
    $request = [DateTimeOffset]::Parse($c.Trace.captures[1].requestedAtUtc, [Globalization.CultureInfo]::InvariantCulture)
    $c.Trace.captures[1].completedAtUtc = Format-Utc ($request.AddTicks(-1))
}
Invoke-CaptureCase 'later request time cannot precede prior completion' $true {
    param($c)
    $priorCompletion = [DateTimeOffset]::Parse($c.Trace.captures[0].completedAtUtc, [Globalization.CultureInfo]::InvariantCulture)
    $c.Trace.captures[1].requestedAtUtc = Format-Utc ($priorCompletion.AddTicks(-1))
}
Invoke-CaptureCase 'reversed process interval rejected' $true { param($c) $c.Ended = $c.Started.AddSeconds(-1) }
Invoke-CaptureCase 'two captures rejected' $true { param($c) $c.Trace.captures = @($c.Trace.captures[0], $c.Trace.captures[1]) }
Invoke-CaptureCase 'four captures rejected' $true { param($c) $c.Trace.captures += $c.Trace.captures[2] }
Invoke-CaptureCase 'two screenshots rejected' $true { param($c) $c.Trace.screenshots = @($c.Trace.screenshots[0], $c.Trace.screenshots[1]) }
Invoke-CaptureCase 'four screenshots rejected' $true { param($c) $c.Trace.screenshots += $c.Trace.screenshots[2] }
Invoke-CaptureCase 'labels must be start combat win' $true { param($c) $c.Trace.captures[0].label = 'combat' }
Invoke-CaptureCase 'screenshots must preserve receipt order' $true { param($c) $c.Trace.screenshots = @($c.Trace.screenshots[1], $c.Trace.screenshots[0], $c.Trace.screenshots[2]) }
Invoke-CaptureCase 'duplicate tokens rejected' $true { param($c) $c.Trace.captures[1].token = $c.Trace.captures[0].token }
Invoke-CaptureCase 'all-zero token rejected' $true { param($c) $c.Trace.captures[0].token = '00000000000000000000000000000000' }
Invoke-CaptureCase 'uppercase token rejected' $true { param($c) $c.Trace.captures[0].token = 'ABCDEF0123456789ABCDEF0123456789' }
Invoke-CaptureCase 'hyphenated token rejected' $true { param($c) $c.Trace.captures[0].token = [Guid]::NewGuid().ToString() }
Invoke-CaptureCase 'token must match native filename suffix' $true { param($c) $c.Trace.captures[0].token = [Guid]::NewGuid().ToString('N') }
Invoke-CaptureCase 'duplicate normalized paths rejected' $true { param($c) $c.Trace.captures[1].path = $c.Trace.captures[0].path; $c.Trace.screenshots[1] = $c.Trace.captures[0].path }
foreach ($bad in @($false, 'true', 1)) {
    Invoke-CaptureCase "success rejects $bad type $($bad.GetType().Name)" $true { param($c) $c.Trace.captures[0].success = $bad }
}
foreach ($bad in @(1, '0', 0.5, $false)) {
    Invoke-CaptureCase "delegate count rejects $bad type $($bad.GetType().Name)" $true { param($c) $c.Trace.captures[0].remainingDelegateBindings = $bad }
}
Invoke-CaptureCase 'wrong numeric width rejected' $true { param($c) $c.Trace.captures[0].width = 1279 }
Invoke-CaptureCase 'string dimension rejected' $true { param($c) $c.Trace.captures[0].height = '720' }
foreach ($bad in @(0, '387927', 387927.5, $true)) {
    Invoke-CaptureCase "fileBytes rejects $bad type $($bad.GetType().Name)" $true { param($c) $c.Trace.captures[0].fileBytes = $bad }
}
Invoke-CaptureCase 'fileBytes must equal validated file length' $true { param($c) $c.Trace.captures[0].fileBytes += 1 }
Invoke-CaptureCase 'four actual PNG files rejected' $true { param($c) [IO.File]::WriteAllBytes((Join-Path $c.RunDirectory 'extra.png'), $sourceBytes) }
Invoke-CaptureCase 'two actual PNG files rejected' $true {
    param($c)
    # Retain the synthetic file as evidence, but move it outside the flat image set.
    Move-Item -LiteralPath $c.Trace.captures[2].path -Destination (Join-Path $c.CaseDirectory 'held-synthetic.png')
}
Invoke-CaptureCase 'missing referenced PNG rejected' $true { param($c) $c.Trace.captures[0].path = Join-Path $c.RunDirectory "missing-$($c.Trace.captures[0].token).png"; $c.Trace.screenshots[0] = $c.Trace.captures[0].path }
Invoke-CaptureCase 'nested screenshot path rejected' $true {
    param($c)
    $nested = Join-Path $c.RunDirectory 'nested'; New-Item -ItemType Directory -Path $nested | Out-Null
    $path = Join-Path $nested ([IO.Path]::GetFileName($c.Trace.captures[0].path)); [IO.File]::WriteAllBytes($path, $sourceBytes)
    $c.Trace.captures[0].path = $path; $c.Trace.screenshots[0] = $path
}
Invoke-CaptureCase 'sibling-prefix screenshot path rejected' $true {
    param($c)
    $sibling = $c.RunDirectory + '-sibling'; New-Item -ItemType Directory -Path $sibling | Out-Null
    $path = Join-Path $sibling ([IO.Path]::GetFileName($c.Trace.captures[0].path)); [IO.File]::WriteAllBytes($path, $sourceBytes)
    $c.Trace.captures[0].path = $path; $c.Trace.screenshots[0] = $path
}
Invoke-CaptureCase 'stale PNG creation rejected' $true { param($c) [IO.File]::SetCreationTimeUtc($c.Trace.captures[0].path, $c.Started.UtcDateTime.AddMinutes(-1)) }
Invoke-CaptureCase 'stale PNG last-write rejected' $true { param($c) [IO.File]::SetLastWriteTimeUtc($c.Trace.captures[0].path, $c.Started.UtcDateTime.AddMinutes(-1)) }
Invoke-CaptureCase 'corrupt PNG rejected through real validator' $true {
    param($c)
    $bytes = [byte[]]$sourceBytes.Clone(); $bytes[0] = 0
    [IO.File]::WriteAllBytes($c.Trace.captures[0].path, $bytes)
}
Invoke-CaptureCase 'reparse run directory rejected' $true {
    param($c)
    $link = Join-Path $c.CaseDirectory 'linked-images'; New-Item -ItemType Junction -Path $link -Target $c.RunDirectory | Out-Null
    $c.RunDirectory = $link
    for ($index = 0; $index -lt 3; ++$index) { $c.Trace.captures[$index].path = Join-Path $link ([IO.Path]::GetFileName($c.Trace.captures[$index].path)); $c.Trace.screenshots[$index] = $c.Trace.captures[$index].path }
}

$passed = @($results | Where-Object { $_.passed }).Count
$failed = $results.Count - $passed
$summary = [ordered]@{
    purpose = 'Synthetic capture validation fixtures; NOT rendered-state, callback, lifecycle, worker or release proof.'
    fixtureDirectory = $fixtureRoot; sourceBytesOnly = $sourcePath
    powershellVersion = $PSVersionTable.PSVersion.ToString()
    captureValidatorSha256 = (Get-FileHash -LiteralPath (Join-Path $projectRoot 'Scripts/CaptureValidation.ps1') -Algorithm SHA256).Hash.ToLowerInvariant()
    pngValidatorSha256 = (Get-FileHash -LiteralPath (Join-Path $projectRoot 'Scripts/PngValidation.ps1') -Algorithm SHA256).Hash.ToLowerInvariant()
    total = $results.Count; passed = $passed; failed = $failed; results = @($results.ToArray())
}
[IO.File]::WriteAllText((Join-Path $fixtureRoot 'summary.json'), ($summary | ConvertTo-Json -Depth 6) + [Environment]::NewLine)
Write-Host "CAPTURE_VALIDATION total=$($results.Count) passed=$passed failed=$failed fixtures=$fixtureRoot"
if ($failed -gt 0) { exit 1 }

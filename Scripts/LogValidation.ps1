function Assert-SeedForgeLog {
    <#
    .SYNOPSIS
    Rejects complete-log errors and warnings except an explicitly chosen profile.
    .DESCRIPTION
    UE58LocalEnvironment enumerates only the observed UE 5.8 / driver 551.61
    diagnostics. It never permits errors. Each allowance is returned with its
    complete original line and rationale; changed fields/text fail closed.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$Path,
        [ValidateSet('None', 'UE58LocalEnvironment')][string]$AllowedWarnings = 'None',
        [ValidateSet('None','Grid','Encounter','CapturePath','RenderUnavailable')][string]$ExpectedFailure = 'None'
    )
    $ErrorActionPreference = 'Stop'
    $file = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($file -isnot [IO.FileInfo] -or $file.Length -eq 0) { throw "Log must be a nonempty file: '$Path'." }
    $allow = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
    # Negative tests require two exact intentional errors; they are classified
    # separately and never returned as a successful positive-run log.
    $expectedErrors = [Collections.Generic.Dictionary[string,int]]::new([StringComparer]::Ordinal)
    $expectedMessage = ''
    if ($ExpectedFailure -ne 'None') {
        $spec = switch ($ExpectedFailure) {
            'Grid' { @('GenerationFailed','GenerationFailed','Generation code=1: Grid dimensions must be in [4, 2048]; received 0x48.') }
            'Encounter' { @('EncounterFailed','EncounterFailed','Encounter planning code=1: Encounter planning requires an intact layout hash and distinct walkable entrance/exit cells.') }
            'CapturePath' { @('SmokeFailed','CaptureFailed','Screenshot processing completed without a matching fresh saved PNG.') }
            'RenderUnavailable' { @('SmokeFailed','CaptureFailed','Owned gameplay capture exceeded its eight-second callback budget.') }
        }
        $expectedMessage = $spec[2]
        $expectedErrors.Add("LogSeedForge: Error: Gameplay run failed code=$($spec[0]) message=$expectedMessage",0)
        $expectedErrors.Add("LogSeedForge: Error: SEEDFORGE_GAMEPLAY_SMOKE_FAILURE code=$($spec[1]) message=$expectedMessage",0)
    }
    if ($AllowedWarnings -eq 'UE58LocalEnvironment') {
        $allow.Add("LogRHI: Warning: Out of date driver found. Using: '551.61' Suggested: '591.86'",
            'Observed installed NVIDIA driver 551.61 versus UE 5.8 suggested 591.86; environment constraint, not project output validation.')
        $allow.Add('LogRHI: Warning: Running with bad GPU drivers but warning dialog will not be shown: bDeviceCanUpdateDriver=0, VendorHasEntries=0, IsUnattended=1, r.WarnOfBadDrivers=1',
            'Exact unattended warning-dialog policy accompanying the observed driver mismatch; no warning settings are changed.')
        $allow.Add('LogD3D12RHI: Warning: Batched command list execution is disabled for async queues due to known bugs in the current driver.',
            'UE selects its own documented safe async-queue fallback for the installed driver; project rendering still requires independent visual checks.')
        $allow.Add("LogConsoleManager: Warning: Console variable 'r.MotionVectorSimulation' used in the render thread. Rendering artifacts could happen. Use ECVF_RenderThreadSafe or don't use in render thread.",
            'Observed stock Engine render-thread CVar diagnostic; no project CVar mutation or suppression; visual gates remain mandatory.')
        $factories = @('ActorLayersWidgetConstructor','AlertHeaderWidgetConstructor','AlertWidgetConstructor',
            'AssetDataItemTypeWidgetConstructor','AssetDataLabelWidgetConstructor','AssetDataVirtualPathWidgetConstructor',
            'AssetNameWidgetConstructor','DiskSizeWidgetConstructor','DynamicAssetDataColumnBaseWidgetConstructor',
            'FolderCompatibilityWidgetConstructor','OutlinerUnsavedHeaderConstructor','OutlinerUnsavedWidgetConstructor',
            'SocketWidgetConstructor','StaticMeshTrianglesWidgetConstructor','TypeInfoWidgetConstructor','UObjectLabelWidgetConstructor','VisibilityWidgetConstructor')
        foreach ($factory in $factories) {
            $allow.Add("LogEditorDataStorageUI: Warning: Unable to register widget factory '$factory' as purpose '0' isn't registered.",
                "Observed stock EditorDataStorageUI startup factory '$factory', exact unregistered purpose 0; unrelated to SeedForge's owned runtime HUD.")
        }
    }
    $lines = @(Get-Content -LiteralPath $file.FullName)
    if ([string]::IsNullOrWhiteSpace(($lines -join "`n"))) { throw "Log contains no diagnostic text: '$($file.FullName)'." }
    $violations = [Collections.Generic.List[string]]::new()
    $records = [Collections.Generic.List[object]]::new()
    $warnings = 0
    $errors = 0
    $expectedErrorCount = 0
    $unexpected = 0
    for ($index = 0; $index -lt $lines.Count; ++$index) {
        $line = $lines[$index]
        $text = [regex]::Replace($line, '^\[[0-9]{4}\.[0-9]{2}\.[0-9]{2}-[0-9]{2}\.[0-9]{2}\.[0-9]{2}:[0-9]{3}\]\[\s*[0-9]+\]', '')
        $isError = $text -match '(?i)\b(?:error|fatal)\s*:|\bfatal error\b|\berror\s+[A-Z]+[0-9]+\b|\b(?:assertion|ensure condition)\s+failed\b|\bunhandled exception\b|\bEXCEPTION_ACCESS_VIOLATION\b|\bcritical error\b|\bcrash detected\b|\bhas crashed\b|\bBUILD FAILED\b|\bInvalid arguments?\s*:|\bUbaServer\s*-\s*bind\b.*\bfailed\b|^Result:\s*(?!Succeeded\b)\S+'
        foreach ($exitMatch in [regex]::Matches($text, '(?i)\bExitCode\s*[=:]\s*(?<code>-?[0-9]+)')) {
            if ($exitMatch.Groups['code'].Value -notmatch '^0+$') { $isError = $true }
        }
        $isWarning = $text -match '(?i)\bwarning\s*:|\bwarning\s+[A-Z]+[0-9]+\b'
        if ($ExpectedFailure -ne 'None' -and $text.Contains('SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS')) { $isError = $true }
        if ($isError) {
            ++$errors
            if ($expectedErrors.ContainsKey($text)) { ++$expectedErrorCount; ++$expectedErrors[$text] }
            else { $violations.Add("$($index + 1): $line") }
        }
        if ($isWarning) {
            ++$warnings
            if (-not $isError -and $allow.ContainsKey($text)) {
                $records.Add([pscustomobject]@{ LineNumber=$index + 1; Line=$line; Text=$text; Reason=$allow[$text] })
            }
            else {
                ++$unexpected
                if (-not $isError) { $violations.Add("$($index + 1): $line") }
            }
        }
    }
    foreach ($entry in $expectedErrors.GetEnumerator()) {
        if ($entry.Value -ne 1) { $violations.Add("Expected exactly one intentional failure diagnostic, found $($entry.Value): $($entry.Key)") }
    }
    if ($violations.Count -gt 0) { throw "Log rejected '$($file.FullName)' (errors=$errors, unexpectedWarnings=$unexpected):`n$($violations -join "`n")" }
    return [pscustomobject]@{
        Path=$file.FullName; Sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        LineCount=$lines.Count; ErrorCount=$errors; WarningCount=$warnings; AllowedWarningCount=$records.Count
        UnexpectedWarningCount=$unexpected; AllowedWarnings=$AllowedWarnings; AllowedRecords=$records.ToArray()
        ExpectedErrorCount=$expectedErrorCount; UnexpectedErrorCount=$errors-$expectedErrorCount
        Verdict=$(if($ExpectedFailure -eq 'None'){'Passed'}else{'ExpectedFailure'})
        ExpectedFailure=$ExpectedFailure; ExpectedFailureMessage=$expectedMessage
    }
}

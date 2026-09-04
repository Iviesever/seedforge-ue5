[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/LogValidation.ps1')
$fixtureRoot = Join-Path $projectRoot ('Artifacts/Reports/LogValidation/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$results = [Collections.Generic.List[object]]::new()
function Check-LogCase([string]$Name, [scriptblock]$Action) {
    try { & $Action; $results.Add([pscustomobject]@{name=$Name; passed=$true; detail='' }); Write-Host "PASS $Name" }
    catch { $results.Add([pscustomobject]@{name=$Name; passed=$false; detail=$_.Exception.Message }); Write-Host "FAIL $Name : $($_.Exception.Message)" }
}
function New-LogFixture([string]$Text) {
    $path = Join-Path $fixtureRoot ('case-' + $results.Count + '.log')
    [IO.File]::WriteAllText($path, $Text, [Text.UTF8Encoding]::new($false))
    return $path
}
function Reject-Log([scriptblock]$Action) {
    $rejected = $false
    try { & $Action | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw 'Unsafe log was accepted.' }
}
Check-LogCase 'clean log returns complete counters and hash' {
    $path = New-LogFixture "LogSeedForge: Display: SYNTHETIC fixture; no release evidence.`n0 Error(s), 0 Warning(s)`nBUILD SUCCESSFUL`n"
    $proof = Assert-SeedForgeLog -Path $path
    if ($proof.ErrorCount -ne 0 -or $proof.WarningCount -ne 0 -or $proof.LineCount -ne 3 -or
        $proof.Sha256 -cne (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()) { throw 'Incorrect log proof.' }
}
$factories = @('ActorLayersWidgetConstructor','AlertHeaderWidgetConstructor','AlertWidgetConstructor',
    'AssetDataItemTypeWidgetConstructor','AssetDataLabelWidgetConstructor','AssetDataVirtualPathWidgetConstructor',
    'AssetNameWidgetConstructor','DiskSizeWidgetConstructor','DynamicAssetDataColumnBaseWidgetConstructor',
    'FolderCompatibilityWidgetConstructor','OutlinerUnsavedHeaderConstructor','OutlinerUnsavedWidgetConstructor',
    'SocketWidgetConstructor','StaticMeshTrianglesWidgetConstructor','TypeInfoWidgetConstructor','UObjectLabelWidgetConstructor','VisibilityWidgetConstructor')
$known = @(
    "LogRHI: Warning: Out of date driver found. Using: '551.61' Suggested: '591.86'",
    'LogRHI: Warning: Running with bad GPU drivers but warning dialog will not be shown: bDeviceCanUpdateDriver=0, VendorHasEntries=0, IsUnattended=1, r.WarnOfBadDrivers=1',
    'LogD3D12RHI: Warning: Batched command list execution is disabled for async queues due to known bugs in the current driver.',
    "LogConsoleManager: Warning: Console variable 'r.MotionVectorSimulation' used in the render thread. Rendering artifacts could happen. Use ECVF_RenderThreadSafe or don't use in render thread."
)
$known += @($factories | ForEach-Object { "LogEditorDataStorageUI: Warning: Unable to register widget factory '$_' as purpose '0' isn't registered." })
foreach ($warning in $known) {
    Check-LogCase "exact environmental warning $($results.Count)" {
        $path = New-LogFixture ("[2026.09.04-05.58.41:801][  0]" + $warning + "`n")
        $proof = Assert-SeedForgeLog -Path $path -AllowedWarnings UE58LocalEnvironment
        if ($proof.WarningCount -ne 1 -or $proof.AllowedWarningCount -ne 1 -or @($proof.AllowedRecords).Count -ne 1 -or
            [string]::IsNullOrWhiteSpace($proof.AllowedRecords[0].Reason) -or $proof.AllowedRecords[0].LineNumber -ne 1) { throw 'Allowed warning evidence missing.' }
    }
}
$bad = @(
    'LogSeedForge: Error: unexpected gameplay failure', 'LogClass: Error: StructProperty FDataflowToolNodeSnapshot::Date is not initialized properly',
    'LogWindows: Fatal: crash', 'Fatal error: [File:Unknown] [Line: 1]', 'Assertion failed: ptr != nullptr',
    'Ensure condition failed: Value == 1', 'Unhandled Exception: EXCEPTION_ACCESS_VIOLATION', 'Unhandled exception. System.InvalidOperationException: failed',
    'error C2039: missing member', 'source.cpp(20): fatal error C1083: missing include', 'source.cs(1,1): error CS0001: failure',
    'LINK : fatal error LNK1104: missing input', 'source.cpp(20): warning C4100: unused parameter', 'LogFoo: Warning: new unexpected warning',
    'UbaServer - bind 0.0.0.0:1345 failed (An attempt was made to access a socket in a way forbidden by its access permissions.)',
    'BUILD FAILED', 'Result: Failed', 'AutomationTool exiting with ExitCode=6 (6)', 'Took 1.0s to run dotnet.exe, ExitCode=1',
    'Invalid argument: -UBADisableRemote', 'LogWindows: Error: === Critical error: ===', 'Crash detected in process',
    "LogRHI: Warning: Out of date driver found. Using: '551.62' Suggested: '591.86'",
    "LogEditorDataStorageUI: Warning: Unable to register widget factory 'UnknownWidgetConstructor' as purpose '0' isn't registered.",
    "LogEditorDataStorageUI: Warning: Unable to register widget factory 'AlertWidgetConstructor' as purpose '1' isn't registered.",
    ($known[0] + ' additional unexpected warning'), ($known[0] + ' Error: unexpected')
)
foreach ($line in $bad) {
    Check-LogCase "reject unsafe line $($results.Count)" { $path = New-LogFixture ($line + "`n"); Reject-Log { Assert-SeedForgeLog -Path $path -AllowedWarnings UE58LocalEnvironment } }
}
Check-LogCase 'known warning is forbidden without explicit profile' { $path = New-LogFixture $known[0]; Reject-Log { Assert-SeedForgeLog -Path $path } }
Check-LogCase 'unknown warning profile is forbidden' { $path = New-LogFixture 'clean'; Reject-Log { Assert-SeedForgeLog -Path $path -AllowedWarnings AllowEverything } }
Check-LogCase 'missing log rejected' { Reject-Log { Assert-SeedForgeLog -Path (Join-Path $fixtureRoot 'missing.log') } }
Check-LogCase 'empty log rejected' { $path = New-LogFixture ''; Reject-Log { Assert-SeedForgeLog -Path $path } }
Check-LogCase 'whitespace-only log rejected' { $path = New-LogFixture " `t`r`n"; Reject-Log { Assert-SeedForgeLog -Path $path } }
Check-LogCase 'directory rejected' { Reject-Log { Assert-SeedForgeLog -Path $fixtureRoot } }
Check-LogCase 'all offending lines retained in diagnostic' {
    $path = New-LogFixture "LogFoo: Warning: first`nLogBar: Error: second`n"
    $message = ''; try { Assert-SeedForgeLog -Path $path | Out-Null } catch { $message = $_.Exception.Message }
    if ($message -notmatch 'first' -or $message -notmatch 'second') { throw 'Whole-log diagnostics omitted a violation.' }
}
Check-LogCase 'fresh observed gameplay environment remains narrowly representable' {
    $path = Join-Path $projectRoot 'Artifacts/Logs/gameplay-smoke-editor-20260904-135834-1b710332b5e54dc6a953217b4719b506.log'
    $proof = Assert-SeedForgeLog -Path $path -AllowedWarnings UE58LocalEnvironment
    if ($proof.WarningCount -ne 25 -or $proof.AllowedWarningCount -ne 25) { throw 'Observed warning count did not match independent source log.' }
}
$passed = @($results | Where-Object passed).Count
$summary = [ordered]@{ purpose='Synthetic log fixtures; not release evidence'; powershellVersion=$PSVersionTable.PSVersion.ToString(); fixtureDirectory=$fixtureRoot; total=$results.Count; passed=$passed; failed=$results.Count-$passed; results=$results.ToArray() }
[IO.File]::WriteAllText((Join-Path $fixtureRoot 'summary.json'), ($summary | ConvertTo-Json -Depth 6))
Write-Host "LOG_VALIDATION total=$($results.Count) passed=$passed failed=$($results.Count-$passed) fixtures=$fixtureRoot"
if ($passed -ne $results.Count) { exit 1 }

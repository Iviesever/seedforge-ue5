function Invoke-SeedForgeContractGit {
    param([string]$ProjectRoot, [string[]]$Arguments)
    $git = Get-Command git -CommandType Application -ErrorAction Stop | Select-Object -First 1
    # Windows PowerShell represents redirected native stderr as ErrorRecords.
    # Capture them without terminating before the actual native exit is read.
    $ErrorActionPreference = 'Continue'
    $nativeOutput = @(& $git.Source --no-optional-locks -C $ProjectRoot @Arguments 2>&1)
    $nativeExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    $message = ($nativeOutput | ForEach-Object { $_.ToString() }) -join "`n"
    if ($nativeExit -ne 0) {
        throw "Git command [$($Arguments -join ' ')] failed with exit code ${nativeExit}: $message"
    }
    return $message.TrimEnd([char[]]@("`r", "`n"))
}

function Get-SeedForgeCleanFingerprint {
    param([string]$ProjectRoot, [string]$Revision, [string]$TreeId)
    $identity = @('SeedForgeVerificationContext/v1', $ProjectRoot.ToLowerInvariant(), $Revision, $TreeId, 'clean') -join "`n"
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($identity))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

function Get-SeedForgeCleanGitState {
    param([string]$ProjectRoot)
    $root = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd([char[]]@('\', '/'))
    $gitRoot = Invoke-SeedForgeContractGit $root @('rev-parse', '--show-toplevel')
    $gitRoot = [IO.Path]::GetFullPath($gitRoot).TrimEnd([char[]]@('\', '/'))
    if (-not $root.Equals($gitRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "ProjectRoot must be the exact Git worktree root, not a child of '$gitRoot'."
    }
    $revision = Invoke-SeedForgeContractGit $root @('rev-parse', '--verify', 'HEAD')
    $tree = Invoke-SeedForgeContractGit $root @('rev-parse', '--verify', 'HEAD^{tree}')
    if ($revision -cnotmatch '\A[0-9a-f]{40}\z' -or $tree -cnotmatch '\A[0-9a-f]{40}\z') {
        throw 'Git HEAD and tree must be exact 40-hex object identities.'
    }
    # Porcelain status can hide edits behind assume-unchanged/skip-worktree.
    # Inspect the index read-only and fail closed: -v uses lowercase tags for
    # assume-unchanged and S for skip-worktree; only ordinary H entries qualify.
    $trackedEntries = Invoke-SeedForgeContractGit $root @('ls-files', '-v', '-z')
    if (-not [string]::IsNullOrEmpty($trackedEntries)) {
        if (-not $trackedEntries.EndsWith([string][char]0, [StringComparison]::Ordinal)) {
            throw 'Git ls-files index flag output is not NUL-terminated.'
        }
        $entries = $trackedEntries.Split([char]0)
        for ($index = 0; $index -lt $entries.Length - 1; ++$index) {
            if ($entries[$index].Length -lt 3 -or -not $entries[$index].StartsWith('H ', [StringComparison]::Ordinal)) {
                $entry = $entries[$index].Replace("`r", '\r').Replace("`n", '\n')
                throw "Verification index flags are not ordinary H entries; assume-unchanged/skip-worktree are forbidden: $entry"
            }
        }
    }
    $status = Invoke-SeedForgeContractGit $root @('-c', 'core.fsmonitor=false', 'status', '--porcelain=v1', '--untracked-files=all')
    if (-not [string]::IsNullOrEmpty($status)) { throw "Verification source is not clean (staged, unstaged or untracked changes):`n$status" }
    return [pscustomobject]@{
        ProjectRoot = $root; ExpectedRevision = $revision; TreeId = $tree
        CleanFingerprint = Get-SeedForgeCleanFingerprint $root $revision $tree
    }
}

function Copy-SeedForgeVerificationIdentity {
    param($Context)
    if ($null -eq $Context) { throw 'Verification context is missing.' }
    $values = @{}
    foreach ($field in @('ProjectRoot', 'ExpectedRevision', 'TreeId', 'CleanFingerprint')) {
        $property = $Context.PSObject.Properties[$field]
        if ($null -eq $property -or $property.Value -isnot [string] -or [string]::IsNullOrWhiteSpace($property.Value)) {
            throw "Verification context '$field' is missing or malformed."
        }
        $values[$field] = $property.Value
    }
    if ($values.ExpectedRevision -cnotmatch '\A[0-9a-f]{40}\z' -or $values.TreeId -cnotmatch '\A[0-9a-f]{40}\z' -or
        $values.CleanFingerprint -cnotmatch '\A[0-9a-f]{64}\z' -or -not [IO.Path]::IsPathRooted($values.ProjectRoot)) {
        throw 'Verification context revision, tree, root or fingerprint is malformed.'
    }
    $root = [IO.Path]::GetFullPath($values.ProjectRoot).TrimEnd([char[]]@('\', '/'))
    if ($root -cne $values.ProjectRoot -or
        (Get-SeedForgeCleanFingerprint $root $values.ExpectedRevision $values.TreeId) -cne $values.CleanFingerprint) {
        throw 'Verification context clean fingerprint or normalized root is invalid.'
    }
    return [pscustomobject]@{
        ProjectRoot = $values.ProjectRoot; ExpectedRevision = $values.ExpectedRevision
        TreeId = $values.TreeId; CleanFingerprint = $values.CleanFingerprint
    }
}

function New-SeedForgeVerificationContext {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$ProjectRoot,
        [AllowNull()][AllowEmptyString()][string]$ExpectedRevision
    )
    $ErrorActionPreference = 'Stop'
    $hasExpected = $PSBoundParameters.ContainsKey('ExpectedRevision')
    if ($hasExpected -and ($null -eq $ExpectedRevision -or $ExpectedRevision -cnotmatch '\A[0-9a-fA-F]{40}\z')) {
        throw 'An explicit ExpectedRevision must be a complete 40-hex Git revision.'
    }
    $state = Get-SeedForgeCleanGitState $ProjectRoot
    if ($hasExpected -and $state.ExpectedRevision -cne $ExpectedRevision.ToLowerInvariant()) {
        throw "Git HEAD '$($state.ExpectedRevision)' differs from expected revision '$ExpectedRevision'."
    }
    return $state
}

function Assert-SeedForgeVerificationContext {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)]$Context)
    $ErrorActionPreference = 'Stop'
    $expected = Copy-SeedForgeVerificationIdentity $Context
    $actual = Get-SeedForgeCleanGitState $expected.ProjectRoot
    if ($actual.ExpectedRevision -cne $expected.ExpectedRevision) { throw "Verification HEAD changed from '$($expected.ExpectedRevision)' to '$($actual.ExpectedRevision)'." }
    if ($actual.TreeId -cne $expected.TreeId) { throw 'Verification Git tree changed.' }
    if ($actual.CleanFingerprint -cne $expected.CleanFingerprint) { throw 'Verification clean fingerprint changed.' }
}

function Invoke-SeedForgeVerifiedStep {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Action
    )
    $ErrorActionPreference = 'Stop'
    $frozen = Copy-SeedForgeVerificationIdentity $Context
    Assert-SeedForgeVerificationContext -Context $frozen
    $actionError = $null
    $afterErrors = [Collections.Generic.List[Exception]]::new()
    $actionOutput = @()
    try { $actionOutput = @(& $Action) }
    catch { $actionError = $_ }
    finally {
        # Never reread a mutable caller context as the expected post-step HEAD.
        try { Assert-SeedForgeVerificationContext -Context $frozen }
        catch { $afterErrors.Add($_.Exception) }
        try {
            $current = Copy-SeedForgeVerificationIdentity $Context
            foreach ($field in @('ProjectRoot', 'ExpectedRevision', 'TreeId', 'CleanFingerprint')) {
                if ($current.$field -cne $frozen.$field) { throw "Verification context identity changed during step '$Name'." }
            }
        }
        catch { $afterErrors.Add($_.Exception) }
    }
    if ($afterErrors.Count -gt 0) {
        $errors = [Collections.Generic.List[Exception]]::new()
        $detail = "Verified step '$Name' failed."
        if ($null -ne $actionError) { $errors.Add($actionError.Exception); $detail += " Action: $($actionError.Exception.Message)." }
        foreach ($error in $afterErrors) { $errors.Add($error); $detail += " Post-step verification: $($error.Message)." }
        throw [AggregateException]::new($detail, $errors.ToArray())
    }
    if ($null -ne $actionError) { $PSCmdlet.ThrowTerminatingError($actionError) }
    return $actionOutput
}

function Get-SeedForgeArtifactFile {
    param([string]$ProjectRoot, [string]$Path, [string]$Label, [switch]$AllowDirectory)
    if ([string]::IsNullOrWhiteSpace($Path)) { throw "Artifact $Label path is missing." }
    if ([IO.Path]::IsPathRooted($Path)) { $fullPath = [IO.Path]::GetFullPath($Path) }
    else { $fullPath = [IO.Path]::GetFullPath((Join-Path $ProjectRoot $Path)) }
    $artifactRoot = [IO.Path]::GetFullPath((Join-Path $ProjectRoot 'Artifacts')).TrimEnd([char[]]@('\', '/'))
    if (-not $fullPath.StartsWith($artifactRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Artifact $Label path escapes ProjectRoot/Artifacts: '$fullPath'."
    }
    $ancestors = [Collections.Generic.List[string]]::new()
    $current = $fullPath
    while (-not [string]::IsNullOrEmpty($current)) { $ancestors.Add($current); $current = [IO.Path]::GetDirectoryName($current) }
    $ancestors.Reverse()
    $file = $null
    foreach ($ancestor in $ancestors) {
        $file = Get-Item -LiteralPath $ancestor -Force -ErrorAction Stop
        if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Artifact $Label traverses a reparse point: '$ancestor'." }
    }
    if ($file -isnot [IO.FileInfo] -and -not ($AllowDirectory -and $file -is [IO.DirectoryInfo])) { throw "Artifact $Label must be a file: '$fullPath'." }
    return $file
}

function Assert-SeedForgeArtifactManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$ManifestPath,
        [AllowNull()][AllowEmptyString()][string]$ChecksumPath
    )
    $ErrorActionPreference = 'Stop'
    if ($Context.PSObject.Properties['IsDiagnostic'] -and $Context.IsDiagnostic) {
        throw 'A diagnostic context cannot certify an artifact manifest.'
    }
    $explicitChecksum = $PSBoundParameters.ContainsKey('ChecksumPath')
    Invoke-SeedForgeVerifiedStep -Context $Context -Name 'artifact manifest validation' -Action {
        $manifestFile = Get-SeedForgeArtifactFile $Context.ProjectRoot $ManifestPath 'manifest'
        try { $manifest = Get-Content -Raw -LiteralPath $manifestFile.FullName | ConvertFrom-Json -ErrorAction Stop }
        catch { throw "Artifact manifest JSON is invalid: '$($manifestFile.FullName)'." }
        foreach ($field in @('sourceRevision', 'archive', 'sha256')) {
            $property = $manifest.PSObject.Properties[$field]
            if ($null -eq $property -or $property.Value -isnot [string] -or [string]::IsNullOrWhiteSpace($property.Value)) { throw "Artifact manifest '$field' is missing or malformed." }
        }
        if ($manifest.sourceRevision -cnotmatch '\A[0-9a-fA-F]{40}\z' -or $manifest.sourceRevision.ToLowerInvariant() -cne $Context.ExpectedRevision) {
            throw 'Artifact manifest sourceRevision differs from the frozen expected revision.'
        }
        if ($manifest.sha256 -cnotmatch '\A[0-9a-fA-F]{64}\z') { throw 'Artifact manifest SHA256 is malformed.' }
        $archive = Get-SeedForgeArtifactFile $Context.ProjectRoot $manifest.archive 'archive'
        $checksumName = $archive.FullName + '.sha256'
        if ($explicitChecksum) { $checksumName = $ChecksumPath }
        $checksum = Get-SeedForgeArtifactFile $Context.ProjectRoot $checksumName 'checksum'
        if (-not $checksum.DirectoryName.Equals($archive.DirectoryName, [StringComparison]::OrdinalIgnoreCase)) { throw 'Artifact checksum must be adjacent to its archive.' }
        $checksumText = Get-Content -Raw -LiteralPath $checksum.FullName
        $checksumMatch = [regex]::Match($checksumText, '\A(?<hash>[0-9a-fA-F]{64})[ \t]+(?<name>[^\r\n]+?)(?:\r?\n)?\z')
        if (-not $checksumMatch.Success -or $checksumMatch.Groups['name'].Value -cne $archive.Name) { throw 'Artifact checksum must name the exact archive basename on one line.' }
        $actualHash = (Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($manifest.sha256.ToLowerInvariant() -cne $actualHash -or $checksumMatch.Groups['hash'].Value.ToLowerInvariant() -cne $actualHash) {
            throw 'Artifact archive SHA256, manifest and checksum do not agree.'
        }
        $manifest.archive = $archive.FullName
        $manifest.sha256 = $actualHash
        $manifest | Add-Member -NotePropertyName ManifestPath -NotePropertyValue $manifestFile.FullName -Force
        $manifest | Add-Member -NotePropertyName ChecksumPath -NotePropertyValue $checksum.FullName -Force
        return $manifest
    }
}

function New-SeedForgeScriptContext {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$ProjectRoot, [Parameter(Mandatory)]$Parameters)
    $ErrorActionPreference = 'Stop'
    $expectedArguments = @{}
    if ($Parameters.Keys -contains 'ExpectedRevision') {
        if ($Parameters.ExpectedRevision -cnotmatch '\A[0-9a-fA-F]{40}\z') {
            throw 'An explicit ExpectedRevision must be a complete 40-hex Git revision.'
        }
        $expectedArguments.ExpectedRevision = $Parameters.ExpectedRevision
    }
    $diagnostic = ($Parameters.Keys -contains 'AllowDirtyDiagnostic') -and [bool]$Parameters.AllowDirtyDiagnostic
    if (-not $diagnostic) {
        $context = New-SeedForgeVerificationContext -ProjectRoot $ProjectRoot @expectedArguments
    }
    else {
        $root = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd([char[]]@('\','/'))
        $gitRoot = [IO.Path]::GetFullPath((Invoke-SeedForgeContractGit $root @('rev-parse','--show-toplevel'))).TrimEnd([char[]]@('\','/'))
        if (-not $root.Equals($gitRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'ProjectRoot must be the exact Git worktree root.' }
        $revision = Invoke-SeedForgeContractGit $root @('rev-parse','--verify','HEAD')
        $tree = Invoke-SeedForgeContractGit $root @('rev-parse','--verify','HEAD^{tree}')
        if ($revision -cnotmatch '\A[0-9a-f]{40}\z' -or $tree -cnotmatch '\A[0-9a-f]{40}\z') { throw 'Diagnostic source still requires exact 40-hex Git identities.' }
        if ($expectedArguments.ContainsKey('ExpectedRevision') -and $revision -cne $expectedArguments.ExpectedRevision.ToLowerInvariant()) {
            throw "Git HEAD '$revision' differs from expected revision '$($expectedArguments.ExpectedRevision)'."
        }
        $context = [pscustomobject]@{ ProjectRoot=$root; ExpectedRevision=$revision; TreeId=$tree; CleanFingerprint=(Get-SeedForgeCleanFingerprint $root $revision $tree) }
        Write-Host "DIAGNOSTIC ONLY: dirty source permitted; no release certification for $revision."
    }
    $context | Add-Member -NotePropertyName IsDiagnostic -NotePropertyValue $diagnostic
    $context | Add-Member -NotePropertyName SourceRevision -NotePropertyValue $(if($diagnostic){"diagnostic-$($context.ExpectedRevision)"}else{$context.ExpectedRevision})
    return $context
}

function Assert-SeedForgeScriptContext {
    param([Parameter(Mandatory)]$Context)
    if ($Context.IsDiagnostic) {
        $frozen = Copy-SeedForgeVerificationIdentity $Context
        if ($Context.SourceRevision -cne "diagnostic-$($frozen.ExpectedRevision)") { throw 'Diagnostic source identity is malformed.' }
        $head = Invoke-SeedForgeContractGit $frozen.ProjectRoot @('rev-parse','--verify','HEAD')
        if ($head -cne $frozen.ExpectedRevision) { throw 'Diagnostic HEAD changed during execution.' }
    }
    else {
        if ($Context.SourceRevision -cne $Context.ExpectedRevision) { throw 'Authoritative script source identity is malformed.' }
        Assert-SeedForgeVerificationContext -Context $Context
    }
}

function Invoke-SeedForgeScriptStep {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context, [Parameter(Mandatory)][string]$Name, [Parameter(Mandatory)][scriptblock]$Action)
    Assert-SeedForgeScriptContext -Context $Context
    if (-not $Context.IsDiagnostic) {
        return Invoke-SeedForgeVerifiedStep -Context $Context -Name $Name -Action $Action
    }
    $frozen = Copy-SeedForgeVerificationIdentity $Context
    $frozen | Add-Member -NotePropertyName IsDiagnostic -NotePropertyValue $true
    $frozen | Add-Member -NotePropertyName SourceRevision -NotePropertyValue $Context.SourceRevision
    $actionError = $null; $afterError = $null; $output = @()
    try { $output = @(& $Action) }
    catch { $actionError = $_ }
    finally {
        try {
            Assert-SeedForgeScriptContext -Context $frozen
            if (-not $Context.IsDiagnostic -or $Context.SourceRevision -cne $frozen.SourceRevision -or $Context.ExpectedRevision -cne $frozen.ExpectedRevision) {
                throw 'Diagnostic context changed during execution.'
            }
        }
        catch { $afterError = $_ }
    }
    if ($null -ne $afterError) {
        if ($null -ne $actionError) { throw [AggregateException]::new("$Name failed: $($actionError.Exception.Message); $($afterError.Exception.Message)", [Exception[]]@($actionError.Exception,$afterError.Exception)) }
        $PSCmdlet.ThrowTerminatingError($afterError)
    }
    if ($null -ne $actionError) { $PSCmdlet.ThrowTerminatingError($actionError) }
    return $output
}

function Get-SeedForgeEvidenceDigest {
    param([string]$ProjectRoot,[string]$Path)
    $file = Get-SeedForgeArtifactFile $ProjectRoot $Path 'evidence'
    $stream = [IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $first = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
        $stream.Position = 0
        $second = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
        if ($first -cne $second) { throw 'Evidence bytes changed during independent reread.' }
        return [pscustomobject]@{ path=$file.FullName; size=$stream.Length; sha256=$second }
    }
    finally { $sha.Dispose(); $stream.Dispose() }
}

function Assert-SeedForgeEvidenceIndex {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context,[Parameter(Mandatory)][string]$Path)
    if ($Context.PSObject.Properties['IsDiagnostic'] -and $Context.IsDiagnostic) { throw 'A diagnostic context cannot certify an evidence index.' }
    Invoke-SeedForgeVerifiedStep -Context $Context -Name 'evidence index independent verification' -Action {
        $file = Get-SeedForgeArtifactFile $Context.ProjectRoot $Path 'index'
        $checksum = Get-SeedForgeArtifactFile $Context.ProjectRoot ($file.FullName + '.sha256') 'index checksum'
        $digest = Get-SeedForgeEvidenceDigest $Context.ProjectRoot $file.FullName
        $expected = Get-Content -Raw -LiteralPath $checksum.FullName
        if ($expected.TrimEnd([char[]]@("`r","`n")) -cne "$($digest.sha256)  $($file.Name)") { throw 'Evidence index checksum mismatch.' }
        $index = Get-Content -Raw -LiteralPath $file.FullName | ConvertFrom-Json
        if ($index.schema -cne 'seedforge.evidence-index' -or $index.schemaVersion -ne 1 -or $index.sourceRevision -cne $Context.ExpectedRevision -or @($index.files).Count -eq 0) {
            throw 'Evidence index schema/revision/membership is invalid.'
        }
        $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in $index.files) {
            $actual = Get-SeedForgeEvidenceDigest $Context.ProjectRoot ([string]$entry.path)
            if (-not $seen.Add($actual.path)) { throw 'Evidence index contains a duplicate path.' }
            if ($actual.path -ieq $file.FullName -or $actual.path -ieq $checksum.FullName) { throw 'Evidence index cannot contain itself or its checksum.' }
            if ($entry.sha256 -cne $actual.sha256 -or $entry.size -ne $actual.size) { throw "Evidence payload checksum/size mismatch: $($actual.path)" }
        }
        $index | Add-Member -NotePropertyName IndexPath -NotePropertyValue $file.FullName -Force
        $index | Add-Member -NotePropertyName Sha256 -NotePropertyValue $digest.sha256 -Force
        return $index
    }
}

function Get-SeedForgeEvidenceFields {
    param($Value)
    $fields = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
    if ($Value -is [Collections.IDictionary]) {
        foreach ($key in $Value.Keys) {
            if ($key -isnot [string]) { throw 'Evidence field names must be strings.' }
            $fields.Add($key,$Value[$key])
        }
    }
    elseif ($Value -is [pscustomobject]) {
        foreach ($property in $Value.PSObject.Properties) { $fields.Add($property.Name,$property.Value) }
    }
    return ,$fields
}

function ConvertTo-SeedForgeEvidenceSize {
    param($Value)
    $numericTypes = @('Byte','SByte','Int16','UInt16','Int32','UInt32','Int64','UInt64','Single','Double','Decimal')
    if ($null -eq $Value -or [Type]::GetTypeCode($Value.GetType()).ToString() -notin $numericTypes) { throw 'Evidence length/size must be a numeric integer.' }
    try { [decimal]$number = $Value }
    catch { throw 'Evidence length/size is invalid.' }
    if ($number -lt 0 -or $number -gt [long]::MaxValue -or [decimal]::Truncate($number) -ne $number) { throw 'Evidence length/size must be a nonnegative Int64 integer.' }
    return [long]$number
}

function New-SeedForgeFrozenEvidenceRecord {
    param([string]$Path,[long]$Size,[string]$Sha256)
    $fields = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
    $fields.Add('path',$Path); $fields.Add('size',$Size); $fields.Add('sha256',$Sha256)
    return ,([Collections.ObjectModel.ReadOnlyDictionary[string,object]]::new($fields))
}

function Copy-SeedForgeExpectedEvidenceRecords {
    param([string]$ProjectRoot,[object[]]$Records)
    if ($null -eq $Records -or $Records.Count -eq 0) { throw 'Explicit nonempty ExpectedRecords are required; current bytes cannot define a new verification baseline.' }
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $copies = [Collections.Generic.List[object]]::new()
    foreach ($record in $Records) {
        $fields = Get-SeedForgeEvidenceFields $record
        foreach ($name in @('path','size','sha256')) { if (-not $fields.ContainsKey($name)) { throw "Expected evidence record is missing '$name'." } }
        if ($fields['path'] -isnot [string] -or $fields['sha256'] -isnot [string] -or $fields['sha256'] -cnotmatch '\A[0-9a-fA-F]{64}\z') { throw 'Expected evidence path or SHA256 is malformed.' }
        $file = Get-SeedForgeArtifactFile $ProjectRoot $fields['path'] 'expected evidence'
        if (-not $seen.Add($file.FullName)) { throw "Duplicate/conflicting expected evidence record: '$($file.FullName)'." }
        $size = ConvertTo-SeedForgeEvidenceSize $fields['size']
        $copies.Add((New-SeedForgeFrozenEvidenceRecord $file.FullName $size $fields['sha256'].ToLowerInvariant()))
    }
    return $copies.ToArray()
}

function Assert-SeedForgeFrozenEvidenceBytes {
    param([string]$ProjectRoot,[object[]]$Records)
    foreach ($record in $Records) {
        $actual = Get-SeedForgeEvidenceDigest $ProjectRoot $record.path
        if ($record.sha256 -cne $actual.sha256 -or $record.size -ne $actual.size) { throw "Evidence digest/size changed from its verified expectation: '$($actual.path)'." }
    }
}

function Get-SeedForgeEvidenceSnapshot {
    <# Captures files immediately after a step and validates all digest claims in
       its returned object graph. Consistent references coalesce; records are
       read-only copies. It does not recursively deserialize referenced JSON. #>
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context, [Parameter(Mandatory)][AllowNull()][AllowEmptyCollection()]$Evidence)
    if ($Context.PSObject.Properties['IsDiagnostic'] -and $Context.IsDiagnostic) { throw 'A diagnostic context cannot create authoritative evidence snapshots.' }
    Invoke-SeedForgeVerifiedStep -Context $Context -Name 'step evidence digest snapshot' -Action {
        $records = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
        $artifactPrefix = [IO.Path]::GetFullPath((Join-Path $Context.ProjectRoot 'Artifacts')).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
        function Add-SnapshotFile([string]$FilePath,$ClaimedHash,$ClaimedSize,[bool]$HasSize) {
            $actual = Get-SeedForgeEvidenceDigest $Context.ProjectRoot $FilePath
            if ($null -ne $ClaimedHash) {
                if ($ClaimedHash -isnot [string] -or $ClaimedHash -cnotmatch '\A[0-9a-fA-F]{64}\z') { throw 'Child evidence SHA256 is malformed.' }
                if ($ClaimedHash.ToLowerInvariant() -cne $actual.sha256) { throw "Child evidence digest/hash mismatch: '$($actual.path)'." }
            }
            if ($HasSize -and (ConvertTo-SeedForgeEvidenceSize $ClaimedSize) -ne $actual.size) { throw "Child evidence length/size mismatch: '$($actual.path)'." }
            if ($records.ContainsKey($actual.path)) {
                if ($records[$actual.path].sha256 -cne $actual.sha256 -or $records[$actual.path].size -ne $actual.size) { throw 'Evidence changed while traversing its graph.' }
            }
            else { $records.Add($actual.path,(New-SeedForgeFrozenEvidenceRecord $actual.path $actual.size $actual.sha256)) }
        }
        function Visit-SnapshotValue($Value,[string]$FieldName,[int]$Depth) {
            if ($Depth -gt 64) { throw 'Evidence graph is cyclic or exceeds the supported depth.' }
            if ($null -eq $Value) { return }
            if ($Value -is [string]) {
                if (-not [IO.Path]::IsPathRooted($Value)) { return }
                $full = [IO.Path]::GetFullPath($Value)
                if (-not $full.StartsWith($artifactPrefix,[StringComparison]::OrdinalIgnoreCase)) {
                    # Engine executable and source-root metadata are not output evidence.
                    if ($FieldName -in @('executable','EngineRoot','ProjectRoot')) { return }
                    throw "Evidence path escapes Artifacts: '$full'."
                }
                $item = Get-SeedForgeArtifactFile $Context.ProjectRoot $full 'graph evidence' -AllowDirectory
                if ($item -is [IO.FileInfo]) { Add-SnapshotFile $full $null $null $false }
                return
            }
            if ($Value -is [Collections.IDictionary] -or $Value -is [pscustomobject]) {
                $fields = Get-SeedForgeEvidenceFields $Value
                $pairs = @(@('Path','Sha256'),@('reportIndex','reportSha256'),@('trace','traceSha256'),@('archive','sha256'),@('ConsoleLog','ConsoleLogSha256'),@('Log','LogSha256'),@('IndexPath','Sha256'))
                $matchedHashes = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
                foreach ($pair in $pairs) {
                    if ($fields.ContainsKey($pair[0]) -and $fields.ContainsKey($pair[1])) {
                        if ($fields[$pair[0]] -isnot [string] -or [string]::IsNullOrWhiteSpace($fields[$pair[0]]) -or $null -eq $fields[$pair[1]]) { throw 'Child evidence digest pair is malformed.' }
                        $size = $null; $hasSize = $false
                        if ($pair[0] -ieq 'Path') {
                            if ($fields.ContainsKey('Length') -and $fields.ContainsKey('size') -and
                                (ConvertTo-SeedForgeEvidenceSize $fields['Length']) -ne (ConvertTo-SeedForgeEvidenceSize $fields['size'])) {
                                throw 'Child evidence Length and size claims conflict.'
                            }
                            if ($fields.ContainsKey('Length')) { $size=$fields['Length'];$hasSize=$true }
                            elseif ($fields.ContainsKey('size')) { $size=$fields['size'];$hasSize=$true }
                        }
                        Add-SnapshotFile $fields[$pair[0]] $fields[$pair[1]] $size $hasSize
                        [void]$matchedHashes.Add($pair[1])
                    }
                }
                foreach ($hashName in @('Sha256','reportSha256','traceSha256','ConsoleLogSha256','LogSha256')) {
                    if ($fields.ContainsKey($hashName) -and -not $matchedHashes.Contains($hashName)) { throw "Child evidence hash '$hashName' has no matching path." }
                }
                foreach ($key in $fields.Keys) { Visit-SnapshotValue $fields[$key] $key ($Depth+1) }
            }
            elseif ($Value -is [Collections.IEnumerable]) { foreach ($child in $Value) { Visit-SnapshotValue $child $FieldName ($Depth+1) } }
        }
        Visit-SnapshotValue $Evidence '' 0
        return @($records.Values)
    }
}

function Assert-SeedForgeEvidenceRecords {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context, [Parameter(Mandatory)][object[]]$Records)
    if ($Context.PSObject.Properties['IsDiagnostic'] -and $Context.IsDiagnostic) { throw 'A diagnostic context cannot certify evidence records.' }
    Invoke-SeedForgeVerifiedStep -Context $Context -Name 'frozen evidence record verification' -Action {
        $frozen = @(Copy-SeedForgeExpectedEvidenceRecords $Context.ProjectRoot $Records)
        Assert-SeedForgeFrozenEvidenceBytes $Context.ProjectRoot $frozen
        return $frozen
    }
}

function Write-SeedForgeEvidenceIndex {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Context,[Parameter(Mandatory)][string]$Path,[Parameter(Mandatory)][string[]]$Paths,[object[]]$ExpectedRecords)
    if ($Context.PSObject.Properties['IsDiagnostic'] -and $Context.IsDiagnostic) { throw 'A diagnostic context cannot certify an evidence index.' }
    $frozenRecords = @(Copy-SeedForgeExpectedEvidenceRecords $Context.ProjectRoot $ExpectedRecords)
    Invoke-SeedForgeVerifiedStep -Context $Context -Name 'evidence index creation' -Action {
        Assert-SeedForgeFrozenEvidenceBytes $Context.ProjectRoot $frozenRecords
        $full = [IO.Path]::GetFullPath($Path)
        $root = [IO.Path]::GetFullPath((Join-Path $Context.ProjectRoot 'Artifacts')).TrimEnd('\','/')
        if (-not $full.StartsWith($root + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence index path escapes Artifacts.' }
        # Require an existing ordinary parent, and check every ancestor before writing.
        $parent = [IO.Path]::GetDirectoryName($full)
        while ($parent) {
            $item = Get-Item -LiteralPath $parent -Force
            if ($item -isnot [IO.DirectoryInfo] -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Evidence index parent is not an ordinary directory.' }
            $parent = [IO.Path]::GetDirectoryName($parent)
        }
        foreach ($target in @($full,($full + '.sha256'))) {
            if (Test-Path -LiteralPath $target) { Get-SeedForgeArtifactFile $Context.ProjectRoot $target 'index output' | Out-Null }
        }
        $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($source in $Paths) {
            $file = Get-SeedForgeArtifactFile $Context.ProjectRoot $source 'index payload'
            if (-not $seen.Add($file.FullName)) { throw 'Evidence index contains a duplicate path.' }
            if ($file.FullName -ieq $full -or $file.FullName -ieq ($full + '.sha256')) { throw 'Evidence index cannot contain itself or its checksum.' }
        }
        if ($seen.Count -ne $frozenRecords.Count) { throw 'Evidence index paths and ExpectedRecords membership differ.' }
        foreach ($record in $frozenRecords) { if (-not $seen.Contains($record.path)) { throw 'Evidence index paths and ExpectedRecords membership differ.' } }
        $index = [ordered]@{ schema='seedforge.evidence-index'; schemaVersion=1; sourceRevision=$Context.ExpectedRevision; files=$frozenRecords }
        Assert-SeedForgeVerificationContext -Context $Context
        Assert-SeedForgeFrozenEvidenceBytes $Context.ProjectRoot $frozenRecords
        [IO.File]::WriteAllText($full,($index | ConvertTo-Json -Depth 7))
        $digest = Get-SeedForgeEvidenceDigest $Context.ProjectRoot $full
        [IO.File]::WriteAllText(($full + '.sha256'),"$($digest.sha256)  $([IO.Path]::GetFileName($full))`r`n")
        return Assert-SeedForgeEvidenceIndex -Context $Context -Path $full
    }
}

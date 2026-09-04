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
    param([string]$ProjectRoot, [string]$Path, [string]$Label)
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
    if ($file -isnot [IO.FileInfo]) { throw "Artifact $Label must be a file: '$fullPath'." }
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

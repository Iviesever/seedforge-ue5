. (Join-Path $PSScriptRoot 'LogValidation.ps1')

function Get-SeedForgeBuildPluginTargetProof {
    <#
    .SYNOPSIS
    Correlates stock UE 5.8 BuildPlugin Win64 invocations, logs and products.
    .DESCRIPTION
    Requires UnrealEditor Development and UnrealGame Development/Shipping.
    HostProject is intentionally not required: stock BuildPlugin deletes it.
    This is evidence-consistency validation, not a signed build attestation.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$ConsoleLog,
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$DiagnosticRoot,
        [Parameter(Mandatory = $true)][ValidateNotNullOrEmpty()][string]$PackageDirectory,
        [Parameter(Mandatory = $true)][DateTimeOffset]$StartedAtUtc
    )
    $ErrorActionPreference = 'Stop'
    $artifactRoot = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $PSScriptRoot) 'Artifacts')).TrimEnd('\','/')
    $started = $StartedAtUtc.UtcDateTime
    $now = [DateTime]::UtcNow
    if ($started -lt [DateTime]::new(2000,1,1) -or $started -gt $now) { throw 'BuildPlugin start time is invalid or in the future.' }

    function Resolve-ProofPath([string]$Path, [string]$Root) {
        if ([string]::IsNullOrWhiteSpace($Path) -or -not [IO.Path]::IsPathRooted($Path)) { throw "Proof path must be absolute: '$Path'." }
        $full = [IO.Path]::GetFullPath($Path)
        $boundary = [IO.Path]::GetFullPath($Root).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
        if (-not $full.StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase)) { throw "Proof path escapes its owned directory: '$full'." }
        return $full
    }
    function Get-ProofItem([string]$Path, [string]$Root, [bool]$Directory = $false, [bool]$Fresh = $true) {
        $full = Resolve-ProofPath $Path $Root
        $parts = [Collections.Generic.List[string]]::new()
        $current = $full
        while (-not [string]::IsNullOrEmpty($current)) { $parts.Add($current); $current=[IO.Path]::GetDirectoryName($current) }
        $parts.Reverse()
        $item = $null
        foreach ($part in $parts) {
            $item=Get-Item -LiteralPath $part -Force -ErrorAction Stop
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Proof path traverses a reparse point: '$part'." }
        }
        if ($Directory) {
            if ($item -isnot [IO.DirectoryInfo]) { throw "Proof directory missing: '$full'." }
        }
        else {
            if ($item -isnot [IO.FileInfo] -or $item.Length -le 0) { throw "Proof file must be nonempty: '$full'." }
            if ($Fresh -and ($item.LastWriteTimeUtc -lt $started -or $item.LastWriteTimeUtc -gt $now.AddSeconds(1))) { throw "Stale or future proof file: '$full'." }
        }
        return $item
    }
    function Read-ProofJson($File) {
        try { return Get-Content -Raw -LiteralPath $File.FullName | ConvertFrom-Json -ErrorAction Stop }
        catch { throw "Invalid product metadata JSON '$($File.FullName)': $($_.Exception.Message)" }
    }
    function Read-ProofField($Object,[string]$Name) {
        if ($null -eq $Object) { throw "Missing product metadata '$Name'." }
        $property=$Object.PSObject.Properties[$Name]
        if ($null -eq $property) { throw "Missing product metadata '$Name'." }
        return ,$property.Value
    }
    function Get-ProofHash($File) {
        return [pscustomobject]@{ Path=$File.FullName; Length=$File.Length; LastWriteTimeUtc=$File.LastWriteTimeUtc.ToString('o'); Sha256=(Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
    function Assert-ProofBinary($File,[bool]$Dll) {
        $reader=[IO.BinaryReader]::new([IO.File]::OpenRead($File.FullName))
        try {
            if ($reader.BaseStream.Length -lt 64) { throw "Truncated packaged binary '$($File.FullName)'." }
            $signature=$reader.ReadUInt16()
            if ($Dll) {
                if ($signature -ne 0x5a4d) { throw "Packaged DLL has no DOS header: '$($File.FullName)'." }
                $reader.BaseStream.Position=0x3c; $offset=$reader.ReadUInt32()
                if ($offset -lt 64 -or $offset + 24 -gt $reader.BaseStream.Length) { throw 'Packaged DLL PE offset is invalid.' }
                $reader.BaseStream.Position=$offset
                if ($reader.ReadUInt32() -ne 0x00004550 -or $reader.ReadUInt16() -ne 0x8664) { throw 'Packaged DLL is not AMD64 PE.' }
                $reader.BaseStream.Position=$offset + 22
                if (($reader.ReadUInt16() -band 0x2000) -eq 0) { throw 'Packaged PE is not marked as a DLL.' }
            }
            elseif ($signature -ne 0x8664) {
                if ($signature -ne 0 -or $reader.ReadUInt16() -ne 0xffff -or $reader.ReadUInt16() -lt 2 -or $reader.ReadUInt16() -ne 0x8664) {
                    throw "Packaged object is not AMD64 COFF/bigobj: '$($File.FullName)'."
                }
            }
        }
        finally { $reader.Dispose() }
    }
    function Split-ProofCommand([string]$Command) {
        # Match UE's command-line quote boundaries. Never interpret field-like
        # text inside another argument's quoted value as a separate switch.
        $tokens=[Collections.Generic.List[string]]::new()
        $token=[Text.StringBuilder]::new()
        $quoted=$false
        foreach($character in $Command.ToCharArray()) {
            if($character -eq '"'){$quoted=-not $quoted}
            elseif([char]::IsWhiteSpace($character) -and -not $quoted){
                if($token.Length -gt 0){$tokens.Add($token.ToString());[void]$token.Clear()}
            }
            else{[void]$token.Append($character)}
        }
        if($quoted){throw 'UBT command line has unbalanced quotes.'}
        if($token.Length -gt 0){$tokens.Add($token.ToString())}
        return $tokens.ToArray()
    }
    function Read-ProofArgument([string]$Command,[string]$Name) {
        $prefix="-$Name="
        $arguments=@(Split-ProofCommand $Command | Where-Object { $_.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase) })
        if($arguments.Count -ne 1 -or $arguments[0].Length -eq $prefix.Length){throw "UBT invocation requires exactly one -$Name argument."}
        return $arguments[0].Substring($prefix.Length)
    }
    function Assert-ProofCommand([string]$Command,[string]$Target,[string]$Configuration,[string]$Log) {
        $tokens=@(Split-ProofCommand $Command)
        $toolIndices=@(for($i=0;$i -lt $tokens.Count;++$i){if([IO.Path]::GetFileName($tokens[$i]) -ieq 'UnrealBuildTool.dll'){$i}})
        if($toolIndices.Count -ne 1 -or $toolIndices[0]+3 -ge $tokens.Count){throw 'UBT invocation has no unique tool/target identity.'}
        $tool=$toolIndices[0]
        if($tokens[$tool+1] -cne $Target -or $tokens[$tool+2] -cne 'Win64' -or $tokens[$tool+3] -cne $Configuration){throw 'UBT invocation target/platform/configuration mismatch.'}
        $expected=@{
            Project=Join-Path $package.FullName 'HostProject/HostProject.uproject'
            plugin=Join-Path $package.FullName 'HostProject/Plugins/SeedForge/SeedForge.uplugin'
            manifest=Join-Path $package.FullName "HostProject/Saved/Manifest-$Target-Win64-$Configuration.xml"
            log=$Log
        }
        foreach ($name in $expected.Keys) {
            $actual=Read-ProofArgument $Command $name
            if (-not [IO.Path]::IsPathRooted($actual) -or -not [IO.Path]::GetFullPath($actual).Equals([IO.Path]::GetFullPath($expected[$name]),[StringComparison]::OrdinalIgnoreCase)) { throw "UBT -$name does not identify the owned package/log." }
        }
    }

    $package=Get-ProofItem $PackageDirectory $artifactRoot $true
    $diagnostics=Get-ProofItem $DiagnosticRoot $artifactRoot $true
    $console=Get-ProofItem $ConsoleLog $artifactRoot
    $consoleProof=Assert-SeedForgeLog -Path $console.FullName
    $consoleText=Get-Content -Raw -LiteralPath $console.FullName
    $invocations=[regex]::Matches($consoleText,'(?m)^Running:[^\r\n]*UnrealBuildTool\.dll[^\r\n]*\r?$')
    if ($invocations.Count -ne 3) { throw 'BuildPlugin requires exactly three actual UBT invocations.' }
    $buildSuccess=[regex]::Matches($consoleText,'(?m)^BUILD SUCCESSFUL\r?$')
    $overallExit=[regex]::Matches($consoleText,'(?m)^AutomationTool exiting with ExitCode=0 \(Success\)\r?$')
    if ($buildSuccess.Count -ne 1 -or $overallExit.Count -ne 1 -or $buildSuccess[0].Index -lt $invocations[2].Index -or $overallExit[0].Index -lt $buildSuccess[0].Index) { throw 'BuildPlugin has no ordered successful UAT completion.' }
    $descriptor=Get-ProofItem (Join-Path $package.FullName 'SeedForge.uplugin') $package.FullName $false $false
    $descriptorJson=Read-ProofJson $descriptor
    $declared=Read-ProofField $descriptorJson 'Modules'
    foreach($name in @('SeedForgeRuntime','SeedForgeEditor','SeedForgeTests')) {
        $entries=@($declared | Where-Object { $_.Name -ceq $name })
        $expectedType=if($name -eq 'SeedForgeRuntime'){'Runtime'}else{'Editor'}
        if($entries.Count -ne 1 -or $entries[0].Type -cne $expectedType){throw "Packaged descriptor module '$name' is missing or has the wrong type."}
    }
    $records=[Collections.Generic.List[object]]::new()
    $specs=@(@('UnrealEditor','Development'),@('UnrealGame','Development'),@('UnrealGame','Shipping'))
    for($index=0;$index -lt 3;++$index) {
        $target=$specs[$index][0]; $config=$specs[$index][1]
        $logPath=Join-Path $diagnostics.FullName "UBA-$target-Win64-$config.txt"
        Assert-ProofCommand $invocations[$index].Value $target $config $logPath
        $end=if($index -lt 2){$invocations[$index+1].Index}else{$consoleText.Length}
        $block=$consoleText.Substring($invocations[$index].Index,$end-$invocations[$index].Index)
        $targetResult=[regex]::Matches($block,'(?m)^Result: Succeeded\r?$')
        $targetExit=[regex]::Matches($block,'(?m)^Took [0-9]+(?:\.[0-9]+)?s to run dotnet\.exe, ExitCode=0\r?$')
        if($targetResult.Count -ne 1 -or $targetExit.Count -ne 1 -or $targetResult[0].Index -gt $targetExit[0].Index){throw "Target '$target $config' lacks ordered successful UBT result/exit evidence."}
        if($index -eq 2 -and $buildSuccess[0].Index -lt $invocations[$index].Index+$targetExit[0].Index){throw 'UAT completion precedes its final UBT exit.'}
        $log=Get-ProofItem $logPath $diagnostics.FullName
        $logProof=Assert-SeedForgeLog -Path $log.FullName
        $logText=Get-Content -Raw -LiteralPath $log.FullName
        $commands=[regex]::Matches($logText,'(?m)^Command line: (?<command>[^\r\n]+)\r?$')
        if($commands.Count -ne 1){throw 'Per-target log requires exactly one UBT command line.'}
        Assert-ProofCommand $commands[0].Groups['command'].Value $target $config $logPath
        $logResults=[regex]::Matches($logText,'(?m)^Result: Succeeded\r?$')
        if($logResults.Count -ne 1 -or $logResults[0].Index -lt $commands[0].Index){throw 'Per-target log does not report success after its command.'}
        $timeMatches=[regex]::Matches($logText,'(?m)^Log started at [^\r\n]+ \((?<utc>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)\)\r?$')
        if($timeMatches.Count -ne 1){throw 'Per-target log has no unique UTC start identity.'}
        $logStarted=[DateTimeOffset]::ParseExact($timeMatches[0].Groups['utc'].Value,"yyyy-MM-dd'T'HH:mm:ss'Z'",[Globalization.CultureInfo]::InvariantCulture,[Globalization.DateTimeStyles]::AssumeUniversal)
        $startFloor=$started.AddTicks(-($started.Ticks % [TimeSpan]::TicksPerSecond))
        if($logStarted.UtcDateTime -lt $startFloor -or $logStarted.UtcDateTime -gt $now){throw 'Per-target log start is stale or in the future.'}
        $products=[Collections.Generic.List[object]]::new()
        if($target -eq 'UnrealEditor') {
            $bin=Join-Path $package.FullName 'Binaries/Win64'
            $modulesFile=Get-ProofItem (Join-Path $bin 'UnrealEditor.modules') $package.FullName
            $moduleJson=Read-ProofJson $modulesFile
            if([string]::IsNullOrWhiteSpace((Read-ProofField $moduleJson 'BuildId'))){throw 'Packaged Editor module BuildId is empty.'}
            $moduleMap=Read-ProofField $moduleJson 'Modules'
            $products.Add((Get-ProofHash $modulesFile))
            foreach($name in @('SeedForgeRuntime','SeedForgeEditor','SeedForgeTests')) {
                $filename=Read-ProofField $moduleMap $name
                if($filename -isnot [string] -or $filename -cne "UnrealEditor-$name.dll"){throw "Packaged Editor module '$name' has an incorrect DLL mapping."}
                $dll=Get-ProofItem (Join-Path $bin $filename) $package.FullName
                Assert-ProofBinary $dll $true
                $products.Add((Get-ProofHash $dll))
            }
        }
        else {
            $moduleRoot=Join-Path $package.FullName "Intermediate/Build/Win64/x64/UnrealGame/$config/SeedForgeRuntime"
            $pre=Get-ProofItem (Join-Path $moduleRoot 'SeedForgeRuntime.precompiled') $package.FullName
            $outputs=Read-ProofField (Read-ProofJson $pre) 'OutputFiles'
            if($outputs -isnot [Array] -or $outputs.Count -eq 0){throw 'Precompiled runtime must list nonempty object outputs.'}
            $products.Add((Get-ProofHash $pre))
            $seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
            foreach($output in $outputs) {
                if($output -isnot [string] -or [string]::IsNullOrWhiteSpace($output)){throw 'Precompiled output must be a path string.'}
                $path=if([IO.Path]::IsPathRooted($output)){$output}else{Join-Path $moduleRoot $output}
                $object=Get-ProofItem $path $moduleRoot
                if($object.Extension -ine '.obj' -or -not $seen.Add($object.FullName)){throw 'Precompiled outputs must be distinct owned object files.'}
                Assert-ProofBinary $object $false
                $products.Add((Get-ProofHash $object))
            }
        }
        $records.Add([pscustomobject]@{
            Label="$target Win64 $config"; Target=$target; Platform='Win64'; Configuration=$config
            ConsoleLog=$console.FullName; ConsoleLogSha256=$consoleProof.Sha256; Log=$log.FullName; LogSha256=$logProof.Sha256
            LogStartedAtUtc=$logStarted.ToString('o'); ExitCode=0; PackageDirectory=$package.FullName; Products=$products.ToArray()
        })
    }
    return $records.ToArray()
}

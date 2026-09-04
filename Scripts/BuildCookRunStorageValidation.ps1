. (Join-Path $PSScriptRoot 'RuntimeStorageValidation.ps1')

function Assert-SeedForgeBuildCookRunStorage {
    <# Fixed UE 5.8 Win64 BuildCookRun evidence: two serial UnrealPak scenarios.
       Cook and complete error/warning auditing remain the caller's responsibility.
       No stdout file is rewritten or promoted to native command-line authority. #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$UatLog,[Parameter(Mandatory)][string]$DiagnosticRoot,
        [Parameter(Mandatory)][string]$ProjectRoot,[Parameter(Mandatory)][string]$EngineRoot,
        [Parameter(Mandatory)][DateTimeOffset]$ProcessStartedAtUtc,[Parameter(Mandatory)][DateTimeOffset]$ProcessEndedAtUtc)
    Set-StrictMode -Version Latest
    $ErrorActionPreference='Stop'
    function Fail([string]$Reason){throw "BuildCookRunStorage: $Reason"}
    function FullPath([string]$Value){
        if([string]::IsNullOrWhiteSpace($Value) -or $Value -notmatch '\A[A-Za-z]:[\\/]' -or $Value.Substring(2).Contains(':') -or $Value.IndexOfAny([char[]]'"<>|?*') -ge 0 -or $Value -match '[\x00-\x1f]'){Fail "Invalid absolute Windows path: '$Value'."}
        return [IO.Path]::GetFullPath($Value.Replace('/','\')).TrimEnd('\')
    }
    function Owned([string]$Value,[string]$Boundary){
        $full=FullPath $Value
        if(-not $full.Equals($Boundary,[StringComparison]::OrdinalIgnoreCase) -and -not $full.StartsWith($Boundary+'\',[StringComparison]::OrdinalIgnoreCase)){Fail "Path outside its owned boundary: '$Value'."}
        $cursor=$full
        while($cursor.Length -ge $root.Length){
            if(Test-Path -LiteralPath $cursor){if(((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){Fail "Reparse component: '$cursor'."}}
            if($cursor.Equals($root,[StringComparison]::OrdinalIgnoreCase)){break};$cursor=[IO.Path]::GetDirectoryName($cursor)
        }
        return $full
    }
    function NoZen([string[]]$Lines,[string]$Name){
        $index=0
        foreach($line in $Lines){$index++
            if($line -match '(?i)\bLogZen(?:ServiceInstance|Server|Storage|Store):|\bStartZenServerForStage\b|(?:\b|_)ZenAutoLaunch\b|\b(?:Reading oplog from Zen|Opening oplog|Using ZenServer HTTP service)\b|(?:Fetched|Loaded).*\bfrom Zen\b|(?:\A|\s)-ZenStore(?:=|\s|\z)|(?:launch(?:ing|ed)?|initializ(?:e|ing|ation)|writ(?:e|ing)|install(?:ing|ed)?).*(?:zenserver|zen\.exe|zen_plugin_versions\.json)'){
                Fail "Zen activity in '$Name' at line $index."
            }
        }
    }
    function ReadLog([string]$Value){
        $path=Owned $Value $diagnostics
        if(-not [IO.Path]::GetDirectoryName($path).Equals($diagnostics,[StringComparison]::OrdinalIgnoreCase)){Fail 'Related logs must be direct children of the isolated diagnostic directory.'}
        if($records.ContainsKey($path)){return $records[$path]}
        $file=Get-Item -LiteralPath $path -Force
        if($file -isnot [IO.FileInfo] -or $file.Length -eq 0){Fail "Missing/empty regular log: '$path'."}
        if($file.LastWriteTimeUtc -lt $ProcessStartedAtUtc.UtcDateTime -or $file.LastWriteTimeUtc -gt $ProcessEndedAtUtc.UtcDateTime){Fail "Log is outside the process freshness interval: '$path'."}
        $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read);$reader=$null;$sha=$null
        try{
            $length=$stream.Length;$sha=[Security.Cryptography.SHA256]::Create();$hash=([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','').ToLowerInvariant();$stream.Position=0
            $reader=New-Object IO.StreamReader($stream,[Text.Encoding]::UTF8,$true);$lines=New-Object 'Collections.Generic.List[string]'
            while(-not $reader.EndOfStream){$lines.Add($reader.ReadLine())}
        }finally{if($null -ne $reader){$reader.Dispose()};if($null -ne $sha){$sha.Dispose()};$stream.Dispose()}
        NoZen $lines.ToArray() $path
        $record=[pscustomobject]@{Path=$path;Sha256=$hash;Length=$length;LineCount=$lines.Count;Lines=$lines.ToArray()}
        $records.Add($path,$record);return $record
    }
    function FileProof($Record){return [pscustomobject]@{Path=$Record.Path;Sha256=$Record.Sha256;Length=$Record.Length;LineCount=$Record.LineCount}}
    function Tokens([string]$Command){
        # Windows backslash/quote rules, scoped only to these observed arguments.
        $result=New-Object 'Collections.Generic.List[string]';$token=New-Object Text.StringBuilder;$quoted=$false;$started=$false;$index=0
        while($index -lt $Command.Length){$c=$Command[$index]
            if($c -eq '\'){$begin=$index;while($index -lt $Command.Length -and $Command[$index] -eq '\'){$index++};$count=$index-$begin;$started=$true
                if($index -lt $Command.Length -and $Command[$index] -eq '"'){for($n=0;$n -lt [math]::Floor($count/2.0);$n++){$null=$token.Append('\')};if($count%2){$null=$token.Append('"');$index++;continue};$c='"'}
                else{for($n=0;$n -lt $count;$n++){$null=$token.Append('\')};continue}
            }
            if($c -eq '"'){if($quoted -and $index+1 -lt $Command.Length -and $Command[$index+1] -eq '"'){$null=$token.Append('"');$index+=2}else{$quoted=-not $quoted;$index++};$started=$true;continue}
            if([char]::IsWhiteSpace($c) -and -not $quoted){if($started){$result.Add($token.ToString());$null=$token.Clear();$started=$false}}
            else{$null=$token.Append($c);$started=$true};$index++
        }
        if($quoted){Fail 'Unclosed quote in process command.'};if($started){$result.Add($token.ToString())};return ,$result.ToArray()
    }
    function SwitchValue([string[]]$ArgumentTokens,[string]$Name,[bool]$Bare=$false){
        $found=@($ArgumentTokens|Where-Object {$_ -match ('\A-'+[regex]::Escape($Name)+'(?:=|\z)')})
        if($found.Count -ne 1){Fail "Expected exactly one '$Name' argument."}
        if($Bare){if($found[0] -cne '-'+$Name){Fail "Expected bare '$Name' argument."};return $true}
        $prefix='-'+$Name+'=';if(-not $found[0].StartsWith($prefix,[StringComparison]::Ordinal) -or $found[0].Length -eq $prefix.Length){Fail "Malformed '$Name' argument."}
        return $found[0].Substring($prefix.Length)
    }
    function ScenarioTuple([string]$Command,[bool]$RequireProject){
        $argumentTokens=Tokens $Command
        if($RequireProject -and ($argumentTokens.Count -eq 0 -or -not (FullPath $argumentTokens[0]).Equals($projectFile,[StringComparison]::OrdinalIgnoreCase))){Fail 'UAT invocation must begin with the owned project file.'}
        if((SwitchValue $argumentTokens 'DDC') -cne 'SeedForgeLocal'){Fail 'UAT/native invocation did not select the owned graph.'}
        $null=SwitchValue $argumentTokens 'DDC-NoDefaultGraph' $true;$cache=Owned (SwitchValue $argumentTokens 'LocalDataCachePath') $root
        if(-not $cache.Equals($ddcRoot,[StringComparison]::OrdinalIgnoreCase)){Fail 'UAT/native cache override is not the owned store.'}
        $multiple=@($argumentTokens|Where-Object {$_ -match '\A-CreateMultiple(?:=|\z)'});$global=@($argumentTokens|Where-Object {$_ -match '\A-CreateGlobalContainer(?:=|\z)'})
        if($multiple.Count -eq 1 -and $global.Count -eq 0){
            if(@($argumentTokens|Where-Object {$_ -match '\A-Commands(?:=|\z)'}).Count){Fail 'Mixed Pak/IoStore scenario arguments.'}
            $response=Owned (SwitchValue $argumentTokens 'CreateMultiple') $diagnostics
            if(-not $response.Equals((Join-Path $diagnostics 'PakCommands.txt'),[StringComparison]::OrdinalIgnoreCase)){Fail 'Pak response path is not this run PakCommands.txt.'}
            return [pscustomobject]@{Scenario='CreateMultiplePaks';Response=$response;Container='';Cooked=''}
        }
        if($global.Count -eq 1 -and $multiple.Count -eq 0){
            $response=Owned (SwitchValue $argumentTokens 'Commands') $diagnostics
            if(-not $response.Equals((Join-Path $diagnostics 'IoStoreCommands.txt'),[StringComparison]::OrdinalIgnoreCase)){Fail 'IoStore response path is not this run IoStoreCommands.txt.'}
            return [pscustomobject]@{Scenario='CreateIoStoreContainers';Response=$response;Container=(Owned (SwitchValue $argumentTokens 'CreateGlobalContainer') $root);Cooked=(Owned (SwitchValue $argumentTokens 'CookedDirectory') $root)}
        }
        Fail 'Unknown or ambiguous UnrealPak scenario.'
    }
    function Comparable([string[]]$Lines){
        $normalized=@($Lines|ForEach-Object {$_.TrimEnd([char[]]" `t")});$count=$normalized.Count
        while($count -gt 0 -and $normalized[$count-1] -ceq ''){$count--}
        if($count -eq 0){return ''};return [string]::Join("`n",[string[]]$normalized[0..($count-1)])
    }
    if($ProcessStartedAtUtc -gt $ProcessEndedAtUtc){Fail 'Reversed process interval.'}
    $root=FullPath $ProjectRoot;$diagnostics=Owned $DiagnosticRoot (Join-Path $root 'Artifacts');$projectFile=Join-Path $root 'SeedForge.uproject';$ddcRoot=Join-Path $root '.cache\DerivedDataCache'
    if(-not(Test-Path -LiteralPath $projectFile -PathType Leaf)){Fail 'ProjectRoot does not contain SeedForge.uproject.'}
    $engine=FullPath $EngineRoot;$executable=Join-Path $engine 'Engine\Binaries\Win64\UnrealPak.exe';$copySource=Join-Path $root 'Saved\Logs\UnrealPak.log'
    $records=New-Object 'Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase)
    $uat=ReadLog $UatLog
    # Inventory directories explicitly so PowerShell-version-specific recursive
    # junction handling cannot read outside the diagnostic tree or hide a log.
    $related=New-Object 'Collections.Generic.List[IO.FileInfo]';$directories=New-Object 'Collections.Generic.Stack[string]';$directories.Push($diagnostics)
    while($directories.Count -gt 0){
        foreach($entry in Get-ChildItem -LiteralPath $directories.Pop() -Force){
            if(($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){Fail "Reparse entry in diagnostic tree: '$($entry.FullName)'."}
            if($entry -is [IO.DirectoryInfo]){$directories.Push($entry.FullName)}
            elseif($entry.Name -like 'UnrealPak*'){$related.Add($entry)}
        }
    }
    $stdout=New-Object 'Collections.Generic.List[object]';$full=New-Object 'Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase)
    foreach($file in $related){$record=ReadLog $file.FullName
        if($file.Name -match '\AUnrealPak-[^\\/]+\.txt\z'){$stdout.Add($record)}
        elseif($file.Name -match '\AUnrealPak_(?:CreateMultiplePaks|CreateIoStoreContainers)-[^\\/]+\.txt\z'){$full.Add($record.Path,$record)}
        else{Fail "Unexpected UnrealPak log: '$($file.Name)'."}
    }
    if($stdout.Count -ne 2 -or $full.Count -ne 2){Fail 'Expected exactly two stdout logs and two complete scenario logs.'}
    $associations=New-Object 'Collections.Generic.List[object]';$usedFull=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $active=$null;$lineNumber=0
    foreach($line in $uat.Lines){$lineNumber++
        if($line -match '\ARunning UnrealPak with arguments: (.*)\z'){
            if($null -ne $active){Fail "Overlapping/unclosed UnrealPak invocation at line $lineNumber."}
            $arguments=$Matches[1];$tuple=ScenarioTuple $arguments $true
            $active=[pscustomobject]@{State='Announced';Tuple=$tuple;Arguments=$arguments;AnnounceLine=$lineNumber;RawAnnounce=$line;RunningLine=0;RawRunning='';ExitLine=0;RawExit='';CopyLine=0;RawCopy='';Full=$null;Body=(New-Object 'Collections.Generic.List[string]')};continue
        }
        if($line -match '\ARunning: '){
            if($null -ne $active){
                if($active.State -cne 'Announced' -or $line -cne ('Running: '+$executable+' '+$active.Arguments)){Fail "Mismatched or interleaved process Running at line $lineNumber."}
                $active.State='Running';$active.RunningLine=$lineNumber;$active.RawRunning=$line;continue
            }
            if($line -match '(?i)UnrealPak\.exe(?:\s|\z)'){Fail "Orphan UnrealPak Running at line $lineNumber."};continue
        }
        if($line -match '\ATook (?:[0-9]+|[0-9]{1,3}(?:,[0-9]{3})+)\.[0-9]{2}s to run UnrealPak\.exe, ExitCode=(-?[0-9]+)\z'){
            $exitCode=$Matches[1]
            if($null -eq $active -or $active.State -cne 'Running' -or $exitCode -cne '0'){Fail "Missing/nonzero/duplicate UnrealPak exit at line $lineNumber."}
            $active.State='Exited';$active.ExitLine=$lineNumber;$active.RawExit=$line;continue
        }
        if($line -match '\ATook .*UnrealPak\.exe'){Fail "Malformed UnrealPak exit record at line $lineNumber."}
        if($line -match '\ASafeCopyFile ' -and $line -match '(?i)UnrealPak(?:\.log|_)'){
            if($null -eq $active -or $active.State -cne 'Exited'){Fail "Orphan/premature complete-log copy at line $lineNumber."}
            $prefix='SafeCopyFile '+$copySource+' '
            if(-not $line.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){Fail "Wrong complete-log copy source at line $lineNumber."}
            $target=Owned $line.Substring($prefix.Length) $diagnostics
            $name=[IO.Path]::GetFileName($target);$expected='UnrealPak_'+$active.Tuple.Scenario+'-'
            if(-not $name.StartsWith($expected,[StringComparison]::Ordinal) -or -not $full.ContainsKey($target) -or -not $usedFull.Add($target)){Fail "Unknown/reused/mismatched complete-log target at line $lineNumber."}
            $active.CopyLine=$lineNumber;$active.RawCopy=$line;$active.Full=$full[$target];$associations.Add($active);$active=$null;continue
        }
        if($null -ne $active -and $active.State -ceq 'Running'){$active.Body.Add($line)}
    }
    if($null -ne $active -or $associations.Count -ne 2 -or $usedFull.Count -ne 2){Fail 'Missing/extra/unclosed UnrealPak associations.'}
    $scenarios=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal);$usedStdout=New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase);$proofs=@()
    foreach($association in $associations){
        if(-not $scenarios.Add($association.Tuple.Scenario)){Fail 'Duplicate scenario.'}
        $body=Comparable $association.Body.ToArray();if($body -ceq ''){Fail 'Empty process output cannot be bound.'}
        $stdoutMatches=@($stdout|Where-Object {(Comparable $_.Lines) -ceq $body})
        if($stdoutMatches.Count -ne 1 -or -not $usedStdout.Add($stdoutMatches[0].Path)){Fail 'Missing/ambiguous/reused stdout content match.'}
        $native=$association.Full;$commandLines=New-Object 'Collections.Generic.List[string]';$exeLines=0
        foreach($line in $native.Lines){$text=$line -replace '\A(?:\[[^\]\r\n]*\]){1,2}\s*',''
            if($text -match '\ALogInit: Command Line: (.*)\z'){$commandLines.Add($Matches[1])}
            if($text -match '\ALogInit: ExecutableName: (.*)\z'){$exeLines++;if($Matches[1] -cne 'UnrealPak.exe'){Fail 'Complete log is not from UnrealPak.'}}
        }
        if($commandLines.Count -ne 1 -or $exeLines -ne 1){Fail 'Complete native log must contain one actual executable and command line.'}
        $tuple=ScenarioTuple $commandLines[0] $false
        foreach($field in @('Scenario','Response','Container','Cooked')){if(-not $tuple.$field.Equals($association.Tuple.$field,[StringComparison]::OrdinalIgnoreCase)){Fail "Native/UAT $field mismatch."}}
        $storage=Assert-SeedForgeRuntimeStorage -Path $native.Path -ProjectRoot $root -RequireDdc:$false
        if($storage.Sha256 -cne $native.Sha256){Fail 'Native log changed between association and storage validation.'}
        $proofs+=[pscustomobject]@{Scenario=$tuple.Scenario;ExitCode=0;AnnounceLineNumber=$association.AnnounceLine;RunningLineNumber=$association.RunningLine;ExitLineNumber=$association.ExitLine;CopyLineNumber=$association.CopyLine;RawAnnounceLine=$association.RawAnnounce;RawRunningLine=$association.RawRunning;RawExitLine=$association.RawExit;RawCopyLine=$association.RawCopy;Stdout=(FileProof $stdoutMatches[0]);NativeLog=(FileProof $native);StorageProof=$storage}
    }
    if($usedStdout.Count -ne 2 -or -not $scenarios.Contains('CreateMultiplePaks') -or -not $scenarios.Contains('CreateIoStoreContainers')){Fail 'Not exactly one of each required scenario and stdout.'}
    # Freeze expectations over original files. Caller evidence indexing retains
    # these digests; no raw transcript is normalized or regenerated on disk.
    foreach($record in $records.Values){$file=Get-Item -LiteralPath $record.Path -Force;if($file.Length -ne $record.Length -or (Get-FileHash -LiteralPath $record.Path -Algorithm SHA256).Hash.ToLowerInvariant() -cne $record.Sha256){Fail 'Original log bytes changed during validation.'}}
    return [pscustomobject]@{Validated=$true;ProcessCount=2;UatLog=(FileProof $uat);Processes=$proofs}
}

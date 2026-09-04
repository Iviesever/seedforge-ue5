function Assert-SeedForgeRuntimeStorage {
    <# Validates observed storage behavior, not all filesystem activity. The
       caller must also audit the whole log and bind it to a completed process.
       RequireDdc=false permits a cache-free packaged runtime, never an unsafe
       observed store/path or a fallback graph. Only the exact legacy shader
       comparison message may resolve a relative path, using an explicitly
       supplied executable directory matched to one native Base Directory.
       No global files are inspected. #>
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path,[Parameter(Mandatory)][string]$ProjectRoot,[bool]$RequireDdc=$true,
        [string]$ExpectedExecutableDirectory)
    Set-StrictMode -Version Latest
    $ErrorActionPreference='Stop'
    function Reject([string]$Reason) { throw "RuntimeStorage: $Reason" }
    function FullPath([string]$Value) {
        if([string]::IsNullOrWhiteSpace($Value) -or $Value -notmatch '\A[A-Za-z]:[\\/]' -or
            $Value.Substring(2).Contains(':') -or $Value.IndexOfAny([char[]]'"<>|?*') -ge 0 -or $Value -match '[\x00-\x1f]') {
            Reject "Expected an absolute local Windows path: '$Value'."
        }
        $full=[IO.Path]::GetFullPath($Value.Replace('/','\')).TrimEnd('\')
        if($full -match '\A[A-Za-z]:\z'){Reject 'Drive roots are not supported project/executable/file paths.'}
        return $full
    }
    function CheckOwnedPath([string]$Value,[bool]$ExactDdc=$false) {
        $full=FullPath $Value
        if(-not $full.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase)) { Reject "Path escapes project: '$Value'." }
        if($ExactDdc -and -not $full.Equals($ddcRoot,[StringComparison]::OrdinalIgnoreCase)) { Reject "Unexpected local DDC path: '$Value'." }
        # Reject existing reparse components without following or reading targets.
        $cursor=$full
        while($cursor.Length -ge $root.Length) {
            if(Test-Path -LiteralPath $cursor) {
                $item=Get-Item -LiteralPath $cursor -Force
                if(($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { Reject "Reparse component in owned path: '$cursor'." }
            }
            if($cursor.Equals($root,[StringComparison]::OrdinalIgnoreCase)){break}
            $cursor=[IO.Path]::GetDirectoryName($cursor)
        }
        return $full
    }
    function CommandTokens([string]$Command) {
        $result=New-Object 'Collections.Generic.List[string]'
        $token=New-Object Text.StringBuilder;$quoted=$false;$started=$false
        $index=0
        while($index -lt $Command.Length) {
            $character=$Command[$index]
            if($character -eq '\') {
                # Windows argument quoting: 2N backslashes before a quote emit
                # N slashes and delimit quoting; 2N+1 emit a literal quote.
                $begin=$index
                while($index -lt $Command.Length -and $Command[$index] -eq '\'){$index++}
                $count=$index-$begin;$started=$true
                if($index -lt $Command.Length -and $Command[$index] -eq '"'){
                    for($n=0;$n -lt [math]::Floor($count/2.0);$n++){$null=$token.Append('\')}
                    if($count%2 -ne 0){$null=$token.Append('"');$index++;continue}
                    $character='"'
                }else{for($n=0;$n -lt $count;$n++){$null=$token.Append('\')};continue}
            }
            if($character -eq '"'){
                if($quoted -and $index+1 -lt $Command.Length -and $Command[$index+1] -eq '"'){$null=$token.Append('"');$index+=2}
                else{$quoted=-not $quoted;$index++}
                $started=$true;continue
            }
            if([char]::IsWhiteSpace($character) -and -not $quoted) {
                if($started){$result.Add($token.ToString());$null=$token.Clear();$started=$false}
            }else{$null=$token.Append($character);$started=$true}
            $index++
        }
        if($quoted){Reject 'Unclosed quote in native command line.'}
        if($started){$result.Add($token.ToString())}
        return ,$result.ToArray()
    }
    $root=FullPath $ProjectRoot
    if(-not(Test-Path -LiteralPath (Join-Path $root 'SeedForge.uproject') -PathType Leaf)){Reject 'ProjectRoot does not contain SeedForge.uproject.'}
    if(((Get-Item -LiteralPath $root -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){Reject 'Project root is a reparse point.'}
    $ddcRoot=Join-Path $root '.cache\DerivedDataCache'
    $logPath=CheckOwnedPath $Path
    if(-not $logPath.StartsWith((Join-Path $root 'Artifacts')+'\',[StringComparison]::OrdinalIgnoreCase)){Reject 'Runtime log must be under project Artifacts.'}
    $file=Get-Item -LiteralPath $logPath -Force
    if($file -isnot [IO.FileInfo] -or $file.Length -eq 0){Reject 'Log must be a nonempty regular file.'}
    # One read handle excludes concurrent writers/deleters; hash and text refer
    # to the same original bytes, with BOM-aware decoding for native UE logs.
    $stream=[IO.File]::Open($logPath,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
    $reader=$null;$sha=$null
    try {
        $length=$stream.Length;$sha=[Security.Cryptography.SHA256]::Create()
        $hash=([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','').ToLowerInvariant()
        $stream.Position=0;$reader=New-Object IO.StreamReader($stream,[Text.Encoding]::UTF8,$true)
        $lines=New-Object 'Collections.Generic.List[string]'
        while(-not $reader.EndOfStream){$lines.Add($reader.ReadLine())}
    }finally{if($null -ne $reader){$reader.Dispose()};if($null -ne $sha){$sha.Dispose()};$stream.Dispose()}
    $nativeBaseDirectories=New-Object 'Collections.Generic.List[string]'
    foreach($line in $lines){
        $text=$line -replace '\A(?:\[[^\]\r\n]*\]){1,2}\s*',''
        if($text -match '\ALogInit:\s*(?:Display:\s*)?Base Directory:\s*(?<path>.+)\z'){$nativeBaseDirectories.Add($Matches.path)}
    }
    $commands=New-Object 'Collections.Generic.List[string]'
    $ddcObserved=$false;$ddcStores=0;$shaderPaths=0;$xgePaths=0;$lineNumber=0;$relativeLegacyPaths=0;$baseDirectoryMatched=$false
    foreach($line in $lines){$lineNumber++
        $text=$line -replace '\A(?:\[[^\]\r\n]*\]){1,2}\s*',''
        if($text -match '\A(?:LogZenServiceInstance|LogZenServer|LogZenStorage|LogZenStore):') { Reject "Zen service/storage activity at line $lineNumber." }
        if($text -notmatch '\A(?<category>Log[A-Za-z0-9_]+):\s*(?:(?:Display|Log|Verbose|VeryVerbose|Warning|Error|Fatal):\s*)?(?<message>.*)\z'){continue}
        $category=$Matches.category;$message=$Matches.message
        if($category -eq 'LogInit' -and $message -match '\ACommand Line:\s*(.*)\z'){$commands.Add($Matches[1]);continue}
        if($message -match '(?i)(?:launch(?:ing|ed)?|initializ(?:e|ing|ation)|writ(?:e|ing)|install(?:ing|ed)?).*(?:zenserver|zen\.exe|Zen (?:Storage|Store|service)|zen_plugin_versions\.json)|(?:Zen (?:Storage|Store|service)|zenserver).*(?:launch|initializ|writ|install)'){
            Reject "Zen initialization/write/launch at line $lineNumber."
        }
        if($category -eq 'LogDerivedDataCache'){
            $ddcObserved=$true
            if($message -match '(?i)fallback|falling back|fall back|(?:using|switching to|selecting) (?:the )?default graph'){Reject "DDC graph fallback at line $lineNumber."}
            if($message -match '(?i)Found (?:environment variable|editor (?:setting|override))'){Reject "Environment/editor DDC override at line $lineNumber."}
            if($message -match '(?i)\A(?:Zen\w*|Shared|Cloud):' -and $message -notmatch "(?i)\A(?:Zen\w*|Shared|Cloud): Disabled because (?:Host is set to 'None'|no path is configured\.)\z"){
                Reject "Active Zen/shared/cloud store at line $lineNumber."
            }
            if($message -match '(?i)Opened pak cache for writing'){Reject "Writable pak store outside the owned graph at line $lineNumber."}
            if($message -match '\A(?<node>[^:]+): Using data cache path (?<path>.+): (?<access>Writable|ReadOnly|DeleteOnly)\z'){
                $node=$Matches.node;$cachePath=$Matches.path;$access=$Matches.access
                $null=CheckOwnedPath $cachePath $true
                if($node -cne 'Local' -or $access -cne 'Writable'){Reject "Expected only Local writable filesystem DDC at line $lineNumber."}
                $ddcStores++;continue
            }
            if($message -match '(?i)Using data cache path'){Reject "Unrecognized cache-store path record at line $lineNumber."}
            if($message -match '\A(?<node>[^:]+): Found command line override LocalDataCachePath=(?<path>.+)\z'){
                $node=$Matches.node;$cachePath=$Matches.path
                if($node -cne 'Local'){Reject "Non-local command-line DDC store at line $lineNumber."}
                $null=CheckOwnedPath $cachePath $true;continue
            }
            if($message -match '\A(?<path>.+): (?:Speed tests took|Performance:).+\z'){$null=CheckOwnedPath $Matches.path $true;continue}
        }
        if($category -eq 'LogShaderCompilers' -and $message -match '(?i)working (?:directory|path)|temporary (?:directory|path)|(?:working|temp).*?[A-Za-z]:[\\/]'){
            $workingPath=$null;$isLegacyComparison=$false
            if($message -match "\ACleaned the shader compiler working directory '(?<path>[^']+)'\.\z"){$workingPath=$Matches.path}
            elseif($message -match '\AGuid format shader working directory is -?[0-9]+ characters bigger than the processId version \((?<path>.+)\)\.\z'){$workingPath=$Matches.path;$isLegacyComparison=$true}
            if($null -eq $workingPath){Reject "Unrecognized shader working-path record at line $lineNumber."}
            if($isLegacyComparison -and $workingPath -notmatch '\A[A-Za-z]:[\\/]'){
                if([string]::IsNullOrWhiteSpace($ExpectedExecutableDirectory) -or $nativeBaseDirectories.Count -ne 1){Reject "Relative legacy shader path requires a caller directory and exactly one native Base Directory at line $lineNumber."}
                $expectedBase=FullPath $ExpectedExecutableDirectory;$nativeBase=FullPath $nativeBaseDirectories[0]
                if(-not $nativeBase.Equals($expectedBase,[StringComparison]::OrdinalIgnoreCase)){Reject "Native Base Directory differs from the expected executable directory at line $lineNumber."}
                if([string]::IsNullOrWhiteSpace($workingPath) -or $workingPath -match '\A[\\/]|[\x00-\x1f]' -or $workingPath.Contains(':') -or $workingPath.IndexOfAny([char[]]'"<>|?*') -ge 0){Reject "Invalid relative legacy shader path at line $lineNumber."}
                # UE's FPaths conversion uses FPlatformProcess::BaseDir, not the
                # shell/provider working directory. Both inputs above are bound
                # before combining, and the result still receives normal checks.
                $workingPath=[IO.Path]::GetFullPath([IO.Path]::Combine($expectedBase,$workingPath.Replace('/','\')))
                $relativeLegacyPaths++;$baseDirectoryMatched=$true
            }
            $null=CheckOwnedPath $workingPath;$shaderPaths++
        }
        if($category -eq 'LogXGEController' -and $message -match '(?i)working (?:directory|path)|temporary (?:directory|path)|(?:working|temp).*?[A-Za-z]:[\\/]'){
            if($message -notmatch '\ACleaning working directory: (?<path>.+)\z'){Reject "Unrecognized XGE working-path record at line $lineNumber."}
            $null=CheckOwnedPath $Matches.path;$xgePaths++
        }
    }
    if($commands.Count -ne 1){Reject 'Expected exactly one native LogInit Command Line record.'}
    $tokens=CommandTokens $commands[0]
    $graph=@($tokens|Where-Object {$_ -match '\A-DDC(?:=|\z)'})
    $noDefault=@($tokens|Where-Object {$_ -match '\A-DDC-NoDefaultGraph(?:=|\z)'})
    $cache=@($tokens|Where-Object {$_ -match '\A-LocalDataCachePath(?:=|\z)'})
    $hasDdcArguments=$graph.Count+$noDefault.Count+$cache.Count -gt 0
    if($RequireDdc -or $ddcObserved -or $hasDdcArguments){
        if($graph.Count -ne 1 -or $graph[0] -cne '-DDC=SeedForgeLocal'){Reject 'Native command line must select exactly the owned DDC graph.'}
        if($noDefault.Count -ne 1 -or $noDefault[0] -cne '-DDC-NoDefaultGraph'){Reject 'Native command line must disable default-graph fallback with one bare switch.'}
        if($cache.Count -ne 1 -or $cache[0] -notmatch '\A-LocalDataCachePath=(.+)\z'){Reject 'Native command line must contain exactly one local cache path override.'}
        $null=CheckOwnedPath $Matches[1] $true
        if(($RequireDdc -or $ddcObserved) -and $ddcStores -ne 1){Reject 'Expected exactly one observed Local writable DDC store.'}
    }
    if(@($tokens|Where-Object {$_ -ieq '-run=Cook'}).Count -gt 0){
        $skip=@($tokens|Where-Object {$_ -match '\A-SkipZenStore(?:=|\z)'})
        if($skip.Count -ne 1 -or $skip[0] -cne '-SkipZenStore'){Reject 'Cook must explicitly skip Zen cooked storage.'}
        $editorOverrides=@($tokens|Where-Object {$_ -match '\A-ini:Editor(?::|=|\z)'})
        if($editorOverrides.Count -ne 1 -or $editorOverrides[0] -cne '-ini:Editor:[EditorDomain]:CookAttachmentsEnabled=False'){
            Reject 'Cook must disable optional EditorDomain attachments with exactly one precise Editor override.'
        }
    }
    return [pscustomobject]@{Path=$logPath;Sha256=$hash;Length=$length;LineCount=$lines.Count;Validated=$true;RequireDdc=$RequireDdc;DdcStoreCount=$ddcStores;ShaderWorkingPathCount=$shaderPaths;XgeWorkingPathCount=$xgePaths;RelativeLegacyShaderPathCount=$relativeLegacyPaths;BaseDirectoryMatched=$baseDirectoryMatched}
}

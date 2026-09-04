[CmdletBinding()]
param([string]$Label='check')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../../Scripts/RuntimeStorageValidation.ps1')
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo ('Artifacts/Reports/RuntimeStorageValidation/'+[datetime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $out -Force | Out-Null
$fixtureRoot=Join-Path $out 'Synthetic Project With Spaces'
New-Item -ItemType Directory -Path $fixtureRoot,(Join-Path $fixtureRoot 'Artifacts'),(Join-Path $fixtureRoot '.cache/Temp'),(Join-Path $fixtureRoot '.cache/DerivedDataCache') -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $fixtureRoot 'SeedForge.uproject'),'{"SyntheticTestFixtureOnly":true}')
function New-SyntheticStorageLog {
    @(
        'SYNTHETIC LOG FIXTURE ONLY - NOT RUNTIME OR RELEASE EVIDENCE',
        "LogInit: Command Line: /Game/Maps/SeedForgeDemo -game -DDC=SeedForgeLocal -DDC-NoDefaultGraph -LocalDataCachePath=`"$fixtureRoot/.cache/DerivedDataCache`"",
        'LogCsvProfiler: Display: Metadata set : zenstreaming="0"',
        'LogDerivedDataCache: Display: ../../../Engine/DerivedDataCache/Compressed.ddp: Opened pak cache for reading. (1667 MiB)',
        "LogDerivedDataCache: Local: Found command line override LocalDataCachePath=$fixtureRoot/.cache/DerivedDataCache",
        "LogDerivedDataCache: $fixtureRoot/.cache/DerivedDataCache: Speed tests took 0.01 seconds.",
        "LogDerivedDataCache: Display: $fixtureRoot/.cache/DerivedDataCache: Performance: Latency=0.01ms. RandomReadSpeed=100.00MBs, RandomWriteSpeed=100.00MBs. Assigned SpeedClass 'Local'",
        "LogDerivedDataCache: Local: Using data cache path $fixtureRoot/.cache/DerivedDataCache: Writable",
        "LogShaderCompilers: Guid format shader working directory is -3 characters bigger than the processId version ($fixtureRoot/.user/Intermediate/Shaders/WorkingDirectory/1234/).",
        "LogShaderCompilers: Cleaned the shader compiler working directory '$fixtureRoot/.cache/Temp/UnrealShaderWorkingDir/TEST-ONLY/'.",
        "LogXGEController: Cleaning working directory: $fixtureRoot/.cache/Temp/UnrealXGEWorkingDir/"
    )
}
$cases=New-Object System.Collections.Generic.List[object]
function Case([string]$Name,[scriptblock]$Mutate,[bool]$Accept=$false,[bool]$RequireDdc=$true){$cases.Add([pscustomobject]@{Name=$Name;Mutate=$Mutate;Accept=$Accept;RequireDdc=$RequireDdc})}
Case 'owned writable DDC and work paths' {} $true
Case 'quoted whole cache argument' {$d.Lines[1]=$d.Lines[1].Replace('-LocalDataCachePath="','"-LocalDataCachePath=')} $true
Case 'native timestamp prefix' {$d.Lines=@($d.Lines|ForEach-Object {'[2026.09.04-10.04.39:095][  0]'+$_})} $true
Case 'case and slash normalization for paths' {$d.Lines=@($d.Lines|ForEach-Object {$_.Replace($fixtureRoot,$fixtureRoot.ToUpperInvariant().Replace('\','/'))})} $true
Case 'disabled shared/cloud are not writes' {$d.Lines+=@("LogDerivedDataCache: Shared: Disabled because no path is configured.","LogDerivedDataCache: Cloud: Disabled because Host is set to 'None'")} $true
Case 'cache-free packaged log' {$d.Lines=@('LogInit: Command Line: -game -unattended','LogCsvProfiler: Display: Metadata set : zenstreaming="0"')} $true $false
Case 'packaged with DDC still validates active store' {} $true $false
Case 'cook explicitly skips Zen store' {$d.Lines[1]+=' -run=Cook -SkipZenStore'} $true
Case 'missing native command line' {$d.Lines=@($d.Lines|Where-Object {$_ -notmatch '^LogInit: Command Line:'})}
Case 'duplicate native command lines' {$d.Lines+=$d.Lines[1]}
Case 'CSV metadata is not native command line' {$d.Lines[1]=$d.Lines[1].Replace('LogInit: Command Line:','LogCsvProfiler: Display: commandline=')}
Case 'missing owned graph' {$d.Lines[1]=$d.Lines[1].Replace(' -DDC=SeedForgeLocal','')}
Case 'wrong graph' {$d.Lines[1]=$d.Lines[1].Replace('-DDC=SeedForgeLocal','-DDC=InstalledDerivedDataBackendGraph')}
Case 'duplicate graph' {$d.Lines[1]+=' -DDC=SeedForgeLocal'}
Case 'conflicting graph before owned graph' {$d.Lines[1]=$d.Lines[1].Replace('-game','-game -DDC=Default')}
Case 'missing fallback prevention' {$d.Lines[1]=$d.Lines[1].Replace(' -DDC-NoDefaultGraph','')}
Case 'false fallback prevention' {$d.Lines[1]=$d.Lines[1].Replace('-DDC-NoDefaultGraph','-DDC-NoDefaultGraph=false')}
Case 'duplicate fallback flag' {$d.Lines[1]+=' -DDC-NoDefaultGraph'}
Case 'missing cache override' {$d.Lines[1]=$d.Lines[1] -replace ' -LocalDataCachePath=.*$',''}
Case 'duplicate cache override' {$d.Lines[1]+=" -LocalDataCachePath=`"$fixtureRoot/.cache/DerivedDataCache`""}
Case 'external command cache path' {$d.Lines[1]=$d.Lines[1].Replace("$fixtureRoot/.cache/DerivedDataCache",'C:/Users/Outside/DerivedDataCache')}
Case 'root prefix is not containment' {$d.Lines[1]=$d.Lines[1].Replace("$fixtureRoot/.cache/DerivedDataCache","$fixtureRoot-evil/.cache/DerivedDataCache")}
Case 'cache traversal escape' {$d.Lines[1]=$d.Lines[1].Replace("$fixtureRoot/.cache/DerivedDataCache","$fixtureRoot/../../outside")}
Case 'cache relative path' {$d.Lines[1]=$d.Lines[1].Replace("$fixtureRoot/.cache/DerivedDataCache",'.cache/DerivedDataCache')}
Case 'UNC cache path' {$d.Lines[1]=$d.Lines[1].Replace("$fixtureRoot/.cache/DerivedDataCache",'\\server\share\cache')}
Case 'switch-looking quoted text is not switches' {$d.Lines[1]='LogInit: Command Line: -ExecCmds="echo -DDC=SeedForgeLocal -DDC-NoDefaultGraph -LocalDataCachePath=bad"'}
Case 'unclosed quoted command' {$d.Lines[1]+=' "unterminated'}
Case 'missing writable local marker' {$d.Lines=@($d.Lines|Where-Object {$_ -notmatch 'Using data cache path'})}
Case 'read-only is not writable' {$d.Lines[7]=$d.Lines[7].Replace('Writable','ReadOnly')}
Case 'external writable store' {$d.Lines[7]=$d.Lines[7].Replace("$fixtureRoot/.cache/DerivedDataCache",'C:/Users/Outside/cache')}
Case 'second external store' {$d.Lines+='LogDerivedDataCache: Other: Using data cache path C:/Users/Outside/cache: Writable'}
Case 'shared writable even with project path' {$d.Lines+="LogDerivedDataCache: Shared: Using data cache path $fixtureRoot/.cache/shared: Writable"}
Case 'cloud store active' {$d.Lines+='LogDerivedDataCache: Cloud: Using HTTP service at https://ddc.invalid/ with namespace ue.ddc. Status: OK!.'}
Case 'Zen service construction' {$d.Lines+="LogZenServiceInstance: Found environment variable UE-LocalDataCachePath=$fixtureRoot/.cache/DerivedDataCache"}
Case 'Zen plugin manifest write' {$d.Lines+="LogZenServiceInstance: Display: Writing plugin version info to 'C:/Users/Outside/Zen/Install/zen_plugin_versions.json'"}
Case 'Zen executable launch' {$d.Lines+="LogZenServiceInstance: Display: Launching executable 'C:/Users/Outside/Zen/Install/zenserver.exe'"}
Case 'Zen backend active' {$d.Lines+='LogDerivedDataCache: Display: ZenLocal: Using ZenServer HTTP service at [::1] with namespace ue.ddc. Status: OK!.'}
Case 'graph fallback' {$d.Lines+='LogDerivedDataCache: Warning: Falling back to default graph because SeedForgeLocal failed.'}
Case 'default graph fallback alternative text' {$d.Lines+='LogDerivedDataCache: Display: Using fallback graph InstalledDerivedDataBackendGraph'}
Case 'environment cache override' {$d.Lines+="LogDerivedDataCache: Local: Found environment variable UE-LocalDataCachePath=$fixtureRoot/.cache/DerivedDataCache"}
Case 'editor preference override' {$d.Lines+='LogDerivedDataCache: Local: Found editor setting GlobalLocalDDCPath=C:/Users/Outside/cache'}
Case 'external speed-test write before store selection' {$d.Lines+='LogDerivedDataCache: C:/Users/Outside/cache: Speed tests took 0.01 seconds.'}
Case 'external performance write test' {$d.Lines+='LogDerivedDataCache: Display: C:/Users/Outside/cache: Performance: Latency=0.1ms. RandomWriteSpeed=100MBs.'}
Case 'writable pak is not owned filesystem graph' {$d.Lines+='LogDerivedDataCache: Display: C:/Users/Outside/DDC.ddp: Opened pak cache for writing.'}
Case 'shader global temporary directory' {$d.Lines[9]="LogShaderCompilers: Cleaned the shader compiler working directory 'C:/Users/Outside/Temp/UnrealShaderWorkingDir/TEST/'."}
Case 'shader global process-ID directory' {$d.Lines[8]='LogShaderCompilers: Guid format shader working directory is 2 characters bigger than the processId version (C:/Users/Outside/Temp/1234/).'}
Case 'XGE global directory' {$d.Lines[10]='LogXGEController: Cleaning working directory: C:/Users/Outside/Temp/UnrealXGEWorkingDir/'}
Case 'XGE traversal directory' {$d.Lines[10]="LogXGEController: Cleaning working directory: $fixtureRoot/../../Outside"}
Case 'XGE prefix escape' {$d.Lines[10]="LogXGEController: Cleaning working directory: $fixtureRoot-evil/Temp/"}
Case 'shader unknown directory format fails closed' {$d.Lines+='LogShaderCompilers: Working directory changed to C:/Users/Outside/Temp/'}
Case 'shader relative path has unknown base' {$d.Lines[9]="LogShaderCompilers: Cleaned the shader compiler working directory '../../../Temp/UnrealShaderWorkingDir/TEST/'."}
Case 'packaged cannot hide unsafe shader path' {$d.Lines=@('LogInit: Command Line: -game',"LogShaderCompilers: Cleaned the shader compiler working directory 'C:/Users/Outside/Temp/TEST/'.")} $false $false
Case 'packaged cannot hide Zen initialization' {$d.Lines=@('LogInit: Command Line: -game','LogZenServiceInstance: Local ZenServer AutoLaunch initialization completed in 0.140 seconds')} $false $false
Case 'packaged DDC activity requires explicit arguments' {$d.Lines[1]='LogInit: Command Line: -game'} $false $false
Case 'cook must skip Zen cooked store' {$d.Lines[1]+=' -run=Cook'}
Case 'cook false SkipZenStore not accepted' {$d.Lines[1]+=' -run=Cook -SkipZenStore=false'}
Case 'empty log' {$d.Lines=@()}
Case 'escaped quote cannot expose nested switch text' {$d.Lines[1]='LogInit: Command Line: -ExecCmds="echo \" -DDC=SeedForgeLocal -DDC-NoDefaultGraph -LocalDataCachePath="'+$fixtureRoot+'/.cache/DerivedDataCache" \""'}
Case 'unrelated escaped quotes preserve actual flags' {$d.Lines[1]+=' -ExecCmds="echo \"nested words\""'} $true
Case 'DDC delete-only is still mutation' {$d.Lines[7]=$d.Lines[7].Replace('Writable','DeleteOnly')}
Case 'duplicate writable local backend' {$d.Lines+=$d.Lines[7]}
Case 'unknown XGE working-path form fails closed' {$d.Lines+='LogXGEController: Working path moved to C:/Users/Outside/Temp/'}
Case 'cache-free packaged runtime retains safe shared arguments' {$d.Lines=@($d.Lines[0],$d.Lines[1],$d.Lines[2])} $true $false
Case 'cache-free packaged runtime still rejects unsafe arguments' {$d.Lines=@($d.Lines[0],$d.Lines[1].Replace('-DDC=SeedForgeLocal','-DDC=Default'),$d.Lines[2])} $false $false
$results=@();$index=0
foreach($case in $cases){$index++;$d=[pscustomobject]@{Lines=(New-SyntheticStorageLog)};& $case.Mutate
    $path=Join-Path $fixtureRoot ("Artifacts/case-$index.log");[IO.File]::WriteAllText($path,($d.Lines -join "`r`n"))
    $originalHash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant();$accepted=$false;$message=''
    try{$proof=Assert-SeedForgeRuntimeStorage -Path $path -ProjectRoot $fixtureRoot -RequireDdc:$case.RequireDdc;$accepted=$true
        if($proof.Validated -ne $true -or $proof.Sha256 -cne $originalHash -or $proof.Path -cne $path){throw 'Returned proof/hash/path mismatch.'}
    }catch{$message=$_.Exception.Message}
    $passed=($accepted -eq $case.Accept) -and (-not $case.Accept -or $message -eq '')
    $results+=[pscustomobject]@{Name=$case.Name;Passed=$passed;ExpectedAccept=$case.Accept;Accepted=$accepted;Message=$message}
    if(-not $passed){Write-Host "FAIL $($case.Name): $message"}
}
$failed=@($results|Where-Object {-not $_.Passed}).Count
$summary=[ordered]@{SyntheticOnly=$true;Label=$Label;PowerShell=$PSVersionTable.PSVersion.ToString();Passed=$results.Count-$failed;Failed=$failed;Total=$results.Count;Cases=$results}
[IO.File]::WriteAllText((Join-Path $out 'summary.json'),($summary|ConvertTo-Json -Depth 7))
Write-Host "$($summary.Passed)/$($summary.Total) passed; $failed failed; $out"
if($failed){exit 1}

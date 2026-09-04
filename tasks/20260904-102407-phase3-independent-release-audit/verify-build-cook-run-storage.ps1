[CmdletBinding()]
param([string]$Label='check')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../../Scripts/BuildCookRunStorageValidation.ps1')
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $repo ('Artifacts/Reports/BuildCookRunStorageValidation/'+[datetime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
$project=Join-Path $out 'Synthetic Project With Spaces'
New-Item -ItemType Directory -Path $project,(Join-Path $project 'Artifacts') -Force|Out-Null
[IO.File]::WriteAllText((Join-Path $project 'SeedForge.uproject'),'{"SyntheticTestFixtureOnly":true}')
$engine='D:\program\UnrealEngine\Epic Games\UE_5.8'
function New-PakFixture($Index) {
    $dir=Join-Path $project "Artifacts/UAT-$Index";New-Item -ItemType Directory -Path $dir|Out-Null
    $flags='-DDC=SeedForgeLocal -DDC-NoDefaultGraph -LocalDataCachePath="'+$project+'\.cache\DerivedDataCache"'
    $scenarioArguments=@(('"'+$project+'\SeedForge.uproject" -CreateMultiple="'+$dir+'\PakCommands.txt" '+$flags),
        ('"'+$project+'\SeedForge.uproject" -CreateGlobalContainer="'+$project+'\Saved\StagedBuilds\Windows\SeedForge\Content\Paks\global.utoc" -Commands="'+$dir+'\IoStoreCommands.txt" -CookedDirectory="'+$project+'\Saved\Cooked\Windows" '+$flags))
    $names=@('CreateMultiplePaks','CreateIoStoreContainers');$files=[ordered]@{};$blocks=@()
    for($i=0;$i -lt 2;$i++) {
        $stdout="UnrealPak-2026.09.04-01.00.0$($i+1).txt";$full="UnrealPak_$($names[$i])-2026.09.04-01.00.0$($i+2).txt"
        $body=@("LogPakFile: Display: SYNTHETIC ONLY scenario=$($names[$i])",'LogPakFile: Display: UnrealPak executed in 0.100000 seconds')
        $files[$stdout]=@($body)+@('')
        $nativeArgs=$scenarioArguments[$i].Substring(('"'+$project+'\SeedForge.uproject" ').Length)+' -nopak'
        $files[$full]=@('SYNTHETIC ONLY - NOT RUNTIME EVIDENCE','LogInit: ExecutableName: UnrealPak.exe',('LogInit: Command Line: '+$nativeArgs))+@($body)
        $blocks+=,[pscustomobject]@{Scenario=$names[$i];Stdout=$stdout;Full=$full;Arguments=$scenarioArguments[$i];Lines=@(
            ('Running UnrealPak with arguments: '+$scenarioArguments[$i]),
            ('SafeFileExists '+$engine+'\Engine\Binaries\Win64\UnrealPak.exe=True'),
            ('Running: '+$engine+'\Engine\Binaries\Win64\UnrealPak.exe '+$scenarioArguments[$i]))+@($body)+@(
            'Took 0.10s to run UnrealPak.exe, ExitCode=0',
            ('SafeDeleteFile '+(Join-Path $dir $full)),
            ('SafeCopyFile '+$project+'\Saved\Logs\UnrealPak.log '+(Join-Path $dir $full)))}
    }
    [pscustomobject]@{Dir=$dir;Files=$files;Blocks=$blocks;Prefix=@('SYNTHETIC UAT FIXTURE ONLY - NOT RUNTIME/RELEASE EVIDENCE');Suffix=@();OldFile='';FutureFile='';LockFile='';RootJunction=$false;NestedJunction=$false;Root=$project;Engine=$engine;Start=[DateTimeOffset]::UtcNow.AddSeconds(-5);End=[DateTimeOffset]::UtcNow.AddSeconds(5)}
}
$cases=New-Object 'Collections.Generic.List[object]'
function Case([string]$Name,[scriptblock]$Change,[bool]$Accept=$false){$cases.Add([pscustomobject]@{Name=$Name;Change=$Change;Accept=$Accept})}
Case 'two exact original associations' {} $true
Case 'stdout need not contain LogInit' {} $true
Case 'only comparison normalizes trailing whitespace' {$f.Files[$f.Blocks[0].Stdout][0]+=" `t";$f.Files[$f.Blocks[0].Stdout]+=@('','')} $true
Case 'native quotes may differ from original invocation' {$key=$f.Blocks[0].Full;$f.Files[$key][2]=$f.Files[$key][2].Replace('-DDC=SeedForgeLocal','"-DDC=SeedForgeLocal"')} $true
Case 'missing invocation' {$f.Blocks=@($f.Blocks[0])}
Case 'extra invocation' {$f.Blocks+=@($f.Blocks[0])}
Case 'orphan real Running line' {$f.Prefix+=('Running: '+$engine+'\Engine\Binaries\Win64\UnrealPak.exe bogus')}
Case 'orphan exit' {$f.Suffix+='Took 0.10s to run UnrealPak.exe, ExitCode=0'}
Case 'missing announce' {$f.Blocks[0].Lines=@($f.Blocks[0].Lines[1..7])}
Case 'announce differs from real arguments' {$f.Blocks[0].Lines[0]+=' -extra'}
Case 'wrong Engine executable' {$f.Blocks[0].Lines[2]=$f.Blocks[0].Lines[2].Replace($engine,'D:\OtherEngine')}
Case 'missing real Running' {$f.Blocks[0].Lines=@($f.Blocks[0].Lines[0..1])+@($f.Blocks[0].Lines[3..7])}
Case 'second process begins before first exits' {$f.Blocks[0].Lines[5]=$f.Blocks[1].Lines[2]}
Case 'nonzero exit' {$f.Blocks[0].Lines[5]='Took 0.10s to run UnrealPak.exe, ExitCode=2'}
Case 'negative exit' {$f.Blocks[0].Lines[5]='Took 0.10s to run UnrealPak.exe, ExitCode=-1'}
Case 'missing exit' {$f.Blocks[0].Lines=@($f.Blocks[0].Lines[0..4])+@($f.Blocks[0].Lines[6..7])}
Case 'copy before exit' {$a=$f.Blocks[0].Lines[5];$f.Blocks[0].Lines[5]=$f.Blocks[0].Lines[7];$f.Blocks[0].Lines[7]=$a}
Case 'missing copy' {$f.Blocks[0].Lines=@($f.Blocks[0].Lines[0..6])}
Case 'wrong copy source' {$f.Blocks[0].Lines[7]=$f.Blocks[0].Lines[7].Replace($project+'\Saved\Logs\UnrealPak.log',$project+'\Saved\Logs\Other.log')}
Case 'copy outside diagnostics' {$f.Blocks[0].Lines[7]='SafeCopyFile '+$project+'\Saved\Logs\UnrealPak.log C:\Outside\UnrealPak_CreateMultiplePaks-2026.09.04-01.00.02.txt'}
Case 'copy target reused' {$f.Blocks[1].Lines[7]=$f.Blocks[0].Lines[7]}
Case 'copy has unknown scenario' {$f.Blocks[0].Lines[7]=$f.Blocks[0].Lines[7].Replace('CreateMultiplePaks','Unknown')}
Case 'missing full log' {$f.Files.Remove($f.Blocks[0].Full)}
Case 'extra full log' {$f.Files['UnrealPak_Extra-2026.09.04-01.00.09.txt']=$f.Files[$f.Blocks[0].Full]}
Case 'missing stdout' {$f.Files.Remove($f.Blocks[0].Stdout)}
Case 'extra stdout' {$f.Files['UnrealPak-2026.09.04-01.00.09.txt']=$f.Files[$f.Blocks[0].Stdout]}
Case 'stdout changed' {$f.Files[$f.Blocks[0].Stdout][0]+=' changed'}
Case 'stdout ambiguous duplicate content' {$f.Files[$f.Blocks[1].Stdout]=$f.Files[$f.Blocks[0].Stdout]}
Case 'full logs swapped' {$a=$f.Files[$f.Blocks[0].Full];$f.Files[$f.Blocks[0].Full]=$f.Files[$f.Blocks[1].Full];$f.Files[$f.Blocks[1].Full]=$a}
Case 'native executable mismatch' {$f.Files[$f.Blocks[0].Full][1]='LogInit: ExecutableName: Other.exe'}
Case 'native command missing' {$k=$f.Blocks[0].Full;$f.Files[$k]=@($f.Files[$k][0..1])+@($f.Files[$k][3..4])}
Case 'native command duplicate' {$k=$f.Blocks[0].Full;$f.Files[$k]+=$f.Files[$k][2]}
Case 'native response path mismatch' {$k=$f.Blocks[0].Full;$f.Files[$k][2]=$f.Files[$k][2].Replace('PakCommands.txt','OtherCommands.txt')}
Case 'native container path mismatch' {$k=$f.Blocks[1].Full;$f.Files[$k][2]=$f.Files[$k][2].Replace('global.utoc','other.utoc')}
Case 'native cooked directory mismatch' {$k=$f.Blocks[1].Full;$f.Files[$k][2]=$f.Files[$k][2].Replace('Cooked\Windows','Cooked\Other')}
Case 'UAT response path escapes' {$old=$f.Dir+'\PakCommands.txt';foreach($n in @(0,2)){$f.Blocks[0].Lines[$n]=$f.Blocks[0].Lines[$n].Replace($old,'C:\Outside\PakCommands.txt')}}
Case 'native DDC wrong graph' {$k=$f.Blocks[0].Full;$f.Files[$k][2]=$f.Files[$k][2].Replace('DDC=SeedForgeLocal','DDC=Default')}
Case 'UAT DDC wrong graph' {foreach($n in @(0,2)){$f.Blocks[0].Lines[$n]=$f.Blocks[0].Lines[$n].Replace('DDC=SeedForgeLocal','DDC=Default')}}
Case 'UAT duplicate graph' {foreach($n in @(0,2)){$f.Blocks[0].Lines[$n]+=' -DDC=SeedForgeLocal'}}
Case 'scenario token only inside unrelated quotes' {foreach($n in @(0,2)){$f.Blocks[0].Lines[$n]=$f.Blocks[0].Lines[$n].Replace('-CreateMultiple="','-ExecCmds="echo -CreateMultiple=')}}
Case 'Zen in UAT outside process block' {$f.Prefix+='LogZenServiceInstance: Display: Writing plugin version info to C:/Outside/zen_plugin_versions.json'}
Case 'Zen in full native log' {$f.Files[$f.Blocks[1].Full]+='LogZenServiceInstance: Found local data cache path=C:/Outside/cache'}
Case 'Zen in stdout and corresponding block' {$z='LogZenStore: Display: Opening oplog test';$f.Files[$f.Blocks[0].Stdout][0]=$z;$f.Blocks[0].Lines[3]=$z}
Case 'Zen stage launch' {foreach($n in @(0,2)){$f.Blocks[0].Lines[$n]+=' -StartZenServerForStage'}}
Case 'old native full log' {$f.OldFile=$f.Blocks[0].Full}
Case 'old stdout' {$f.OldFile=$f.Blocks[0].Stdout}
Case 'old UAT log' {$f.OldFile='Log.txt'}
Case 'future log timestamp' {$f.FutureFile=$f.Blocks[0].Full}
Case 'reversed process interval' {$a=$f.Start;$f.Start=$f.End;$f.End=$a}
Case 'malformed Took duration is not process evidence' {$f.Blocks[0].Lines[5]='Took nonsense to run UnrealPak.exe, ExitCode=0'}
Case 'duplicate copy cannot reuse a completed association' {$f.Suffix+=$f.Blocks[1].Lines[7]}
Case 'active writer prevents native-log snapshot' {$f.LockFile=$f.Blocks[0].Full}
Case 'diagnostic root junction is rejected' {$f.RootJunction=$true}
Case 'UAT escaped quote cannot promote nested switches' {foreach($n in @(0,2)){$f.Blocks[0].Lines[$n]=$f.Blocks[0].Lines[$n].Replace('-CreateMultiple="','-ExecCmds="echo \" -CreateMultiple="')+' \""'}}
Case 'nested diagnostic junction must not be followed or ignored' {$f.NestedJunction=$true}
$results=@();$index=0
foreach($case in $cases){$index++;$f=New-PakFixture $index;& $case.Change
    $uat=@($f.Prefix);foreach($block in $f.Blocks){$uat+=@($block.Lines)};$uat+=@($f.Suffix);$f.Files['Log.txt']=$uat
    foreach($name in @($f.Files.Keys)){[IO.File]::WriteAllText((Join-Path $f.Dir $name),($f.Files[$name] -join "`r`n"))}
    if($f.OldFile){[IO.File]::SetLastWriteTimeUtc((Join-Path $f.Dir $f.OldFile),[datetime]::UtcNow.AddHours(-1))}
    if($f.FutureFile){[IO.File]::SetLastWriteTimeUtc((Join-Path $f.Dir $f.FutureFile),[datetime]::UtcNow.AddHours(1))}
    $hash=(Get-FileHash -LiteralPath (Join-Path $f.Dir 'Log.txt') -Algorithm SHA256).Hash.ToLowerInvariant();$accepted=$false;$message='';$lock=$null;$callDir=$f.Dir
    $beforeHashes=@{};foreach($name in @($f.Files.Keys)){$beforeHashes[$name]=(Get-FileHash -LiteralPath (Join-Path $f.Dir $name) -Algorithm SHA256).Hash}
    if($f.RootJunction){$callDir=$f.Dir+'-junction';New-Item -ItemType Junction -Path $callDir -Target $f.Dir|Out-Null}
    if($f.NestedJunction){$target=Join-Path $out 'Synthetic Junction Target';New-Item -ItemType Directory -Path $target -Force|Out-Null;New-Item -ItemType Junction -Path (Join-Path $f.Dir 'UnexpectedLink') -Target $target|Out-Null}
    if($f.LockFile){$lock=[IO.File]::Open((Join-Path $f.Dir $f.LockFile),[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::ReadWrite)}
    try{$proof=Assert-SeedForgeBuildCookRunStorage -UatLog (Join-Path $callDir 'Log.txt') -DiagnosticRoot $callDir -ProjectRoot $f.Root -EngineRoot $f.Engine -ProcessStartedAtUtc $f.Start -ProcessEndedAtUtc $f.End;$accepted=$true
        if($proof.Validated -ne $true -or $proof.ProcessCount -ne 2 -or $proof.UatLog.Sha256 -cne $hash){throw 'Proof/count/original hash mismatch.'}
        if($case.Accept){foreach($process in $proof.Processes){foreach($record in @($process.Stdout,$process.NativeLog,$process.StorageProof)){if($record.Sha256 -cne $beforeHashes[[IO.Path]::GetFileName($record.Path)].ToLowerInvariant()){throw 'Original stdout/native hash mismatch.'}}}}
    }catch{$message=$_.Exception.Message}finally{if($null -ne $lock){$lock.Dispose()}}
    foreach($name in @($f.Files.Keys)){if((Get-FileHash -LiteralPath (Join-Path $f.Dir $name) -Algorithm SHA256).Hash -cne $beforeHashes[$name]){throw 'Validator mutated original log bytes.'}}
    $passed=($accepted -eq $case.Accept) -and (-not $case.Accept -or $message -eq '')
    $results+=[pscustomobject]@{Name=$case.Name;Passed=$passed;ExpectedAccept=$case.Accept;Accepted=$accepted;Message=$message}
    if(-not $passed){Write-Host "FAIL $($case.Name): $message"}
}
$failed=@($results|Where-Object {-not $_.Passed}).Count
$summary=[ordered]@{SyntheticOnly=$true;Label=$Label;PowerShell=$PSVersionTable.PSVersion.ToString();Passed=$results.Count-$failed;Failed=$failed;Total=$results.Count;Cases=$results}
[IO.File]::WriteAllText((Join-Path $out 'summary.json'),($summary|ConvertTo-Json -Depth 8))
Write-Host "$($summary.Passed)/$($summary.Total) passed; $failed failed; $out"
if($failed){exit 1}

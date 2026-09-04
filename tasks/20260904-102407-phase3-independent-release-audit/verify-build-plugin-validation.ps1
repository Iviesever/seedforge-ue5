[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/BuildPluginValidation.ps1')
$fixtureRoot = Join-Path $projectRoot ('Artifacts/Reports/BuildPluginValidation/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
$results = [Collections.Generic.List[object]]::new()
function Write-ProofText([string]$Path, [string]$Text) { [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false)) }
function New-TargetFixture([bool]$NoSpaces = $false) {
    $caseName = if ($NoSpaces) { 'case-' } else { 'case ' }
    $root = Join-Path $fixtureRoot ($caseName + $results.Count)
    $package = Join-Path $root 'package'
    $logs = Join-Path $root 'diagnostics'
    $bin = Join-Path $package 'Binaries/Win64'
    New-Item -ItemType Directory -Path $package,$logs,$bin | Out-Null
    $started = [DateTimeOffset]::UtcNow.AddSeconds(-3)
    $descriptor = Join-Path $package 'SeedForge.uplugin'
    Write-ProofText $descriptor '{"FileVersion":3,"Modules":[{"Name":"SeedForgeRuntime","Type":"Runtime"},{"Name":"SeedForgeEditor","Type":"Editor"},{"Name":"SeedForgeTests","Type":"Editor"}]}'
    $modules = Join-Path $bin 'UnrealEditor.modules'
    Write-ProofText $modules '{"BuildId":"SYNTHETIC-NOT-RELEASE","Modules":{"SeedForgeRuntime":"UnrealEditor-SeedForgeRuntime.dll","SeedForgeEditor":"UnrealEditor-SeedForgeEditor.dll","SeedForgeTests":"UnrealEditor-SeedForgeTests.dll"}}'
    $dlls = @()
    foreach ($module in @('SeedForgeRuntime','SeedForgeEditor','SeedForgeTests')) {
        $dll = Join-Path $bin "UnrealEditor-$module.dll"
        # Minimal PE header fixture only; this is intentionally not executable code.
        $bytes = [byte[]]::new(512); $bytes[0]=0x4d; $bytes[1]=0x5a; $bytes[0x3c]=0x80
        $bytes[0x80]=0x50; $bytes[0x81]=0x45; $bytes[0x84]=0x64; $bytes[0x85]=0x86; $bytes[0x97]=0x20
        [IO.File]::WriteAllBytes($dll,$bytes); $dlls += $dll
    }
    $targetLogs = @(); $precompiled = @(); $objects = @(); $blocks = @()
    foreach ($spec in @(@('UnrealEditor','Development'),@('UnrealGame','Development'),@('UnrealGame','Shipping'))) {
        $target = $spec[0]; $config = $spec[1]
        $log = Join-Path $logs "UBA-$target-Win64-$config.txt"
        $hostProject = Join-Path $package 'HostProject/HostProject.uproject'
        $hostPlugin = Join-Path $package 'HostProject/Plugins/SeedForge/SeedForge.uplugin'
        $manifest = Join-Path $package "HostProject/Saved/Manifest-$target-Win64-$config.xml"
        $command = '"D:\SYNTHETIC ENGINE\UnrealBuildTool.dll" ' + "$target Win64 $config " + '-Project="' + $hostProject + '" -plugin="' + $hostPlugin + '" -manifest="' + $manifest + '" -log="' + $log + '"'
        $logText = 'Log started at synthetic (' + [DateTimeOffset]::UtcNow.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'") + ")`nCommand line: $command`nBuilding $target...`nResult: Succeeded`n"
        Write-ProofText $log $logText; $targetLogs += $log
        $blocks += "Running: D:\synthetic\dotnet.exe $command`nLog file: $log`nBuilding $target...`nResult: Succeeded`nTook 1.00s to run dotnet.exe, ExitCode=0`n"
        if ($target -eq 'UnrealGame') {
            $objRoot = Join-Path $package "Intermediate/Build/Win64/x64/UnrealGame/$config/SeedForgeRuntime"
            New-Item -ItemType Directory -Path $objRoot | Out-Null
            $object = Join-Path $objRoot 'Module.SeedForgeRuntime.cpp.obj'
            $bytes = [byte[]]::new(64); $bytes[0]=0x64; $bytes[1]=0x86; [IO.File]::WriteAllBytes($object,$bytes)
            $pre = Join-Path $objRoot 'SeedForgeRuntime.precompiled'
            Write-ProofText $pre '{"OutputFiles":["Module.SeedForgeRuntime.cpp.obj"],"ToolChainVersion":"synthetic"}'
            $precompiled += $pre; $objects += $object
        }
    }
    $console = Join-Path $root 'console.log'
    Write-ProofText $console (($blocks -join '') + "BUILD SUCCESSFUL`nAutomationTool exiting with ExitCode=0 (Success)`n")
    return [pscustomobject]@{ Root=$root; Package=$package; Logs=$logs; Console=$console; Started=$started; Blocks=$blocks; TargetLogs=$targetLogs; Dlls=$dlls; Modules=$modules; Precompiled=$precompiled; Objects=$objects }
}
function Check-TargetCase([string]$Name,[scriptblock]$Action) {
    $fixture = New-TargetFixture ($Name -eq 'reject argument-inside-quoted-value')
    try { & $Action $fixture; $results.Add([pscustomobject]@{name=$Name; passed=$true; detail='' }); Write-Host "PASS $Name" }
    catch { $results.Add([pscustomobject]@{name=$Name; passed=$false; detail=$_.Exception.Message }); Write-Host "FAIL $Name : $($_.Exception.Message)" }
}
function Read-TargetProof($f) { Get-SeedForgeBuildPluginTargetProof -ConsoleLog $f.Console -DiagnosticRoot $f.Logs -PackageDirectory $f.Package -StartedAtUtc $f.Started }
function Reject-TargetProof($f) {
    $rejected=$false; try { Read-TargetProof $f | Out-Null } catch { $rejected=$true }
    if (-not $rejected) { throw 'Incomplete or forged target evidence was accepted.' }
}
Check-TargetCase 'three targets require logs and packaged products without HostProject' {
    param($f)
    $proof = @(Read-TargetProof $f)
    if ($proof.Count -ne 3 -or ($proof.Label -join ',') -cne 'UnrealEditor Win64 Development,UnrealGame Win64 Development,UnrealGame Win64 Shipping') { throw 'Actual three-target proof missing.' }
    foreach ($record in $proof) {
        if ($record.LogSha256 -cnotmatch '^[0-9a-f]{64}$' -or @($record.Products).Count -lt 2) { throw 'Target evidence hashes/products missing.' }
        foreach ($product in $record.Products) { if ($product.Sha256 -cnotmatch '^[0-9a-f]{64}$' -or $product.Length -le 0) { throw 'Product was not independently hashed.' } }
    }
    if (Test-Path -LiteralPath (Join-Path $f.Package 'HostProject')) { throw 'Fixture incorrectly depends on retained host.' }
}
$modes = @('missing-console','empty-console','stale-console','missing-target','duplicate-target','wrong-platform','target-exit','missing-target-exit',
    'missing-overall-success','overall-exit','exit-before-result','log-result-before-command','argument-inside-quoted-value',
    'console-bind-failure','target-bind-failure','missing-log','stale-log','stale-log-header','wrong-log-command',
    'escaped-log','wrong-project','duplicate-log-argument','missing-dll','bad-dll','stale-dll','missing-modules','wrong-module-map',
    'missing-precompiled','malformed-precompiled','empty-outputs','escaped-object','duplicate-object','missing-object','empty-object','stale-object','wrong-object-architecture','reparse-products')
foreach ($mode in $modes) {
    Check-TargetCase "reject $mode" {
        param($f)
        switch ($mode) {
            'missing-console' { $f.Console = Join-Path $f.Root 'missing.log' }
            'empty-console' { Write-ProofText $f.Console '' }
            'stale-console' { [IO.File]::SetLastWriteTimeUtc($f.Console,$f.Started.UtcDateTime.AddMinutes(-1)) }
            'missing-target' { Write-ProofText $f.Console (($f.Blocks[0..1] -join '') + "BUILD SUCCESSFUL`nAutomationTool exiting with ExitCode=0 (Success)`n") }
            'duplicate-target' { [IO.File]::AppendAllText($f.Console,$f.Blocks[0]) }
            'wrong-platform' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace('UnrealGame Win64 Shipping','UnrealGame Linux Shipping')) }
            'target-exit' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace('Took 1.00s to run dotnet.exe, ExitCode=0','Took 1.00s to run dotnet.exe, ExitCode=6')) }
            'missing-target-exit' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace('Took 1.00s to run dotnet.exe, ExitCode=0','')) }
            'missing-overall-success' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace('BUILD SUCCESSFUL','')) }
            'overall-exit' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace('AutomationTool exiting with ExitCode=0','AutomationTool exiting with ExitCode=1')) }
            'exit-before-result' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace("Result: Succeeded`nTook 1.00s to run dotnet.exe, ExitCode=0", "Took 1.00s to run dotnet.exe, ExitCode=0`nResult: Succeeded")) }
            'log-result-before-command' { Write-ProofText $f.TargetLogs[1] ("Result: Succeeded`n" + (Get-Content -Raw -LiteralPath $f.TargetLogs[1]).Replace("Result: Succeeded`n",'')) }
            'argument-inside-quoted-value' {
                $hostProject=Join-Path $f.Package 'HostProject/HostProject.uproject'
                # Move the real -Project field into another argument's value.
                foreach($path in @($f.Console)+$f.TargetLogs){
                    $text=Get-Content -Raw -LiteralPath $path
                    Write-ProofText $path ($text.Replace('-Project="'+$hostProject+'"','-Ignored=" -Project='+$hostProject+' trailing-text"'))
                }
            }
            'console-bind-failure' { [IO.File]::AppendAllText($f.Console,"`nUbaServer - bind 0.0.0.0:1345 failed (synthetic)") }
            'target-bind-failure' { [IO.File]::AppendAllText($f.TargetLogs[1],"`nUbaServer - bind 0.0.0.0:1345 failed (synthetic)") }
            'missing-log' { Move-Item -LiteralPath $f.TargetLogs[1] -Destination (Join-Path $f.Logs 'held.txt') }
            'stale-log' { [IO.File]::SetLastWriteTimeUtc($f.TargetLogs[1],$f.Started.UtcDateTime.AddMinutes(-1)) }
            'stale-log-header' { Write-ProofText $f.TargetLogs[1] ((Get-Content -Raw -LiteralPath $f.TargetLogs[1]) -replace '\([0-9]{4}-[^)]+Z\)','(2001-01-01T00:00:00Z)') }
            'wrong-log-command' { Write-ProofText $f.TargetLogs[1] ((Get-Content -Raw -LiteralPath $f.TargetLogs[1]).Replace('UnrealGame Win64 Development','UnrealGame Win64 Shipping')) }
            'escaped-log' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace($f.TargetLogs[1],(Join-Path $f.Root 'elsewhere.txt'))) }
            'wrong-project' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace('HostProject.uproject','OtherProject.uproject')) }
            'duplicate-log-argument' { Write-ProofText $f.Console ((Get-Content -Raw -LiteralPath $f.Console).Replace(' -log=',' -log="duplicate.txt" -log=')) }
            'missing-dll' { Move-Item -LiteralPath $f.Dlls[1] -Destination (Join-Path $f.Root 'held.dll') }
            'bad-dll' { Write-ProofText $f.Dlls[1] 'synthetic non-PE file' }
            'stale-dll' { [IO.File]::SetLastWriteTimeUtc($f.Dlls[1],$f.Started.UtcDateTime.AddMinutes(-1)) }
            'missing-modules' { Move-Item -LiteralPath $f.Modules -Destination (Join-Path $f.Root 'held.modules') }
            'wrong-module-map' { Write-ProofText $f.Modules ((Get-Content -Raw -LiteralPath $f.Modules).Replace('UnrealEditor-SeedForgeEditor.dll','UnrealEditor-SeedForgeRuntime.dll')) }
            'missing-precompiled' { Move-Item -LiteralPath $f.Precompiled[1] -Destination (Join-Path $f.Root 'held.precompiled') }
            'malformed-precompiled' { Write-ProofText $f.Precompiled[1] '{malformed' }
            'empty-outputs' { Write-ProofText $f.Precompiled[1] '{"OutputFiles":[]}' }
            'escaped-object' { Write-ProofText $f.Precompiled[1] '{"OutputFiles":["../../../../../../../../escaped.obj"]}' }
            'duplicate-object' { Write-ProofText $f.Precompiled[1] '{"OutputFiles":["Module.SeedForgeRuntime.cpp.obj","Module.SeedForgeRuntime.cpp.obj"]}' }
            'missing-object' { Move-Item -LiteralPath $f.Objects[1] -Destination (Join-Path $f.Root 'held.obj') }
            'empty-object' { Write-ProofText $f.Objects[1] '' }
            'stale-object' { [IO.File]::SetLastWriteTimeUtc($f.Objects[1],$f.Started.UtcDateTime.AddMinutes(-1)) }
            'wrong-object-architecture' { $bytes=[byte[]]::new(64); $bytes[0]=0x4c; $bytes[1]=0x01; [IO.File]::WriteAllBytes($f.Objects[1],$bytes) }
            'reparse-products' {
                $bin=Join-Path $f.Package 'Binaries/Win64'; $held=Join-Path $f.Root 'held-binaries'
                $ownedPrefix=[IO.Path]::GetFullPath($f.Root).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
                foreach($candidate in @($bin,$held)){
                    if(-not [IO.Path]::GetFullPath($candidate).StartsWith($ownedPrefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture directory move escaped its exact owned case root.'}
                }
                Move-Item -LiteralPath $bin -Destination $held
                New-Item -ItemType Junction -Path $bin -Target $held | Out-Null
            }
        }
        Reject-TargetProof $f
    }
}
$passed=@($results | Where-Object passed).Count
$summary=[ordered]@{ purpose='Synthetic logs and header-only product fixtures; NOT executed builds or release artifacts'; powershellVersion=$PSVersionTable.PSVersion.ToString(); fixtureDirectory=$fixtureRoot; total=$results.Count; passed=$passed; failed=$results.Count-$passed; results=$results.ToArray() }
[IO.File]::WriteAllText((Join-Path $fixtureRoot 'summary.json'),($summary | ConvertTo-Json -Depth 6))
Write-Host "BUILD_PLUGIN_VALIDATION total=$($results.Count) passed=$passed failed=$($results.Count-$passed) fixtures=$fixtureRoot"
if($passed -ne $results.Count){exit 1}

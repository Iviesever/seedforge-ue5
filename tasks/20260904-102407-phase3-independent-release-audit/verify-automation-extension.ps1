[CmdletBinding()]
param([string]$EngineRoot='D:\program\UnrealEngine\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$projectRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$fixtureRoot=Join-Path $projectRoot ('Artifacts/Reports/AutomationExtension/'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N'))
$cacheRoot=Join-Path $projectRoot '.cache/AutomationExtension'
New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
$dotnet=Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/DotNet/10.0/win-x64/dotnet.exe'
if(-not(Test-Path -LiteralPath $dotnet -PathType Leaf)){throw 'Bundled UE 5.8 dotnet was not found.'}
$prior=@{}
$settings=@{
    DOTNET_CLI_HOME=Join-Path $cacheRoot 'cli'; NUGET_PACKAGES=Join-Path $cacheRoot 'nuget/packages'
    NUGET_HTTP_CACHE_PATH=Join-Path $cacheRoot 'nuget/http'; NUGET_SCRATCH=Join-Path $cacheRoot 'nuget/scratch'
    DOTNET_CLI_TELEMETRY_OPTOUT='1'; DOTNET_SKIP_FIRST_TIME_EXPERIENCE='1'; DOTNET_CLI_WORKLOAD_UPDATE_NOTIFY_DISABLE='1'
    DOTNET_NOLOGO='1'; MSBUILDDISABLENODEREUSE='1'; SEEDFORGE_ENGINE_ROOT=$EngineRoot
    TEMP=Join-Path $cacheRoot 'temp'; TMP=Join-Path $cacheRoot 'temp'
}
foreach($key in $settings.Keys){$prior[$key]=[Environment]::GetEnvironmentVariable($key,'Process')}
function Invoke-LocalDotnet([string[]]$Arguments,[string]$LogName){
    $ErrorActionPreference='Continue'
    & $dotnet @Arguments 2>&1 | Tee-Object -FilePath (Join-Path $fixtureRoot $LogName)
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($code -ne 0){throw "Local dotnet exit $code. See $fixtureRoot/$LogName"}
}
try{
    foreach($key in $settings.Keys){[Environment]::SetEnvironmentVariable($key,$settings[$key],'Process')}
    New-Item -ItemType Directory -Force -Path $settings.DOTNET_CLI_HOME,$settings.NUGET_PACKAGES,$settings.NUGET_HTTP_CACHE_PATH,$settings.NUGET_SCRATCH,$settings.TEMP | Out-Null
    $projects=@('Scripts/Automation/SeedForgeBuild.Automation.csproj','Scripts/Automation/Tests/SeedForgeBuild.ArgumentTests.csproj')
    for($index=0;$index -lt $projects.Count;++$index){
        $project=Join-Path $projectRoot $projects[$index]
        Invoke-LocalDotnet @('restore',$project,'--configfile',(Join-Path $projectRoot 'Scripts/Automation/NuGet.Config'),('-p:EngineRoot='+$EngineRoot),'--disable-parallel') "restore-$index.log"
        Invoke-LocalDotnet @('build',$project,'-c','Release','--no-restore','--disable-build-servers',('-p:EngineRoot='+$EngineRoot),'-p:UseSharedCompilation=false','-nodeReuse:false') "build-$index.log"
    }
    $assembly=Join-Path $projectRoot 'Artifacts/Automation/SeedForgeBuild.Automation/net10.0/SeedForgeBuild.Automation.dll'
    $tests=Join-Path $projectRoot 'Artifacts/Automation/SeedForgeBuild.ArgumentTests/net10.0/SeedForgeBuild.ArgumentTests.dll'
    $ErrorActionPreference='Continue'
    & $dotnet $tests $projectRoot $fixtureRoot $assembly $EngineRoot 2>&1 | Tee-Object -FilePath (Join-Path $fixtureRoot 'tests.log')
    $testExit=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    $summary=[ordered]@{purpose='Local adapter/argument harness only; no UBT/UAT/Editor execution'; fixtureDirectory=$fixtureRoot; powershellVersion=$PSVersionTable.PSVersion.ToString(); assembly=$assembly; assemblySha256=(Get-FileHash -LiteralPath $assembly -Algorithm SHA256).Hash.ToLowerInvariant(); testExit=$testExit}
    [IO.File]::WriteAllText((Join-Path $fixtureRoot 'summary.json'),($summary|ConvertTo-Json))
    Write-Host "AUTOMATION_EXTENSION_FIXTURE $fixtureRoot"
    if($testExit -ne 0){exit $testExit}
}
finally{foreach($key in $prior.Keys){[Environment]::SetEnvironmentVariable($key,$prior[$key],'Process')}}

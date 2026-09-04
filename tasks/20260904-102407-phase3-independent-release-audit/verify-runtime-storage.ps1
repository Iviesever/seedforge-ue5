[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$projectRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/BuildEnvironment.ps1')
$arguments=@(Get-SeedForgeRuntimeArguments -ProjectRoot $projectRoot)
$expected=@('-DDC=SeedForgeLocal','-DDC-NoDefaultGraph',('-LocalDataCachePath='+(Join-Path $projectRoot '.cache/DerivedDataCache')))
if(($arguments -join '|') -cne ($expected -join '|')){throw 'Runtime cache argument contract failed.'}
$source=Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Config/DefaultEngine.ini')
if($source -notmatch '(?m)^SeedForgeLocal=\(ProjectPak,InstalledProjectPak,EnginePak=InstalledEnginePak,Local=SeedForgeLocalStore\)\s*$'){throw 'Non-Zen named graph is absent or changed.'}
if($source -notmatch '(?m)^SeedForgeLocalStore=\(Type=FileSystem,Path="%GAMEDIR%\.cache/DerivedDataCache",CommandLineOverride=LocalDataCachePath,UnusedFileAge=34,PromptIfMissing=true\)\s*$'){throw 'Explicit local filesystem store contract failed.'}
$launchers=@('Test','Smoke','Report','CreateDemoMap','CaptureDemo','CaptureInspector','TestGameplay','TestRunFailure','TestInputSelfTest','PackageDemo')
foreach($launcher in $launchers){$body=Get-Content -Raw -LiteralPath (Join-Path $projectRoot "Scripts/$launcher.ps1");if($body -notmatch 'Get-SeedForgeRuntimeArguments -ProjectRoot \$projectRoot'){throw "$launcher omits runtime storage arguments."}}
foreach($launcher in $launchers){$body=Get-Content -Raw -LiteralPath (Join-Path $projectRoot "Scripts/$launcher.ps1");if($body -notmatch 'Assert-SeedForgeRuntimeStorage -Path'){throw "$launcher omits post-process runtime storage validation."}}
$package=Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Scripts/PackageDemo.ps1')
if($package -notmatch 'SkipZenStore' -or $package -notmatch 'AdditionalCookerOptions'){throw 'Cooker does not select the confined non-Zen store.'}
if($package -notmatch [regex]::Escape('-ini:Editor:[EditorDomain]:CookAttachmentsEnabled=False')){throw 'Cooker must disable the independent EditorDomain Zen attachment client.'}
if($package -notmatch 'AdditionalPakOptions' -or $package -match 'AdditionalIoStoreOptions'){throw 'Common storage arguments must use AdditionalPakOptions exactly once; UE 5.8 IoStore inherits them.'}
Write-Host 'Runtime storage contract passed: local graph/store, explicit paths, no fallback, 10 launchers and cooker.'

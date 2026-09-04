[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$descriptor=Get-Content -Raw -LiteralPath (Join-Path $root 'SeedForge.uproject') | ConvertFrom-Json
$plugins=@($descriptor.Plugins)
foreach($name in @('SeedForge','PythonScriptPlugin','Fab','Bridge','MetaHumanSDK')){
    $matches=@($plugins|Where-Object {$_.Name -ceq $name})
    if($matches.Count -ne 1){throw "Project must state exactly one '$name' plugin policy."}
    $expected=$name -in @('SeedForge','PythonScriptPlugin')
    if($matches[0].Enabled -isnot [bool] -or $matches[0].Enabled -ne $expected){throw "Project plugin '$name' enabled policy is wrong."}
}
$ownedSource=@(Get-ChildItem -LiteralPath (Join-Path $root 'Source'),(Join-Path $root 'Plugins/SeedForge/Source') -File -Recurse)
foreach($file in $ownedSource){if(Select-String -LiteralPath $file.FullName -Pattern '\b(Fab|Bridge|MetaHuman|EOSShared|OnlineSubsystemEOS|OnlineServicesEOS)\b' -Quiet){throw "Owned gameplay/source unexpectedly depends on disabled online/editor content tooling: $($file.FullName)"}}
Write-Host 'Project plugin isolation passed: required project tooling enabled; unused Fab/MetaHuman EOS roots disabled; no owned source dependency.'

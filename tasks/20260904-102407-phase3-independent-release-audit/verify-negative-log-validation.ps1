[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$projectRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $projectRoot 'Scripts/LogValidation.ps1')
$runRoot=Join-Path $projectRoot ('Artifacts/Reports/NegativeLogValidation/'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $runRoot | Out-Null
$specs=@{
    Grid=@('GenerationFailed','GenerationFailed','Generation code=1: Grid dimensions must be in [4, 2048]; received 0x48.')
    Encounter=@('EncounterFailed','EncounterFailed','Encounter planning code=1: Encounter planning requires an intact layout hash and distinct walkable entrance/exit cells.')
    CapturePath=@('SmokeFailed','CaptureFailed','Screenshot processing completed without a matching fresh saved PNG.')
    RenderUnavailable=@('SmokeFailed','CaptureFailed','Owned gameplay capture exceeded its eight-second callback budget.')
}
$results=[Collections.Generic.List[object]]::new()
foreach($case in $specs.Keys) {
    foreach($mutation in @('valid','missing-run','missing-smoke','duplicate','unrelated-error','unexpected-warning','false-success','wrong-message','positive-mode')) {
        $spec=$specs[$case]
        $lines=@("LogSeedForge: Error: Gameplay run failed code=$($spec[0]) message=$($spec[2])","LogSeedForge: Error: SEEDFORGE_GAMEPLAY_SMOKE_FAILURE code=$($spec[1]) message=$($spec[2])")
        switch($mutation) {
            'missing-run' {$lines=@($lines[1])}
            'missing-smoke' {$lines=@($lines[0])}
            'duplicate' {$lines+= $lines[1]}
            'unrelated-error' {$lines+='LogClass: Error: Unexpected engine failure.'}
            'unexpected-warning' {$lines+='LogSeedForge: Warning: Unexpected regression.'}
            'false-success' {$lines+='LogSeedForge: Display: SEEDFORGE_GAMEPLAY_SMOKE_SUCCESS'}
            'wrong-message' {$lines[0]+=' changed'}
        }
        $path=Join-Path $runRoot "$case-$mutation.log"
        [IO.File]::WriteAllText($path,($lines -join "`r`n"))
        $passed=$false;$message=''
        try {
            if($mutation -eq 'positive-mode') {$proof=Assert-SeedForgeLog -Path $path}
            else {$proof=Assert-SeedForgeLog -Path $path -ExpectedFailure $case}
            if($mutation -eq 'valid') {
                $passed=$proof.ErrorCount -eq 2 -and $proof.ExpectedErrorCount -eq 2 -and $proof.UnexpectedErrorCount -eq 0 -and $proof.Verdict -ceq 'ExpectedFailure'
            }
            $message='Returned without rejection.'
        }
        catch {$message=$_.Exception.Message;$passed=$mutation -ne 'valid' -and $message -notmatch 'parameter cannot be found|not recognized'}
        $results.Add([pscustomobject]@{case=$case;mutation=$mutation;passed=$passed;message=$message})
        Write-Host "$(if($passed){'PASS'}else{'FAIL'}) $case $mutation"
    }
}
$failed=@($results|Where-Object {-not $_.passed}).Count
[IO.File]::WriteAllText((Join-Path $runRoot 'summary.json'),([ordered]@{total=$results.Count;failed=$failed;cases=$results}|ConvertTo-Json -Depth 5))
Write-Host "NEGATIVE_LOG_VALIDATION total=$($results.Count) failed=$failed output=$runRoot"
if($failed){exit 1}

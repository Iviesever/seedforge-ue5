[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../../Scripts/GameplayPathValidation.ps1')
. (Join-Path $PSScriptRoot 'input-selftest-synthetic-fixture.ps1')
$cases=New-Object 'Collections.Generic.List[object]'
function Case([string]$Name,[scriptblock]$Mutate,[bool]$Accept=$false){$cases.Add([pscustomobject]@{Name=$Name;Mutate=$Mutate;Accept=$Accept})}
Case 'complete synthetic path before first capture' {} $true
Case 'first capture in same frame as last move' {$t.captures[0].requestedFrame='15'} $true
Case 'first/latest samples aggregate more than two real moves' {$p=$t.pathEvidence;$p.movementSamples[0].to.y=995;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=987;$p.movementSamples[1].to.y=977;$p.observedMoveCount='3';$p.totalDistance=23;$p.totalDeltaSeconds=0.15} $true
Case 'off-center start and small retained samples' {$p=$t.pathEvidence;$p.movementSamples[0].from.x=9003;$p.movementSamples[0].from.y=990;$p.movementSamples[0].to.x=9002.985;$p.movementSamples[0].to.y=989.05;$p.movementSamples[0].deltaSeconds=0.004;$p.movementSamples[1].sequence='23';$p.movementSamples[1].from.x=9002.7;$p.movementSamples[1].from.y=971;$p.movementSamples[1].to.x=9002.689;$p.movementSamples[1].to.y=970.3033333333333;$p.movementSamples[1].deltaSeconds=0.003;$p.observedMoveCount='23';$p.totalDistance=20;$p.totalDeltaSeconds=0.08} $true
Case 'missing proof' {$t.PSObject.Properties.Remove('pathEvidence')}
Case 'missing canonical cells' {$t.PSObject.Properties.Remove('walkableCells')}
Case 'wrong schema' {$t.schema='seedforge.input-selftest'}
Case 'string schema version' {$t.schemaVersion='1'}
Case 'failed trace' {$t.result='Failed'}
Case 'nonempty failure code' {$t.failureCode='None'}
Case 'nonempty failure message' {$t.failureMessage='bad'}
Case 'wrong default golden layout' {$t.layoutHash='1'}
Case 'wrong default golden encounter' {$t.encounterHash='1'}
Case 'zero top generation' {$t.runGeneration='0'}
Case 'numeric request' {$t.appliedRequestId=11}
Case 'overflow request' {$t.appliedRequestId='18446744073709551616'}
Case 'noncanonical unsigned string' {$t.runGeneration='07'}
Case 'uint newline suffix' {$t.runGeneration="7`n"}
Case 'passive delegate leak' {$t.remainingPathDelegateBindings=1}
Case 'string passive count' {$t.remainingPathDelegateBindings='0'}
Case 'unknown enemy ID' {$t.pathEvidence.stableId=5}
Case 'string stable ID' {$t.pathEvidence.stableId='0'}
Case 'stale path generation' {$t.pathEvidence.runGeneration='6'}
Case 'stale path request' {$t.pathEvidence.sourceRequestId='10'}
Case 'zero path revision' {$t.pathEvidence.pathRevision='0'}
Case 'path status not success' {$t.pathEvidence.status='AlreadyAtGoal'}
Case 'string complete' {$t.pathEvidence.complete='true'}
Case 'incomplete path' {$t.pathEvidence.complete=$false}
Case 'zero expanded nodes' {$t.pathEvidence.expandedNodes=0}
Case 'over expansion budget' {$t.pathEvidence.expandedNodes=1025}
Case 'empty path' {$t.pathEvidence.cells=@()}
Case 'oversized path' {$t.pathEvidence.cells=@($t.pathEvidence.cells[0])*1026}
Case 'empty walkable' {$t.walkableCells=@()}
Case 'too many walkable' {$t.walkableCells=@($t.walkableCells[0])*4097}
Case 'duplicate walkable' {$t.walkableCells+=@($t.walkableCells[0])}
Case 'noncanonical walkable order' {$t.walkableCells=@($t.walkableCells[-1])+$t.walkableCells[0..($t.walkableCells.Count-2)]}
Case 'cell integer overflow' {$t.pathEvidence.cells[0].x=2147483648}
Case 'fractional cell' {$t.pathEvidence.cells[0].x=45.5}
Case 'off-walkable path' {$t.pathEvidence.cells[0].x=-100}
Case 'start differs from route' {$t.pathEvidence.start.x=44}
Case 'goal differs from route' {$t.pathEvidence.goal.x=44}
Case 'noncardinal route' {$t.pathEvidence.cells[1].x=46;$t.pathEvidence.goal.x=46;$t.pathEvidence.waypoints[0].x=9200}
Case 'missing waypoint' {$t.pathEvidence.waypoints=@()}
Case 'wrong waypoint conversion' {$t.pathEvidence.waypoints[0].z=59}
Case 'missing move' {$t.pathEvidence.movementSamples=@($t.pathEvidence.movementSamples[0])}
Case 'extra move retained' {$t.pathEvidence.movementSamples+=@($t.pathEvidence.movementSamples[1])}
Case 'stale move revision' {$t.pathEvidence.movementSamples[1].pathRevision='2'}
Case 'same movement sequence' {$t.pathEvidence.movementSamples[1].sequence='1'}
Case 'same movement frame' {$t.pathEvidence.movementSamples[1].frame='14'}
Case 'movement after first capture' {$t.captures[0].requestedFrame='14'}
Case 'wrong consumed target' {$t.pathEvidence.movementSamples[0].target.x+=1}
Case 'waypoint index outside route' {$t.pathEvidence.movementSamples[0].waypointIndex=1}
Case 'first sample starts away from route start' {$t.pathEvidence.movementSamples[0].from.y=800}
Case 'off-plane motion' {$t.pathEvidence.movementSamples[1].to.z+=1}
Case 'backwards motion' {$t.pathEvidence.movementSamples[1].to.y=1000}
Case 'off-segment motion' {$t.pathEvidence.movementSamples[1].to.x+=3}
Case 'overspeed motion' {$t.pathEvidence.movementSamples[1].deltaSeconds=0.001}
Case 'zero delta' {$t.pathEvidence.movementSamples[1].deltaSeconds=0}
Case 'NaN position' {$t.pathEvidence.movementSamples[1].to.x=[double]::NaN}
Case 'infinite distance' {$t.pathEvidence.totalDistance=[double]::PositiveInfinity}
Case 'finite aggregate overflow' {$t.pathEvidence.totalDeltaSeconds=1e308}
Case 'finite sample overflow' {$t.pathEvidence.movementSamples[0].deltaSeconds=1e308}
Case 'insufficient displacement' {$t.pathEvidence.totalDistance=19}
Case 'zero total time' {$t.pathEvidence.totalDeltaSeconds=0}
Case 'impossible aggregate count' {$t.pathEvidence.observedMoveCount='18446744073709551615'}
Case 'two retained moves not contiguous' {$t.pathEvidence.movementSamples[1].from.y=989}
Case 'count two advances waypoint before arrival' {$p=$t.pathEvidence;$p.cells+=@([pscustomobject]@{x=45;y=3});$p.goal=[pscustomobject]@{x=45;y=3};$p.waypoints+=@([pscustomobject]@{x=9000;y=600;z=58});$p.movementSamples[1].waypointIndex=1;$p.movementSamples[1].target.y=600}
Case 'count two skips a waypoint' {$p=$t.pathEvidence;$p.cells+=@([pscustomobject]@{x=45;y=3},[pscustomobject]@{x=45;y=2});$p.goal=[pscustomobject]@{x=45;y=2};$p.waypoints+=@([pscustomobject]@{x=9000;y=600;z=58},[pscustomobject]@{x=9000;y=400;z=58});$p.movementSamples[1].waypointIndex=2;$p.movementSamples[1].target.y=400}
Case 'count two advances after actual waypoint arrival' {$p=$t.pathEvidence;$p.cells+=@([pscustomobject]@{x=45;y=3});$p.goal=[pscustomobject]@{x=45;y=3};$p.waypoints+=@([pscustomobject]@{x=9000;y=600;z=58});$p.movementSamples[0].from.y=900.05;$p.movementSamples[0].to.y=800.05;$p.movementSamples[0].deltaSeconds=0.5;$p.movementSamples[1].from.y=800.05;$p.movementSamples[1].to.y=790.05;$p.movementSamples[1].target.y=600;$p.movementSamples[1].waypointIndex=1;$p.totalDistance=110;$p.totalDeltaSeconds=0.55} $true
Case 'count two total disagrees' {$t.pathEvidence.totalDistance=21}
Case 'continued past completion threshold' {$p=$t.pathEvidence;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=980;$p.movementSamples[1].to.y=970;$p.observedMoveCount='3';$p.totalDistance=30;$p.totalDeltaSeconds=0.15}
Case 'aggregate hides impossible gap distance' {$p=$t.pathEvidence;$p.movementSamples[0].to.y=995;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=850;$p.movementSamples[1].to.y=840;$p.observedMoveCount='3';$p.totalDistance=20;$p.totalDeltaSeconds=0.15}
Case 'aggregate hides impossible gap time' {$p=$t.pathEvidence;$p.movementSamples[0].to.y=995;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=987;$p.movementSamples[1].to.y=977;$p.observedMoveCount='3';$p.totalDistance=23;$p.totalDeltaSeconds=0.100001}
Case 'missing first receipt' {$t.captures=@($t.captures[1],$t.captures[2])}
Case 'receipt label order' {$t.captures[0].label='combat'}
Case 'receipt request mismatch' {$t.captures[1].sourceRequestId='12'}
Case 'receipt generation mismatch' {$t.captures[2].runGeneration='8'}
Case 'numeric requested frame' {$t.captures[0].requestedFrame=20}
$results=@()
foreach($case in $cases){
    $inputTrace=New-SyntheticInputSelfTest
    $t=[pscustomobject]@{schema='seedforge.gameplay-smoke';schemaVersion=1;result='Passed';failureCode='';failureMessage='';seed='24301';layoutHash=$inputTrace.initial.layoutHash;encounterHash=$inputTrace.initial.encounterHash;runGeneration='7';appliedRequestId='11';actorCounts=[pscustomobject]@{enemies=5};remainingPathDelegateBindings=0;walkableCells=$inputTrace.walkableCells;pathEvidence=$inputTrace.pathEvidence;captures=@()}
    foreach($label in @('start','combat','win')){$t.captures+=@([pscustomobject]@{label=$label;runGeneration='7';sourceRequestId='11';requestedFrame='20'})}
    & $case.Mutate
    $accepted=$false;$message=''
    try{$proof=Assert-SeedForgeGameplayPath -Trace $t;$accepted=$true;if(-not $proof.Validated){throw 'Missing compact proof.'}}catch{$message=$_.Exception.Message}
    $pass=($accepted -eq $case.Accept) -and (-not $case.Accept -or $message -eq '')
    $results+=@([pscustomobject]@{Name=$case.Name;Passed=$pass;Accepted=$accepted;ExpectedAccept=$case.Accept;Message=$message})
    if(-not $pass){Write-Host "FAIL $($case.Name): $message"}
}
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $root ('Artifacts/Reports/GameplayPathValidation/'+[datetime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $out | Out-Null
$failed=@($results|Where-Object {-not $_.Passed}).Count
$summary=[ordered]@{SyntheticOnly=$true;PowerShell=$PSVersionTable.PSVersion.ToString();Passed=$results.Count-$failed;Failed=$failed;Total=$results.Count;Cases=$results}
[IO.File]::WriteAllText((Join-Path $out 'summary.json'),($summary|ConvertTo-Json -Depth 8))
Write-Host "$($summary.Passed)/$($summary.Total) passed; $failed failed; $out"
if($failed){exit 1}

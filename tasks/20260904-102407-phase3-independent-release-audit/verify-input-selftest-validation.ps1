[CmdletBinding()]
param([string]$Label='check')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../../Scripts/InputSelfTestValidation.ps1')
. (Join-Path $PSScriptRoot 'input-selftest-synthetic-fixture.ps1')
$cases=New-Object System.Collections.Generic.List[object]
function Case([string]$Name,[scriptblock]$Mutate,[bool]$Accept=$false){$cases.Add([pscustomobject]@{Name=$Name;Mutate=$Mutate;Accept=$Accept})}
Case 'complete synthetic shape (not runtime evidence)' {} $true
Case 'diagnostic remains diagnostic' {$t.sourceIdentity='diagnostic-'+$t.sourceRevision;$t.sourceKind='diagnostic';$argsForCall.SourceIdentity=$t.sourceIdentity} $true
Case 'opaque actor paths are not files' {$old=$t.runs[0].actors[0].key;$new='/Game/Maps/Demo.Demo:PersistentLevel.Player#17';$t.runs[0].actors[0].key=$new;foreach($e in $t.inputEvents){if($e.actorKey -ceq $old){$e.actorKey=$new}};foreach($s in $t.setups){if($s.actorKey -ceq $old){$s.actorKey=$new}}} $true
Case 'aggregate retains first and latest, not adjacent samples' {$p=$t.pathEvidence;$p.movementSamples[0].to.y=995;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=987;$p.movementSamples[1].to.y=977;$p.observedMoveCount='3';$p.totalDistance=23;$p.totalDeltaSeconds=0.15} $true
Case 'aim begins already pointing at first target' {$t.inputEvents[0].before.x=1;$t.inputEvents[0].before.y=0} $true
Case 'fractional UTC width and DateTimeOffset process arguments' {$t.startedAtUtc='2026-09-04T00:00:00.1Z';$argsForCall.ProcessStartedAtUtc=[DateTimeOffset]'2026-09-04T00:00:00Z';$argsForCall.ProcessEndedAtUtc=[DateTimeOffset]'2026-09-04T00:00:13Z'} $true
Case 'small retained samples with off-center first position' {$p=$t.pathEvidence;$p.movementSamples[0].from.x=9003;$p.movementSamples[0].from.y=990;$p.movementSamples[0].to.x=9002.985;$p.movementSamples[0].to.y=989.05;$p.movementSamples[0].deltaSeconds=0.004;$p.movementSamples[1].sequence='23';$p.movementSamples[1].from.x=9002.7;$p.movementSamples[1].from.y=971;$p.movementSamples[1].to.x=9002.689;$p.movementSamples[1].to.y=970.3033333333333;$p.movementSamples[1].deltaSeconds=0.003;$p.observedMoveCount='23';$p.totalDistance=20;$p.totalDeltaSeconds=0.08} $true
Case 'enum is not a JSON string' {$t.inputEvents[0].action=@('AimX')}
Case 'schema is not a JSON string' {$t.schema=@('seedforge.input-selftest')}
Case 'mode is not a JSON string' {$t.mode=@('ordinary')}
Case 'source kind is not a JSON string' {$t.sourceKind=@('clean')}
Case 'path status is not a JSON string' {$t.pathEvidence.status=@('Success')}
Case 'transition source is not a JSON string' {$t.transitions[0].from=@('Playing')}
Case 'input effects overlap next injection' {$t.inputEvents[0].effectFrame='24';$t.inputEvents[0].effectAtUtc='2026-09-04T00:00:02.400Z'}
Case 'setup after all associated measurements' {$t.setups[0].frame='119';$t.setups[0].atUtc='2026-09-04T00:00:11.900Z'}
Case 'event loss time precedes dash' {$t.transitions[0].atUtc='2026-09-04T00:00:05.800Z'}
Case 'run time after input begins' {$t.runs[0].atUtc='2026-09-04T00:00:02.100Z'}
Case 'superseded rapid request applied' {$t.runs[3].snapshot.appliedRequestId='44';$t.final.appliedRequestId='44';$t.transitions[9].snapshot.appliedRequestId='44'}
Case 'same-seed cell changed' {$t.runs[1].actors[2].cell.x=44}
Case 'actor aliases controller' {$t.runs[0].actors[0].key=$t.runs[0].controllerKey}
Case 'persistent actors alias' {$t.runs[0].hudKey=$t.runs[0].controllerKey}
Case 'setup label absent' {$t.setups[0].label=''}
Case 'path first move starts wrong cell' {$t.pathEvidence.movementSamples[0].from.y=800;$t.pathEvidence.movementSamples[0].to.y=810}
Case 'path from not finite' {$t.pathEvidence.movementSamples[0].from.x=[double]::NaN}
Case 'path aggregate skips sequence capacity' {$t.pathEvidence.observedMoveCount='18446744073709551615'}
Case 'path retained samples noncontiguous at count two' {$t.pathEvidence.movementSamples[1].from.y=980;$t.pathEvidence.movementSamples[1].to.y=970}
Case 'path count two advances before waypoint arrival' {$p=$t.pathEvidence;$p.cells+=@([pscustomobject]@{x=45;y=3});$p.goal=[pscustomobject]@{x=45;y=3};$p.waypoints+=@([pscustomobject]@{x=9000;y=600;z=58});$p.movementSamples[1].waypointIndex=1;$p.movementSamples[1].target.y=600}
Case 'path count two skips waypoint' {$p=$t.pathEvidence;$p.cells+=@([pscustomobject]@{x=45;y=3},[pscustomobject]@{x=45;y=2});$p.goal=[pscustomobject]@{x=45;y=2};$p.waypoints+=@([pscustomobject]@{x=9000;y=600;z=58},[pscustomobject]@{x=9000;y=400;z=58});$p.movementSamples[1].waypointIndex=2;$p.movementSamples[1].target.y=400}
Case 'path count two advances after arrival' {$p=$t.pathEvidence;$p.cells+=@([pscustomobject]@{x=45;y=3});$p.goal=[pscustomobject]@{x=45;y=3};$p.waypoints+=@([pscustomobject]@{x=9000;y=600;z=58});$p.movementSamples[0].from.y=900.05;$p.movementSamples[0].to.y=800.05;$p.movementSamples[0].deltaSeconds=0.5;$p.movementSamples[1].from.y=800.05;$p.movementSamples[1].to.y=790.05;$p.movementSamples[1].target.y=600;$p.movementSamples[1].waypointIndex=1;$p.totalDistance=110;$p.totalDeltaSeconds=0.55} $true
Case 'canonical uint rejects trailing newline' {$t.inputEvents[0].injectedFrame="20`n"}
Case 'field spelling is case sensitive' {$t.PSObject.Properties.Remove('schema');$t | Add-Member NoteProperty Schema 'seedforge.input-selftest'}
Case 'path continued accumulating after completion' {$p=$t.pathEvidence;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=980;$p.movementSamples[1].to.y=970;$p.observedMoveCount='3';$p.totalDistance=30;$p.totalDeltaSeconds=0.15}
Case 'path aggregate hides impossible gap distance' {$p=$t.pathEvidence;$p.movementSamples[0].to.y=995;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=850;$p.movementSamples[1].to.y=840;$p.observedMoveCount='3';$p.totalDistance=20;$p.totalDeltaSeconds=0.15}
Case 'path aggregate hides impossible gap time' {$p=$t.pathEvidence;$p.movementSamples[0].to.y=995;$p.movementSamples[1].sequence='3';$p.movementSamples[1].from.y=987;$p.movementSamples[1].to.y=977;$p.observedMoveCount='3';$p.totalDistance=23;$p.totalDeltaSeconds=0.100001}
Case 'native normalizes revision but retains input identity case' {$t.sourceIdentity=$t.sourceIdentity.ToUpperInvariant();$argsForCall.SourceIdentity=$t.sourceIdentity} $true
Case 'dash finite inputs overflow derived motion' {$t.inputEvents[8].after.x=1e308;$t.inputEvents[8].after.y=1e308}
Case 'dash excessive motion' {$t.inputEvents[8].after.x=9300;$t.inputEvents[8].after.y=1100}
Case 'dash insufficient motion' {$t.inputEvents[8].after.x=9002;$t.inputEvents[8].after.y=802}
Case 'dash opposite direction' {$t.inputEvents[8].after.x=8980;$t.inputEvents[8].after.y=780}
Case 'dash off walkable' {$t.inputEvents[8].before.x=-1000;$t.inputEvents[8].before.y=-1000;$t.inputEvents[8].after.x=-980;$t.inputEvents[8].after.y=-980}
Case 'path finite delta overflows derived speed bound' {$p=$t.pathEvidence;$p.movementSamples[1].sequence='3';$p.observedMoveCount='3';$p.totalDeltaSeconds=1e308}
Case 'path finite sample delta overflows derived speed bound' {$t.pathEvidence.movementSamples[0].deltaSeconds=1e308;$t.pathEvidence.totalDeltaSeconds=1e308}
Case 'wrong engine minor version' {$t.engineVersion='5.7.0-actual-looking'}
Case 'malformed engine version' {$t.engineVersion='arbitrary'}
Case 'non-numeric engine patch version' {$t.engineVersion='5.8.X-actual-looking'}
Case 'earlier setup frame shares rounded injection UTC' {$t.setups[0].atUtc=$t.inputEvents[2].injectedAtUtc} $true
Case 'wrong schema' {$t.schema='seedforge.gameplay-smoke'}
Case 'schema version string' {$t.schemaVersion='1'}
Case 'false string' {$t.sourceVerified='false'}
Case 'native claims source verified' {$t.sourceVerified=$true}
Case 'diagnostic laundered clean' {$t.sourceIdentity='diagnostic-'+$t.sourceRevision;$argsForCall.SourceIdentity=$t.sourceIdentity}
Case 'source mismatch' {$t.sourceRevision='0000000000000000000000000000000000000000'}
Case 'source input mismatch' {$argsForCall.SourceIdentity='0000000000000000000000000000000000000000'}
Case 'smoke enabled' {$t.gameplaySmokeEnabled=$true}
Case 'failed envelope' {$t.result='Failed'}
Case 'nonempty success failure' {$t.failureCode='None'}
Case 'failure message retained' {$t.failureMessage='failed'}
Case 'numeric seed loses precision' {$t.expectedInitialSeed=24301}
Case 'noncanonical uint' {$t.inputEvents[0].injectedFrame='020'}
Case 'uint overflow' {$t.pathEvidence.pathRevision='18446744073709551616'}
Case 'negative uint' {$t.runs[0].snapshot.appliedRequestId='-1'}
Case 'bad UTC offset' {$t.startedAtUtc='2026-09-04T00:00:00.100+00:00'}
Case 'bad calendar date' {$t.startedAtUtc='2026-02-30T00:00:00Z'}
Case 'outside process interval' {$argsForCall.ProcessStartedAtUtc='2026-09-04T00:00:01Z'}
Case 'long watchdog' {$t.completedAtUtc='2026-09-04T00:00:31Z';$argsForCall.ProcessEndedAtUtc='2026-09-04T00:00:32Z'}
Case 'completed before started frame' {$t.completedFrame='0'}
Case 'delegate leak' {$t.remainingDelegateBindings=1}
Case 'pressed key leak' {$t.remainingPressedKeys=1}
Case 'double completion' {$t.completionCount=2}
Case 'array masquerades object' {$t.runs=$t.runs[0]}
Case 'missing run' {$t.runs=@($t.runs[0..2])}
Case 'run label' {$t.runs[1].label='new'}
Case 'initial snapshot mismatch' {$t.initial.playerHealth=99}
Case 'final snapshot mismatch' {$t.final.exitUnlocked=$true}
Case 'wrong next seed' {$t.runs[2].snapshot.seed='24301'}
Case 'wrong rapid offset' {$t.runs[3].snapshot.runGeneration='10'}
Case 'wrong same hash' {$t.runs[1].snapshot.layoutHash='1'}
Case 'wrong golden hash' {$t.initial.layoutHash='1';$t.runs[0].snapshot.layoutHash='1';$t.runs[1].snapshot.layoutHash='1'}
Case 'zero fresh identity' {$t.runs[2].snapshot.encounterHash='0'}
Case 'old pending state' {$t.runs[1].snapshot.pendingRequestId='22'}
Case 'not full health' {$t.runs[1].snapshot.playerHealth=99}
Case 'core reset failed' {$t.runs[1].snapshot.collectedCoreCount=1}
Case 'timer reset failed' {$t.runs[1].resources.repathTimerActive=$false}
Case 'cooldown reset failed' {$t.runs[1].dashCooldownRemaining=0.1}
Case 'count numeric string' {$t.runs[0].hudCount='1'}
Case 'duplicate HUD' {$t.runs[1].hudOverlayCount=2}
Case 'persistent controller replaced' {$t.runs[1].controllerKey='other'}
Case 'old actors survive' {$t.runs[1].previousActorsDestroyed=$false}
Case 'actor reused' {$t.runs[1].actors[0].key=$t.runs[0].actors[0].key}
Case 'role duplicated' {$t.runs[0].actors[2].role='player'}
Case 'invalid stable ID' {$t.runs[0].actors[5].stableId=5}
Case 'wrong owner' {$t.runs[0].actors[0].ownerKey='synthetic-coordinator'}
Case 'visualization sentinel' {$t.runs[0].actors[1].cell.x=1}
Case 'actor key oversized' {$t.runs[0].actors[0].key='x'*257}
Case 'walkable duplicate' {$t.walkableCells=@($t.walkableCells)+@($t.walkableCells[0])}
Case 'walkable unsorted' {$swap=$t.walkableCells[0];$t.walkableCells[0]=$t.walkableCells[1];$t.walkableCells[1]=$swap}
Case 'cell numeric string' {$t.walkableCells[0].x='43'}
Case 'cell out of int32 range' {$t.walkableCells[0].x=2147483648}
Case 'path incomplete' {$t.pathEvidence.complete=$false}
Case 'path wrong request' {$t.pathEvidence.sourceRequestId='22'}
Case 'path stale revision' {$t.pathEvidence.movementSamples[0].pathRevision='2'}
Case 'path unknown enemy' {$t.pathEvidence.stableId=100}
Case 'path noncardinal' {$t.pathEvidence.cells[1].x=44}
Case 'path off walkable' {$t.pathEvidence.cells[0].x=-200}
Case 'path waypoint wrong' {$t.pathEvidence.waypoints[0].x+=10}
Case 'path sample target wrong' {$t.pathEvidence.movementSamples[0].target.x+=10}
Case 'path sample teleported off segment' {$t.pathEvidence.movementSamples[0].to.x+=10}
Case 'path overspeed' {$t.pathEvidence.movementSamples[0].deltaSeconds=0.001}
Case 'path zero motion' {$t.pathEvidence.movementSamples[0].to.y=1000}
Case 'path duplicate sequence' {$t.pathEvidence.movementSamples[1].sequence='1'}
Case 'path excess count' {$t.pathEvidence.observedMoveCount='3'}
Case 'path aggregate wrong' {$t.pathEvidence.totalDistance=40}
Case 'path too short' {$t.pathEvidence.totalDistance=19}
Case 'path aggregate unbounded' {$t.pathEvidence.totalDeltaSeconds=0.001}
Case 'path sample missing' {$t.pathEvidence.movementSamples=@($t.pathEvidence.movementSamples[0])}
Case 'path vector null' {$t.pathEvidence.movementSamples[0].from.x=$null}
Case 'path vector infinity' {$t.pathEvidence.movementSamples[0].from.x=[double]::PositiveInfinity}
Case 'path vector boolean' {$t.pathEvidence.movementSamples[0].from.x=$true}
Case 'path wrong consumed index' {$t.pathEvidence.movementSamples[0].waypointIndex=1}
Case 'event missing' {$t.inputEvents=@($t.inputEvents[0..11])}
Case 'wrong key' {$t.inputEvents[4].key='A'}
Case 'unconfirmed event' {$t.inputEvents[0].confirmed=$false}
Case 'event stale identity' {$t.inputEvents[0].sourceRequestId='10'}
Case 'event release absent' {$t.inputEvents[2].releaseFrame='0'}
Case 'mouse fake release' {$t.inputEvents[0].releaseFrame='21'}
Case 'event effect before injection' {$t.inputEvents[0].effectFrame='19'}
Case 'event outside envelope' {$t.inputEvents[0].effectAtUtc='2026-09-04T00:00:15Z'}
Case 'aim missing effect' {$t.inputEvents[1].after.x=1;$t.inputEvents[1].after.y=0}
Case 'aim before not normalized' {$t.inputEvents[0].before.y=-100}
Case 'attack target swapped' {$t.inputEvents[3].actorKey='synthetic-run-7-enemy-1'}
Case 'attack did not kill' {$t.inputEvents[3].targetAliveAfter=$true}
Case 'attack wrong count' {$t.inputEvents[3].liveEnemiesAfter=3}
Case 'attack cooldown not consumed' {$t.inputEvents[2].cooldownAfter=0}
Case 'attack before cooldown ready' {$t.inputEvents[3].injectedAtUtc='2026-09-04T00:00:02.500Z'}
Case 'movement wrong direction' {$t.inputEvents[4].after.x=8980}
Case 'movement too short' {$t.inputEvents[4].after.x=9005}
Case 'movement too long' {$t.inputEvents[4].after.x=9100}
Case 'movement off safe cells' {$t.inputEvents[4].before.x=-1000;$t.inputEvents[4].after.x=-980}
Case 'dash wrong launch' {$t.inputEvents[8].launchVelocity.y=0}
Case 'dash no motion' {$t.inputEvents[8].after.x=9000;$t.inputEvents[8].after.y=800}
Case 'dash missing cooldown' {$t.inputEvents[8].cooldownAfter=0}
Case 'unused field nonzero' {$t.inputEvents[4].liveEnemiesBefore=5}
Case 'restart wrong actor' {$t.inputEvents[9].actorKey='synthetic-run-7-player-0'}
Case 'restart prior identity substituted' {$t.inputEvents[9].sourceRequestId='11'}
Case 'rapid separate frame' {$t.inputEvents[12].injectedFrame='101'}
Case 'queue missing' {$t.queuedRuns=@($t.queuedRuns[0..2])}
Case 'queue bad prior state' {$t.queuedRuns[3].stateBefore='Playing'}
Case 'queue stale pending' {$t.queuedRuns[3].snapshot.pendingRequestId='44'}
Case 'queue has applied hash' {$t.queuedRuns[1].snapshot.layoutHash='1'}
Case 'queue wrong input frame' {$t.queuedRuns[0].frame='63'}
Case 'transition missing' {$t.transitions=@($t.transitions[0..8])}
Case 'transition invented state' {$t.transitions[1].from='Playing'}
Case 'transition stale pending identity' {$t.transitions[1].snapshot.pendingRequestId='22'}
Case 'lost still has health' {$t.transitions[0].snapshot.playerHealth=1}
Case 'final playing wrong queued identity' {$t.transitions[9].snapshot.appliedRequestId='44'}
Case 'setup unknown actor' {$t.setups[0].actorKey='unknown'}
Case 'setup identity mismatch' {$t.setups[0].runGeneration='8'}
Case 'setup not canonical position' {$t.setups[0].position.x+=10}
Case 'setup during attack window' {$t.setups[0].frame='24';$t.setups[0].atUtc='2026-09-04T00:00:02.400Z'}
Case 'setup during path window' {$t.setups[0].frame='14';$t.setups[0].atUtc='2026-09-04T00:00:01.400Z'}
Case 'setup outside envelope' {$t.setups[0].frame='0'}
Case 'setup cap' {$t.setups=@($t.setups[0])*17}
$results=@()
foreach($case in $cases){
    $t=New-SyntheticInputSelfTest
    $argsForCall=@{Trace=$t;SourceIdentity=$t.sourceIdentity;Seed='24301';ProcessStartedAtUtc='2026-09-04T00:00:00Z';ProcessEndedAtUtc='2026-09-04T00:00:13Z'}
    & $case.Mutate
    $accepted=$false;$message=''
    try{$proof=Assert-SeedForgeInputSelfTest @argsForCall;$accepted=$true
        if($case.Accept -and ($proof.Validated -ne $true -or $proof.SourceIdentity -cne $t.sourceIdentity)){throw 'Compact proof did not preserve identity.'}
    }catch{$message=$_.Exception.Message}
    $passed=($accepted -eq $case.Accept) -and (-not $case.Accept -or $message -eq '')
    $results+=[pscustomobject]@{Name=$case.Name;Passed=$passed;ExpectedAccept=$case.Accept;Accepted=$accepted;Message=$message}
    if(-not $passed){Write-Host "FAIL $($case.Name): $message"}
}
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$out=Join-Path $root ('Artifacts/Reports/InputSelfTestValidation/'+[datetime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $out -Force | Out-Null
$failed=@($results | Where-Object {-not $_.Passed}).Count
$summary=[ordered]@{SyntheticOnly=$true;Label=$Label;PowerShell=$PSVersionTable.PSVersion.ToString();Passed=$results.Count-$failed;Failed=$failed;Total=$results.Count;Cases=$results}
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $out 'summary.json') -Encoding UTF8
Write-Host "$($summary.Passed)/$($summary.Total) passed; $failed failed; $out"
if($failed){exit 1}

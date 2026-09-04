# TEST-ONLY synthetic values, never runtime/release evidence. Mirrors the native
# MakeSyntheticSuccessEvidence shape and timings, not a simulation of gameplay.
function New-SyntheticInputSelfTest {
    function V([double]$x=0,[double]$y=0,[double]$z=0) { [ordered]@{x=$x;y=$y;z=$z} }
    function C($x,$y) { [ordered]@{x=$x;y=$y} }
    function T($f) { ([datetime]::SpecifyKind([datetime]'2026-09-04',[DateTimeKind]::Utc)).AddSeconds($f*0.1).ToString('yyyy-MM-ddTHH:mm:ss.fffZ') }
    function CloneSynthetic($v) { $v | ConvertTo-Json -Depth 40 -Compress | ConvertFrom-Json }
    $revision='2035e925ceb73fd2aa24de2646a0ad6197216dc8'
    $seeds=@('24301','24301','17261500834634580984','15318071228730309607')
    $runs=@(); $gens=@('7','8','9','11'); $reqs=@('11','22','33','55'); $frames=@(10,70,90,110)
    for($i=0;$i -lt 4;$i++) {
        $actors=@(); $roles=@('player','visualization','core','core','core','enemy','enemy','enemy','enemy','enemy','exit')
        $ids=@(0,0,0,1,2,0,1,2,3,4,0)
        for($a=0;$a -lt 11;$a++) {
            $cell=C 45 4; if($a -eq 1){$cell=C 0 0}; if($a -eq 5){$cell=C 45 5}
            $actors+= [ordered]@{key="synthetic-run-$($gens[$i])-$($roles[$a])-$($ids[$a])";role=$roles[$a];stableId=$ids[$a];ownerKey=$(if($a -eq 0){'synthetic-controller'}else{'synthetic-coordinator'});cell=$cell}
        }
        $snapshot=[ordered]@{seed=$seeds[$i];layoutHash=$(if($i -lt 2){'7425849530159566348'}else{"$($i+100)"});encounterHash=$(if($i -lt 2){'15303214708604970503'}else{"$($i+200)"});runGeneration=$gens[$i];pendingRequestId='0';appliedRequestId=$reqs[$i];runState='Playing';playerHealth=100;playerMaxHealth=100;collectedCoreCount=0;requiredCoreCount=3;exitUnlocked=$false;failureCode='None';failureMessage=''}
        $runs+= [ordered]@{label=@('initial','same','new','rapid')[$i];frame="$($frames[$i])";atUtc=(T $frames[$i]);snapshot=$snapshot;resources=[ordered]@{interactionTimerActive=$true;repathTimerActive=$true;attackCooldownRemaining=0};dashCooldownRemaining=0;coordinatorKey='synthetic-coordinator';controllerKey='synthetic-controller';hudKey='synthetic-hud';coordinatorCount=1;controllerCount=1;hudCount=1;hudOverlayCount=1;previousActorsDestroyed=($i -gt 0);actors=$actors}
    }
    # Old Phase2 layout is fixture input only; it supplies the canonical default
    # membership without pretending the synthetic actor/hash records are live.
    $layout=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'input-selftest-layout-synthetic-fixture.json') -Raw | ConvertFrom-Json
    $cellSet=@{}
    foreach($room in $layout.layout.rooms){for($y=$room.min[1];$y -lt $room.min[1]+$room.size[1];$y++){for($x=$room.min[0];$x -lt $room.min[0]+$room.size[0];$x++){$cellSet["$x,$y"]=C $x $y}}}
    foreach($cell in $layout.layout.corridorCells){$cellSet["$($cell[0]),$($cell[1])"]=C $cell[0] $cell[1]}
    $walkable=@($cellSet.Values | Sort-Object { $_.y },{ $_.x })
    $path=[ordered]@{stableId=0;runGeneration='7';sourceRequestId='11';pathRevision='1';start=(C 45 5);goal=(C 45 4);status='Success';expandedNodes=2;cells=@((C 45 5),(C 45 4));waypoints=@((V 9000 800 58));movementSamples=@(
        [ordered]@{sequence='1';frame='14';pathRevision='1';waypointIndex=0;from=(V 9000 1000 58);target=(V 9000 800 58);to=(V 9000 990 58);deltaSeconds=0.05},
        [ordered]@{sequence='2';frame='15';pathRevision='1';waypointIndex=0;from=(V 9000 990 58);target=(V 9000 800 58);to=(V 9000 980 58);deltaSeconds=0.05});observedMoveCount='2';totalDistance=20;totalDeltaSeconds=0.1;complete=$true}
    $actions=@('AimX','AimY','AttackFirst','AttackKill','MoveW','MoveS','MoveA','MoveD','Dash','RestartSameSeed','StartNewSeed','RapidNewSeed','RapidRestartSameSeed')
    $keys=@('MouseMove','MouseMove','LeftMouseButton','LeftMouseButton','W','S','A','D','SpaceBar','R','N','N','R');$iframes=@(20,22,24,30,40,44,48,52,58,62,80,100,100);$effects=@(21,23,25,31,41,45,49,53,59,70,90,100,110)
    $events=@()
    for($i=0;$i -lt 13;$i++) {
        $e=[ordered]@{action=$actions[$i];key=$keys[$i];actorKey=$(if($i -ge 9){'synthetic-controller'}else{'synthetic-run-7-player-0'});runGeneration=$(if($i -lt 9){'7'}else{"$($i-1)"});sourceRequestId=$(if($i -lt 9){'11'}else{"$(($i-7)*11)"});injectedFrame="$($iframes[$i])";effectFrame="$($effects[$i])";releaseFrame=$(if($i -lt 2){'0'}else{"$($iframes[$i]+1)"});injectedAtUtc=(T $iframes[$i]);effectAtUtc=(T $effects[$i]);before=(V);after=(V);launchVelocity=(V);cooldownBefore=0;cooldownAfter=0;liveEnemiesBefore=0;liveEnemiesAfter=0;targetAliveBefore=$false;targetAliveAfter=$false;confirmed=$true}
        if($i -eq 0){$e.before=V 0 -1 0;$e.after=V 1 0 0};if($i -eq 1){$e.before=V 1 0 0;$e.after=V 0 1 0}
        if($i -in 2,3){$e.actorKey='synthetic-run-7-enemy-0';$e.liveEnemiesBefore=5;$e.liveEnemiesAfter=$(if($i -eq 2){5}else{4});$e.targetAliveBefore=$true;$e.targetAliveAfter=($i -eq 2);$e.cooldownAfter=0.45}
        if($i -ge 4 -and $i -le 8){$e.before=V 9000 800 96;$e.after=V 9000 800 96}
        if($i -eq 4){$e.after.x+=20};if($i -eq 5){$e.after.x-=20};if($i -eq 6){$e.after.y-=20};if($i -eq 7){$e.after.y+=20}
        if($i -eq 8){$e.launchVelocity=V (1200/[math]::Sqrt(2)) (1200/[math]::Sqrt(2)) 0;$e.after.x+=20/[math]::Sqrt(2);$e.after.y+=20/[math]::Sqrt(2);$e.cooldownAfter=1.25}
        $events+=$e
    }
    $queues=@();$qframes=@(62,80,100,100)
    for($i=0;$i -lt 4;$i++){$s=CloneSynthetic $runs[[math]::Min($i+1,3)].snapshot;$s.runState='Generating';$s.runGeneration="$($i+8)";$s.pendingRequestId="$((2+$i)*11)";$s.appliedRequestId='0';$s.layoutHash='0';$s.encounterHash='0';$s.requiredCoreCount=0;$queues+=[ordered]@{stateBefore=@('Lost','Playing','Playing','Generating')[$i];snapshot=$s;frame="$($qframes[$i])";atUtc=(T $qframes[$i])}}
    $lost=CloneSynthetic $runs[0].snapshot;$lost.runState='Lost';$lost.playerHealth=0
    $transitions=@([ordered]@{from='Playing';to='Lost';snapshot=$lost;frame='60';atUtc=(T 60)})
    for($i=0;$i -lt 3;$i++){
        foreach($state in @('Restarting','Generating')){$s=CloneSynthetic $queues[$i].snapshot;$s.pendingRequestId='0';$s.runState=$state;if($i -eq 0){$s.playerHealth=0};$transitions+=[ordered]@{from=$(if($state -eq 'Generating'){'Restarting'}elseif($i -eq 0){'Lost'}else{'Playing'});to=$state;snapshot=$s;frame=$queues[$i].frame;atUtc=$queues[$i].atUtc}}
        $transitions+=[ordered]@{from='Generating';to='Playing';snapshot=(CloneSynthetic $runs[$i+1].snapshot);frame=$runs[$i+1].frame;atUtc=$runs[$i+1].atUtc}
    }
    $setups=@([ordered]@{label='attack-target';actorKey='synthetic-run-7-enemy-0';runGeneration='7';sourceRequestId='11';frame='23';atUtc=(T 23);cell=(C 45 5);position=(V 9000 1000 58)})
    foreach($f in @(39,43,47,51)){$setups+=[ordered]@{label='movement-pad';actorKey='synthetic-run-7-player-0';runGeneration='7';sourceRequestId='11';frame="$f";atUtc=(T $f);cell=(C 45 4);position=(V 9000 800 96)}}
    $trace=[ordered]@{schema='seedforge.input-selftest';schemaVersion=1;sourceIdentity=$revision;sourceRevision=$revision;sourceKind='clean';sourceVerified=$false;engineVersion='5.8.0-synthetic-value-fixture';mode='ordinary';gameplaySmokeEnabled=$false;expectedInitialSeed='24301';startedAtUtc=(T 1);completedAtUtc=(T 120);startedFrame='1';completedFrame='120';completionCount=1;remainingDelegateBindings=0;remainingPressedKeys=0;result='Passed';failureCode='';failureMessage='';initial=(CloneSynthetic $runs[0].snapshot);final=(CloneSynthetic $runs[3].snapshot);runs=$runs;walkableCells=$walkable;pathEvidence=$path;inputEvents=$events;queuedRuns=$queues;transitions=$transitions;setups=$setups}
    $json=$trace | ConvertTo-Json -Depth 40 -Compress
    if((Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')){return ConvertFrom-Json -InputObject $json -DateKind String}
    return ConvertFrom-Json -InputObject $json
}

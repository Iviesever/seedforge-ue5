# Native trace validation only; process/source authority belongs to the caller.
function Assert-SeedForgeInputSelfTest {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Trace, [Parameter(Mandatory)][string]$SourceIdentity,
        [Parameter(Mandatory)]$Seed, [Parameter(Mandatory)]$ProcessStartedAtUtc,
        [Parameter(Mandatory)]$ProcessEndedAtUtc)
    Set-StrictMode -Version Latest
    $ErrorActionPreference='Stop'
    Add-Type -AssemblyName System.Numerics
    function Require($Condition,[string]$Message) { if(-not $Condition){throw "InputSelfTest: $Message"} }
    function Object($Value,[string[]]$Fields) {
        Require ($null -ne $Value -and $Value -is [pscustomobject]) 'Expected a JSON object.'
        foreach($field in $Fields){$property=$Value.PSObject.Properties[$field];Require ($null -ne $property -and $property.Name -ceq $field) "Missing/miscased field $field."}
    }
    function TextValue($Value,[int]$Max=256,[bool]$Empty=$false) {
        Require ($Value -is [string] -and $Value.Length -le $Max -and ($Empty -or -not [string]::IsNullOrWhiteSpace($Value))) 'Expected a bounded JSON string.'
    }
    function Number($Value) {
        Require ($null -ne $Value -and $Value.GetType().FullName -in @('System.Int32','System.Int64','System.Double','System.Decimal','System.Single','System.UInt32','System.UInt64','System.Int16','System.Byte')) 'Expected a JSON number.'
        Require (-not [double]::IsNaN($Value) -and -not [double]::IsInfinity($Value)) 'Nonfinite number.'
    }
    function Integer($Value,$Min=0,$Max=2147483647) { Number $Value; Require ($Value -ge $Min -and $Value -le $Max -and [math]::Truncate($Value) -eq $Value) 'Integer outside bounds.' }
    function Boolean($Value) { Require ($Value -is [bool]) 'Expected a JSON boolean.' }
    function UInt($Value) {
        Require ($Value -is [string] -and $Value -cmatch '\A(0|[1-9][0-9]{0,19})\z') 'Expected canonical uint64 decimal string.'
        $parsed=[uint64]0; Require ([uint64]::TryParse($Value,[ref]$parsed)) 'uint64 overflow.'
        return [System.Numerics.BigInteger]::Parse($Value,[Globalization.CultureInfo]::InvariantCulture)
    }
    function Utc($Value) {
        Require ($Value -is [string] -and $Value -cmatch '\A[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{1,7})?Z\z') 'Expected UTC ISO-Z string.'
        $time=[DateTimeOffset]::MinValue
        Require ([DateTimeOffset]::TryParseExact($Value,[string[]]@("yyyy-MM-dd'T'HH:mm:ss'Z'","yyyy-MM-dd'T'HH:mm:ss.FFFFFFF'Z'"),[Globalization.CultureInfo]::InvariantCulture,[Globalization.DateTimeStyles]::AssumeUniversal,[ref]$time)) 'Invalid UTC date.'
        return $time
    }
    function ArrayValue($Value,$Min,$Max) { Require ($Value -is [array] -and $Value.Count -ge $Min -and $Value.Count -le $Max) 'Array size/type outside contract.' }
    function Vector($Value) { Object $Value @('x','y','z'); foreach($axis in @('x','y','z')){Number $Value.$axis} }
    function Cell($Value) { Object $Value @('x','y');Integer $Value.x -2147483648 2147483647;Integer $Value.y -2147483648 2147483647 }
    function CellKey($Value) { return "$($Value.x),$($Value.y)" }
    function Near($A,$B,$Tolerance=0.1) { return [math]::Abs($A.x-$B.x) -le $Tolerance -and [math]::Abs($A.y-$B.y) -le $Tolerance -and [math]::Abs($A.z-$B.z) -le $Tolerance }
    function Distance($A,$B,$XY=$false) { $z=0;if(-not $XY){$z=$A.z-$B.z};$distance=[math]::Sqrt([math]::Pow($A.x-$B.x,2)+[math]::Pow($A.y-$B.y,2)+$z*$z);Number $distance;return $distance }
    function OnCells($Position,$Cells,$Margin=0) { foreach($c in $Cells){if([math]::Abs($Position.x-$c.x*200.0) -le 100.1-$Margin -and [math]::Abs($Position.y-$c.y*200.0) -le 100.1-$Margin){return $true}};return $false }
    function Snapshot($S) {
        Object $S @('seed','layoutHash','encounterHash','runGeneration','pendingRequestId','appliedRequestId','runState','playerHealth','playerMaxHealth','collectedCoreCount','requiredCoreCount','exitUnlocked','failureCode','failureMessage')
        foreach($f in $uintFields){$null=UInt $S.$f}
        TextValue $S.runState;Require ($S.runState -cin @('Generating','Playing','Won','Lost','Restarting','Failed')) 'Unknown run state.'
        Number $S.playerHealth;Number $S.playerMaxHealth;Integer $S.collectedCoreCount;Integer $S.requiredCoreCount;Boolean $S.exitUnlocked
        TextValue $S.failureCode 128;TextValue $S.failureMessage 1024 $true
        Require ($S.failureCode -ceq 'None' -and $S.failureMessage -ceq '') 'Successful scenario contains failed gameplay snapshot.'
    }
    function SameSnapshot($A,$B) { foreach($f in $snapshotFields){Require ($A.$f -ceq $B.$f) "Snapshot mismatch: $f."} }
    function Stamp($Frame,$AtUtc) { $f=UInt $Frame;$time=Utc $AtUtc;Require ($f -ge $startedFrame -and $f -le $completedFrame -and $time -ge $startedAt -and $time -le $completedAt) 'Observation outside trace envelope.' }
    $uintFields=@('seed','layoutHash','encounterHash','runGeneration','pendingRequestId','appliedRequestId')
    $snapshotFields=$uintFields+@('runState','playerHealth','playerMaxHealth','collectedCoreCount','requiredCoreCount','exitUnlocked','failureCode','failureMessage')
    Object $Trace @('schema','schemaVersion','sourceIdentity','sourceRevision','sourceKind','sourceVerified','engineVersion','mode','gameplaySmokeEnabled','expectedInitialSeed','startedAtUtc','completedAtUtc','startedFrame','completedFrame','completionCount','remainingDelegateBindings','remainingPressedKeys','result','failureCode','failureMessage','initial','final','runs','walkableCells','pathEvidence','inputEvents','transitions','queuedRuns','setups')
    foreach($f in @('schema','sourceKind','mode','result')){TextValue $Trace.$f}
    Require ($Trace.schema -ceq 'seedforge.input-selftest') 'Wrong schema.';Integer $Trace.schemaVersion 1 1
    TextValue $Trace.sourceIdentity 128;TextValue $Trace.sourceRevision 40
    Require ($SourceIdentity -cmatch '\A(diagnostic-)?[a-fA-F0-9]{40}\z' -and $Trace.sourceIdentity -ceq $SourceIdentity) 'Source identity mismatch.'
    $kind='clean';$revision=$SourceIdentity;if($SourceIdentity.StartsWith('diagnostic-')){$kind='diagnostic';$revision=$SourceIdentity.Substring(11)}
    $revision=$revision.ToLowerInvariant()
    Require ($Trace.sourceKind -ceq $kind -and $Trace.sourceRevision -ceq $revision) 'Source classification mismatch.'
    Boolean $Trace.sourceVerified;Boolean $Trace.gameplaySmokeEnabled
    Require (-not $Trace.sourceVerified -and -not $Trace.gameplaySmokeEnabled -and $Trace.mode -ceq 'ordinary') 'Source authority or ordinary mode violated.'
    TextValue $Trace.engineVersion 128
    Require ($Trace.engineVersion -cmatch '\A5\.8\.[0-9]+(?:-|\z)') 'Trace was not produced by the required UE 5.8 version.'
    Require ($Trace.result -ceq 'Passed' -and $Trace.failureCode -is [string] -and $Trace.failureCode -ceq '' -and $Trace.failureMessage -is [string] -and $Trace.failureMessage -ceq '') 'Not a successful empty-failure envelope.'
    Integer $Trace.completionCount 1 1;Integer $Trace.remainingDelegateBindings 0 0;Integer $Trace.remainingPressedKeys 0 0
    $expectedSeed=UInt $Trace.expectedInitialSeed
    $seedString=[Convert]::ToString($Seed,[Globalization.CultureInfo]::InvariantCulture);Require ($expectedSeed -eq (UInt $seedString)) 'Initial seed does not match invocation.'
    $startedAt=Utc $Trace.startedAtUtc;$completedAt=Utc $Trace.completedAtUtc;$startedFrame=UInt $Trace.startedFrame;$completedFrame=UInt $Trace.completedFrame
    try{$processStart=[DateTimeOffset]$ProcessStartedAtUtc;$processEnd=[DateTimeOffset]$ProcessEndedAtUtc}catch{throw 'InputSelfTest: Invalid process interval.'}
    Require ($completedAt -ge $startedAt -and ($completedAt-$startedAt).TotalSeconds -le 30 -and $startedAt -ge $processStart -and $completedAt -le $processEnd -and $processStart -le $processEnd -and $completedFrame -ge $startedFrame) 'Invalid trace/process interval.'
    Snapshot $Trace.initial;Snapshot $Trace.final
    ArrayValue $Trace.runs 4 4;ArrayValue $Trace.walkableCells 1 4096;ArrayValue $Trace.inputEvents 13 13;ArrayValue $Trace.transitions 10 10;ArrayValue $Trace.queuedRuns 4 4;ArrayValue $Trace.setups 0 16
    SameSnapshot $Trace.initial $Trace.runs[0].snapshot;SameSnapshot $Trace.final $Trace.runs[3].snapshot
    $initialRun=UInt $Trace.initial.runGeneration;Require ($initialRun -gt 0 -and $initialRun -le [System.Numerics.BigInteger]::Parse('18446744073709551611')) 'Initial generation cannot advance four times.'
    $mod=[System.Numerics.BigInteger]::Parse('18446744073709551616');$mult=[System.Numerics.BigInteger]::Parse('6364136223846793005');$inc=[System.Numerics.BigInteger]::Parse('1442695040888963407')
    $nextSeed=($expectedSeed*$mult+$inc)%$mod;$rapidSeed=($nextSeed*$mult+$inc)%$mod;$seeds=@($expectedSeed,$expectedSeed,$nextSeed,$rapidSeed)
    $walkable=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal);$previousCell=$null
    foreach($c in $Trace.walkableCells){Cell $c;Require ($walkable.Add((CellKey $c))) 'Duplicate walkable cell.';if($null -ne $previousCell){Require ($c.y -gt $previousCell.y -or ($c.y -eq $previousCell.y -and $c.x -gt $previousCell.x)) 'Noncanonical walkable order.'};$previousCell=$c}
    $actorKeys=New-Object 'System.Collections.Generic.Dictionary[string,object]' ([StringComparer]::Ordinal)
    $persistent=$Trace.runs[0];$playerKey='';$enemyKey='';$labels=@('initial','same','new','rapid');$offsets=@(0,1,2,4);$lastFrame=$startedFrame;$lastTime=$startedAt;$lastRequest=[System.Numerics.BigInteger]::Zero
    for($i=0;$i -lt 4;$i++){
        $r=$Trace.runs[$i];Object $r @('label','frame','atUtc','snapshot','resources','dashCooldownRemaining','coordinatorKey','controllerKey','hudKey','coordinatorCount','controllerCount','hudCount','hudOverlayCount','previousActorsDestroyed','actors');Snapshot $r.snapshot;$s=$r.snapshot;Stamp $r.frame $r.atUtc
        TextValue $r.label;Require ($r.label -ceq $labels[$i] -and (UInt $r.frame) -ge $lastFrame -and (Utc $r.atUtc) -ge $lastTime) 'Run order mismatch.'
        Require ((UInt $s.seed) -eq $seeds[$i] -and (UInt $s.runGeneration) -eq $initialRun+$offsets[$i] -and (UInt $s.appliedRequestId) -gt $lastRequest -and $s.pendingRequestId -ceq '0' -and (UInt $s.layoutHash) -gt 0 -and (UInt $s.encounterHash) -gt 0) 'Run identity mismatch.'
        Require ($s.runState -ceq 'Playing' -and $s.playerHealth -eq 100 -and $s.playerMaxHealth -eq 100 -and $s.collectedCoreCount -eq 0 -and $s.requiredCoreCount -eq 3 -and -not $s.exitUnlocked) 'Fresh run reset mismatch.'
        Object $r.resources @('interactionTimerActive','repathTimerActive','attackCooldownRemaining');Boolean $r.resources.interactionTimerActive;Boolean $r.resources.repathTimerActive;Number $r.resources.attackCooldownRemaining;Number $r.dashCooldownRemaining
        Require ($r.resources.interactionTimerActive -and $r.resources.repathTimerActive -and $r.resources.attackCooldownRemaining -eq 0 -and $r.dashCooldownRemaining -eq 0) 'Fresh run resources not reset.'
        foreach($f in @('coordinatorCount','controllerCount','hudCount','hudOverlayCount')){Integer $r.$f 1 1}
        foreach($f in @('coordinatorKey','controllerKey','hudKey')){TextValue $r.$f;Require ($r.$f -ceq $persistent.$f) 'Persistent actor changed.'}
        Require ($r.coordinatorKey -cne $r.controllerKey -and $r.coordinatorKey -cne $r.hudKey -and $r.controllerKey -cne $r.hudKey) 'Persistent roles alias each other.'
        Boolean $r.previousActorsDestroyed;Require ($i -eq 0 -or $r.previousActorsDestroyed) 'Previous actors survived restart.'
        ArrayValue $r.actors 11 11;$roles=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
        foreach($a in $r.actors){Object $a @('key','role','stableId','ownerKey','cell');TextValue $a.key;TextValue $a.ownerKey;TextValue $a.role;Integer $a.stableId 0 4294967295;Cell $a.cell
            Require (-not $actorKeys.ContainsKey($a.key) -and $a.key -cnotin @($r.coordinatorKey,$r.controllerKey,$r.hudKey) -and $roles.Add("$($a.role):$($a.stableId)")) 'Duplicate actor identity/role.'
            Require (($a.role -cin @('player','visualization','exit') -and $a.stableId -eq 0) -or ($a.role -ceq 'core' -and $a.stableId -lt 3) -or ($a.role -ceq 'enemy' -and $a.stableId -lt 5)) 'Invalid actor role/stable ID.'
            $owner=$r.coordinatorKey;if($a.role -ceq 'player'){$owner=$r.controllerKey};Require ($a.ownerKey -ceq $owner) 'Wrong actor owner.'
            if($a.role -ceq 'visualization'){Require ($a.cell.x -eq 0 -and $a.cell.y -eq 0) 'Visualization sentinel mismatch.'}elseif($i -lt 2){Require ($walkable.Contains((CellKey $a.cell))) 'Initial/same actor outside walkable cells.'}
            if($i -eq 1){$same=@($persistent.actors | Where-Object {$_.role -ceq $a.role -and $_.stableId -eq $a.stableId})[0];Require ((CellKey $a.cell) -ceq (CellKey $same.cell)) 'Same-seed actor cell changed.'}
            $actorKeys.Add($a.key,[pscustomobject]@{Run=$s.runGeneration;Request=$s.appliedRequestId;Role=$a.role})
            if($i -eq 0 -and $a.role -ceq 'player'){$playerKey=$a.key};if($i -eq 0 -and $a.role -ceq 'enemy' -and $a.stableId -eq $Trace.pathEvidence.stableId){$enemyKey=$a.key}
        }
        $lastFrame=UInt $r.frame;$lastTime=Utc $r.atUtc;$lastRequest=UInt $s.appliedRequestId
    }
    foreach($f in @('layoutHash','encounterHash')){Require ($Trace.runs[0].snapshot.$f -ceq $Trace.runs[1].snapshot.$f) 'Same-seed canonical hash changed.'}
    if($expectedSeed -eq 24301){Require ($Trace.initial.layoutHash -ceq '7425849530159566348' -and $Trace.initial.encounterHash -ceq '15303214708604970503') 'Default golden identity mismatch.'}
    # The native codec independently regenerates layouts/encounters. This external
    # validator does not infer a canonical layout hash from just walkable cells.
    $p=$Trace.pathEvidence;Object $p @('stableId','runGeneration','sourceRequestId','pathRevision','start','goal','status','expandedNodes','cells','waypoints','movementSamples','observedMoveCount','totalDistance','totalDeltaSeconds','complete')
    TextValue $p.status
    Integer $p.stableId 0 4294967295;foreach($f in @('runGeneration','sourceRequestId','pathRevision','observedMoveCount')){$null=UInt $p.$f};Cell $p.start;Cell $p.goal;Integer $p.expandedNodes 1 1024;Boolean $p.complete;Number $p.totalDistance;Number $p.totalDeltaSeconds
    Require ($p.complete -and $p.status -ceq 'Success' -and $p.runGeneration -ceq $Trace.initial.runGeneration -and $p.sourceRequestId -ceq $Trace.initial.appliedRequestId -and (UInt $p.pathRevision) -gt 0 -and $enemyKey -cne '') 'Path ownership/status mismatch.'
    ArrayValue $p.cells 2 1025;ArrayValue $p.waypoints ($p.cells.Count-1) ($p.cells.Count-1);ArrayValue $p.movementSamples 2 2
    Require ((CellKey $p.start) -ceq (CellKey $p.cells[0]) -and (CellKey $p.goal) -ceq (CellKey $p.cells[-1])) 'Path endpoints mismatch.'
    $route=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    for($i=0;$i -lt $p.cells.Count;$i++){ $c=$p.cells[$i];Cell $c;Require ($walkable.Contains((CellKey $c)) -and $route.Add((CellKey $c))) 'Path is not simple/walkable.'
        if($i -gt 0){$prior=$p.cells[$i-1];Require (([math]::Abs($c.x-$prior.x)+[math]::Abs($c.y-$prior.y)) -eq 1) 'Path is not cardinal.';$w=$p.waypoints[$i-1];Vector $w;Require (Near $w ([pscustomobject]@{x=$c.x*200.0;y=$c.y*200.0;z=58})) 'Waypoint conversion mismatch.'}
    }
    $sequence=[System.Numerics.BigInteger]::Zero;$moveFrame=UInt $Trace.runs[0].frame;$lastIndex=0;$sumDistance=0.0;$sumDelta=0.0
    for($i=0;$i -lt 2;$i++){$m=$p.movementSamples[$i];Object $m @('sequence','frame','pathRevision','waypointIndex','from','target','to','deltaSeconds');$seq=UInt $m.sequence;$frame=UInt $m.frame;$null=UInt $m.pathRevision;Integer $m.waypointIndex 0 ($p.waypoints.Count-1);foreach($f in @('from','target','to')){Vector $m.$f};Number $m.deltaSeconds
        Require ($seq -gt $sequence -and $frame -ge $moveFrame -and $frame -le $completedFrame -and $m.pathRevision -ceq $p.pathRevision -and $m.waypointIndex -ge $lastIndex -and $m.deltaSeconds -gt 0) 'Invalid movement sample ordering.'
        Require (Near $m.target $p.waypoints[$m.waypointIndex]) 'Movement target not consumed waypoint.'
        Require ((OnCells $m.from $p.cells) -and (OnCells $m.to $p.cells) -and [math]::Abs($m.from.z-58) -le 0.1 -and [math]::Abs($m.to.z-58) -le 0.1) 'Movement outside path plane/cells.'
        if($i -eq 0){Require ($m.waypointIndex -eq 0 -and (OnCells $m.from @($p.start))) 'First move does not start at path start.'}
        $distance=Distance $m.from $m.to;$length=Distance $m.from $m.target
        $speedBound=260.0*$m.deltaSeconds+0.1;Number $speedBound
        Require ($distance -gt 0 -and $distance -le $speedBound -and $length -gt 0) 'Movement speed/distance invalid.'
        $dot=($m.to.x-$m.from.x)*($m.target.x-$m.from.x)+($m.to.y-$m.from.y)*($m.target.y-$m.from.y)+($m.to.z-$m.from.z)*($m.target.z-$m.from.z)
        $fraction=[math]::Max(0.0,[math]::Min(1.0,$dot/($length*$length)));$closest=[pscustomobject]@{x=$m.from.x+$fraction*($m.target.x-$m.from.x);y=$m.from.y+$fraction*($m.target.y-$m.from.y);z=$m.from.z+$fraction*($m.target.z-$m.from.z)}
        Require ($dot -gt 0 -and (Near $m.to $closest)) 'Movement is not on consumed segment.'
        $sumDistance+=$distance;$sumDelta+=$m.deltaSeconds;$sequence=$seq;$moveFrame=$frame;$lastIndex=$m.waypointIndex
    }
    $count=UInt $p.observedMoveCount;$aggregateBound=260.0*$p.totalDeltaSeconds+0.1*[double]$count;Number $aggregateBound;Number $sumDistance;Number $sumDelta
    Require ($count -ge 2 -and $count -le (UInt $p.movementSamples[1].sequence)-(UInt $p.movementSamples[0].sequence)+1 -and $p.totalDistance -ge 20 -and $p.totalDeltaSeconds -gt 0 -and $p.totalDistance -le $aggregateBound -and $p.totalDistance+0.1 -ge $sumDistance -and $p.totalDeltaSeconds+0.000001 -ge $sumDelta) 'Invalid movement aggregate.'
    if($count -eq 2){Require ((Near $p.movementSamples[1].from $p.movementSamples[0].to) -and [math]::Abs($p.totalDistance-$sumDistance) -le 0.1 -and [math]::Abs($p.totalDeltaSeconds-$sumDelta) -le 0.000001) 'Two-sample aggregate not contiguous/exact.'}
    else{Require ($p.totalDistance-(Distance $p.movementSamples[1].from $p.movementSamples[1].to) -lt 20.0) 'Path kept accumulating after its completion threshold.'}
    $queueSeeds=@($expectedSeed,$nextSeed,$rapidSeed,$rapidSeed);$priorStates=@('Lost','Playing','Playing','Generating');$lastRequest=UInt $Trace.initial.appliedRequestId;$lastFrame=$startedFrame;$lastTime=$startedAt
    for($i=0;$i -lt 4;$i++){$q=$Trace.queuedRuns[$i];Object $q @('stateBefore','snapshot','frame','atUtc');Snapshot $q.snapshot;Stamp $q.frame $q.atUtc;$s=$q.snapshot
        TextValue $q.stateBefore
        Require ($q.stateBefore -ceq $priorStates[$i] -and $s.runState -ceq 'Generating' -and (UInt $s.runGeneration) -eq $initialRun+$i+1 -and (UInt $s.seed) -eq $queueSeeds[$i] -and (UInt $s.pendingRequestId) -gt $lastRequest -and $s.appliedRequestId -ceq '0' -and $s.layoutHash -ceq '0' -and $s.encounterHash -ceq '0' -and (UInt $q.frame) -ge $lastFrame -and (Utc $q.atUtc) -ge $lastTime) 'Queued run identity/order mismatch.'
        $lastRequest=UInt $s.pendingRequestId;$lastFrame=UInt $q.frame;$lastTime=Utc $q.atUtc
    }
    foreach($pair in @(@(0,1),@(1,2),@(3,3))){Require ($Trace.queuedRuns[$pair[0]].snapshot.pendingRequestId -ceq $Trace.runs[$pair[1]].snapshot.appliedRequestId) 'Applied request not final queued request.'}
    Require ($Trace.queuedRuns[2].frame -ceq $Trace.queuedRuns[3].frame) 'Rapid queues not one input frame.'
    $from=@('Playing','Lost','Restarting','Generating','Playing','Restarting','Generating','Playing','Restarting','Generating');$to=@('Lost','Restarting','Generating','Playing','Restarting','Generating','Playing','Restarting','Generating','Playing');$lastFrame=$startedFrame;$lastTime=$startedAt
    for($i=0;$i -lt 10;$i++){$tr=$Trace.transitions[$i];Object $tr @('from','to','snapshot','frame','atUtc');Snapshot $tr.snapshot;Stamp $tr.frame $tr.atUtc
        TextValue $tr.from;TextValue $tr.to
        Require ($tr.from -ceq $from[$i] -and $tr.to -ceq $to[$i] -and $tr.snapshot.runState -ceq $tr.to -and (UInt $tr.frame) -ge $lastFrame -and (Utc $tr.atUtc) -ge $lastTime) 'Transition sequence mismatch.'
        if($i -eq 0){foreach($f in $snapshotFields){if($f -cin @('runState','playerHealth')){continue};Require ($tr.snapshot.$f -ceq $Trace.initial.$f) 'Lost identity mismatch.'};Require ($tr.snapshot.playerHealth -eq 0) 'Lost retains player health.'}
        elseif($i -in 3,6,9){$r=$Trace.runs[$i/3];SameSnapshot $tr.snapshot $r.snapshot;Require ($tr.frame -ceq $r.frame -and (Utc $tr.atUtc) -le (Utc $r.atUtc)) 'Playing snapshot not correlated with apply.'}
        else{$q=$Trace.queuedRuns[[math]::Floor(($i-1)/3)];Require ($tr.snapshot.runGeneration -ceq $q.snapshot.runGeneration -and $tr.snapshot.seed -ceq $q.snapshot.seed -and $tr.frame -ceq $q.frame -and (Utc $tr.atUtc) -le (Utc $q.atUtc)) 'Pending transition identity mismatch.';foreach($f in @('pendingRequestId','appliedRequestId','layoutHash','encounterHash')){Require ($tr.snapshot.$f -ceq '0') 'Transition contains applied/pending hash.'}}
        $lastFrame=UInt $tr.frame;$lastTime=Utc $tr.atUtc
    }
    $actions=@('AimX','AimY','AttackFirst','AttackKill','MoveW','MoveS','MoveA','MoveD','Dash','RestartSameSeed','StartNewSeed','RapidNewSeed','RapidRestartSameSeed');$keys=@('MouseMove','MouseMove','LeftMouseButton','LeftMouseButton','W','S','A','D','SpaceBar','R','N','N','R');$zero=[pscustomobject]@{x=0;y=0;z=0};$lastFrame=UInt $Trace.runs[0].frame;$lastTime=Utc $Trace.runs[0].atUtc
    for($i=0;$i -lt 13;$i++){$e=$Trace.inputEvents[$i];Object $e @('action','key','actorKey','runGeneration','sourceRequestId','injectedFrame','effectFrame','releaseFrame','injectedAtUtc','effectAtUtc','before','after','launchVelocity','cooldownBefore','cooldownAfter','liveEnemiesBefore','liveEnemiesAfter','targetAliveBefore','targetAliveAfter','confirmed')
        TextValue $e.action;TextValue $e.key
        foreach($f in @('runGeneration','sourceRequestId','injectedFrame','effectFrame','releaseFrame')){$null=UInt $e.$f};Stamp $e.injectedFrame $e.injectedAtUtc;Stamp $e.effectFrame $e.effectAtUtc;TextValue $e.actorKey;foreach($f in @('before','after','launchVelocity')){Vector $e.$f};Number $e.cooldownBefore;Number $e.cooldownAfter;Integer $e.liveEnemiesBefore 0 5;Integer $e.liveEnemiesAfter 0 5;foreach($f in @('targetAliveBefore','targetAliveAfter','confirmed')){Boolean $e.$f}
        $run=$Trace.initial.runGeneration;$request=$Trace.initial.appliedRequestId;if($i -ge 9){$run=$Trace.queuedRuns[$i-9].snapshot.runGeneration;$request=$Trace.queuedRuns[$i-9].snapshot.pendingRequestId}
        Require ($e.action -ceq $actions[$i] -and $e.key -ceq $keys[$i] -and $e.confirmed -and $e.runGeneration -ceq $run -and $e.sourceRequestId -ceq $request -and (UInt $e.injectedFrame) -ge $lastFrame -and (Utc $e.injectedAtUtc) -ge $lastTime -and (UInt $e.effectFrame) -ge (UInt $e.injectedFrame) -and (Utc $e.effectAtUtc) -ge (Utc $e.injectedAtUtc)) 'Input attribution/order mismatch.'
        if($i -lt 11){$next=$Trace.inputEvents[$i+1];Require ((UInt $e.effectFrame) -le (UInt $next.injectedFrame) -and (Utc $e.effectAtUtc) -le (Utc $next.injectedAtUtc)) 'Input measurement overlaps the next injection.'}
        if($i -lt 2){Require ($e.releaseFrame -ceq '0') 'Mouse has key release.'}else{Require ((UInt $e.releaseFrame) -gt (UInt $e.injectedFrame) -and (UInt $e.releaseFrame) -le $completedFrame) 'Key was not released.'}
        if($i -notin 2,3){Require ($e.liveEnemiesBefore -eq 0 -and $e.liveEnemiesAfter -eq 0 -and -not $e.targetAliveBefore -and -not $e.targetAliveAfter) 'Unused enemy effects are nonzero.'}
        if($i -notin 2,3,8){Require ($e.cooldownBefore -eq 0 -and $e.cooldownAfter -eq 0) 'Unused cooldown effects are nonzero.'}
        if($i -ne 8){Require (Near $e.launchVelocity $zero 0) 'Unused launch effect is nonzero.'}
        if($i -in 2,3,9,10,11,12){Require ((Near $e.before $zero 0) -and (Near $e.after $zero 0)) 'Unused position effects are nonzero.'}
        if($i -lt 2){$target=[pscustomobject]@{x=$(if($i -eq 0){1}else{0});y=$(if($i -eq 0){0}else{1});z=0};Require ($e.actorKey -ceq $playerKey -and (Near $e.after $target 0.05) -and [math]::Abs((Distance $e.before $zero $true)-1) -le 0.05 -and [math]::Abs($e.before.z) -le 0.05) 'Aim effect not normalized/requested.'}
        if($i -in 2,3){Require ($e.actorKey -ceq $enemyKey -and $e.targetAliveBefore -and $e.targetAliveAfter -eq ($i -eq 2) -and $e.liveEnemiesBefore -eq 5 -and $e.liveEnemiesAfter -eq $(if($i -eq 2){5}else{4}) -and $e.cooldownBefore -ge 0 -and $e.cooldownBefore -le 0.01 -and $e.cooldownAfter -gt 0 -and $e.cooldownAfter -le 0.47) 'Attack effect/cooldown missing.';if($i -eq 3){Require (((Utc $e.injectedAtUtc)-(Utc $Trace.inputEvents[2].injectedAtUtc)).TotalSeconds -ge 0.448) 'Attack cooldown interval not observed.'}}
        if($i -ge 4 -and $i -le 7){$distance=Distance $e.before $e.after $true;$component=($e.after.x-$e.before.x);if($i -ge 6){$component=$e.after.y-$e.before.y};if($i -in 5,6){$component=-$component};Require ($e.actorKey -ceq $playerKey -and $distance -ge 19.9 -and $distance -le 80 -and $component/$distance -ge 0.98 -and (OnCells $e.before $Trace.walkableCells 42) -and (OnCells $e.after $Trace.walkableCells 42)) 'Cardinal player movement/collision bound missing.'}
        if($i -eq 8){$launch=[pscustomobject]@{x=1200/[math]::Sqrt(2);y=1200/[math]::Sqrt(2);z=0};$distance=Distance $e.before $e.after $true
            Require ($e.actorKey -ceq $playerKey -and (Near $e.launchVelocity $launch 1) -and $distance -ge 19.9 -and $distance -le 200 -and (($e.after.x-$e.before.x)+($e.after.y-$e.before.y))/([math]::Sqrt(2)*$distance) -ge 0.98 -and (OnCells $e.before $Trace.walkableCells) -and (OnCells $e.after $Trace.walkableCells) -and $e.cooldownBefore -ge 0 -and $e.cooldownBefore -le 0.01 -and $e.cooldownAfter -gt 0 -and $e.cooldownAfter -le 1.27) 'Diagonal Dash/motion/cooldown missing.'
        }
        if($i -ge 9){$q=$Trace.queuedRuns[$i-9];Require ($e.actorKey -ceq $persistent.controllerKey -and $e.injectedFrame -ceq $q.frame -and (Utc $e.injectedAtUtc) -le (Utc $q.atUtc) -and (Utc $e.effectAtUtc) -ge (Utc $q.atUtc)) 'Restart input/queue correlation missing.'
            if($i -eq 11){Require ($e.effectFrame -ceq $e.injectedFrame) 'Rapid N effect not immediate queue.'}else{$r=$Trace.runs[$(@{9=1;10=2;12=3}[$i])];Require ($e.effectFrame -ceq $r.frame -and (Utc $e.effectAtUtc) -ge (Utc $r.atUtc)) 'Restart effect not applied snapshot.'}
        }
        $lastFrame=UInt $e.injectedFrame;$lastTime=Utc $e.injectedAtUtc
    }
    Require ($Trace.inputEvents[11].injectedFrame -ceq $Trace.inputEvents[12].injectedFrame -and (UInt $p.movementSamples[1].frame) -le (UInt $Trace.inputEvents[0].injectedFrame) -and (UInt $Trace.inputEvents[8].effectFrame) -le (UInt $Trace.transitions[0].frame) -and (UInt $Trace.transitions[0].frame) -le (UInt $Trace.inputEvents[9].injectedFrame)) 'Measurement windows or rapid batch mismatch.'
    Require ((Utc $Trace.inputEvents[8].effectAtUtc) -le (Utc $Trace.transitions[0].atUtc) -and (Utc $Trace.transitions[0].atUtc) -le (Utc $Trace.inputEvents[9].injectedAtUtc)) 'Lost time not between Dash and restart.'
    foreach($setup in $Trace.setups){Object $setup @('label','actorKey','runGeneration','sourceRequestId','frame','atUtc','cell','position');TextValue $setup.label;TextValue $setup.actorKey;Stamp $setup.frame $setup.atUtc;$null=UInt $setup.runGeneration;$null=UInt $setup.sourceRequestId;Cell $setup.cell;Vector $setup.position
        Require ($actorKeys.ContainsKey($setup.actorKey)) 'Setup actor is unknown.';$owner=$actorKeys[$setup.actorKey]
        Require ($setup.runGeneration -ceq $owner.Run -and $setup.sourceRequestId -ceq $owner.Request -and $setup.runGeneration -ceq $Trace.initial.runGeneration -and $walkable.Contains((CellKey $setup.cell)) -and [math]::Abs($setup.position.x-$setup.cell.x*200.0) -le 0.1 -and [math]::Abs($setup.position.y-$setup.cell.y*200.0) -le 0.1) 'Setup identity/canonical position mismatch.'
        $hasMeasurement=$false;$precedesMeasurement=$false
        foreach($e in $Trace.inputEvents){if($e.actorKey -ceq $setup.actorKey){$hasMeasurement=$true
                # Millisecond ISO serialization can share a UTC value across
                # frames. Strict frame separation plus UTC consistency proves
                # ordering without inventing finer timestamp precision.
                $before=(UInt $setup.frame) -lt (UInt $e.injectedFrame) -and (Utc $setup.atUtc) -le (Utc $e.injectedAtUtc)
                $after=(UInt $setup.frame) -gt (UInt $e.effectFrame) -and (Utc $setup.atUtc) -ge (Utc $e.effectAtUtc)
                Require ($before -or $after) 'Setup overlaps measured input.'
                if($before){$precedesMeasurement=$true}}
        }
        Require (-not $hasMeasurement -or $precedesMeasurement) 'Setup does not precede an affected measurement.'
        if($setup.actorKey -ceq $enemyKey){Require (-not ((UInt $setup.frame) -ge (UInt $p.movementSamples[0].frame) -and (UInt $setup.frame) -le (UInt $p.movementSamples[1].frame))) 'Setup overlaps enemy path proof.'}
    }
    return [pscustomobject]@{Validated=$true;SourceIdentity=$SourceIdentity;SourceRevision=$revision;SourceKind=$kind;SourceVerified=$false;RunCount=4;InputEventCount=13;TransitionCount=10;QueuedRunCount=4;InitialSeed=$Trace.initial.seed;FinalSeed=$Trace.final.seed;InitialRunGeneration=$Trace.initial.runGeneration;FinalRunGeneration=$Trace.final.runGeneration;PathCellCount=$p.cells.Count;ObservedMoveCount=$p.observedMoveCount;PathTotalDistance=$p.totalDistance}
}

# Additive smoke-path evidence only. Caller still owns source/process/PNG gates.
# Native codec recomputes A*; this independently checks serialized relationships.
function Assert-SeedForgeGameplayPath {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Trace)
    Set-StrictMode -Version Latest
    $ErrorActionPreference='Stop'
    Add-Type -AssemblyName System.Numerics
    function Require($Condition,[string]$Message) { if(-not $Condition){throw "GameplayPath: $Message"} }
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
    function ArrayValue($Value,$Min,$Max) { Require ($Value -is [array] -and $Value.Count -ge $Min -and $Value.Count -le $Max) 'Array size/type outside contract.' }
    function Vector($Value) { Object $Value @('x','y','z'); foreach($axis in @('x','y','z')){Number $Value.$axis} }
    function Cell($Value) { Object $Value @('x','y');Integer $Value.x -2147483648 2147483647;Integer $Value.y -2147483648 2147483647 }
    function CellKey($Value) { return "$($Value.x),$($Value.y)" }
    function Near($A,$B,$Tolerance=0.1) { return [math]::Abs($A.x-$B.x) -le $Tolerance -and [math]::Abs($A.y-$B.y) -le $Tolerance -and [math]::Abs($A.z-$B.z) -le $Tolerance }
    function Distance($A,$B,$XY=$false) { $z=0;if(-not $XY){$z=$A.z-$B.z};$distance=[math]::Sqrt([math]::Pow($A.x-$B.x,2)+[math]::Pow($A.y-$B.y,2)+$z*$z);Number $distance;return $distance }
    function OnCells($Position,$Cells,$Margin=0) { foreach($c in $Cells){if([math]::Abs($Position.x-$c.x*200.0) -le 100.1-$Margin -and [math]::Abs($Position.y-$c.y*200.0) -le 100.1-$Margin){return $true}};return $false }
    Object $Trace @('schema','schemaVersion','result','failureCode','failureMessage','seed','layoutHash','encounterHash','runGeneration','appliedRequestId','actorCounts','remainingPathDelegateBindings','walkableCells','pathEvidence','captures')
    TextValue $Trace.schema;Integer $Trace.schemaVersion 1 1
    Require ($Trace.schema -ceq 'seedforge.gameplay-smoke') 'Wrong schema.'
    foreach($f in @('result','failureCode','failureMessage')){TextValue $Trace.$f 1024 $true}
    Require ($Trace.result -ceq 'Passed' -and $Trace.failureCode -ceq '' -and $Trace.failureMessage -ceq '') 'Expected successful empty-failure envelope.'
    foreach($f in @('seed','layoutHash','encounterHash','runGeneration','appliedRequestId')){$null=UInt $Trace.$f}
    Require ((UInt $Trace.runGeneration) -gt 0 -and (UInt $Trace.appliedRequestId) -gt 0 -and (UInt $Trace.layoutHash) -gt 0 -and (UInt $Trace.encounterHash) -gt 0) 'Missing applied identity.'
    if($Trace.seed -ceq '24301'){Require ($Trace.layoutHash -ceq '7425849530159566348' -and $Trace.encounterHash -ceq '15303214708604970503') 'Default golden identity mismatch.'}
    Integer $Trace.remainingPathDelegateBindings 0 0
    Object $Trace.actorCounts @('enemies');Integer $Trace.actorCounts.enemies 5 5
    ArrayValue $Trace.walkableCells 1 4096
    $walkable=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal);$previousCell=$null
    foreach($c in $Trace.walkableCells){Cell $c;Require ($walkable.Add((CellKey $c))) 'Duplicate walkable cell.';if($null -ne $previousCell){Require ($c.y -gt $previousCell.y -or ($c.y -eq $previousCell.y -and $c.x -gt $previousCell.x)) 'Noncanonical walkable order.'};$previousCell=$c}
    ArrayValue $Trace.captures 3 3;$labels=@('start','combat','win')
    for($i=0;$i -lt 3;$i++){
        $receipt=$Trace.captures[$i];Object $receipt @('label','runGeneration','sourceRequestId','requestedFrame')
        TextValue $receipt.label;foreach($f in @('runGeneration','sourceRequestId','requestedFrame')){$null=UInt $receipt.$f}
        Require ($receipt.label -ceq $labels[$i] -and $receipt.runGeneration -ceq $Trace.runGeneration -and $receipt.sourceRequestId -ceq $Trace.appliedRequestId) 'Path/capture ownership or order mismatch.'
    }
    $completedFrame=UInt $Trace.captures[0].requestedFrame
    $p=$Trace.pathEvidence;Object $p @('stableId','runGeneration','sourceRequestId','pathRevision','start','goal','status','expandedNodes','cells','waypoints','movementSamples','observedMoveCount','totalDistance','totalDeltaSeconds','complete')
    TextValue $p.status
    Integer $p.stableId 0 4294967295;foreach($f in @('runGeneration','sourceRequestId','pathRevision','observedMoveCount')){$null=UInt $p.$f};Cell $p.start;Cell $p.goal;Integer $p.expandedNodes 1 1024;Boolean $p.complete;Number $p.totalDistance;Number $p.totalDeltaSeconds
    Require ($p.complete -and $p.status -ceq 'Success' -and $p.runGeneration -ceq $Trace.runGeneration -and $p.sourceRequestId -ceq $Trace.appliedRequestId -and (UInt $p.pathRevision) -gt 0 -and $p.stableId -lt $Trace.actorCounts.enemies) 'Path ownership/status mismatch.'
    ArrayValue $p.cells 2 1025;ArrayValue $p.waypoints ($p.cells.Count-1) ($p.cells.Count-1);ArrayValue $p.movementSamples 2 2
    Require ((CellKey $p.start) -ceq (CellKey $p.cells[0]) -and (CellKey $p.goal) -ceq (CellKey $p.cells[-1])) 'Path endpoints mismatch.'
    $route=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    for($i=0;$i -lt $p.cells.Count;$i++){ $c=$p.cells[$i];Cell $c;Require ($walkable.Contains((CellKey $c)) -and $route.Add((CellKey $c))) 'Path is not simple/walkable.'
        if($i -gt 0){$prior=$p.cells[$i-1];Require (([math]::Abs($c.x-$prior.x)+[math]::Abs($c.y-$prior.y)) -eq 1) 'Path is not cardinal.';$w=$p.waypoints[$i-1];Vector $w;Require (Near $w ([pscustomobject]@{x=$c.x*200.0;y=$c.y*200.0;z=58})) 'Waypoint conversion mismatch.'}
    }
    $sequence=[System.Numerics.BigInteger]::Zero;$moveFrame=[System.Numerics.BigInteger]::MinusOne;$lastIndex=0;$sumDistance=0.0;$sumDelta=0.0
    for($i=0;$i -lt 2;$i++){$m=$p.movementSamples[$i];Object $m @('sequence','frame','pathRevision','waypointIndex','from','target','to','deltaSeconds');$seq=UInt $m.sequence;$frame=UInt $m.frame;$null=UInt $m.pathRevision;Integer $m.waypointIndex 0 ($p.waypoints.Count-1);foreach($f in @('from','target','to')){Vector $m.$f};Number $m.deltaSeconds
        Require ($seq -gt $sequence -and $frame -gt $moveFrame -and $frame -le $completedFrame -and $m.pathRevision -ceq $p.pathRevision -and $m.waypointIndex -ge $lastIndex -and $m.deltaSeconds -gt 0 -and $m.deltaSeconds -le 30) 'Invalid movement sample ordering.'
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
    Require ($count -ge 2 -and $count -le (UInt $p.movementSamples[1].sequence)-(UInt $p.movementSamples[0].sequence)+1 -and $p.totalDistance -ge 20 -and $p.totalDeltaSeconds -gt 0 -and $p.totalDeltaSeconds -le 30 -and $p.totalDistance -le $aggregateBound -and $p.totalDistance+0.1 -ge $sumDistance -and $p.totalDeltaSeconds+0.000001 -ge $sumDelta) 'Invalid movement aggregate.'
    if($count -eq 2){
        $first=$p.movementSamples[0];$last=$p.movementSamples[1]
        Require ($last.waypointIndex -le $first.waypointIndex+1 -and ($last.waypointIndex -eq $first.waypointIndex -or (Distance $first.to $p.waypoints[$first.waypointIndex] $true) -le 4.1)) 'Two-sample waypoint advanced without arrival.'
        Require ((Near $last.from $first.to) -and [math]::Abs($p.totalDistance-$sumDistance) -le 0.1 -and [math]::Abs($p.totalDeltaSeconds-$sumDelta) -le 0.000001) 'Two-sample aggregate not contiguous/exact.'
    }
    else{
        $gap=Distance $p.movementSamples[0].to $p.movementSamples[1].from
        $hiddenDelta=$p.totalDeltaSeconds-$sumDelta;Number $hiddenDelta
        $hiddenBound=260.0*[math]::Max(0.0,$hiddenDelta)+0.1*[double]($count-2)+0.1;Number $hiddenBound
        Require ($p.totalDistance+0.1 -ge $sumDistance+$gap -and $hiddenDelta -ge -0.000001 -and $gap -le $hiddenBound) 'Movement aggregate cannot cover omitted displacement/time.'
        Require ($p.totalDistance-(Distance $p.movementSamples[1].from $p.movementSamples[1].to) -lt 20.0) 'Path kept accumulating after its completion threshold.'
    }
    return [pscustomobject]@{Validated=$true;RunGeneration=$Trace.runGeneration;SourceRequestId=$Trace.appliedRequestId;PathRevision=$p.pathRevision;StableId=$p.stableId;PathCellCount=$p.cells.Count;ObservedMoveCount=$p.observedMoveCount;PathTotalDistance=$p.totalDistance;FirstCaptureRequestedFrame=$Trace.captures[0].requestedFrame;RemainingPathDelegateBindings=0}
}

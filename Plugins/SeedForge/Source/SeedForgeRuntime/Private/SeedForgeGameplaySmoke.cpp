#include "SeedForgeGameplaySmoke.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"

FSeedForgeGameplaySmokePathObserver::~FSeedForgeGameplaySmokePathObserver() { Cancel(); }
bool FSeedForgeGameplaySmokePathObserver::Start(ASeedForgeGameplayCoordinator& Coordinator)
{
    Cancel();
    UWorld* World = Coordinator.GetWorld();
    const auto Snapshot = Coordinator.GetSnapshot();
    if (!World || Coordinator.IsActorBeingDestroyed() || Snapshot.RunState != ESeedForgeRunState::Playing
        || Snapshot.RunGeneration == 0 || Snapshot.AppliedRequestId == 0 || Snapshot.PendingRequestId != 0) { return false; }
    TArray<FIntPoint> Cells = Coordinator.GetLayout().GetCanonicalWalkableCells();
    if (Cells.IsEmpty() || Cells.Num() > FSeedForgeGameplaySmokeTrace::MaxWalkableCells) { return false; }
    ASeedForgeEnemyPawn* Enemy = nullptr;
    for (TActorIterator<ASeedForgeEnemyPawn> It(World); It; ++It)
    {
        if (It->IsActorBeingDestroyed() || It->GetOwner() != &Coordinator) { continue; }
        if (!Enemy || It->GetStableId() < Enemy->GetStableId()) { Enemy = *It; }
    }
    if (!Enemy) { return false; }
    Owner = &Coordinator; Target = Enemy; OwningWorld = World;
    Walkable = MoveTemp(Cells); Tuning = Coordinator.GetTuning();
    RunGeneration = Snapshot.RunGeneration; SourceRequestId = Snapshot.AppliedRequestId;
    PathHandle = Coordinator.OnEnemyPathApplied().AddRaw(this, &FSeedForgeGameplaySmokePathObserver::OnPath);
    QueueHandle = Coordinator.OnRunQueued().AddRaw(this, &FSeedForgeGameplaySmokePathObserver::OnQueued);
    WorldHandle = FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FSeedForgeGameplaySmokePathObserver::OnWorld);
    return true;
}
bool FSeedForgeGameplaySmokePathObserver::MatchesCurrentRun() const
{
    const auto* Coordinator = Owner.Get(); const auto* Enemy = Target.Get();
    if (!Coordinator || Coordinator->IsActorBeingDestroyed() || !Enemy || Enemy->IsActorBeingDestroyed()
        || Enemy->GetOwner() != Coordinator || Coordinator->GetWorld() != OwningWorld.Get()) { return false; }
    const auto Snapshot = Coordinator->GetSnapshot();
    return Snapshot.RunState == ESeedForgeRunState::Playing && Snapshot.RunGeneration == RunGeneration
        && Snapshot.AppliedRequestId == SourceRequestId && Snapshot.PendingRequestId == 0;
}
void FSeedForgeGameplaySmokePathObserver::Detach()
{
    FWorldDelegates::OnWorldPostActorTick.Remove(WorldHandle); WorldHandle.Reset();
    if (auto* Coordinator = Owner.Get())
    { Coordinator->OnEnemyPathApplied().Remove(PathHandle); Coordinator->OnRunQueued().Remove(QueueHandle); }
    PathHandle.Reset(); QueueHandle.Reset();
}
void FSeedForgeGameplaySmokePathObserver::Cancel()
{
    Detach(); Proof.Reset(); Owner.Reset(); Target.Reset(); OwningWorld.Reset(); Walkable.Reset();
    RunGeneration = SourceRequestId = 0;
}
void FSeedForgeGameplaySmokePathObserver::OnPath(ASeedForgeEnemyPawn* Enemy, uint64 Run, uint64 Request,
    const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result)
{
    if (Enemy != Target.Get() || Run != RunGeneration || Request != SourceRequestId) { return; }
    if (!MatchesCurrentRun()) { Cancel(); return; }
    Proof.ObserveAppliedPath(Enemy->GetStableId(), Run, Request, Start, Goal, Result, Enemy->GetPathSnapshot(), Walkable, Tuning.CellSize);
}
void FSeedForgeGameplaySmokePathObserver::OnQueued(ESeedForgeRunState StateBefore, const FSeedForgeGameplaySnapshot& Snapshot)
{
    static_cast<void>(StateBefore); static_cast<void>(Snapshot);
    Cancel();
}
void FSeedForgeGameplaySmokePathObserver::OnWorld(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
    if (World != OwningWorld.Get() || TickType != LEVELTICK_All || DeltaSeconds <= 0) { return; }
    if (!MatchesCurrentRun()) { Cancel(); return; }
    const auto Movement = Target->GetPathSnapshot().LastMove;
    const auto& Evidence = Proof.GetEvidence();
    if (!Evidence.MovementSamples.IsEmpty() && Movement.Frame <= Evidence.MovementSamples.Last().Frame) { return; }
    Proof.ObserveMovement(Target->GetStableId(), RunGeneration, SourceRequestId, Movement, Tuning.EnemyMoveSpeed);
    if (Proof.IsComplete()) { Detach(); }
}
bool FSeedForgeGameplaySmokePathObserver::IsComplete() const { return Proof.IsComplete(); }
const FSeedForgeEnemyPathEvidence& FSeedForgeGameplaySmokePathObserver::GetEvidence() const { return Proof.GetEvidence(); }
int32 FSeedForgeGameplaySmokePathObserver::GetRemainingDelegateBindings() const
{
    return (PathHandle.IsValid() ? 1 : 0) + (QueueHandle.IsValid() ? 1 : 0) + (WorldHandle.IsValid() ? 1 : 0);
}

namespace SeedForge::GameplaySmoke::Private
{
    bool Finite(const FVector& Value)
    { return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z); }
    bool OnCells(const FVector& Position, const TArray<FIntPoint>& Cells)
    {
        for (const auto& Cell : Cells)
        {
            if (FMath::Abs(Position.X - static_cast<double>(Cell.X) * 200.0) <= 100.1
                && FMath::Abs(Position.Y - static_cast<double>(Cell.Y) * 200.0) <= 100.1) { return true; }
        }
        return false;
    }
    using FWriter = TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>;
    void UInt(const TSharedRef<FWriter>& Writer, const TCHAR* Key, uint64 Value)
    { Writer->WriteValue(Key, FString::Printf(TEXT("%llu"), Value)); }
    void Number(const TSharedRef<FWriter>& Writer, const TCHAR* Key, double Value)
    { if (FMath::IsFinite(Value)) { Writer->WriteValue(Key, Value); } else { Writer->WriteNull(Key); } }
    void Cell(const TSharedRef<FWriter>& Writer, const FIntPoint& Value)
    { Writer->WriteValue(TEXT("x"), Value.X); Writer->WriteValue(TEXT("y"), Value.Y); }
    void Vector(const TSharedRef<FWriter>& Writer, const FVector& Value)
    { Number(Writer, TEXT("x"), Value.X); Number(Writer, TEXT("y"), Value.Y); Number(Writer, TEXT("z"), Value.Z); }
    void NamedVector(const TSharedRef<FWriter>& Writer, const TCHAR* Key, const FVector& Value)
    { Writer->WriteObjectStart(Key); Vector(Writer, Value); Writer->WriteObjectEnd(); }
    FString PathJson(const FSeedForgeEnemyPathEvidence& Path)
    {
        FString Json;
        const auto W = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
        W->WriteObjectStart(); W->WriteValue(TEXT("stableId"), Path.StableId);
        UInt(W, TEXT("runGeneration"), Path.RunGeneration); UInt(W, TEXT("sourceRequestId"), Path.SourceRequestId); UInt(W, TEXT("pathRevision"), Path.PathRevision);
        W->WriteObjectStart(TEXT("start")); Cell(W, Path.Start); W->WriteObjectEnd();
        W->WriteObjectStart(TEXT("goal")); Cell(W, Path.Goal); W->WriteObjectEnd();
        W->WriteValue(TEXT("status"), Path.Status == ESeedForgePathStatus::Success ? TEXT("Success") : TEXT("InvalidInput"));
        W->WriteValue(TEXT("expandedNodes"), Path.ExpandedNodes);
        W->WriteArrayStart(TEXT("cells"));
        for (int32 I = 0; I < FMath::Min(Path.Cells.Num(), FSeedForgeEnemyPathProof::MaxPathCells); ++I)
        { W->WriteObjectStart(); Cell(W, Path.Cells[I]); W->WriteObjectEnd(); }
        W->WriteArrayEnd(); W->WriteArrayStart(TEXT("waypoints"));
        for (int32 I = 0; I < FMath::Min(Path.Waypoints.Num(), FSeedForgeEnemyPathProof::MaxPathCells - 1); ++I)
        { W->WriteObjectStart(); Vector(W, Path.Waypoints[I]); W->WriteObjectEnd(); }
        W->WriteArrayEnd(); W->WriteArrayStart(TEXT("movementSamples"));
        for (int32 I = 0; I < FMath::Min(Path.MovementSamples.Num(), 2); ++I)
        {
            const auto& M = Path.MovementSamples[I]; W->WriteObjectStart();
            UInt(W, TEXT("sequence"), M.Sequence); UInt(W, TEXT("frame"), M.Frame); UInt(W, TEXT("pathRevision"), M.PathRevision);
            W->WriteValue(TEXT("waypointIndex"), M.WaypointIndex);
            NamedVector(W, TEXT("from"), M.From); NamedVector(W, TEXT("target"), M.Target); NamedVector(W, TEXT("to"), M.To);
            Number(W, TEXT("deltaSeconds"), M.DeltaSeconds); W->WriteObjectEnd();
        }
        W->WriteArrayEnd(); UInt(W, TEXT("observedMoveCount"), Path.ObservedMoveCount);
        Number(W, TEXT("totalDistance"), Path.TotalDistance); Number(W, TEXT("totalDeltaSeconds"), Path.TotalDeltaSeconds);
        W->WriteValue(TEXT("complete"), Path.bComplete); W->WriteObjectEnd(); W->Close();
        return Json;
    }

    FString EscapeJson(const FString& Value)
    {
        FString Escaped;
        Escaped.Reserve(Value.Len() + 8);
        for (const TCHAR Character : Value)
        {
            switch (Character)
            {
            case TEXT('"'): Escaped += TEXT("\\\""); break;
            case TEXT('\\'): Escaped += TEXT("\\\\"); break;
            case TEXT('\b'): Escaped += TEXT("\\b"); break;
            case TEXT('\f'): Escaped += TEXT("\\f"); break;
            case TEXT('\n'): Escaped += TEXT("\\n"); break;
            case TEXT('\r'): Escaped += TEXT("\\r"); break;
            case TEXT('\t'): Escaped += TEXT("\\t"); break;
            default:
                if (Character < 0x20)
                {
                    Escaped += FString::Printf(TEXT("\\u%04x"), static_cast<uint32>(Character));
                }
                else
                {
                    Escaped.AppendChar(Character);
                }
                break;
            }
        }
        return Escaped;
    }

    void AppendString(FString& Json, const FString& Value)
    {
        Json += TEXT("\"");
        Json += EscapeJson(Value);
        Json += TEXT("\"");
    }

    void AppendStringArray(FString& Json, const TArray<FString>& Values)
    {
        Json += TEXT("[");
        for (int32 Index = 0; Index < Values.Num(); ++Index)
        {
            if (Index > 0)
            {
                Json += TEXT(",");
            }
            AppendString(Json, Values[Index]);
        }
        Json += TEXT("]");
    }
}

bool FSeedForgeGameplaySmokeCodec::ValidatePathEvidence(const FSeedForgeGameplaySmokeTrace& Trace, FString& OutError)
{
    using namespace SeedForge::GameplaySmoke::Private;
    auto Reject = [&](const TCHAR* Error) { OutError = Error; return false; };
    const auto& P = Trace.PathEvidence;
    if (Trace.RunGeneration == 0 || Trace.AppliedRequestId == 0 || Trace.RemainingPathDelegateBindings != 0
        || Trace.ActorCounts.Enemies <= 0 || P.StableId >= static_cast<uint32>(Trace.ActorCounts.Enemies)
        || !P.bComplete || P.Status != ESeedForgePathStatus::Success || P.RunGeneration != Trace.RunGeneration
        || P.SourceRequestId != Trace.AppliedRequestId || P.PathRevision == 0)
    { return Reject(TEXT("Missing or stale same-run path evidence.")); }
    if (Trace.WalkableCells.IsEmpty() || Trace.WalkableCells.Num() > FSeedForgeGameplaySmokeTrace::MaxWalkableCells
        || P.Cells.Num() < 2 || P.Cells.Num() > FSeedForgeEnemyPathProof::MaxPathCells
        || P.Waypoints.Num() != P.Cells.Num() - 1 || P.MovementSamples.Num() != 2
        || Trace.Captures.Num() != 3 || Trace.ScreenshotPaths.Num() != 3)
    { return Reject(TEXT("Path/capture evidence exceeds or misses its bounded shape.")); }
    for (int32 I = 1; I < Trace.WalkableCells.Num(); ++I)
    {
        const auto A = Trace.WalkableCells[I-1]; const auto B = Trace.WalkableCells[I];
        if (!(B.Y > A.Y || (B.Y == A.Y && B.X > A.X))) { return Reject(TEXT("Walkable cells are not unique canonical order.")); }
    }
    const TCHAR* Labels[] = {TEXT("start"), TEXT("combat"), TEXT("win")};
    TSet<FGuid> Tokens; TSet<FString> Paths;
    for (int32 I = 0; I < 3; ++I)
    {
        const auto& C = Trace.Captures[I]; const auto& R = C.Request;
        if (R.Label != Labels[I] || R.RunGeneration != Trace.RunGeneration || R.SourceRequestId != Trace.AppliedRequestId
            || !C.bSuccess || C.RemainingDelegateBindings != 0 || !R.Token.IsValid() || Tokens.Contains(R.Token)
            || R.Path.IsEmpty() || Paths.Contains(R.Path) || R.Path != Trace.ScreenshotPaths[I]
            || R.RequestedFrame > C.RenderedFrame || C.RenderedFrame > C.CapturedFrame || C.CapturedFrame > C.CompletedFrame
            || R.RequestedAtUtc.GetTicks() <= 0 || R.RequestedAtUtc > C.CompletedAtUtc
            || (I > 0 && (R.RequestedFrame < Trace.Captures[I-1].CompletedFrame || R.RequestedAtUtc < Trace.Captures[I-1].CompletedAtUtc)))
        { return Reject(TEXT("Path/capture correlation or lifecycle is invalid.")); }
        Tokens.Add(R.Token); Paths.Add(R.Path);
    }
    FSeedForgePathRequest Request;
    Request.Start = P.Start; Request.Goal = P.Goal; Request.WalkableCells = Trace.WalkableCells; Request.MaxExpandedNodes = 1024;
    const auto Expected = FSeedForgeGridPathfinder::FindPath(Request);
    if (!Expected.IsSuccess() || P.Cells != Expected.Path || P.ExpandedNodes != Expected.ExpandedNodes)
    { return Reject(TEXT("Recorded route does not match the native A* result.")); }
    for (int32 I = 0; I < P.Waypoints.Num(); ++I)
    {
        if (!Finite(P.Waypoints[I]) || !P.Waypoints[I].Equals(FSeedForgeGameplayMath::CellToWorld(P.Cells[I+1], 200, 58), 0.1))
        { return Reject(TEXT("Stored path waypoints do not match the route.")); }
    }
    double SumDistance = 0.0, SumDelta = 0.0;
    for (int32 I = 0; I < 2; ++I)
    {
        const auto& M = P.MovementSamples[I];
        if (M.Sequence == 0 || M.PathRevision != P.PathRevision || M.Frame > Trace.Captures[0].Request.RequestedFrame
            || !P.Waypoints.IsValidIndex(M.WaypointIndex) || !Finite(M.From) || !Finite(M.Target) || !Finite(M.To)
            || !FMath::IsFinite(M.DeltaSeconds) || M.DeltaSeconds <= 0 || M.DeltaSeconds > 30
            || !M.Target.Equals(P.Waypoints[M.WaypointIndex], 0.1)
            || FMath::Abs(M.From.Z - 58) > 0.1 || FMath::Abs(M.To.Z - 58) > 0.1
            || !OnCells(M.From, P.Cells) || !OnCells(M.To, P.Cells)
            || (I == 0 && (M.WaypointIndex != 0 || !OnCells(M.From, TArray<FIntPoint>{P.Start})))
            || (I == 1 && (M.Sequence <= P.MovementSamples[0].Sequence || M.Frame <= P.MovementSamples[0].Frame
                || M.WaypointIndex < P.MovementSamples[0].WaypointIndex)))
        { return Reject(TEXT("Movement sample attribution, ordering or route membership is invalid.")); }
        const double Distance = FVector::Dist(M.From, M.To);
        if (!FMath::IsFinite(Distance) || Distance <= 0 || Distance > 260.0 * M.DeltaSeconds + 0.1
            || FVector::DotProduct(M.To-M.From, M.Target-M.From) <= 0
            || !M.To.Equals(FMath::ClosestPointOnSegment(M.To, M.From, M.Target), 0.1))
        { return Reject(TEXT("Movement sample is not bounded motion toward its consumed waypoint.")); }
        SumDistance += Distance; SumDelta += M.DeltaSeconds;
    }
    const auto& First = P.MovementSamples[0]; const auto& Last = P.MovementSamples[1];
    if (P.ObservedMoveCount < 2 || P.ObservedMoveCount > Last.Sequence - First.Sequence + 1
        || !FMath::IsFinite(P.TotalDistance) || P.TotalDistance < 20 || !FMath::IsFinite(P.TotalDeltaSeconds)
        || P.TotalDeltaSeconds <= 0 || P.TotalDeltaSeconds > 30
        || P.TotalDistance > 260.0 * P.TotalDeltaSeconds + 0.1 * static_cast<double>(P.ObservedMoveCount)
        || P.TotalDistance + 0.1 < SumDistance || P.TotalDeltaSeconds + 0.000001 < SumDelta)
    { return Reject(TEXT("Movement aggregates do not match the bounded retained observations.")); }
    if (P.ObservedMoveCount == 2)
    {
        if (!Last.From.Equals(First.To, 0.1) || Last.WaypointIndex > First.WaypointIndex + 1
            || (Last.WaypointIndex != First.WaypointIndex && FVector::Dist2D(First.To, First.Target) > 4.1)
            || FMath::Abs(P.TotalDistance - SumDistance) > 0.1 || FMath::Abs(P.TotalDeltaSeconds - SumDelta) > 0.000001)
        { return Reject(TEXT("Two retained moves must be contiguous with valid waypoint arrival.")); }
    }
    else
    {
        const double Gap = FVector::Dist(First.To, Last.From);
        const double HiddenDelta = P.TotalDeltaSeconds - SumDelta;
        const double HiddenSpeedBound = 260.0 * FMath::Max(0.0, HiddenDelta)
            + 0.1 * static_cast<double>(P.ObservedMoveCount - 2) + 0.1;
        if (!FMath::IsFinite(Gap) || !FMath::IsFinite(HiddenSpeedBound) || HiddenDelta < -0.000001
            || P.TotalDistance + 0.1 < SumDistance + Gap || Gap > HiddenSpeedBound)
        { return Reject(TEXT("Hidden movement gap exceeds its aggregate distance or time budget.")); }
        if (P.TotalDistance - FVector::Dist(Last.From, Last.To) >= 20.0)
        { return Reject(TEXT("Movement proof accumulated after its completion threshold.")); }
    }
    OutError.Reset();
    return true;
}

FString FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(
    const FSeedForgeGameplaySmokeTrace& Trace)
{
    using namespace SeedForge::GameplaySmoke::Private;

    FString PathError;
    const bool bPassed = Trace.bSuccess && ValidatePathEvidence(Trace, PathError);
    FString Json;
    Json.Reserve(1024 + Trace.Actions.Num() * 48 + Trace.ScreenshotPaths.Num() * 160);
    Json += TEXT("{\"schema\":\"seedforge.gameplay-smoke\",\"schemaVersion\":1,\"gitSha\":");
    AppendString(Json, Trace.GitSha);
    Json += TEXT(",\"engineVersion\":");
    AppendString(Json, Trace.EngineVersion);
    Json += FString::Printf(
        TEXT(",\"seed\":\"%llu\",\"layoutHash\":\"%llu\",\"encounterHash\":\"%llu\",\"runGeneration\":\"%llu\",\"appliedRequestId\":\"%llu\","),
        Trace.Seed,
        Trace.LayoutHash,
        Trace.EncounterHash,
        Trace.RunGeneration,
        Trace.AppliedRequestId);
    Json += FString::Printf(
        TEXT("\"actorCounts\":{\"players\":%d,\"dataCores\":%d,\"enemies\":%d,\"exits\":%d},"),
        Trace.ActorCounts.Players,
        Trace.ActorCounts.DataCores,
        Trace.ActorCounts.Enemies,
        Trace.ActorCounts.Exits);
    Json += TEXT("\"stateTransitions\":");
    AppendStringArray(Json, Trace.StateTransitions);
    Json += TEXT(",\"actions\":");
    AppendStringArray(Json, Trace.Actions);
    Json += TEXT(",\"screenshots\":");
    AppendStringArray(Json, Trace.ScreenshotPaths);
    Json += TEXT(",\"captures\":[");
    for (int32 Index = 0; Index < Trace.Captures.Num(); ++Index)
    {
        if (Index > 0) { Json += TEXT(","); }
        const FSeedForgeCaptureReceipt& Capture = Trace.Captures[Index];
        Json += TEXT("{\"token\":");
        AppendString(Json, Capture.Request.Token.ToString(EGuidFormats::DigitsLower));
        Json += TEXT(",\"label\":");
        AppendString(Json, Capture.Request.Label);
        Json += TEXT(",\"path\":");
        AppendString(Json, Capture.Request.Path);
        Json += FString::Printf(TEXT(",\"runGeneration\":\"%llu\",\"sourceRequestId\":\"%llu\""),
            Capture.Request.RunGeneration, Capture.Request.SourceRequestId);
        Json += TEXT(",\"requestedAtUtc\":");
        AppendString(Json, Capture.Request.RequestedAtUtc.ToIso8601());
        Json += TEXT(",\"completedAtUtc\":");
        AppendString(Json, Capture.CompletedAtUtc.ToIso8601());
        Json += FString::Printf(TEXT(",\"requestedFrame\":\"%llu\",\"renderedFrame\":\"%llu\",\"capturedFrame\":\"%llu\",\"completedFrame\":\"%llu\""),
            Capture.Request.RequestedFrame, Capture.RenderedFrame, Capture.CapturedFrame, Capture.CompletedFrame);
        Json += FString::Printf(TEXT(",\"width\":%d,\"height\":%d,\"fileBytes\":%lld,\"success\":%s"),
            Capture.Request.Size.X, Capture.Request.Size.Y, Capture.FileBytes, Capture.bSuccess ? TEXT("true") : TEXT("false"));
        Json += FString::Printf(TEXT(",\"remainingDelegateBindings\":%d}"), Capture.RemainingDelegateBindings);
    }
    Json += TEXT("]");
    Json += TEXT(",\"walkableCells\":[");
    for (int32 I = 0; I < FMath::Min(Trace.WalkableCells.Num(), FSeedForgeGameplaySmokeTrace::MaxWalkableCells); ++I)
    { if (I > 0) { Json += TEXT(","); } Json += FString::Printf(TEXT("{\"x\":%d,\"y\":%d}"), Trace.WalkableCells[I].X, Trace.WalkableCells[I].Y); }
    Json += TEXT("],\"pathEvidence\":"); Json += PathJson(Trace.PathEvidence);
    Json += FString::Printf(TEXT(",\"remainingPathDelegateBindings\":%d"), Trace.RemainingPathDelegateBindings);
    Json += bPassed ? TEXT(",\"result\":\"Passed\"") : TEXT(",\"result\":\"Failed\"");
    Json += TEXT(",\"failureCode\":");
    AppendString(Json, Trace.bSuccess && !bPassed ? FString(TEXT("MissingPathEvidence")) : Trace.FailureCode);
    Json += TEXT(",\"failureMessage\":");
    AppendString(Json, Trace.bSuccess && !bPassed ? PathError : Trace.FailureMessage);
    Json += TEXT("}");
    return Json;
}

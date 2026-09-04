#include "SeedForgeInputSelfTest.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Misc/EngineVersion.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeEncounter.h"
#include "SeedForgeGameplayActors.h"
#include "SeedForgeGameplayCoordinator.h"
#include "SeedForgePreviewActor.h"
#include "SeedForgeRuntime.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Input/Events.h"
#include "InputKeyEventArgs.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"
#include "Layout/Children.h"

namespace SeedForge::InputSelfTest::Private
{
    using FWriter = TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>;
    const TCHAR* Actions[] = {TEXT("AimX"), TEXT("AimY"), TEXT("AttackFirst"), TEXT("AttackKill"),
        TEXT("MoveW"), TEXT("MoveS"), TEXT("MoveA"), TEXT("MoveD"), TEXT("Dash"),
        TEXT("RestartSameSeed"), TEXT("StartNewSeed"), TEXT("RapidNewSeed"), TEXT("RapidRestartSameSeed")};
    const TCHAR* Keys[] = {TEXT("MouseMove"), TEXT("MouseMove"), TEXT("LeftMouseButton"), TEXT("LeftMouseButton"),
        TEXT("W"), TEXT("S"), TEXT("A"), TEXT("D"), TEXT("SpaceBar"), TEXT("R"), TEXT("N"), TEXT("N"), TEXT("R")};
    const FVector MoveDirections[] = {FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, -1, 0), FVector(0, 1, 0)};

    bool Live(const AActor* Actor) { return IsValid(Actor) && !Actor->IsActorBeingDestroyed(); }
    FString ActorKey(const AActor* Actor)
    { return Actor ? FString::Printf(TEXT("%s#%u"), *Actor->GetPathName(), Actor->GetUniqueID()) : FString(); }
    int32 CountHudWidgets(const TSharedRef<SWidget>& Widget, int32 Depth, int32& Visited)
    {
        if (++Visited > 4096 || Depth > 64) { return 100; }
        int32 Count = Widget->GetTag() == TEXT("SeedForgeHUD") ? 1 : 0;
        FChildren* Children = Widget->GetChildren();
        for (int32 I = 0; Children && I < Children->Num() && Count < 2; ++I)
        { Count += CountHudWidgets(Children->GetChildAt(I), Depth + 1, Visited); }
        return Count;
    }

    bool Finite(const FVector& V) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); }
    uint64 NextSeed(uint64 Seed) { return Seed * 6364136223846793005ULL + 1442695040888963407ULL; }
    bool OnCells(const FVector& Position, const TArray<FIntPoint>& Cells, double Margin = 0.0)
    {
        for (const FIntPoint& Cell : Cells)
        {
            if (FMath::Abs(Position.X - Cell.X * 200.0) <= 100.1 - Margin
                && FMath::Abs(Position.Y - Cell.Y * 200.0) <= 100.1 - Margin) { return true; }
        }
        return false;
    }
    bool SameSnapshot(const FSeedForgeGameplaySnapshot& A, const FSeedForgeGameplaySnapshot& B)
    {
        return A.Seed == B.Seed && A.LayoutHash == B.LayoutHash && A.EncounterHash == B.EncounterHash
            && A.RunGeneration == B.RunGeneration && A.PendingRequestId == B.PendingRequestId
            && A.AppliedRequestId == B.AppliedRequestId && A.RunState == B.RunState
            && A.PlayerHealth == B.PlayerHealth && A.PlayerMaxHealth == B.PlayerMaxHealth
            && A.CollectedCoreCount == B.CollectedCoreCount && A.RequiredCoreCount == B.RequiredCoreCount
            && A.bExitUnlocked == B.bExitUnlocked && A.FailureCode == B.FailureCode && A.FailureMessage == B.FailureMessage;
    }
    const TCHAR* StateName(ESeedForgeRunState State)
    {
        switch (State)
        {
        case ESeedForgeRunState::Playing: return TEXT("Playing");
        case ESeedForgeRunState::Won: return TEXT("Won");
        case ESeedForgeRunState::Lost: return TEXT("Lost");
        case ESeedForgeRunState::Restarting: return TEXT("Restarting");
        case ESeedForgeRunState::Failed: return TEXT("Failed");
        default: return TEXT("Generating");
        }
    }
    void UInt(const TSharedRef<FWriter>& W, const TCHAR* Key, uint64 Value) { W->WriteValue(Key, FString::Printf(TEXT("%llu"), Value)); }
    void Number(const TSharedRef<FWriter>& W, const TCHAR* Key, double Value)
    {
        if (FMath::IsFinite(Value)) { W->WriteValue(Key, Value); } else { W->WriteNull(Key); }
    }
    void Vector(const TSharedRef<FWriter>& W, const TCHAR* Key, const FVector& V)
    {
        W->WriteObjectStart(Key); Number(W, TEXT("x"), V.X); Number(W, TEXT("y"), V.Y); Number(W, TEXT("z"), V.Z); W->WriteObjectEnd();
    }
    void Cell(const TSharedRef<FWriter>& W, const FIntPoint& P)
    {
        W->WriteValue(TEXT("x"), P.X); W->WriteValue(TEXT("y"), P.Y);
    }
    void Snapshot(const TSharedRef<FWriter>& W, const TCHAR* Key, const FSeedForgeGameplaySnapshot& S)
    {
        W->WriteObjectStart(Key);
        UInt(W, TEXT("seed"), S.Seed); UInt(W, TEXT("layoutHash"), S.LayoutHash); UInt(W, TEXT("encounterHash"), S.EncounterHash);
        UInt(W, TEXT("runGeneration"), S.RunGeneration); UInt(W, TEXT("pendingRequestId"), S.PendingRequestId); UInt(W, TEXT("appliedRequestId"), S.AppliedRequestId);
        W->WriteValue(TEXT("runState"), StateName(S.RunState)); Number(W, TEXT("playerHealth"), S.PlayerHealth); Number(W, TEXT("playerMaxHealth"), S.PlayerMaxHealth);
        W->WriteValue(TEXT("collectedCoreCount"), S.CollectedCoreCount); W->WriteValue(TEXT("requiredCoreCount"), S.RequiredCoreCount);
        W->WriteValue(TEXT("exitUnlocked"), S.bExitUnlocked); W->WriteValue(TEXT("failureCode"), LexToString(S.FailureCode)); W->WriteValue(TEXT("failureMessage"), S.FailureMessage.Left(1024));
        W->WriteObjectEnd();
    }
    void Path(const TSharedRef<FWriter>& W, const FSeedForgeEnemyPathEvidence& P)
    {
        W->WriteObjectStart(TEXT("pathEvidence")); W->WriteValue(TEXT("stableId"), P.StableId);
        UInt(W, TEXT("runGeneration"), P.RunGeneration); UInt(W, TEXT("sourceRequestId"), P.SourceRequestId); UInt(W, TEXT("pathRevision"), P.PathRevision);
        W->WriteObjectStart(TEXT("start")); Cell(W, P.Start); W->WriteObjectEnd(); W->WriteObjectStart(TEXT("goal")); Cell(W, P.Goal); W->WriteObjectEnd();
        W->WriteValue(TEXT("status"), P.Status == ESeedForgePathStatus::Success ? TEXT("Success") : TEXT("InvalidInput")); W->WriteValue(TEXT("expandedNodes"), P.ExpandedNodes);
        W->WriteArrayStart(TEXT("cells")); for (int32 I=0; I<FMath::Min(P.Cells.Num(), FSeedForgeEnemyPathProof::MaxPathCells); ++I) { W->WriteObjectStart(); Cell(W,P.Cells[I]); W->WriteObjectEnd(); } W->WriteArrayEnd();
        W->WriteArrayStart(TEXT("waypoints")); for (int32 I=0; I<FMath::Min(P.Waypoints.Num(), FSeedForgeEnemyPathProof::MaxPathCells-1); ++I)
        { W->WriteObjectStart(); Number(W,TEXT("x"),P.Waypoints[I].X); Number(W,TEXT("y"),P.Waypoints[I].Y); Number(W,TEXT("z"),P.Waypoints[I].Z); W->WriteObjectEnd(); } W->WriteArrayEnd();
        W->WriteArrayStart(TEXT("movementSamples")); for (int32 I=0; I<FMath::Min(P.MovementSamples.Num(),2); ++I)
        {
            const auto& M=P.MovementSamples[I]; W->WriteObjectStart(); UInt(W,TEXT("sequence"),M.Sequence); UInt(W,TEXT("frame"),M.Frame); UInt(W,TEXT("pathRevision"),M.PathRevision);
            W->WriteValue(TEXT("waypointIndex"),M.WaypointIndex); Vector(W,TEXT("from"),M.From); Vector(W,TEXT("target"),M.Target); Vector(W,TEXT("to"),M.To); Number(W,TEXT("deltaSeconds"),M.DeltaSeconds); W->WriteObjectEnd();
        } W->WriteArrayEnd();
        UInt(W,TEXT("observedMoveCount"),P.ObservedMoveCount); Number(W,TEXT("totalDistance"),P.TotalDistance); Number(W,TEXT("totalDeltaSeconds"),P.TotalDeltaSeconds); W->WriteValue(TEXT("complete"),P.bComplete); W->WriteObjectEnd();
    }
    const TCHAR* SourceKindName(ESeedForgeInputSourceKind Kind)
    {
        switch (Kind)
        {
        case ESeedForgeInputSourceKind::CleanRevision: return TEXT("clean");
        case ESeedForgeInputSourceKind::DiagnosticRevision: return TEXT("diagnostic");
        default: return TEXT("unverified");
        }
    }

    const TCHAR* FailureName(ESeedForgeInputSelfTestFailure Code)
    {
        switch (Code)
        {
        case ESeedForgeInputSelfTestFailure::None: return TEXT("None");
        case ESeedForgeInputSelfTestFailure::InvalidSourceIdentity: return TEXT("InvalidSourceIdentity");
        case ESeedForgeInputSelfTestFailure::ViewportUnavailable: return TEXT("ViewportUnavailable");
        case ESeedForgeInputSelfTestFailure::ViewportNotReady: return TEXT("ViewportNotReady");
        case ESeedForgeInputSelfTestFailure::Cancelled: return TEXT("Cancelled");
        case ESeedForgeInputSelfTestFailure::ObservationLimitExceeded: return TEXT("ObservationLimitExceeded");
        case ESeedForgeInputSelfTestFailure::SelfTestTimeout: return TEXT("SelfTestTimeout");
        case ESeedForgeInputSelfTestFailure::InvalidArguments: return TEXT("InvalidArguments");
        case ESeedForgeInputSelfTestFailure::TraceWriteFailed: return TEXT("TraceWriteFailed");
        default: return TEXT("MissingInputEvidence");
        }
    }
}

const TCHAR* LexToString(ESeedForgeInputSelfTestFailure Code)
{
    return SeedForge::InputSelfTest::Private::FailureName(Code);
}

bool FSeedForgeInputSelfTestCodec::ParseSourceIdentity(const FString& Source,
    ESeedForgeInputSourceKind& OutKind, FString& OutRevision)
{
    OutKind = ESeedForgeInputSourceKind::Unverified;
    OutRevision.Reset();
    const FString Prefix(TEXT("diagnostic-"));
    const bool bDiagnostic = Source.StartsWith(Prefix, ESearchCase::CaseSensitive);
    const int32 Offset = bDiagnostic ? Prefix.Len() : 0;
    if (Source.Len() != Offset + 40) { return false; }
    for (int32 Index = Offset; Index < Source.Len(); ++Index)
    {
        const TCHAR Character = Source[Index];
        if (!((Character >= TEXT('0') && Character <= TEXT('9'))
            || (Character >= TEXT('a') && Character <= TEXT('f'))
            || (Character >= TEXT('A') && Character <= TEXT('F'))))
        {
            return false;
        }
    }
    OutRevision = Source.Mid(Offset).ToLower();
    OutKind = bDiagnostic ? ESeedForgeInputSourceKind::DiagnosticRevision : ESeedForgeInputSourceKind::CleanRevision;
    return true;
}

bool FSeedForgeInputSelfTestCodec::ParseOptions(const TCHAR* CommandLine,
    FSeedForgeInputSelfTestOptions& OutOptions, FString& OutError)
{
    OutOptions = {};
    OutError.Reset();
    if (!CommandLine) { OutError = TEXT("Missing command line."); return false; }
    OutOptions.bEnabled = FParse::Param(CommandLine, TEXT("SeedForgeInputSelfTest"));
    if (!OutOptions.bEnabled) { return true; }
    FParse::Value(CommandLine, TEXT("SeedForgeInputTrace="), OutOptions.TracePath);
    FParse::Value(CommandLine, TEXT("SeedForgeGitSha="), OutOptions.SourceIdentity);
    const FString Arguments(CommandLine);
    if (FParse::Param(CommandLine, TEXT("SeedForgeGameplaySmoke"))
        || Arguments.Contains(TEXT("SeedForgeCapturePath="), ESearchCase::IgnoreCase)
        || Arguments.Contains(TEXT("SeedForgeCaptureRoot="), ESearchCase::IgnoreCase)
        || Arguments.Contains(TEXT("SeedForgeGameplayTrace="), ESearchCase::IgnoreCase)
        || Arguments.Contains(TEXT("SeedForgeGameplayCaptureDir="), ESearchCase::IgnoreCase))
    {
        OutError = TEXT("Input self-test cannot run with gameplay-smoke or capture-auto-exit arguments.");
        return false;
    }
    ESeedForgeInputSourceKind Kind;
    FString Revision;
    if (!ParseSourceIdentity(OutOptions.SourceIdentity, Kind, Revision))
    {
        OutError = TEXT("Input self-test requires a 40-hex or diagnostic-<40hex> source identity.");
        return false;
    }
    if (OutOptions.TracePath.IsEmpty() || FPaths::IsRelative(OutOptions.TracePath)
        || !FPaths::GetExtension(OutOptions.TracePath).Equals(TEXT("json"), ESearchCase::IgnoreCase))
    {
        OutError = TEXT("Input self-test requires an absolute .json SeedForgeInputTrace path.");
        return false;
    }
    OutOptions.TracePath = FPaths::ConvertRelativePathToFull(OutOptions.TracePath);
    FPaths::MakeStandardFilename(OutOptions.TracePath);
    FString SeedText;
    if (FParse::Value(CommandLine, TEXT("SeedForgeSeed="), SeedText))
    {
        if (!LexTryParseString(OutOptions.Seed, *SeedText)
            || SeedText != FString::Printf(TEXT("%llu"), OutOptions.Seed))
        {
            OutError = TEXT("SeedForgeSeed must be a canonical uint64 decimal value.");
            return false;
        }
    }
    return true;
}

bool FSeedForgeInputSelfTestCodec::ValidateEvidence(const FSeedForgeInputSelfTestTrace& Trace, FString& OutError)
{
    if (Trace.InputEvents.Num() > FSeedForgeInputSelfTestTrace::MaxInputEvents
        || Trace.Transitions.Num() > FSeedForgeInputSelfTestTrace::MaxTransitions
        || Trace.QueuedRuns.Num() > FSeedForgeInputSelfTestTrace::MaxQueuedRuns
        || Trace.Runs.Num() > FSeedForgeInputSelfTestTrace::MaxRuns
        || Trace.Setups.Num() > FSeedForgeInputSelfTestTrace::MaxSetups
        || Trace.WalkableCells.Num() > FSeedForgeInputSelfTestTrace::MaxWalkableCells
        || Trace.PathEvidence.Cells.Num() > FSeedForgeEnemyPathProof::MaxPathCells
        || Trace.PathEvidence.Waypoints.Num() >= FSeedForgeEnemyPathProof::MaxPathCells
        || Trace.PathEvidence.MovementSamples.Num() > FSeedForgeEnemyPathProof::MaxRetainedMovementSamples)
    {
        OutError = TEXT("ObservationLimitExceeded");
        return false;
    }
    for (const FSeedForgeInputRunObservation& Run : Trace.Runs)
    {
        if (Run.Actors.Num() > FSeedForgeInputRunObservation::MaxActors)
        {
            OutError = TEXT("ObservationLimitExceeded");
            return false;
        }
    }
    using namespace SeedForge::InputSelfTest::Private;
    auto Reject = [&](const TCHAR* Message) { OutError = Message; return false; };
    ESeedForgeInputSourceKind Kind;
    FString Revision;
    if (!ParseSourceIdentity(Trace.SourceIdentity, Kind, Revision) || Kind != Trace.SourceKind || Revision != Trace.SourceRevision)
    { return Reject(TEXT("SourceIdentityMismatch")); }
    if (!Trace.bSuccess || Trace.FailureCode != ESeedForgeInputSelfTestFailure::None || !Trace.FailureMessage.IsEmpty()
        || Trace.Mode != TEXT("ordinary") || Trace.bGameplaySmokeEnabled || Trace.CompletionCount != 1
        || Trace.RemainingDelegateBindings != 0 || Trace.RemainingPressedKeys != 0
        || Trace.StartedAtUtc.GetTicks() <= 0 || Trace.CompletedAtUtc < Trace.StartedAtUtc
        || (Trace.CompletedAtUtc - Trace.StartedAtUtc).GetTotalSeconds() > 30.0
        || Trace.CompletedFrame < Trace.StartedFrame)
    { return Reject(TEXT("InvalidCompletionEnvelope")); }
    if (Trace.Runs.Num() != 4 || Trace.InputEvents.Num() != 13 || Trace.QueuedRuns.Num() != 4 || Trace.Transitions.Num() != 10)
    { return Reject(TEXT("MissingInputEvidence")); }
    auto InEnvelope = [&](uint64 Frame, const FDateTime& Time)
    { return Frame >= Trace.StartedFrame && Frame <= Trace.CompletedFrame && Time >= Trace.StartedAtUtc && Time <= Trace.CompletedAtUtc; };
    if (!SameSnapshot(Trace.Initial, Trace.Runs[0].Snapshot) || !SameSnapshot(Trace.Final, Trace.Runs[3].Snapshot)
        || Trace.Initial.Seed != Trace.ExpectedInitialSeed || Trace.Initial.RunGeneration == 0 || Trace.Initial.RunGeneration > MAX_uint64 - 4)
    { return Reject(TEXT("InitialFinalMismatch")); }
    const uint64 Seeds[] = {Trace.ExpectedInitialSeed, Trace.ExpectedInitialSeed, NextSeed(Trace.ExpectedInitialSeed), NextSeed(NextSeed(Trace.ExpectedInitialSeed))};
    const uint64 RunOffsets[] = {0, 1, 2, 4};
    const TCHAR* Labels[] = {TEXT("initial"), TEXT("same"), TEXT("new"), TEXT("rapid")};
    TSet<FString> ActorKeys;
    FString PlayerKey;
    FString EnemyKey;
    uint64 LastFrame = Trace.StartedFrame;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const auto& R = Trace.Runs[Index]; const auto& S = R.Snapshot;
        const auto Layout = FSeedForgeGenerator::Generate(Seeds[Index], {});
        const auto Encounter = FSeedForgeEncounterPlanner::Generate(Layout.Layout, {});
        if (!Layout.IsSuccess() || !Encounter.IsSuccess() || R.Label != Labels[Index] || !InEnvelope(R.Frame, R.AtUtc) || R.Frame < LastFrame
            || S.Seed != Seeds[Index] || S.LayoutHash != Layout.Layout.CanonicalHash || S.EncounterHash != Encounter.Plan.CanonicalHash
            || S.RunGeneration != Trace.Initial.RunGeneration + RunOffsets[Index] || S.AppliedRequestId == 0 || S.PendingRequestId != 0
            || (Index > 0 && S.AppliedRequestId <= Trace.Runs[Index - 1].Snapshot.AppliedRequestId)
            || S.RunState != ESeedForgeRunState::Playing || S.FailureCode != ESeedForgeRunFailureCode::None || !S.FailureMessage.IsEmpty()
            || S.PlayerHealth != 100 || S.PlayerMaxHealth != 100 || S.CollectedCoreCount != 0 || S.RequiredCoreCount != 3 || S.bExitUnlocked
            || !R.Resources.bInteractionTimerActive || !R.Resources.bRepathTimerActive || R.Resources.AttackCooldownRemaining != 0
            || R.DashCooldownRemaining != 0 || R.CoordinatorCount != 1 || R.ControllerCount != 1 || R.HudCount != 1 || R.HudOverlayCount != 1
            || R.CoordinatorKey.IsEmpty() || R.ControllerKey.IsEmpty() || R.HudKey.IsEmpty() || R.Actors.Num() != 11
            || (Index > 0 && (!R.bPreviousActorsDestroyed || R.CoordinatorKey != Trace.Runs[0].CoordinatorKey
                || R.ControllerKey != Trace.Runs[0].ControllerKey || R.HudKey != Trace.Runs[0].HudKey)))
        { return Reject(TEXT("RunIdentityOwnershipOrResetMismatch")); }
        if (Index == 0 && Trace.WalkableCells != Layout.Layout.GetCanonicalWalkableCells()) { return Reject(TEXT("WalkableIdentityMismatch")); }
        TSet<FString> Roles;
        for (const auto& A : R.Actors)
        {
            const FString RoleId = FString::Printf(TEXT("%s:%u"), *A.Role, A.StableId);
            if (A.Key.IsEmpty() || A.Key.Len() > 256 || ActorKeys.Contains(A.Key) || Roles.Contains(RoleId)
                || A.OwnerKey != (A.Role == TEXT("player") ? R.ControllerKey : R.CoordinatorKey))
            { return Reject(TEXT("DuplicateOrWrongActorOwnership")); }
            ActorKeys.Add(A.Key); Roles.Add(RoleId);
            FIntPoint ExpectedCell;
            if (A.Role == TEXT("player") && A.StableId == 0) { ExpectedCell = Layout.Layout.Entrance; if (Index == 0) { PlayerKey = A.Key; } }
            else if (A.Role == TEXT("visualization") && A.StableId == 0) { ExpectedCell = FIntPoint::ZeroValue; }
            else if (A.Role == TEXT("exit") && A.StableId == 0) { ExpectedCell = Layout.Layout.Exit; }
            else if (A.Role == TEXT("core") && A.StableId < static_cast<uint32>(Encounter.Plan.DataCores.Num())) { ExpectedCell = Encounter.Plan.DataCores[static_cast<int32>(A.StableId)].Cell; }
            else if (A.Role == TEXT("enemy") && A.StableId < static_cast<uint32>(Encounter.Plan.Enemies.Num()))
            { ExpectedCell = Encounter.Plan.Enemies[static_cast<int32>(A.StableId)].Cell; if (Index == 0 && A.StableId == Trace.PathEvidence.StableId) { EnemyKey = A.Key; } }
            else { return Reject(TEXT("InvalidActorRole")); }
            if (A.Cell != ExpectedCell) { return Reject(TEXT("ActorCellMismatch")); }
        }
        LastFrame = R.Frame;
    }
    const auto& P = Trace.PathEvidence;
    if (!P.bComplete || P.Status != ESeedForgePathStatus::Success || P.RunGeneration != Trace.Initial.RunGeneration
        || P.SourceRequestId != Trace.Initial.AppliedRequestId || P.PathRevision == 0 || P.Cells.Num() < 2
        || P.Waypoints.Num() != P.Cells.Num() - 1 || P.MovementSamples.Num() != 2 || P.ObservedMoveCount < 2
        || !FMath::IsFinite(P.TotalDistance) || P.TotalDistance < 20 || !FMath::IsFinite(P.TotalDeltaSeconds) || P.TotalDeltaSeconds <= 0
        || P.TotalDistance > 260.0 * P.TotalDeltaSeconds + 0.1 * static_cast<double>(P.ObservedMoveCount) || EnemyKey.IsEmpty())
    { return Reject(TEXT("InvalidPathProof")); }
    FSeedForgePathRequest PathRequest; PathRequest.Start = P.Start; PathRequest.Goal = P.Goal; PathRequest.WalkableCells = Trace.WalkableCells; PathRequest.MaxExpandedNodes = 1024;
    const auto ExpectedPath = FSeedForgeGridPathfinder::FindPath(PathRequest);
    if (ExpectedPath.Status != ESeedForgePathStatus::Success || P.Cells != ExpectedPath.Path || P.ExpandedNodes != ExpectedPath.ExpandedNodes)
    { return Reject(TEXT("PathDoesNotMatchAStar")); }
    for (int32 I = 0; I < P.Waypoints.Num(); ++I)
    { if (!Finite(P.Waypoints[I]) || !P.Waypoints[I].Equals(FSeedForgeGameplayMath::CellToWorld(P.Cells[I + 1], 200, 58), 0.1)) { return Reject(TEXT("WaypointMismatch")); } }
    uint64 Sequence = 0; uint64 MoveFrame = Trace.StartedFrame;
    for (const auto& M : P.MovementSamples)
    {
        if (M.Sequence <= Sequence || M.Frame < MoveFrame || M.Frame > Trace.CompletedFrame || M.PathRevision != P.PathRevision
            || !P.Waypoints.IsValidIndex(M.WaypointIndex) || !Finite(M.From) || !Finite(M.Target) || !Finite(M.To)
            || !FMath::IsFinite(M.DeltaSeconds) || M.DeltaSeconds <= 0 || !M.Target.Equals(P.Waypoints[M.WaypointIndex], 0.1)
            || FVector::Dist(M.From, M.To) <= 0 || FVector::Dist(M.From, M.To) > 260.0 * M.DeltaSeconds + 0.1
            || !M.To.Equals(FMath::ClosestPointOnSegment(M.To, M.From, M.Target), 0.1)
            || !OnCells(M.From, P.Cells) || !OnCells(M.To, P.Cells)) { return Reject(TEXT("InvalidMovementSample")); }
        Sequence = M.Sequence; MoveFrame = M.Frame;
    }
    if (P.ObservedMoveCount > P.MovementSamples.Last().Sequence - P.MovementSamples[0].Sequence + 1) { return Reject(TEXT("InvalidMovementCount")); }
    const ESeedForgeRunState PriorStates[] = {ESeedForgeRunState::Lost, ESeedForgeRunState::Playing, ESeedForgeRunState::Playing, ESeedForgeRunState::Generating};
    const uint64 QueueSeeds[] = {Seeds[1], Seeds[2], Seeds[3], Seeds[3]};
    uint64 LastRequest = Trace.Initial.AppliedRequestId;
    for (int32 I = 0; I < 4; ++I)
    {
        const auto& Q = Trace.QueuedRuns[I]; const auto& S = Q.Snapshot;
        if (Q.StateBefore != PriorStates[I] || !InEnvelope(Q.Frame, Q.AtUtc) || S.RunState != ESeedForgeRunState::Generating
            || S.RunGeneration != Trace.Initial.RunGeneration + I + 1 || S.Seed != QueueSeeds[I]
            || S.PendingRequestId <= LastRequest || S.AppliedRequestId || S.LayoutHash || S.EncounterHash)
        { return Reject(TEXT("QueuedIdentityMismatch")); }
        LastRequest = S.PendingRequestId;
    }
    if (Trace.QueuedRuns[0].Snapshot.PendingRequestId != Trace.Runs[1].Snapshot.AppliedRequestId
        || Trace.QueuedRuns[1].Snapshot.PendingRequestId != Trace.Runs[2].Snapshot.AppliedRequestId
        || Trace.QueuedRuns[3].Snapshot.PendingRequestId != Trace.Final.AppliedRequestId
        || Trace.QueuedRuns[2].Frame != Trace.QueuedRuns[3].Frame) { return Reject(TEXT("AppliedOrRapidIdentityMismatch")); }
    const ESeedForgeRunState From[] = {ESeedForgeRunState::Playing,ESeedForgeRunState::Lost,ESeedForgeRunState::Restarting,ESeedForgeRunState::Generating,
        ESeedForgeRunState::Playing,ESeedForgeRunState::Restarting,ESeedForgeRunState::Generating,ESeedForgeRunState::Playing,ESeedForgeRunState::Restarting,ESeedForgeRunState::Generating};
    const ESeedForgeRunState To[] = {ESeedForgeRunState::Lost,ESeedForgeRunState::Restarting,ESeedForgeRunState::Generating,ESeedForgeRunState::Playing,
        ESeedForgeRunState::Restarting,ESeedForgeRunState::Generating,ESeedForgeRunState::Playing,ESeedForgeRunState::Restarting,ESeedForgeRunState::Generating,ESeedForgeRunState::Playing};
    LastFrame = Trace.StartedFrame;
    for (int32 I = 0; I < 10; ++I)
    {
        const auto& T = Trace.Transitions[I];
        if (T.From != From[I] || T.To != To[I] || T.Snapshot.RunState != T.To || !InEnvelope(T.Frame, T.AtUtc) || T.Frame < LastFrame)
        { return Reject(TEXT("TransitionSequenceMismatch")); }
        if (I == 0)
        {
            auto Lost = Trace.Initial; Lost.RunState = ESeedForgeRunState::Lost; Lost.PlayerHealth = 0;
            if (!SameSnapshot(T.Snapshot, Lost)) { return Reject(TEXT("LostSnapshotMismatch")); }
        }
        else if (I == 3 || I == 6 || I == 9)
        {
            if (!SameSnapshot(T.Snapshot, Trace.Runs[I / 3].Snapshot)) { return Reject(TEXT("PlayingTransitionIdentityMismatch")); }
        }
        else
        {
            const auto& Q = Trace.QueuedRuns[(I - 1) / 3];
            if (T.Snapshot.RunGeneration != Q.Snapshot.RunGeneration || T.Snapshot.Seed != Q.Snapshot.Seed
                || T.Snapshot.LayoutHash || T.Snapshot.EncounterHash || T.Snapshot.AppliedRequestId || T.Snapshot.PendingRequestId
                || T.Frame != Q.Frame || T.AtUtc > Q.AtUtc) { return Reject(TEXT("PendingTransitionIdentityMismatch")); }
        }
        LastFrame = T.Frame;
    }
    LastFrame = Trace.StartedFrame;
    for (int32 I = 0; I < 13; ++I)
    {
        const auto& E = Trace.InputEvents[I];
        const uint64 Run = I < 9 ? Trace.Initial.RunGeneration : Trace.QueuedRuns[I - 9].Snapshot.RunGeneration;
        const uint64 Req = I < 9 ? Trace.Initial.AppliedRequestId : Trace.QueuedRuns[I - 9].Snapshot.PendingRequestId;
        if (E.Action != Actions[I] || E.Key != Keys[I] || !E.bConfirmed || E.RunGeneration != Run || E.SourceRequestId != Req
            || !InEnvelope(E.InjectedFrame, E.InjectedAtUtc) || !InEnvelope(E.EffectFrame, E.EffectAtUtc)
            || E.InjectedFrame > E.EffectFrame || E.InjectedAtUtc > E.EffectAtUtc || E.InjectedFrame < LastFrame
            || (I >= 2 && (E.ReleaseFrame <= E.InjectedFrame || E.ReleaseFrame > Trace.CompletedFrame))
            || !Finite(E.Before) || !Finite(E.After) || !Finite(E.LaunchVelocity)
            || !FMath::IsFinite(E.CooldownBefore) || !FMath::IsFinite(E.CooldownAfter)) { return Reject(TEXT("InputAttributionOrSequenceMismatch")); }
        if (I < 2 && (E.ActorKey != PlayerKey || E.ReleaseFrame != 0 || !E.After.Equals(I == 0 ? FVector::ForwardVector : FVector::RightVector, 0.05))) { return Reject(TEXT("AimEffectMissing")); }
        if (I == 2 || I == 3)
        {
            if (E.ActorKey != EnemyKey || !E.bTargetAliveBefore || E.bTargetAliveAfter != (I == 2)
                || E.LiveEnemiesBefore != 5 || E.LiveEnemiesAfter != (I == 2 ? 5 : 4) || E.CooldownBefore > 0.01 || E.CooldownAfter <= 0 || E.CooldownAfter > 0.47)
            { return Reject(TEXT("AttackEffectMissing")); }
            if (I == 3 && (E.InjectedAtUtc - Trace.InputEvents[2].InjectedAtUtc).GetTotalSeconds() < 0.448) { return Reject(TEXT("AttackCooldownNotObserved")); }
        }
        if (I >= 4 && I <= 7)
        {
            const FVector Delta = E.After - E.Before;
            if (E.ActorKey != PlayerKey || Delta.Size2D() < 19.9 || Delta.Size2D() > 80
                || FVector::DotProduct(Delta.GetSafeNormal2D(), MoveDirections[I - 4]) < 0.98
                || !OnCells(E.Before, Trace.WalkableCells, 42) || !OnCells(E.After, Trace.WalkableCells, 42)) { return Reject(TEXT("MovementEffectMissing")); }
        }
        if (I == 8)
        {
            // Finite endpoints do not imply finite subtraction or squared distance.
            const double DX = E.After.X - E.Before.X;
            const double DY = E.After.Y - E.Before.Y;
            const double DZ = E.After.Z - E.Before.Z;
            const double Distance = FMath::Sqrt(DX * DX + DY * DY);
            if (!FMath::IsFinite(DX) || !FMath::IsFinite(DY) || !FMath::IsFinite(DZ) || !FMath::IsFinite(Distance)
                || Distance < 19.9 || Distance > 200) { return Reject(TEXT("DashEffectMissing")); }
            const FVector Direction(DX / Distance, DY / Distance, 0);
            if (E.ActorKey != PlayerKey || !E.LaunchVelocity.Equals(FVector(1,1,0).GetSafeNormal() * 1200, 1.0)
                || FVector::DotProduct(Direction, FVector(1,1,0).GetSafeNormal()) < 0.98
                || !OnCells(E.Before, Trace.WalkableCells) || !OnCells(E.After, Trace.WalkableCells)
                || E.CooldownBefore > 0.01 || E.CooldownAfter <= 0 || E.CooldownAfter > 1.27)
            { return Reject(TEXT("DashEffectMissing")); }
        }
        if (I >= 9 && E.ActorKey != Trace.Runs[0].ControllerKey) { return Reject(TEXT("RestartControllerMismatch")); }
        LastFrame = E.InjectedFrame;
    }
    if (Trace.InputEvents[11].InjectedFrame != Trace.InputEvents[12].InjectedFrame) { return Reject(TEXT("RapidInputsNotOneBatch")); }
    if (Trace.PathEvidence.MovementSamples.Last().Frame > Trace.InputEvents[0].InjectedFrame
        || Trace.InputEvents[8].EffectFrame > Trace.Transitions[0].Frame
        || Trace.Transitions[0].Frame > Trace.InputEvents[9].InjectedFrame) { return Reject(TEXT("MeasurementWindowMismatch")); }
    for (int32 I = 9; I < 13; ++I)
    {
        const auto& E = Trace.InputEvents[I]; const auto& Q = Trace.QueuedRuns[I-9];
        if (E.InjectedFrame != Q.Frame || E.InjectedAtUtc > Q.AtUtc || E.EffectAtUtc < Q.AtUtc)
        { return Reject(TEXT("QueuedInputCorrelationMismatch")); }
    }
    for (const auto& S : Trace.Setups)
    {
        if (S.ActorKey.IsEmpty() || !ActorKeys.Contains(S.ActorKey) || !InEnvelope(S.Frame, S.AtUtc)
            || S.RunGeneration != Trace.Initial.RunGeneration || S.SourceRequestId != Trace.Initial.AppliedRequestId
            || !Finite(S.Position) || !Trace.WalkableCells.Contains(S.Cell)
            || FMath::Abs(S.Position.X - S.Cell.X * 200.0) > 0.1 || FMath::Abs(S.Position.Y - S.Cell.Y * 200.0) > 0.1)
        { return Reject(TEXT("InvalidSetupEvidence")); }
    }
    OutError.Reset();
    return true;
}

FString FSeedForgeInputSelfTestCodec::ExportCanonicalJson(const FSeedForgeInputSelfTestTrace& Trace)
{
    using namespace SeedForge::InputSelfTest::Private;
    ESeedForgeInputSourceKind Kind = ESeedForgeInputSourceKind::Unverified;
    FString Revision;
    const bool bValidSource = ParseSourceIdentity(Trace.SourceIdentity, Kind, Revision);
    FString ValidationError;
    const bool bPassed = Trace.bSuccess && ValidateEvidence(Trace, ValidationError);
    ESeedForgeInputSelfTestFailure Failure = Trace.FailureCode;
    if (!bValidSource) { Failure = ESeedForgeInputSelfTestFailure::InvalidSourceIdentity; }
    else if (bPassed) { Failure = ESeedForgeInputSelfTestFailure::None; }
    else if (Trace.bSuccess || Failure == ESeedForgeInputSelfTestFailure::None)
    {
        Failure = ESeedForgeInputSelfTestFailure::MissingInputEvidence;
    }
    FString Json;
    const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
    Writer->WriteObjectStart();
    Writer->WriteValue(TEXT("schema"), TEXT("seedforge.input-selftest"));
    Writer->WriteValue(TEXT("schemaVersion"), 1);
    Writer->WriteValue(TEXT("sourceIdentity"), Trace.SourceIdentity.Left(128));
    Writer->WriteValue(TEXT("sourceRevision"), Revision);
    Writer->WriteValue(TEXT("sourceKind"), SourceKindName(Kind));
    // Native syntax classification is not proof that a checkout/build was clean.
    Writer->WriteValue(TEXT("sourceVerified"), false);
    Writer->WriteValue(TEXT("engineVersion"), Trace.EngineVersion.Left(128));
    Writer->WriteValue(TEXT("mode"), Trace.Mode.Left(32));
    Writer->WriteValue(TEXT("gameplaySmokeEnabled"), Trace.bGameplaySmokeEnabled);
    Writer->WriteValue(TEXT("expectedInitialSeed"), FString::Printf(TEXT("%llu"), Trace.ExpectedInitialSeed));
    Writer->WriteValue(TEXT("startedAtUtc"), Trace.StartedAtUtc.ToIso8601());
    Writer->WriteValue(TEXT("completedAtUtc"), Trace.CompletedAtUtc.ToIso8601());
    Writer->WriteValue(TEXT("startedFrame"), FString::Printf(TEXT("%llu"), Trace.StartedFrame));
    Writer->WriteValue(TEXT("completedFrame"), FString::Printf(TEXT("%llu"), Trace.CompletedFrame));
    Writer->WriteValue(TEXT("completionCount"), Trace.CompletionCount);
    Writer->WriteValue(TEXT("remainingDelegateBindings"), Trace.RemainingDelegateBindings);
    Writer->WriteValue(TEXT("remainingPressedKeys"), Trace.RemainingPressedKeys);
    Snapshot(Writer,TEXT("initial"),Trace.Initial); Snapshot(Writer,TEXT("final"),Trace.Final); Path(Writer,Trace.PathEvidence);
    Writer->WriteArrayStart(TEXT("walkableCells")); for (int32 I=0; I<FMath::Min(Trace.WalkableCells.Num(),FSeedForgeInputSelfTestTrace::MaxWalkableCells); ++I) { Writer->WriteObjectStart(); Cell(Writer,Trace.WalkableCells[I]); Writer->WriteObjectEnd(); } Writer->WriteArrayEnd();
    Writer->WriteArrayStart(TEXT("runs")); for (int32 I=0; I<FMath::Min(Trace.Runs.Num(),FSeedForgeInputSelfTestTrace::MaxRuns); ++I)
    {
        const auto& R=Trace.Runs[I]; Writer->WriteObjectStart(); Writer->WriteValue(TEXT("label"),R.Label); UInt(Writer,TEXT("frame"),R.Frame); Writer->WriteValue(TEXT("atUtc"),R.AtUtc.ToIso8601()); Snapshot(Writer,TEXT("snapshot"),R.Snapshot);
        Writer->WriteObjectStart(TEXT("resources")); Writer->WriteValue(TEXT("interactionTimerActive"),R.Resources.bInteractionTimerActive); Writer->WriteValue(TEXT("repathTimerActive"),R.Resources.bRepathTimerActive); Number(Writer,TEXT("attackCooldownRemaining"),R.Resources.AttackCooldownRemaining); Writer->WriteObjectEnd();
        Number(Writer,TEXT("dashCooldownRemaining"),R.DashCooldownRemaining); Writer->WriteValue(TEXT("coordinatorKey"),R.CoordinatorKey); Writer->WriteValue(TEXT("controllerKey"),R.ControllerKey); Writer->WriteValue(TEXT("hudKey"),R.HudKey);
        Writer->WriteValue(TEXT("coordinatorCount"),R.CoordinatorCount); Writer->WriteValue(TEXT("controllerCount"),R.ControllerCount); Writer->WriteValue(TEXT("hudCount"),R.HudCount); Writer->WriteValue(TEXT("hudOverlayCount"),R.HudOverlayCount); Writer->WriteValue(TEXT("previousActorsDestroyed"),R.bPreviousActorsDestroyed);
        Writer->WriteArrayStart(TEXT("actors")); for (int32 J=0; J<FMath::Min(R.Actors.Num(),FSeedForgeInputRunObservation::MaxActors); ++J)
        { const auto& A=R.Actors[J]; Writer->WriteObjectStart(); Writer->WriteValue(TEXT("key"),A.Key.Left(256)); Writer->WriteValue(TEXT("role"),A.Role); Writer->WriteValue(TEXT("stableId"),A.StableId); Writer->WriteValue(TEXT("ownerKey"),A.OwnerKey.Left(256)); Writer->WriteObjectStart(TEXT("cell")); Cell(Writer,A.Cell); Writer->WriteObjectEnd(); Writer->WriteObjectEnd(); } Writer->WriteArrayEnd(); Writer->WriteObjectEnd();
    } Writer->WriteArrayEnd();
    Writer->WriteArrayStart(TEXT("setups")); for (int32 I=0; I<FMath::Min(Trace.Setups.Num(),FSeedForgeInputSelfTestTrace::MaxSetups); ++I)
    { const auto& S=Trace.Setups[I]; Writer->WriteObjectStart(); Writer->WriteValue(TEXT("label"),S.Label); Writer->WriteValue(TEXT("actorKey"),S.ActorKey.Left(256)); UInt(Writer,TEXT("runGeneration"),S.RunGeneration); UInt(Writer,TEXT("sourceRequestId"),S.SourceRequestId); UInt(Writer,TEXT("frame"),S.Frame); Writer->WriteValue(TEXT("atUtc"),S.AtUtc.ToIso8601()); Writer->WriteObjectStart(TEXT("cell")); Cell(Writer,S.Cell); Writer->WriteObjectEnd(); Vector(Writer,TEXT("position"),S.Position); Writer->WriteObjectEnd(); } Writer->WriteArrayEnd();
    Writer->WriteArrayStart(TEXT("inputEvents")); for (int32 I=0; I<FMath::Min(Trace.InputEvents.Num(),FSeedForgeInputSelfTestTrace::MaxInputEvents); ++I)
    {
        const auto& E=Trace.InputEvents[I]; Writer->WriteObjectStart(); Writer->WriteValue(TEXT("action"),E.Action); Writer->WriteValue(TEXT("key"),E.Key); Writer->WriteValue(TEXT("actorKey"),E.ActorKey.Left(256)); UInt(Writer,TEXT("runGeneration"),E.RunGeneration); UInt(Writer,TEXT("sourceRequestId"),E.SourceRequestId);
        UInt(Writer,TEXT("injectedFrame"),E.InjectedFrame); UInt(Writer,TEXT("effectFrame"),E.EffectFrame); UInt(Writer,TEXT("releaseFrame"),E.ReleaseFrame); Writer->WriteValue(TEXT("injectedAtUtc"),E.InjectedAtUtc.ToIso8601()); Writer->WriteValue(TEXT("effectAtUtc"),E.EffectAtUtc.ToIso8601());
        Vector(Writer,TEXT("before"),E.Before); Vector(Writer,TEXT("after"),E.After); Vector(Writer,TEXT("launchVelocity"),E.LaunchVelocity); Number(Writer,TEXT("cooldownBefore"),E.CooldownBefore); Number(Writer,TEXT("cooldownAfter"),E.CooldownAfter); Writer->WriteValue(TEXT("liveEnemiesBefore"),E.LiveEnemiesBefore); Writer->WriteValue(TEXT("liveEnemiesAfter"),E.LiveEnemiesAfter); Writer->WriteValue(TEXT("targetAliveBefore"),E.bTargetAliveBefore); Writer->WriteValue(TEXT("targetAliveAfter"),E.bTargetAliveAfter); Writer->WriteValue(TEXT("confirmed"),E.bConfirmed); Writer->WriteObjectEnd();
    } Writer->WriteArrayEnd();
    Writer->WriteArrayStart(TEXT("transitions")); for (int32 I=0; I<FMath::Min(Trace.Transitions.Num(),FSeedForgeInputSelfTestTrace::MaxTransitions); ++I)
    { const auto& T=Trace.Transitions[I]; Writer->WriteObjectStart(); Writer->WriteValue(TEXT("from"),StateName(T.From)); Writer->WriteValue(TEXT("to"),StateName(T.To)); Snapshot(Writer,TEXT("snapshot"),T.Snapshot); UInt(Writer,TEXT("frame"),T.Frame); Writer->WriteValue(TEXT("atUtc"),T.AtUtc.ToIso8601()); Writer->WriteObjectEnd(); } Writer->WriteArrayEnd();
    Writer->WriteArrayStart(TEXT("queuedRuns")); for (int32 I=0; I<FMath::Min(Trace.QueuedRuns.Num(),FSeedForgeInputSelfTestTrace::MaxQueuedRuns); ++I)
    { const auto& Q=Trace.QueuedRuns[I]; Writer->WriteObjectStart(); Writer->WriteValue(TEXT("stateBefore"),StateName(Q.StateBefore)); Snapshot(Writer,TEXT("snapshot"),Q.Snapshot); UInt(Writer,TEXT("frame"),Q.Frame); Writer->WriteValue(TEXT("atUtc"),Q.AtUtc.ToIso8601()); Writer->WriteObjectEnd(); } Writer->WriteArrayEnd();
    Writer->WriteValue(TEXT("result"), bPassed ? TEXT("Passed") : TEXT("Failed"));
    Writer->WriteValue(TEXT("failureCode"), bPassed ? TEXT("") : FailureName(Failure));
    Writer->WriteValue(TEXT("failureMessage"), bPassed ? FString() : (ValidationError.IsEmpty() ? Trace.FailureMessage.Left(1024) : ValidationError));
    Writer->WriteObjectEnd();
    Writer->Close();
    return Json;
}

USeedForgeInputSelfTestComponent::USeedForgeInputSelfTestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool USeedForgeInputSelfTestComponent::Start(const FString& SourceIdentity, uint64 ExpectedInitialSeed)
{
    if (bStarted) { return false; }
    bStarted = true;
    Trace = {};
    Trace.SourceIdentity = SourceIdentity.Left(128);
    Trace.ExpectedInitialSeed = ExpectedInitialSeed;
    Trace.StartedAtUtc = FDateTime::UtcNow();
    Trace.StartedFrame = GFrameCounter;
    Trace.EngineVersion = FEngineVersion::Current().ToString();
    if (!FSeedForgeInputSelfTestCodec::ParseSourceIdentity(SourceIdentity, Trace.SourceKind, Trace.SourceRevision))
    {
        FinishFailure(ESeedForgeInputSelfTestFailure::InvalidSourceIdentity,
            TEXT("Source identity must be 40 hex digits or diagnostic- followed by 40 hex digits."));
        return false;
    }
    UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    if (!ViewportClient || !ViewportClient->GetGameViewport())
    {
        FinishFailure(ESeedForgeInputSelfTestFailure::ViewportUnavailable,
            TEXT("Ordinary input proof requires the owning real scene viewport."));
        return false;
    }
    Controller = Cast<ASeedForgePlayerController>(GetOwner());
    if (!Controller.IsValid())
    { FinishFailure(ESeedForgeInputSelfTestFailure::InvalidArguments, TEXT("Input proof must belong to the production controller.")); return false; }
    StartedSeconds = FPlatformTime::Seconds();
    WorldHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &USeedForgeInputSelfTestComponent::ObserveWorld);
    return true;
}

bool USeedForgeInputSelfTestComponent::CheckBudget()
{
    if (!IsRunning()) { return false; }
    if (FPlatformTime::Seconds() - StartedSeconds > 30.0)
    {
        FinishFailure(ESeedForgeInputSelfTestFailure::SelfTestTimeout,
            FString::Printf(TEXT("Ordinary input observation exceeded 30 seconds (phase=%d effect=%d)."), static_cast<int32>(Phase), EffectIndex));
        return false;
    }
    return true;
}

bool USeedForgeInputSelfTestComponent::ResolveCoordinator()
{
    if (Coordinator.IsValid()) { return true; }
    ASeedForgeGameplayCoordinator* Found = nullptr;
    for (TActorIterator<ASeedForgeGameplayCoordinator> It(GetWorld()); It; ++It)
    {
        if (!SeedForge::InputSelfTest::Private::Live(*It)) { continue; }
        if (Found) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Duplicate Coordinator.")); return false; }
        Found = *It;
    }
    if (!Found) { return false; }
    Coordinator = Found;
    TransitionHandle = Found->OnRunStateChanged().AddUObject(this, &USeedForgeInputSelfTestComponent::ObserveTransition);
    QueueHandle = Found->OnRunQueued().AddUObject(this, &USeedForgeInputSelfTestComponent::ObserveQueued);
    PathHandle = Found->OnEnemyPathApplied().AddUObject(this, &USeedForgeInputSelfTestComponent::ObservePath);
    return true;
}

void USeedForgeInputSelfTestComponent::ObserveTransition(ESeedForgeRunState From, ESeedForgeRunState To,
    const FSeedForgeGameplaySnapshot& Snapshot)
{
    if (!IsRunning() || Trace.Runs.IsEmpty()) { return; }
    if (Trace.Transitions.Num() >= FSeedForgeInputSelfTestTrace::MaxTransitions)
    { FinishFailure(ESeedForgeInputSelfTestFailure::ObservationLimitExceeded, TEXT("Transition observation cap.")); return; }
    Trace.Transitions.Add({From, To, Snapshot, GFrameCounter, FDateTime::UtcNow()});
}

void USeedForgeInputSelfTestComponent::ObserveQueued(ESeedForgeRunState StateBefore, const FSeedForgeGameplaySnapshot& Snapshot)
{
    if (!IsRunning() || Trace.Runs.IsEmpty()) { return; }
    if (Trace.QueuedRuns.Num() >= FSeedForgeInputSelfTestTrace::MaxQueuedRuns)
    { FinishFailure(ESeedForgeInputSelfTestFailure::ObservationLimitExceeded, TEXT("Queued-run observation cap.")); return; }
    Trace.QueuedRuns.Add({StateBefore, Snapshot, GFrameCounter, FDateTime::UtcNow()});
}

void USeedForgeInputSelfTestComponent::ObservePath(ASeedForgeEnemyPawn* Enemy, uint64 Run, uint64 Request,
    const FIntPoint& Start, const FIntPoint& Goal, const FSeedForgePathResult& Result)
{
    if (!IsRunning() || Phase != EPhase::AwaitPath || Enemy != TargetEnemy.Get()
        || Run != Trace.Initial.RunGeneration || Request != Trace.Initial.AppliedRequestId) { return; }
    PathProof.ObserveAppliedPath(Enemy->GetStableId(), Run, Request, Start, Goal, Result,
        Enemy->GetPathSnapshot(), Trace.WalkableCells, Coordinator->GetTuning().CellSize);
}

bool USeedForgeInputSelfTestComponent::CaptureRun(const FString& Label)
{
    using namespace SeedForge::InputSelfTest::Private;
    ASeedForgePlayerCharacter* Player = Cast<ASeedForgePlayerCharacter>(Controller->GetPawn());
    ASeedForgeHUD* Hud = Cast<ASeedForgeHUD>(Controller->GetHUD());
    UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    const auto Widget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
    if (!Live(Player) || !Live(Hud) || !Widget.IsValid()) { return false; }
    FSeedForgeInputRunObservation R;
    int32 Visited = 0;
    R.HudOverlayCount = CountHudWidgets(Widget.ToSharedRef(), 0, Visited);
    if (R.HudOverlayCount == 0) { return false; } // Observe normal HUD construction, no settling delay.
    R.Label = Label; R.Frame = GFrameCounter; R.AtUtc = FDateTime::UtcNow();
    R.Snapshot = Coordinator->GetSnapshot(); R.Resources = Coordinator->GetRunResourceSnapshot();
    R.DashCooldownRemaining = Player->GetDashCooldownRemaining();
    R.CoordinatorKey = ActorKey(Coordinator.Get()); R.ControllerKey = ActorKey(Controller.Get()); R.HudKey = ActorKey(Hud);
    R.bPreviousActorsDestroyed = true;
    for (const auto& Old : PreviousActors) { if (Live(Old.Get())) { R.bPreviousActorsDestroyed = false; } }
    TArray<TWeakObjectPtr<AActor>> CurrentActors;
    const auto Cells = Coordinator->GetLayout().GetCanonicalWalkableCells();
    if (Cells.Num() > FSeedForgeInputSelfTestTrace::MaxWalkableCells)
    { FinishFailure(ESeedForgeInputSelfTestFailure::ObservationLimitExceeded, TEXT("Canonical walkable evidence cap.")); return false; }
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It; if (!Live(Actor)) { continue; }
        R.CoordinatorCount += Actor->IsA<ASeedForgeGameplayCoordinator>() ? 1 : 0;
        R.ControllerCount += Actor->IsA<APlayerController>() ? 1 : 0;
        R.HudCount += Actor->IsA<AHUD>() ? 1 : 0;
        FSeedForgeInputActorObservation A;
        if (Actor->IsA<ASeedForgePlayerCharacter>())
        {
            A.Role = TEXT("player");
            FSeedForgeGameplayMath::WorldToNearestWalkableCell(Actor->GetActorLocation(), Cells, Coordinator->GetTuning().CellSize, A.Cell);
        }
        else if (Actor->IsA<ASeedForgePreviewActor>()) { A.Role = TEXT("visualization"); }
        else if (const auto* Core = Cast<ASeedForgeCorePickup>(Actor)) { A.Role = TEXT("core"); A.StableId = Core->GetStableId(); A.Cell = Core->GetSpawnCell(); }
        else if (const auto* Enemy = Cast<ASeedForgeEnemyPawn>(Actor)) { A.Role = TEXT("enemy"); A.StableId = Enemy->GetStableId(); A.Cell = Enemy->GetSpawnCell(); }
        else if (const auto* Exit = Cast<ASeedForgeExitActor>(Actor)) { A.Role = TEXT("exit"); A.Cell = Exit->GetSpawnCell(); }
        else { continue; }
        if (R.Actors.Num() >= FSeedForgeInputRunObservation::MaxActors)
        { FinishFailure(ESeedForgeInputSelfTestFailure::ObservationLimitExceeded, TEXT("Live run actor cap.")); return false; }
        A.Key = ActorKey(Actor); A.OwnerKey = ActorKey(Actor->GetOwner()); R.Actors.Add(MoveTemp(A)); CurrentActors.Add(Actor);
    }
    if (Trace.Runs.Num() >= FSeedForgeInputSelfTestTrace::MaxRuns)
    { FinishFailure(ESeedForgeInputSelfTestFailure::ObservationLimitExceeded, TEXT("Run snapshot cap.")); return false; }
    PreviousActors = MoveTemp(CurrentActors);
    Trace.Runs.Add(MoveTemp(R));
    if (Trace.Runs.Num() == 1) { Trace.Initial = Trace.Runs[0].Snapshot; Trace.WalkableCells = Cells; }
    Trace.Final = Trace.Runs.Last().Snapshot;
    return true;
}

bool USeedForgeInputSelfTestComponent::PlaceActor(AActor* Actor, const FIntPoint& Cell, const FString& Label, float Height)
{
    using namespace SeedForge::InputSelfTest::Private;
    if (!Live(Actor) || !Trace.WalkableCells.Contains(Cell) || Trace.Setups.Num() >= FSeedForgeInputSelfTestTrace::MaxSetups)
    { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Invalid or excessive setup placement.")); return false; }
    const FVector Position = FSeedForgeGameplayMath::CellToWorld(Cell, Coordinator->GetTuning().CellSize, Height);
    if (!Actor->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics)
        || !Actor->GetActorLocation().Equals(Position, 0.1))
    { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Canonical setup placement failed.")); return false; }
    if (Actor == Controller->GetPawn() && Controller->PlayerCameraManager)
    { Controller->PlayerCameraManager->SetGameCameraCutThisFrame(); }
    const auto S = Coordinator->GetSnapshot();
    Trace.Setups.Add({Label, ActorKey(Actor), S.RunGeneration, S.AppliedRequestId, GFrameCounter, FDateTime::UtcNow(), Cell, Actor->GetActorLocation()});
    SetupFrame = GFrameCounter;
    return true;
}

bool USeedForgeInputSelfTestComponent::PreparePath()
{
    bool bFoundPad = false;
    const auto& Plan = Coordinator->GetEncounterPlan();
    for (const FIntPoint& Cell : Trace.WalkableCells)
    {
        if (!Trace.WalkableCells.Contains(Cell + FIntPoint(1,0)) || !Trace.WalkableCells.Contains(Cell + FIntPoint(-1,0))
            || !Trace.WalkableCells.Contains(Cell + FIntPoint(0,1)) || !Trace.WalkableCells.Contains(Cell + FIntPoint(0,-1))
            || !Trace.WalkableCells.Contains(Cell + FIntPoint(2,0))) { continue; }
        bool bSafe = true;
        for (const auto& Core : Plan.DataCores) { if (FVector2D(Cell - Core.Cell).Size() < 2.0) { bSafe = false; } }
        if (FVector2D(Cell - Plan.Exit.Cell).Size() < 2.0) { bSafe = false; }
        if (bSafe) { PadCell = Cell; bFoundPad = true; break; }
    }
    if (!bFoundPad) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("No canonical open measurement pad.")); return false; }
    TArray<ASeedForgeEnemyPawn*> Enemies;
    for (TActorIterator<ASeedForgeEnemyPawn> It(GetWorld()); It; ++It)
    { if (SeedForge::InputSelfTest::Private::Live(*It) && It->GetOwner() == Coordinator.Get()) { Enemies.Add(*It); } }
    Enemies.Sort([](const ASeedForgeEnemyPawn& A, const ASeedForgeEnemyPawn& B) { return A.GetStableId() < B.GetStableId(); });
    if (Enemies.Num() != 5) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Initial enemy ownership count.")); return false; }
    TargetEnemy = Enemies[0];
    if (!PlaceActor(Controller->GetPawn(), PadCell, TEXT("path-player"), 96)
        || !PlaceActor(Enemies[0], PadCell + FIntPoint(2,0), TEXT("path-target"), 58)) { return false; }
    TArray<FIntPoint> Far = Trace.WalkableCells;
    Far.Sort([this](const FIntPoint& A, const FIntPoint& B)
    { const double DA = FVector2D(A-PadCell).SizeSquared(); const double DB = FVector2D(B-PadCell).SizeSquared();
        return DA == DB ? (A.X == B.X ? A.Y < B.Y : A.X < B.X) : DA > DB; });
    for (int32 I = 1; I < Enemies.Num(); ++I)
    {
        if (Far.Num() < I || FVector2D(Far[I-1]-PadCell).Size() < 8.0
            || !PlaceActor(Enemies[I], Far[I-1], TEXT("distant-live-enemy"), 58))
        { if (IsRunning()) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("No distant live-enemy setup cells.")); } return false; }
    }
    PathProof.Reset(); Phase = EPhase::AwaitPath;
    return true;
}

void USeedForgeInputSelfTestComponent::BeforeInput(float DeltaSeconds, bool bGamePaused)
{
    if (!CheckBudget() || bGamePaused || DeltaSeconds <= 0) { return; }
    ReleaseInputs(false);
    if (bHeldPointerIntent)
    {
        const APawn* Player = Controller->GetPawn();
        if (!Player) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Held pointer intent lost its player.")); return; }
        // Slate may refresh this viewport cache from its hardware cursor between
        // game frames. Re-feed held intent before normal input/PlayerTick; this is
        // not another proof event and never changes the OS cursor or gameplay aim.
        if (!InjectPointer(Player->GetActorLocation() + HeldPointerDirection * 300)) { return; }
    }
    if (Phase == EPhase::Ready && bPrepared && HeldInputs.IsEmpty() && GFrameCounter > SetupFrame) { BeginEffect(); }
}

void USeedForgeInputSelfTestComponent::InjectKey(const FKey& Key, int32 EventIndex, bool bReleaseNextFrame)
{
    UGameViewportClient* V = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    if (!V || !V->GetGameViewport()) { FinishFailure(ESeedForgeInputSelfTestFailure::ViewportUnavailable, TEXT("Viewport lost during key injection.")); return; }
    HeldInputs.Add({Key, EventIndex, GFrameCounter, bReleaseNextFrame});
    V->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0f, -1,
        IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false, V->GetGameViewport()));
}

void USeedForgeInputSelfTestComponent::ReleaseInputs(bool bAll)
{
    UGameViewportClient* V = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    for (int32 I = HeldInputs.Num()-1; I >= 0; --I)
    {
        const FHeldInput Held = HeldInputs[I];
        if (!bAll && (!Held.bReleaseNextFrame || GFrameCounter <= Held.Frame)) { continue; }
        if (V && V->GetGameViewport())
        { V->InputKey(FInputKeyEventArgs::CreateSimulated(Held.Key, IE_Released, 0.0f, -1,
            IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false, V->GetGameViewport())); }
        if (Trace.InputEvents.IsValidIndex(Held.EventIndex)) { Trace.InputEvents[Held.EventIndex].ReleaseFrame = GFrameCounter; }
        HeldInputs.RemoveAt(I);
    }
}

bool USeedForgeInputSelfTestComponent::InjectPointer(const FVector& Target)
{
    UGameViewportClient* V = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    FSceneViewport* Scene = V ? V->GetGameViewport() : nullptr;
    if (!Scene) { FinishFailure(ESeedForgeInputSelfTestFailure::ViewportUnavailable, TEXT("Viewport lost during pointer injection.")); return false; }
    const FGeometry& Geometry = Scene->GetCachedGeometry();
    const FIntPoint Size = Scene->GetSizeXY();
    const FVector2D LocalSize = Geometry.GetLocalSize();
    FVector2D Pixel;
    if (Scene->HasMouseCapture() || !Scene->IsCursorVisible() || !Controller->bShowMouseCursor || Size.X <= 0 || Size.Y <= 0 || LocalSize.X <= 0 || LocalSize.Y <= 0
        || !Controller->ProjectWorldLocationToScreen(Target, Pixel) || Pixel.X < 1 || Pixel.Y < 1 || Pixel.X >= Size.X-1 || Pixel.Y >= Size.Y-1)
    { FinishFailure(ESeedForgeInputSelfTestFailure::ViewportNotReady, TEXT("Pointer proof requires uncaptured visible cursor and a projected in-viewport target.")); return false; }
    const FVector2D Local(Pixel.X * LocalSize.X / Size.X, Pixel.Y * LocalSize.Y / Size.Y);
    const FVector2D Absolute = Geometry.LocalToAbsolute(Local);
    const TSet<FKey> Buttons;
    const FPointerEvent Event(IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), 0, Absolute, Absolute,
        Buttons, EKeys::Invalid, 0, FModifierKeysState());
    // OnMouseMove can restore an OS cursor hidden by an earlier capture. Its
    // read-only cursor query exposes that otherwise private state; reject it.
    if (Scene->OnCursorQuery(Geometry, Event).GetCursorType() == EMouseCursor::None)
    { FinishFailure(ESeedForgeInputSelfTestFailure::ViewportNotReady, TEXT("Pointer cursor is hidden or capture-restoration is pending.")); return false; }
    // This updates the owning viewport's cached cursor for normal PC deprojection;
    // no OS cursor operation or direct gameplay aim call is permitted here.
    Scene->OnMouseMove(Geometry, Event);
    return true;
}

void USeedForgeInputSelfTestComponent::PrepareEffect()
{
    if (bPrepared || !HeldInputs.IsEmpty()) { return; }
    if (EffectIndex >= 13) { FinishSuccess(); return; }
    auto* Player = Cast<ASeedForgePlayerCharacter>(Controller->GetPawn());
    if (!Player) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Missing player before effect.")); return; }
    if (EffectIndex == 3 && Coordinator->GetRunResourceSnapshot().AttackCooldownRemaining > 0) { return; }
    if (EffectIndex >= 4 && EffectIndex <= 8)
    {
        if (Player->GetVelocity().Size2D() >= 1.0) { return; }
        if (!PlaceActor(Player, PadCell, TEXT("movement-pad"), 96)) { return; }
    }
    if (EffectIndex == 2 && !PlaceActor(TargetEnemy.Get(), PadCell + FIntPoint(0,1), TEXT("attack-target"), 58)) { return; }
    if (EffectIndex == 9 && !bLossRequested)
    {
        bLossRequested = true;
        Coordinator->ApplyPlayerDamage(Coordinator->GetSnapshot().PlayerHealth);
        const auto Resources = Coordinator->GetRunResourceSnapshot();
        if (Coordinator->GetSnapshot().RunState != ESeedForgeRunState::Lost
            || Resources.bInteractionTimerActive || Resources.bRepathTimerActive)
        { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Public damage did not produce clean Lost state.")); return; }
        for (TActorIterator<ASeedForgeEnemyPawn> It(GetWorld()); It; ++It)
        { if (It->GetOwner() == Coordinator.Get() && !It->GetPathSnapshot().Waypoints.IsEmpty())
            { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Terminal enemy path was not cleared.")); return; } }
    }
    bPrepared = true;
    SetupFrame = GFrameCounter;
}

void USeedForgeInputSelfTestComponent::BeginEffect()
{
    using namespace SeedForge::InputSelfTest::Private;
    auto* Player = Cast<ASeedForgePlayerCharacter>(Controller->GetPawn());
    if (!Live(Player)) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Missing player at injection.")); return; }
    const auto S = Coordinator->GetSnapshot();
    const int32 Last = EffectIndex == 11 ? 12 : EffectIndex;
    for (int32 I = EffectIndex; I <= Last; ++I)
    {
        FSeedForgeInputEffectObservation E;
        E.Action = Actions[I]; E.Key = Keys[I]; E.RunGeneration = S.RunGeneration; E.SourceRequestId = S.AppliedRequestId;
        E.InjectedFrame = GFrameCounter; E.InjectedAtUtc = FDateTime::UtcNow();
        E.ActorKey = ActorKey(I == 2 || I == 3 ? static_cast<AActor*>(TargetEnemy.Get()) : I >= 9 ? static_cast<AActor*>(Controller.Get()) : Player);
        if (I < 2) { E.Before = Player->GetAimDirection(); }
        if (I >= 4 && I <= 8) { E.Before = Player->GetActorLocation(); LastPhysicalPosition = E.Before; }
        if (I == 2 || I == 3)
        { E.CooldownBefore = Coordinator->GetRunResourceSnapshot().AttackCooldownRemaining;
            E.LiveEnemiesBefore = Coordinator->GetLiveEnemyActorCount(); E.bTargetAliveBefore = Live(TargetEnemy.Get());
            const FVector TargetPosition = E.bTargetAliveBefore ? TargetEnemy->GetActorLocation() : FVector::ZeroVector;
            const FVector Offset = TargetPosition - Player->GetActorLocation();
            const double ForwardDot = FVector::DotProduct(Player->GetAimDirection(), Offset.GetSafeNormal2D());
            const auto TargetPath = E.bTargetAliveBefore ? TargetEnemy->GetPathSnapshot() : FSeedForgeEnemyPathSnapshot();
            // Bounded diagnostic: exactly once per scheduled attack, never a retry.
            UE_LOG(LogSeedForge, Display,
                TEXT("SEEDFORGE_INPUT_ATTACK_PRE action=%s frame=%llu run=%llu request=%llu player=%s target=%s aim=%s distance=%.6f dot=%.6f cooldown=%.6f key_down=%s path_revision=%llu waypoint_index=%d"),
                *E.Action, GFrameCounter, E.RunGeneration, E.SourceRequestId, *Player->GetActorLocation().ToString(),
                *TargetPosition.ToString(), *Player->GetAimDirection().ToString(), Offset.Size2D(), ForwardDot, E.CooldownBefore,
                Controller->IsInputKeyDown(EKeys::LeftMouseButton) ? TEXT("true") : TEXT("false"), TargetPath.Revision, TargetPath.NextWaypointIndex);
        }
        if (I == 8) { E.CooldownBefore = Player->GetDashCooldownRemaining(); }
        Trace.InputEvents.Add(MoveTemp(E));
    }
    PendingEvent = Last; Phase = EPhase::AwaitEffect; bPrepared = false;
    if (EffectIndex < 2)
    {
        HeldPointerDirection = EffectIndex == 0 ? FVector::ForwardVector : FVector::RightVector;
        bHeldPointerIntent = true;
        InjectPointer(Player->GetActorLocation() + HeldPointerDirection * 300);
    }
    else if (EffectIndex == 8)
    { InjectKey(EKeys::W, 8, true); InjectKey(EKeys::D, 8, true); InjectKey(EKeys::SpaceBar, 8, true); }
    else if (EffectIndex == 11) { InjectKey(EKeys::N, 11, true); InjectKey(EKeys::R, 12, true); }
    else { InjectKey(FKey(Keys[EffectIndex]), EffectIndex, !(EffectIndex >= 4 && EffectIndex <= 7)); }
}

void USeedForgeInputSelfTestComponent::CompleteEffect(int32 Index)
{
    auto& E = Trace.InputEvents[Index]; E.EffectFrame = GFrameCounter; E.EffectAtUtc = FDateTime::UtcNow(); E.bConfirmed = true;
    if (Index == 3) { bHeldPointerIntent = false; HeldPointerDirection = FVector::ZeroVector; }
    if (Index == PendingEvent) { EffectIndex = Index + 1; Phase = EPhase::Ready; bPrepared = false; }
}

void USeedForgeInputSelfTestComponent::AfterInput(float DeltaSeconds, bool bGamePaused)
{
    using namespace SeedForge::InputSelfTest::Private;
    if (!CheckBudget() || bGamePaused || DeltaSeconds <= 0 || Phase != EPhase::AwaitEffect || !Trace.InputEvents.IsValidIndex(PendingEvent)) { return; }
    auto& E = Trace.InputEvents[PendingEvent];
    auto* Player = Cast<ASeedForgePlayerCharacter>(Controller->GetPawn());
    if ((PendingEvent == 2 || PendingEvent == 3) && GFrameCounter == E.InjectedFrame)
    {
        // Attack resolves synchronously through normal input dispatch. Retain its
        // immediate result: polling later would erase a missed attack's consumed
        // cooldown and make nondelivery indistinguishable from target rejection.
        E.CooldownAfter = Coordinator->GetRunResourceSnapshot().AttackCooldownRemaining;
        E.LiveEnemiesAfter = Coordinator->GetLiveEnemyActorCount(); E.bTargetAliveAfter = Live(TargetEnemy.Get());
        UE_LOG(LogSeedForge, Display,
            TEXT("SEEDFORGE_INPUT_ATTACK_POST action=%s frame=%llu run=%llu request=%llu key_down=%s just_pressed=%s cooldown=%.6f enemies=%d target_alive=%s"),
            *E.Action, GFrameCounter, E.RunGeneration, E.SourceRequestId,
            Controller->IsInputKeyDown(EKeys::LeftMouseButton) ? TEXT("true") : TEXT("false"),
            Controller->WasInputKeyJustPressed(EKeys::LeftMouseButton) ? TEXT("true") : TEXT("false"),
            E.CooldownAfter, E.LiveEnemiesAfter, E.bTargetAliveAfter ? TEXT("true") : TEXT("false"));
        if (E.CooldownAfter > 0 && E.bTargetAliveAfter == (PendingEvent == 2) && E.LiveEnemiesAfter == (PendingEvent == 2 ? 5 : 4))
        { CompleteEffect(PendingEvent); }
    }
    else if (PendingEvent == 8 && Player && GFrameCounter == E.InjectedFrame)
    {
        // CharacterMovement consumes PendingLaunchVelocity after input. Latch
        // this real launch/cooldown only once; later movement frames must not
        // replace it with the consumed zero while waiting for 20 units of proof.
        E.LaunchVelocity = Player->GetCharacterMovement()->PendingLaunchVelocity;
        E.CooldownAfter = Player->GetDashCooldownRemaining();
    }
    else if (PendingEvent >= 9)
    {
        const int32 ExpectedQueues = PendingEvent - 8;
        if (Trace.QueuedRuns.Num() != ExpectedQueues) { return; }
        const auto S = Coordinator->GetSnapshot();
        bool bOldGone = Controller->GetPawn() == nullptr;
        for (const auto& Old : PreviousActors) { bOldGone &= !Live(Old.Get()); }
        if (!bOldGone || S.RunState != ESeedForgeRunState::Generating || S.LayoutHash || S.EncounterHash || S.AppliedRequestId
            || S.PendingRequestId != Trace.QueuedRuns.Last().Snapshot.PendingRequestId)
        { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Restart retained old possession, ownership or identity.")); return; }
        for (int32 I = PendingEvent == 12 ? 11 : PendingEvent; I <= PendingEvent; ++I)
        { Trace.InputEvents[I].RunGeneration = Trace.QueuedRuns[I-9].Snapshot.RunGeneration;
            Trace.InputEvents[I].SourceRequestId = Trace.QueuedRuns[I-9].Snapshot.PendingRequestId; }
        if (PendingEvent == 12) { CompleteEffect(11); }
        Phase = EPhase::AwaitRun;
    }
}

void USeedForgeInputSelfTestComponent::ObserveWorld(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
    using namespace SeedForge::InputSelfTest::Private;
    if (World != GetWorld() || TickType != LEVELTICK_All || !CheckBudget() || DeltaSeconds <= 0 || !ResolveCoordinator()) { return; }
    if (Phase == EPhase::AwaitInitial)
    {
        if (Coordinator->GetSnapshot().RunState == ESeedForgeRunState::Playing && CaptureRun(TEXT("initial"))) { PreparePath(); }
        return;
    }
    if (Phase == EPhase::AwaitPath && Live(TargetEnemy.Get()))
    {
        PathProof.ObserveMovement(TargetEnemy->GetStableId(), Trace.Initial.RunGeneration, Trace.Initial.AppliedRequestId,
            TargetEnemy->GetPathSnapshot().LastMove, Coordinator->GetTuning().EnemyMoveSpeed);
        if (PathProof.IsComplete())
        { Trace.PathEvidence = PathProof.GetEvidence(); Coordinator->OnEnemyPathApplied().Remove(PathHandle); PathHandle.Reset(); Phase = EPhase::Ready; }
    }
    if (Phase == EPhase::AwaitEffect && Trace.InputEvents.IsValidIndex(PendingEvent))
    {
        auto* Player = Cast<ASeedForgePlayerCharacter>(Controller->GetPawn());
        if (!Live(Player)) { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Missing player during physical effect.")); return; }
        auto& E = Trace.InputEvents[PendingEvent];
        if (PendingEvent < 2)
        {
            E.After = Player->GetAimDirection();
            if (E.After.Equals(PendingEvent == 0 ? FVector::ForwardVector : FVector::RightVector, 0.05)) { CompleteEffect(PendingEvent); }
        }
        else if (PendingEvent >= 4 && PendingEvent <= 8)
        {
            const FVector Position = Player->GetActorLocation();
            const double MaxSpeed = PendingEvent == 8 ? Coordinator->GetTuning().DashImpulse : Coordinator->GetTuning().PlayerMoveSpeed;
            if (!Finite(Position) || FVector::Dist2D(Position, LastPhysicalPosition) > MaxSpeed * DeltaSeconds + 0.1
                || !OnCells(Position, Trace.WalkableCells, 42))
            { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("Physical input movement violated speed/capsule walkable bounds.")); return; }
            LastPhysicalPosition = Position; E.After = Position;
            if ((PendingEvent < 8 && FVector::Dist2D(E.Before, E.After) >= 20)
                || (PendingEvent == 8 && E.LaunchVelocity.Size2D() > 0 && FVector::Dist2D(E.Before, E.After) >= 20))
            {
                for (auto& Held : HeldInputs) { Held.bReleaseNextFrame = true; }
                CompleteEffect(PendingEvent);
            }
        }
    }
    if (Phase == EPhase::AwaitRun)
    {
        const auto S = Coordinator->GetSnapshot(); const auto& Q = Trace.QueuedRuns.Last().Snapshot;
        if (S.RunState == ESeedForgeRunState::Playing)
        {
            if (S.AppliedRequestId != Q.PendingRequestId || S.RunGeneration != Q.RunGeneration)
            { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, TEXT("A superseded request applied.")); return; }
            const TCHAR* Labels[] = {TEXT("initial"), TEXT("same"), TEXT("new"), TEXT("rapid")};
            if (Trace.Runs.Num() < 4 && CaptureRun(Labels[Trace.Runs.Num()])) { CompleteEffect(PendingEvent); }
        }
    }
    if (Phase == EPhase::Ready && IsRunning()) { PrepareEffect(); }
}

void USeedForgeInputSelfTestComponent::Cleanup()
{
    bHeldPointerIntent = false; HeldPointerDirection = FVector::ZeroVector;
    FWorldDelegates::OnWorldPostActorTick.Remove(WorldHandle); WorldHandle.Reset();
    if (Coordinator.IsValid())
    {
        Coordinator->OnRunStateChanged().Remove(TransitionHandle); Coordinator->OnRunQueued().Remove(QueueHandle);
        Coordinator->OnEnemyPathApplied().Remove(PathHandle);
    }
    TransitionHandle.Reset(); QueueHandle.Reset(); PathHandle.Reset();
    ReleaseInputs(true);
    if (Controller.IsValid()) { Controller->FlushPressedKeys(); }
    Trace.RemainingDelegateBindings = (WorldHandle.IsValid() ? 1 : 0) + (TransitionHandle.IsValid() ? 1 : 0)
        + (QueueHandle.IsValid() ? 1 : 0) + (PathHandle.IsValid() ? 1 : 0);
    Trace.RemainingPressedKeys = HeldInputs.Num();
    if (Controller.IsValid())
    {
        for (const FKey& Key : {EKeys::W, EKeys::S, EKeys::A, EKeys::D, EKeys::SpaceBar, EKeys::LeftMouseButton, EKeys::R, EKeys::N})
        { Trace.RemainingPressedKeys += Controller->IsInputKeyDown(Key) ? 1 : 0; }
    }
}

void USeedForgeInputSelfTestComponent::FinishSuccess()
{
    if (!IsRunning()) { return; }
    Cleanup();
    Trace.CompletedAtUtc = FDateTime::UtcNow(); Trace.CompletedFrame = GFrameCounter; Trace.CompletionCount = 1;
    Trace.bSuccess = true; Trace.FailureCode = ESeedForgeInputSelfTestFailure::None; Trace.FailureMessage.Reset();
    FString Error;
    if (!FSeedForgeInputSelfTestCodec::ValidateEvidence(Trace, Error))
    { FinishFailure(ESeedForgeInputSelfTestFailure::MissingInputEvidence, Error); return; }
    bFinished = true; Phase = EPhase::Complete; Finished.Broadcast(Trace);
}
void USeedForgeInputSelfTestComponent::Cancel()
{
    if (bStarted && !bFinished) { FinishFailure(ESeedForgeInputSelfTestFailure::Cancelled, TEXT("Input proof was cancelled.")); }
}

void USeedForgeInputSelfTestComponent::FinishFailure(ESeedForgeInputSelfTestFailure Code, const FString& Message)
{
    if (bFinished) { return; }
    bFinished = true;
    Cleanup();
    Trace.bSuccess = false;
    Trace.FailureCode = Code;
    Trace.FailureMessage = Message.Left(1024);
    Trace.CompletedAtUtc = FDateTime::UtcNow();
    Trace.CompletedFrame = GFrameCounter;
    Trace.CompletionCount = 1;
    Finished.Broadcast(Trace);
}

void USeedForgeInputSelfTestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Cancel();
    Super::EndPlay(EndPlayReason);
}

bool USeedForgeInputSelfTestComponent::IsRunning() const { return bStarted && !bFinished; }
const FSeedForgeInputSelfTestTrace& USeedForgeInputSelfTestComponent::GetTrace() const { return Trace; }
FSeedForgeInputSelfTestFinished& USeedForgeInputSelfTestComponent::OnFinished() { return Finished; }

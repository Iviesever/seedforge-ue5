#include "SeedForgeGameplaySmoke.h"

namespace SeedForge::GameplaySmoke::Private
{
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

FString FSeedForgeGameplaySmokeCodec::ExportCanonicalJson(
    const FSeedForgeGameplaySmokeTrace& Trace)
{
    using namespace SeedForge::GameplaySmoke::Private;

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
    Json += Trace.bSuccess ? TEXT(",\"result\":\"Passed\"") : TEXT(",\"result\":\"Failed\"");
    Json += TEXT(",\"failureCode\":");
    AppendString(Json, Trace.FailureCode);
    Json += TEXT(",\"failureMessage\":");
    AppendString(Json, Trace.FailureMessage);
    Json += TEXT("}");
    return Json;
}

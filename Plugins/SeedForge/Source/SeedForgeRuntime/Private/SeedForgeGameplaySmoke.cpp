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
        TEXT(",\"seed\":\"%llu\",\"layoutHash\":\"%llu\",\"encounterHash\":\"%llu\","),
        Trace.Seed,
        Trace.LayoutHash,
        Trace.EncounterHash);
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
    Json += Trace.bSuccess ? TEXT(",\"result\":\"Passed\"") : TEXT(",\"result\":\"Failed\"");
    Json += TEXT(",\"failureCode\":");
    AppendString(Json, Trace.FailureCode);
    Json += TEXT(",\"failureMessage\":");
    AppendString(Json, Trace.FailureMessage);
    Json += TEXT("}");
    return Json;
}

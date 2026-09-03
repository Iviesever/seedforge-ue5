#include "SeedForgeLayoutCodec.h"

FString FSeedForgeLayoutCodec::ExportCanonicalJson(const FSeedForgeLayoutDocument& Document)
{
    return TEXT("{}");
}

FSeedForgeDocumentResult FSeedForgeLayoutCodec::ImportCanonicalJson(const FString& Json)
{
    return FSeedForgeDocumentResult::Failure(
        ESeedForgeDocumentErrorCode::MalformedJson,
        TEXT("PACT-21 codec is not implemented."));
}

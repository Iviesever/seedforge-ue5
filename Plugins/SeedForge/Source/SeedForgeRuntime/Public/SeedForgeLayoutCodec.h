#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"

struct SEEDFORGERUNTIME_API FSeedForgeLayoutDocument
{
    static constexpr int32 CurrentSchemaVersion = 1;
    static constexpr int32 CurrentGeneratorVersion = 1;

    int32 SchemaVersion = CurrentSchemaVersion;
    int32 GeneratorVersion = CurrentGeneratorVersion;
    FSeedForgeConfig Config;
    FSeedForgeLayout Layout;

    bool operator==(const FSeedForgeLayoutDocument& Other) const = default;
};

enum class ESeedForgeDocumentErrorCode : uint8
{
    None,
    MalformedJson,
    MissingField,
    InvalidFieldType,
    UnsupportedSchema,
    UnsupportedSchemaVersion,
    UnsupportedGeneratorVersion,
    InvalidUnsignedInteger,
    NonCanonicalData,
    HashMismatch,
    InvalidConfiguration,
    LayoutValidationFailed
};

struct SEEDFORGERUNTIME_API FSeedForgeDocumentResult
{
    ESeedForgeDocumentErrorCode ErrorCode = ESeedForgeDocumentErrorCode::None;
    FString ErrorMessage;
    FSeedForgeLayoutDocument Document;

    bool IsSuccess() const
    {
        return ErrorCode == ESeedForgeDocumentErrorCode::None;
    }

    static FSeedForgeDocumentResult Success(FSeedForgeLayoutDocument&& InDocument)
    {
        FSeedForgeDocumentResult Result;
        Result.Document = MoveTemp(InDocument);
        return Result;
    }

    static FSeedForgeDocumentResult Failure(
        ESeedForgeDocumentErrorCode InCode,
        FString InMessage)
    {
        FSeedForgeDocumentResult Result;
        Result.ErrorCode = InCode;
        Result.ErrorMessage = MoveTemp(InMessage);
        return Result;
    }
};

class SEEDFORGERUNTIME_API FSeedForgeLayoutCodec
{
public:
    static FString ExportCanonicalJson(const FSeedForgeLayoutDocument& Document);
    static FSeedForgeDocumentResult ImportCanonicalJson(const FString& Json);
};

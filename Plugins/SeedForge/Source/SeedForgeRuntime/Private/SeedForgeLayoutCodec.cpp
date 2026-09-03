#include "SeedForgeLayoutCodec.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeValidator.h"

namespace SeedForge::Document::Private
{
    constexpr TCHAR SchemaName[] = TEXT("seedforge.layout");

    bool IsCanonicalPointLess(const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y != Right.Y ? Left.Y < Right.Y : Left.X < Right.X;
    }

    bool IsCanonicalRoomLess(const FSeedForgeRoom& Left, const FSeedForgeRoom& Right)
    {
        if (Left.Min != Right.Min)
        {
            return IsCanonicalPointLess(Left.Min, Right.Min);
        }
        if (Left.Size.Y != Right.Size.Y)
        {
            return Left.Size.Y < Right.Size.Y;
        }
        return Left.Size.X < Right.Size.X;
    }

    bool CanonicalizeLayout(FSeedForgeLayout& Layout)
    {
        const TArray<FSeedForgeRoom> OriginalRooms = Layout.Rooms;
        const TArray<FIntPoint> OriginalCorridors = Layout.CorridorCells;

        Layout.Rooms.Sort(IsCanonicalRoomLess);
        Layout.CorridorCells.Sort(IsCanonicalPointLess);
        int32 WriteIndex = 0;
        for (const FIntPoint& Cell : Layout.CorridorCells)
        {
            if (WriteIndex == 0 || Layout.CorridorCells[WriteIndex - 1] != Cell)
            {
                Layout.CorridorCells[WriteIndex++] = Cell;
            }
        }
        Layout.CorridorCells.SetNum(WriteIndex, EAllowShrinking::No);
        return Layout.Rooms != OriginalRooms || Layout.CorridorCells != OriginalCorridors;
    }

    void AppendPoint(FString& Json, const FIntPoint& Point)
    {
        Json += FString::Printf(TEXT("[%d,%d]"), Point.X, Point.Y);
    }

    void AppendConfig(FString& Json, const FSeedForgeConfig& Config)
    {
        Json += FString::Printf(
            TEXT("{\"gridWidth\":%d,\"gridHeight\":%d,\"roomCount\":%d,")
            TEXT("\"minRoomWidth\":%d,\"maxRoomWidth\":%d,")
            TEXT("\"minRoomHeight\":%d,\"maxRoomHeight\":%d,")
            TEXT("\"roomPadding\":%d,\"maxPlacementAttempts\":%d}"),
            Config.GridWidth,
            Config.GridHeight,
            Config.RoomCount,
            Config.MinRoomWidth,
            Config.MaxRoomWidth,
            Config.MinRoomHeight,
            Config.MaxRoomHeight,
            Config.RoomPadding,
            Config.MaxPlacementAttempts);
    }

    void AppendLayout(FString& Json, const FSeedForgeLayout& Layout)
    {
        Json += TEXT("{\"entrance\":");
        AppendPoint(Json, Layout.Entrance);
        Json += TEXT(",\"exit\":");
        AppendPoint(Json, Layout.Exit);
        Json += TEXT(",\"rooms\":[");
        for (int32 Index = 0; Index < Layout.Rooms.Num(); ++Index)
        {
            if (Index > 0)
            {
                Json += TEXT(",");
            }
            Json += TEXT("{\"min\":");
            AppendPoint(Json, Layout.Rooms[Index].Min);
            Json += TEXT(",\"size\":");
            AppendPoint(Json, Layout.Rooms[Index].Size);
            Json += TEXT("}");
        }
        Json += TEXT("],\"corridorCells\":[");
        for (int32 Index = 0; Index < Layout.CorridorCells.Num(); ++Index)
        {
            if (Index > 0)
            {
                Json += TEXT(",");
            }
            AppendPoint(Json, Layout.CorridorCells[Index]);
        }
        Json += TEXT("]}");
    }

    FSeedForgeDocumentResult Failure(
        ESeedForgeDocumentErrorCode Code,
        const FString& Path,
        const FString& Detail)
    {
        return FSeedForgeDocumentResult::Failure(
            Code,
            FString::Printf(TEXT("%s: %s"), *Path, *Detail));
    }

    bool HasRequiredType(
        const FJsonObject& Object,
        const TCHAR* Field,
        EJson Type,
        const FString& Path,
        FSeedForgeDocumentResult& Error)
    {
        if (!Object.HasField(Field))
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::MissingField,
                Path + TEXT(".") + Field,
                TEXT("required field is missing"));
            return false;
        }
        if (!Object.HasTypedField(Field, Type))
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::InvalidFieldType,
                Path + TEXT(".") + Field,
                TEXT("field has the wrong JSON type"));
            return false;
        }
        return true;
    }

    bool ReadString(
        const FJsonObject& Object,
        const TCHAR* Field,
        const FString& Path,
        FString& Value,
        FSeedForgeDocumentResult& Error)
    {
        if (!HasRequiredType(Object, Field, EJson::String, Path, Error))
        {
            return false;
        }
        Object.TryGetStringField(Field, Value);
        return true;
    }

    bool ReadInt32(
        const FJsonObject& Object,
        const TCHAR* Field,
        const FString& Path,
        int32& Value,
        FSeedForgeDocumentResult& Error)
    {
        if (!HasRequiredType(Object, Field, EJson::Number, Path, Error))
        {
            return false;
        }
        if (!Object.TryGetNumberField(Field, Value))
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::InvalidFieldType,
                Path + TEXT(".") + Field,
                TEXT("number must be an exact signed 32-bit integer"));
            return false;
        }
        return true;
    }

    bool ReadObject(
        const FJsonObject& Object,
        const TCHAR* Field,
        const FString& Path,
        const TSharedPtr<FJsonObject>*& Value,
        FSeedForgeDocumentResult& Error)
    {
        if (!HasRequiredType(Object, Field, EJson::Object, Path, Error))
        {
            return false;
        }
        Object.TryGetObjectField(Field, Value);
        return Value != nullptr && Value->IsValid();
    }

    bool ReadArray(
        const FJsonObject& Object,
        const TCHAR* Field,
        const FString& Path,
        const TArray<TSharedPtr<FJsonValue>>*& Value,
        FSeedForgeDocumentResult& Error)
    {
        if (!HasRequiredType(Object, Field, EJson::Array, Path, Error))
        {
            return false;
        }
        Object.TryGetArrayField(Field, Value);
        return Value != nullptr;
    }

    bool ParseUInt64Decimal(
        const FString& Text,
        const FString& Path,
        uint64& Value,
        FSeedForgeDocumentResult& Error)
    {
        if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == TEXT('0')))
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::InvalidUnsignedInteger,
                Path,
                TEXT("value must be a canonical unsigned decimal string"));
            return false;
        }

        uint64 Parsed = 0;
        for (const TCHAR Character : Text)
        {
            if (Character < TEXT('0') || Character > TEXT('9'))
            {
                Error = Failure(
                    ESeedForgeDocumentErrorCode::InvalidUnsignedInteger,
                    Path,
                    TEXT("value must contain decimal digits only"));
                return false;
            }
            const uint64 Digit = static_cast<uint64>(Character - TEXT('0'));
            if (Parsed > (MAX_uint64 - Digit) / 10ULL)
            {
                Error = Failure(
                    ESeedForgeDocumentErrorCode::InvalidUnsignedInteger,
                    Path,
                    TEXT("value exceeds uint64 range"));
                return false;
            }
            Parsed = Parsed * 10ULL + Digit;
        }
        Value = Parsed;
        return true;
    }

    bool ReadPoint(
        const TSharedPtr<FJsonValue>& Value,
        const FString& Path,
        FIntPoint& Point,
        FSeedForgeDocumentResult& Error)
    {
        if (!Value.IsValid() || Value->Type != EJson::Array)
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::InvalidFieldType,
                Path,
                TEXT("point must be a two-element integer array"));
            return false;
        }
        const TArray<TSharedPtr<FJsonValue>>* Components = nullptr;
        if (!Value->TryGetArray(Components) || Components == nullptr || Components->Num() != 2)
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::InvalidFieldType,
                Path,
                TEXT("point must contain exactly two elements"));
            return false;
        }
        if (!(*Components)[0].IsValid()
            || !(*Components)[1].IsValid()
            || (*Components)[0]->Type != EJson::Number
            || (*Components)[1]->Type != EJson::Number
            || !(*Components)[0]->TryGetNumber(Point.X)
            || !(*Components)[1]->TryGetNumber(Point.Y))
        {
            Error = Failure(
                ESeedForgeDocumentErrorCode::InvalidFieldType,
                Path,
                TEXT("point components must be exact signed 32-bit integers"));
            return false;
        }
        return true;
    }

    bool ReadPointField(
        const FJsonObject& Object,
        const TCHAR* Field,
        const FString& Path,
        FIntPoint& Point,
        FSeedForgeDocumentResult& Error)
    {
        if (!HasRequiredType(Object, Field, EJson::Array, Path, Error))
        {
            return false;
        }
        return ReadPoint(Object.TryGetField(Field), Path + TEXT(".") + Field, Point, Error);
    }

    bool ReadConfig(
        const FJsonObject& Object,
        FSeedForgeConfig& Config,
        FSeedForgeDocumentResult& Error)
    {
        const FString Path = TEXT("$.config");
        return ReadInt32(Object, TEXT("gridWidth"), Path, Config.GridWidth, Error)
            && ReadInt32(Object, TEXT("gridHeight"), Path, Config.GridHeight, Error)
            && ReadInt32(Object, TEXT("roomCount"), Path, Config.RoomCount, Error)
            && ReadInt32(Object, TEXT("minRoomWidth"), Path, Config.MinRoomWidth, Error)
            && ReadInt32(Object, TEXT("maxRoomWidth"), Path, Config.MaxRoomWidth, Error)
            && ReadInt32(Object, TEXT("minRoomHeight"), Path, Config.MinRoomHeight, Error)
            && ReadInt32(Object, TEXT("maxRoomHeight"), Path, Config.MaxRoomHeight, Error)
            && ReadInt32(Object, TEXT("roomPadding"), Path, Config.RoomPadding, Error)
            && ReadInt32(Object, TEXT("maxPlacementAttempts"), Path, Config.MaxPlacementAttempts, Error);
    }

    bool ReadLayout(
        const FJsonObject& Object,
        FSeedForgeLayout& Layout,
        FSeedForgeDocumentResult& Error)
    {
        const FString Path = TEXT("$.layout");
        if (!ReadPointField(Object, TEXT("entrance"), Path, Layout.Entrance, Error)
            || !ReadPointField(Object, TEXT("exit"), Path, Layout.Exit, Error))
        {
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Rooms = nullptr;
        if (!ReadArray(Object, TEXT("rooms"), Path, Rooms, Error))
        {
            return false;
        }
        Layout.Rooms.Reserve(Rooms->Num());
        for (int32 Index = 0; Index < Rooms->Num(); ++Index)
        {
            const FString RoomPath = FString::Printf(TEXT("$.layout.rooms[%d]"), Index);
            const TSharedPtr<FJsonValue>& RoomValue = (*Rooms)[Index];
            const TSharedPtr<FJsonObject>* RoomObject = nullptr;
            if (!RoomValue.IsValid()
                || RoomValue->Type != EJson::Object
                || !RoomValue->TryGetObject(RoomObject)
                || RoomObject == nullptr
                || !RoomObject->IsValid())
            {
                Error = Failure(
                    ESeedForgeDocumentErrorCode::InvalidFieldType,
                    RoomPath,
                    TEXT("room must be an object"));
                return false;
            }

            FSeedForgeRoom Room;
            if (!ReadPointField(**RoomObject, TEXT("min"), RoomPath, Room.Min, Error)
                || !ReadPointField(**RoomObject, TEXT("size"), RoomPath, Room.Size, Error))
            {
                return false;
            }
            Layout.Rooms.Add(Room);
        }

        const TArray<TSharedPtr<FJsonValue>>* Corridors = nullptr;
        if (!ReadArray(Object, TEXT("corridorCells"), Path, Corridors, Error))
        {
            return false;
        }
        Layout.CorridorCells.Reserve(Corridors->Num());
        for (int32 Index = 0; Index < Corridors->Num(); ++Index)
        {
            FIntPoint Cell;
            if (!ReadPoint(
                    (*Corridors)[Index],
                    FString::Printf(TEXT("$.layout.corridorCells[%d]"), Index),
                    Cell,
                    Error))
            {
                return false;
            }
            Layout.CorridorCells.Add(Cell);
        }
        return true;
    }
}

FString FSeedForgeLayoutCodec::ExportCanonicalJson(const FSeedForgeLayoutDocument& Document)
{
    FSeedForgeLayout CanonicalLayout = Document.Layout;
    SeedForge::Document::Private::CanonicalizeLayout(CanonicalLayout);
    CanonicalLayout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(CanonicalLayout);

    FString Json;
    Json.Reserve(512 + CanonicalLayout.Rooms.Num() * 48 + CanonicalLayout.CorridorCells.Num() * 16);
    Json += FString::Printf(
        TEXT("{\"schema\":\"%s\",\"schemaVersion\":%d,\"generatorVersion\":%d,")
        TEXT("\"seed\":\"%llu\",\"canonicalHash\":\"%llu\",\"config\":"),
        SeedForge::Document::Private::SchemaName,
        Document.SchemaVersion,
        Document.GeneratorVersion,
        CanonicalLayout.Seed,
        CanonicalLayout.CanonicalHash);
    SeedForge::Document::Private::AppendConfig(Json, Document.Config);
    Json += TEXT(",\"layout\":");
    SeedForge::Document::Private::AppendLayout(Json, CanonicalLayout);
    Json += TEXT("}");
    return Json;
}

FSeedForgeDocumentResult FSeedForgeLayoutCodec::ImportCanonicalJson(const FString& Json)
{
    using namespace SeedForge::Document::Private;

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return Failure(
            ESeedForgeDocumentErrorCode::MalformedJson,
            TEXT("$"),
            FString::Printf(TEXT("malformed JSON (%s)"), *Reader->GetErrorMessage()));
    }

    FSeedForgeDocumentResult Error;
    FString Schema;
    if (!ReadString(*Root, TEXT("schema"), TEXT("$"), Schema, Error))
    {
        return Error;
    }
    if (Schema != SchemaName)
    {
        return Failure(
            ESeedForgeDocumentErrorCode::UnsupportedSchema,
            TEXT("$.schema"),
            FString::Printf(TEXT("unsupported schema '%s'"), *Schema));
    }

    FSeedForgeLayoutDocument Document;
    if (!ReadInt32(*Root, TEXT("schemaVersion"), TEXT("$"), Document.SchemaVersion, Error)
        || !ReadInt32(*Root, TEXT("generatorVersion"), TEXT("$"), Document.GeneratorVersion, Error))
    {
        return Error;
    }
    if (Document.SchemaVersion != FSeedForgeLayoutDocument::CurrentSchemaVersion)
    {
        return Failure(
            ESeedForgeDocumentErrorCode::UnsupportedSchemaVersion,
            TEXT("$.schemaVersion"),
            FString::Printf(TEXT("unsupported schema version %d"), Document.SchemaVersion));
    }
    if (Document.GeneratorVersion != FSeedForgeLayoutDocument::CurrentGeneratorVersion)
    {
        return Failure(
            ESeedForgeDocumentErrorCode::UnsupportedGeneratorVersion,
            TEXT("$.generatorVersion"),
            FString::Printf(TEXT("unsupported generator version %d"), Document.GeneratorVersion));
    }

    FString SeedText;
    FString HashText;
    if (!ReadString(*Root, TEXT("seed"), TEXT("$"), SeedText, Error)
        || !ReadString(*Root, TEXT("canonicalHash"), TEXT("$"), HashText, Error)
        || !ParseUInt64Decimal(SeedText, TEXT("$.seed"), Document.Layout.Seed, Error))
    {
        return Error;
    }
    uint64 SuppliedHash = 0;
    if (!ParseUInt64Decimal(HashText, TEXT("$.canonicalHash"), SuppliedHash, Error))
    {
        return Error;
    }

    const TSharedPtr<FJsonObject>* ConfigObject = nullptr;
    if (!ReadObject(*Root, TEXT("config"), TEXT("$"), ConfigObject, Error)
        || !ReadConfig(**ConfigObject, Document.Config, Error))
    {
        return Error;
    }

    const FSeedForgeResult ConfigResult = FSeedForgeGenerator::ValidateConfig(Document.Config);
    if (!ConfigResult.IsSuccess())
    {
        return Failure(
            ESeedForgeDocumentErrorCode::InvalidConfiguration,
            TEXT("$.config"),
            ConfigResult.ErrorMessage);
    }

    const TSharedPtr<FJsonObject>* LayoutObject = nullptr;
    if (!ReadObject(*Root, TEXT("layout"), TEXT("$"), LayoutObject, Error)
        || !ReadLayout(**LayoutObject, Document.Layout, Error))
    {
        return Error;
    }

    const uint64 OrderedHash = FSeedForgeGenerator::ComputeCanonicalHash(Document.Layout);
    const bool bChangedOrder = CanonicalizeLayout(Document.Layout);
    const uint64 CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(Document.Layout);
    if (SuppliedHash != CanonicalHash)
    {
        if (bChangedOrder && SuppliedHash == OrderedHash)
        {
            return Failure(
                ESeedForgeDocumentErrorCode::NonCanonicalData,
                TEXT("$.canonicalHash"),
                TEXT("hash depends on non-canonical array order"));
        }
        return Failure(
            ESeedForgeDocumentErrorCode::HashMismatch,
            TEXT("$.canonicalHash"),
            FString::Printf(
                TEXT("supplied hash %llu does not match recomputed hash %llu"),
                SuppliedHash,
                CanonicalHash));
    }
    Document.Layout.CanonicalHash = CanonicalHash;

    const FSeedForgeValidationResult Validation = FSeedForgeValidator::Validate(
        Document.Layout,
        Document.Config);
    if (!Validation.IsValid())
    {
        return Failure(
            ESeedForgeDocumentErrorCode::LayoutValidationFailed,
            TEXT("$.layout"),
            Validation.ErrorMessage);
    }

    return FSeedForgeDocumentResult::Success(MoveTemp(Document));
}

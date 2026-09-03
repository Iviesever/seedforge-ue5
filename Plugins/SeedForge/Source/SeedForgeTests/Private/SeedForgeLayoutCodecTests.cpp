#include "Misc/AutomationTest.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeLayoutCodec.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::Tests
{
    FSeedForgeLayoutDocument GenerateDocument(FAutomationTestBase& Test, uint64 Seed)
    {
        FSeedForgeLayoutDocument Document;
        const FSeedForgeResult Generated = FSeedForgeGenerator::Generate(Seed, Document.Config);
        Test.TestTrue(FString::Printf(TEXT("Seed %llu generates"), Seed), Generated.IsSuccess());
        if (Generated.IsSuccess())
        {
            Document.Layout = Generated.Layout;
        }
        return Document;
    }

    FString ReplaceRequired(
        FAutomationTestBase& Test,
        const FString& Source,
        const FString& Before,
        const FString& After)
    {
        Test.TestTrue(
            FString::Printf(TEXT("Fixture contains %s"), *Before),
            Source.Contains(Before, ESearchCase::CaseSensitive));
        return Source.Replace(*Before, *After, ESearchCase::CaseSensitive);
    }

    FString SwapFirstTwoRooms(FAutomationTestBase& Test, const FString& Source)
    {
        const FString Prefix = TEXT("\"rooms\":[");
        const int32 RoomsStart = Source.Find(Prefix, ESearchCase::CaseSensitive);
        Test.TestTrue(TEXT("Fixture contains rooms array"), RoomsStart != INDEX_NONE);
        if (RoomsStart == INDEX_NONE)
        {
            return Source;
        }

        const int32 FirstStart = RoomsStart + Prefix.Len();
        const int32 FirstEnd = Source.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FirstStart);
        const int32 Separator = FirstEnd + 1;
        const int32 SecondEnd = Source.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Separator + 1);
        Test.TestTrue(TEXT("Fixture contains at least two rooms"), FirstEnd != INDEX_NONE && SecondEnd != INDEX_NONE);
        if (FirstEnd == INDEX_NONE || SecondEnd == INDEX_NONE)
        {
            return Source;
        }

        const FString First = Source.Mid(FirstStart, FirstEnd - FirstStart + 1);
        const FString Second = Source.Mid(Separator + 1, SecondEnd - Separator);
        return Source.Left(FirstStart) + Second + TEXT(",") + First + Source.Mid(SecondEnd + 1);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeDocumentUInt64PrecisionTest,
    "SeedForge.Document.UInt64Precision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDocumentUInt64PrecisionTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Document = SeedForge::Tests::GenerateDocument(*this, MAX_uint64);
    const FString Json = FSeedForgeLayoutCodec::ExportCanonicalJson(Document);
    TestTrue(
        TEXT("Maximum uint64 seed is encoded as an exact decimal string"),
        Json.Contains(TEXT("\"seed\":\"18446744073709551615\"")));
    TestTrue(
        TEXT("Canonical hash is encoded as an exact decimal string"),
        Json.Contains(FString::Printf(TEXT("\"canonicalHash\":\"%llu\""), Document.Layout.CanonicalHash)));

    const FSeedForgeDocumentResult Imported = FSeedForgeLayoutCodec::ImportCanonicalJson(Json);
    TestTrue(TEXT("Maximum uint64 seed document imports"), Imported.IsSuccess());
    if (Imported.IsSuccess())
    {
        TestTrue(TEXT("Maximum uint64 seed and hash survive round trip"), Imported.Document == Document);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeDocumentCanonicalBytesTest,
    "SeedForge.Document.CanonicalBytes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDocumentCanonicalBytesTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Document = SeedForge::Tests::GenerateDocument(*this, 0x5EEDULL);
    const FString First = FSeedForgeLayoutCodec::ExportCanonicalJson(Document);
    const FString Second = FSeedForgeLayoutCodec::ExportCanonicalJson(Document);
    TestEqual(TEXT("Repeated exports are byte-identical"), First, Second);
    TestFalse(TEXT("Canonical JSON has no line breaks"), First.Contains(TEXT("\n")) || First.Contains(TEXT("\r")));
    TestFalse(TEXT("Canonical JSON has no formatting spaces"), First.Contains(TEXT(": ")));

    const TCHAR* OrderedFields[] = {
        TEXT("\"schema\":"),
        TEXT("\"schemaVersion\":"),
        TEXT("\"generatorVersion\":"),
        TEXT("\"seed\":"),
        TEXT("\"canonicalHash\":"),
        TEXT("\"config\":"),
        TEXT("\"layout\":")};
    int32 Previous = INDEX_NONE;
    for (const TCHAR* Field : OrderedFields)
    {
        const int32 Current = First.Find(Field, ESearchCase::CaseSensitive);
        TestTrue(FString::Printf(TEXT("Field %s exists"), Field), Current != INDEX_NONE);
        TestTrue(FString::Printf(TEXT("Field %s is in fixed order"), Field), Current > Previous);
        Previous = Current;
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeDocumentSpecificErrorsTest,
    "SeedForge.Document.SpecificErrors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDocumentSpecificErrorsTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Document = SeedForge::Tests::GenerateDocument(*this, 101);
    const FString Valid = FSeedForgeLayoutCodec::ExportCanonicalJson(Document);
    const auto ExpectError = [this](
        const TCHAR* Label,
        const FString& Json,
        ESeedForgeDocumentErrorCode Expected)
    {
        const FSeedForgeDocumentResult Result = FSeedForgeLayoutCodec::ImportCanonicalJson(Json);
        TestFalse(FString::Printf(TEXT("%s is rejected"), Label), Result.IsSuccess());
        TestEqual(FString::Printf(TEXT("%s has a typed error"), Label), Result.ErrorCode, Expected);
        TestFalse(FString::Printf(TEXT("%s has an actionable message"), Label), Result.ErrorMessage.IsEmpty());
    };

    ExpectError(TEXT("Malformed JSON"), TEXT("{"), ESeedForgeDocumentErrorCode::MalformedJson);
    ExpectError(
        TEXT("Missing seed"),
        SeedForge::Tests::ReplaceRequired(
            *this,
            Valid,
            FString::Printf(TEXT("\"seed\":\"%llu\","), Document.Layout.Seed),
            TEXT("")),
        ESeedForgeDocumentErrorCode::MissingField);
    ExpectError(
        TEXT("Wrong schema version type"),
        SeedForge::Tests::ReplaceRequired(*this, Valid, TEXT("\"schemaVersion\":1"), TEXT("\"schemaVersion\":\"1\"")),
        ESeedForgeDocumentErrorCode::InvalidFieldType);
    ExpectError(
        TEXT("Unsupported schema"),
        SeedForge::Tests::ReplaceRequired(*this, Valid, TEXT("seedforge.layout"), TEXT("seedforge.other")),
        ESeedForgeDocumentErrorCode::UnsupportedSchema);
    ExpectError(
        TEXT("Unsupported schema version"),
        SeedForge::Tests::ReplaceRequired(*this, Valid, TEXT("\"schemaVersion\":1"), TEXT("\"schemaVersion\":2")),
        ESeedForgeDocumentErrorCode::UnsupportedSchemaVersion);
    ExpectError(
        TEXT("Unsupported generator version"),
        SeedForge::Tests::ReplaceRequired(*this, Valid, TEXT("\"generatorVersion\":1"), TEXT("\"generatorVersion\":2")),
        ESeedForgeDocumentErrorCode::UnsupportedGeneratorVersion);
    ExpectError(
        TEXT("Unsigned overflow"),
        SeedForge::Tests::ReplaceRequired(
            *this,
            Valid,
            FString::Printf(TEXT("\"seed\":\"%llu\""), Document.Layout.Seed),
            TEXT("\"seed\":\"18446744073709551616\"")),
        ESeedForgeDocumentErrorCode::InvalidUnsignedInteger);
    ExpectError(
        TEXT("Wrong entrance type"),
        SeedForge::Tests::ReplaceRequired(
            *this,
            Valid,
            FString::Printf(
                TEXT("\"entrance\":[%d,%d]"),
                Document.Layout.Entrance.X,
                Document.Layout.Entrance.Y),
            TEXT("\"entrance\":false")),
        ESeedForgeDocumentErrorCode::InvalidFieldType);
    ExpectError(
        TEXT("Tampered hash"),
        SeedForge::Tests::ReplaceRequired(
            *this,
            Valid,
            FString::Printf(TEXT("\"canonicalHash\":\"%llu\""), Document.Layout.CanonicalHash),
            TEXT("\"canonicalHash\":\"0\"")),
        ESeedForgeDocumentErrorCode::HashMismatch);

    FSeedForgeLayoutDocument InvalidConfig = Document;
    InvalidConfig.Config.GridWidth = 0;
    ExpectError(
        TEXT("Invalid configuration"),
        FSeedForgeLayoutCodec::ExportCanonicalJson(InvalidConfig),
        ESeedForgeDocumentErrorCode::InvalidConfiguration);

    FSeedForgeLayoutDocument InvalidTopology = Document;
    InvalidTopology.Layout.Exit = FIntPoint(999, 999);
    InvalidTopology.Layout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(InvalidTopology.Layout);
    ExpectError(
        TEXT("Invalid topology"),
        FSeedForgeLayoutCodec::ExportCanonicalJson(InvalidTopology),
        ESeedForgeDocumentErrorCode::LayoutValidationFailed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeDocumentHundredSeedRoundTripTest,
    "SeedForge.Document.HundredSeedRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDocumentHundredSeedRoundTripTest::RunTest(const FString& Parameters)
{
    for (uint64 Seed = 0; Seed < 100; ++Seed)
    {
        const FSeedForgeLayoutDocument Document = SeedForge::Tests::GenerateDocument(*this, Seed);
        const FSeedForgeDocumentResult Imported = FSeedForgeLayoutCodec::ImportCanonicalJson(
            FSeedForgeLayoutCodec::ExportCanonicalJson(Document));
        if (!Imported.IsSuccess() || !(Imported.Document == Document))
        {
            AddError(FString::Printf(
                TEXT("Seed %llu failed canonical round trip: %s"),
                Seed,
                *Imported.ErrorMessage));
            return false;
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeDocumentCanonicalizesCollectionsTest,
    "SeedForge.Document.CanonicalizesCollections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDocumentCanonicalizesCollectionsTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Document = SeedForge::Tests::GenerateDocument(*this, 202);
    const FString Canonical = FSeedForgeLayoutCodec::ExportCanonicalJson(Document);
    const FString Unsorted = SeedForge::Tests::SwapFirstTwoRooms(*this, Canonical);
    const FSeedForgeDocumentResult Imported = FSeedForgeLayoutCodec::ImportCanonicalJson(Unsorted);
    TestTrue(TEXT("Unsorted arrays with the canonical hash import"), Imported.IsSuccess());
    if (Imported.IsSuccess())
    {
        TestTrue(TEXT("Imported arrays are normalized"), Imported.Document == Document);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeDocumentRejectsOrderedHashTest,
    "SeedForge.Document.RejectsOrderedHash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeDocumentRejectsOrderedHashTest::RunTest(const FString& Parameters)
{
    FSeedForgeLayoutDocument Document = SeedForge::Tests::GenerateDocument(*this, 303);
    const FString Canonical = FSeedForgeLayoutCodec::ExportCanonicalJson(Document);
    Document.Layout.Rooms.Swap(0, 1);
    const uint64 OrderedHash = FSeedForgeGenerator::ComputeCanonicalHash(Document.Layout);
    FString Unsorted = SeedForge::Tests::SwapFirstTwoRooms(*this, Canonical);
    Unsorted = SeedForge::Tests::ReplaceRequired(
        *this,
        Unsorted,
        FString::Printf(TEXT("\"canonicalHash\":\"%llu\""), Document.Layout.CanonicalHash),
        FString::Printf(TEXT("\"canonicalHash\":\"%llu\""), OrderedHash));

    const FSeedForgeDocumentResult Imported = FSeedForgeLayoutCodec::ImportCanonicalJson(Unsorted);
    TestFalse(TEXT("Order-dependent hash is rejected"), Imported.IsSuccess());
    TestEqual(
        TEXT("Order-dependent hash reports non-canonical data"),
        Imported.ErrorCode,
        ESeedForgeDocumentErrorCode::NonCanonicalData);
    return true;
}

#endif

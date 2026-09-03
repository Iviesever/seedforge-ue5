#include "Misc/AutomationTest.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeLayoutDiff.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SeedForge::DiffTests
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

    bool IsPointLess(const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y != Right.Y ? Left.Y < Right.Y : Left.X < Right.X;
    }

    bool IsRoomLess(const FSeedForgeRoom& Left, const FSeedForgeRoom& Right)
    {
        if (Left.Min != Right.Min)
        {
            return IsPointLess(Left.Min, Right.Min);
        }
        if (Left.Size.Y != Right.Size.Y)
        {
            return Left.Size.Y < Right.Size.Y;
        }
        return Left.Size.X < Right.Size.X;
    }

    template<typename ItemType, typename LessType>
    bool IsStrictlySorted(const TArray<ItemType>& Items, LessType Less)
    {
        for (int32 Index = 1; Index < Items.Num(); ++Index)
        {
            if (!Less(Items[Index - 1], Items[Index]))
            {
                return false;
            }
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeIdenticalLayoutDiffTest,
    "SeedForge.Diff.IdenticalIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeIdenticalLayoutDiffTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Document = SeedForge::DiffTests::GenerateDocument(*this, 101);
    const FSeedForgeLayoutDifference Difference = FSeedForgeLayoutDiffer::Compare(Document, Document);
    TestTrue(TEXT("Identical documents have an empty diff"), Difference.IsEmpty());
    TestEqual(TEXT("No rooms are added"), Difference.AddedRooms.Num(), 0);
    TestEqual(TEXT("No rooms are removed"), Difference.RemovedRooms.Num(), 0);
    TestEqual(TEXT("No cells are added"), Difference.AddedWalkableCells.Num(), 0);
    TestEqual(TEXT("No cells are removed"), Difference.RemovedWalkableCells.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeEndpointOnlyLayoutDiffTest,
    "SeedForge.Diff.EndpointOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeEndpointOnlyLayoutDiffTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Left = SeedForge::DiffTests::GenerateDocument(*this, 202);
    FSeedForgeLayoutDocument Right = Left;
    Swap(Right.Layout.Entrance, Right.Layout.Exit);
    Right.Layout.CanonicalHash = FSeedForgeGenerator::ComputeCanonicalHash(Right.Layout);

    const FSeedForgeLayoutDifference Difference = FSeedForgeLayoutDiffer::Compare(Left, Right);
    TestFalse(TEXT("Endpoint change is non-empty"), Difference.IsEmpty());
    TestTrue(TEXT("Entrance change is represented"), Difference.bEntranceChanged);
    TestTrue(TEXT("Exit change is represented"), Difference.bExitChanged);
    TestEqual(TEXT("Endpoint change adds no rooms"), Difference.AddedRooms.Num(), 0);
    TestEqual(TEXT("Endpoint change removes no rooms"), Difference.RemovedRooms.Num(), 0);
    TestEqual(TEXT("Endpoint change adds no cells"), Difference.AddedWalkableCells.Num(), 0);
    TestEqual(TEXT("Endpoint change removes no cells"), Difference.RemovedWalkableCells.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeStableLayoutDiffTest,
    "SeedForge.Diff.StableSortedTopology",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeStableLayoutDiffTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Left = SeedForge::DiffTests::GenerateDocument(*this, 101);
    const FSeedForgeLayoutDocument Right = SeedForge::DiffTests::GenerateDocument(*this, 202);
    const FSeedForgeLayoutDifference First = FSeedForgeLayoutDiffer::Compare(Left, Right);
    const FSeedForgeLayoutDifference Second = FSeedForgeLayoutDiffer::Compare(Left, Right);

    TestTrue(TEXT("Representative layouts produce room changes"), First.AddedRooms.Num() > 0 || First.RemovedRooms.Num() > 0);
    TestTrue(TEXT("Representative layouts produce cell changes"), First.AddedWalkableCells.Num() > 0 || First.RemovedWalkableCells.Num() > 0);
    TestTrue(TEXT("Added rooms use canonical order"), SeedForge::DiffTests::IsStrictlySorted(First.AddedRooms, SeedForge::DiffTests::IsRoomLess));
    TestTrue(TEXT("Removed rooms use canonical order"), SeedForge::DiffTests::IsStrictlySorted(First.RemovedRooms, SeedForge::DiffTests::IsRoomLess));
    TestTrue(TEXT("Added cells use canonical order"), SeedForge::DiffTests::IsStrictlySorted(First.AddedWalkableCells, SeedForge::DiffTests::IsPointLess));
    TestTrue(TEXT("Removed cells use canonical order"), SeedForge::DiffTests::IsStrictlySorted(First.RemovedWalkableCells, SeedForge::DiffTests::IsPointLess));
    TestTrue(TEXT("Repeated room additions are identical"), First.AddedRooms == Second.AddedRooms);
    TestTrue(TEXT("Repeated cell removals are identical"), First.RemovedWalkableCells == Second.RemovedWalkableCells);
    TestEqual(
        TEXT("Repeated JSON summaries are byte-identical"),
        FSeedForgeLayoutDiffer::ExportCanonicalJson(First),
        FSeedForgeLayoutDiffer::ExportCanonicalJson(Second));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeReverseLayoutDiffTest,
    "SeedForge.Diff.ReverseSwapsSets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeReverseLayoutDiffTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Left = SeedForge::DiffTests::GenerateDocument(*this, 101);
    const FSeedForgeLayoutDocument Right = SeedForge::DiffTests::GenerateDocument(*this, 202);
    const FSeedForgeLayoutDifference Forward = FSeedForgeLayoutDiffer::Compare(Left, Right);
    const FSeedForgeLayoutDifference Reverse = FSeedForgeLayoutDiffer::Compare(Right, Left);

    TestTrue(TEXT("Reverse additions equal forward removals"), Reverse.AddedRooms == Forward.RemovedRooms);
    TestTrue(TEXT("Reverse removals equal forward additions"), Reverse.RemovedRooms == Forward.AddedRooms);
    TestTrue(TEXT("Reverse cell additions equal forward removals"), Reverse.AddedWalkableCells == Forward.RemovedWalkableCells);
    TestTrue(TEXT("Reverse cell removals equal forward additions"), Reverse.RemovedWalkableCells == Forward.AddedWalkableCells);
    TestEqual(TEXT("Reverse left seed is forward right seed"), Reverse.LeftSeed, Forward.RightSeed);
    TestEqual(TEXT("Reverse right hash is forward left hash"), Reverse.RightHash, Forward.LeftHash);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeLayoutDiffSummaryTest,
    "SeedForge.Diff.DeterministicSummaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeLayoutDiffSummaryTest::RunTest(const FString& Parameters)
{
    const FSeedForgeLayoutDocument Left = SeedForge::DiffTests::GenerateDocument(*this, 101);
    const FSeedForgeLayoutDocument Right = SeedForge::DiffTests::GenerateDocument(*this, 202);
    const FSeedForgeLayoutDifference Difference = FSeedForgeLayoutDiffer::Compare(Left, Right);
    const FString Human = Difference.ToHumanSummary();
    const FString Json = FSeedForgeLayoutDiffer::ExportCanonicalJson(Difference);

    TestTrue(TEXT("Human summary identifies both seeds"), Human.Contains(TEXT("101 -> 202")));
    TestTrue(TEXT("Human summary reports room counts"), Human.Contains(FString::Printf(TEXT("rooms +%d/-%d"), Difference.AddedRooms.Num(), Difference.RemovedRooms.Num())));
    TestTrue(TEXT("JSON identifies left seed exactly"), Json.Contains(TEXT("\"leftSeed\":\"101\"")));
    TestTrue(TEXT("JSON identifies right seed exactly"), Json.Contains(TEXT("\"rightSeed\":\"202\"")));
    TestTrue(TEXT("JSON reports added room count"), Json.Contains(FString::Printf(TEXT("\"addedRoomCount\":%d"), Difference.AddedRooms.Num())));
    TestTrue(TEXT("JSON includes added room identities"), Json.Contains(TEXT("\"addedRooms\":[")));
    TestTrue(TEXT("JSON includes removed cell identities"), Json.Contains(TEXT("\"removedWalkableCells\":[")));
    return true;
}

#endif

#include "SeedForgeLayoutDiff.h"

bool FSeedForgeLayoutDifference::IsEmpty() const
{
    return true;
}

FString FSeedForgeLayoutDifference::ToHumanSummary() const
{
    return TEXT("PACT-22 diff is not implemented.");
}

FSeedForgeLayoutDifference FSeedForgeLayoutDiffer::Compare(
    const FSeedForgeLayoutDocument& Left,
    const FSeedForgeLayoutDocument& Right)
{
    return {};
}

FString FSeedForgeLayoutDiffer::ExportCanonicalJson(const FSeedForgeLayoutDifference& Difference)
{
    return TEXT("{}");
}

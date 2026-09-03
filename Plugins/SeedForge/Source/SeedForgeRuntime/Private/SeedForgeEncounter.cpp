#include "SeedForgeEncounter.h"

FSeedForgeEncounterResult FSeedForgeEncounterPlanner::Generate(
    const FSeedForgeLayout& Layout,
    const FSeedForgeEncounterConfig& Config)
{
    return FSeedForgeEncounterResult::Failure(
        ESeedForgeEncounterErrorCode::InvalidPlan,
        TEXT("PACT-31 RED encounter stub."));
}

FSeedForgeEncounterResult FSeedForgeEncounterPlanner::Validate(
    const FSeedForgeEncounterPlan& Plan,
    const FSeedForgeLayout& Layout,
    const FSeedForgeEncounterConfig& Config)
{
    return FSeedForgeEncounterResult::Failure(
        ESeedForgeEncounterErrorCode::InvalidPlan,
        TEXT("PACT-31 RED encounter validation stub."));
}

uint64 FSeedForgeEncounterPlanner::ComputeCanonicalHash(
    const FSeedForgeEncounterPlan& Plan,
    const FSeedForgeEncounterConfig& Config)
{
    return 0;
}

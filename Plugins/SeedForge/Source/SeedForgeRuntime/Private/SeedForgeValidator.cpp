#include "SeedForgeValidator.h"

FSeedForgeValidationResult FSeedForgeValidator::Validate(
    const FSeedForgeLayout& Layout,
    const FSeedForgeConfig& Config)
{
    return FSeedForgeValidationResult::Invalid(
        ESeedForgeErrorCode::NotImplemented,
        TEXT("Layout validation has not been implemented yet."));
}


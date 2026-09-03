using UnrealBuildTool;

public class SeedForgeTests : ModuleRules
{
    public SeedForgeTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "SeedForgeRuntime"
        });
    }
}


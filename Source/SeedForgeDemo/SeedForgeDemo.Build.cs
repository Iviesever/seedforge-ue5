using UnrealBuildTool;

public class SeedForgeDemo : ModuleRules
{
    public SeedForgeDemo(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "SeedForgeRuntime"
        });
    }
}


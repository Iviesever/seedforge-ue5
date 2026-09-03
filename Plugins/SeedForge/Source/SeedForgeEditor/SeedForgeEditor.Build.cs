using UnrealBuildTool;

public class SeedForgeEditor : ModuleRules
{
    public SeedForgeEditor(ReadOnlyTargetRules Target) : base(Target)
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

using UnrealBuildTool;

public class SeedForgeRuntime : ModuleRules
{
    public SeedForgeRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore"
        });

        PrivateDependencyModuleNames.AddRange(new[] { "Json", "ImageCore", "Slate", "SlateCore", "ApplicationCore" });
    }
}

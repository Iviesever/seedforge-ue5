using AutomationTool;
using EpicGames.Core;
using UnrealBuildBase;
using UnrealBuildTool;

namespace SeedForge.Automation;

public sealed class BuildPlugin_Win64 : BuildPlugin.TargetPlatform
{
    [Obsolete("UE 5.8 requires the FileReference UBT DLL overload.")]
    public override void CompilePluginWithUBT(string UBTExe, FileReference HostProjectFile,
        FileReference HostProjectPluginFile, PluginDescriptor Plugin, string TargetName, TargetType TargetType,
        UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration,
        List<FileReference> ManifestFileNames, string InAdditionalArgs)
        => throw new NotSupportedException("SeedForge's UE 5.8 adapter does not support the legacy string UBT executable overload.");

    public override void CompilePluginWithUBT(FileReference UnrealBuildToolDll, FileReference HostProjectFile,
        FileReference HostProjectPluginFile, PluginDescriptor Plugin, string TargetName, TargetType TargetType,
        UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration,
        List<FileReference> ManifestFileNames, string InAdditionalArgs)
    {
        ArgumentNullException.ThrowIfNull(UnrealBuildToolDll);
        ArgumentNullException.ThrowIfNull(HostProjectFile);
        ArgumentNullException.ThrowIfNull(HostProjectPluginFile);
        ArgumentNullException.ThrowIfNull(Plugin);
        ArgumentNullException.ThrowIfNull(ManifestFileNames);
        if (!Path.GetFileName(UnrealBuildToolDll.FullName).Equals("UnrealBuildTool.dll", StringComparison.OrdinalIgnoreCase))
        {
            throw new NotSupportedException("Adapter requires the stock UnrealBuildTool.dll entry point.");
        }
        string root = FindRepositoryRoot();
        SeedForgeBuildPlan plan = SeedForgeBuildArguments.Create(root, HostProjectFile.FullName,
            HostProjectPluginFile.FullName, TargetName, TargetType.ToString(), Platform.ToString(),
            Configuration.ToString(), InAdditionalArgs, Environment.GetCommandLineArgs());
        var manifest = new FileReference(plan.ManifestPath);
        if (ManifestFileNames.Any(value => value.FullName.Equals(manifest.FullName, StringComparison.OrdinalIgnoreCase)))
        {
            throw new ArgumentException("The target's manifest was already registered.");
        }
        ManifestFileNames.Add(manifest);
        Console.WriteLine($"SeedForge BuildPlugin adapter: {TargetName} {Platform} {Configuration}; x64; direct remote-disable.");
        CommandUtils.RunUBT(CmdEnv, UnrealBuildToolDll, HostProjectFile, TargetName, Platform, Configuration, plan.Arguments);
    }

    private static string FindRepositoryRoot()
    {
        DirectoryInfo? directory = new FileInfo(typeof(BuildPlugin_Win64).Assembly.Location).Directory;
        while (directory != null)
        {
            if (File.Exists(Path.Combine(directory.FullName, "SeedForge.uproject"))) { return directory.FullName; }
            directory = directory.Parent;
        }
        throw new NotSupportedException("Load the SeedForge adapter from this repository's project-local build output.");
    }
}

using System.Reflection;
using System.Runtime.Loader;
using System.Text.Json;
using SeedForge.Automation;

if (args.Length != 4) throw new ArgumentException("Expected repository, fixture, adapter DLL and EngineRoot.");
string root = Path.GetFullPath(args[0]);
string fixture = Path.GetFullPath(args[1]);
string adapterDll = Path.GetFullPath(args[2]);
string engine = Path.GetFullPath(args[3]);
var results = new List<object>();
int failed = 0;
string hostProject = Path.Combine(fixture, "synthetic package", "HostProject", "HostProject.uproject");
string hostPlugin = Path.Combine(Path.GetDirectoryName(hostProject)!, "Plugins", "SeedForge", "SeedForge.uplugin");
string extra = "  -NoPCH -NoSharedPCH -DisableUnity -Define=\"literal -architecture=arm64\"  ";
var beforeEnvironment = Environment.GetEnvironmentVariables().Cast<System.Collections.DictionaryEntry>()
    .Where(x => x.Key.ToString() == "UBT_EXTRA_ARGS" || x.Key.ToString()!.StartsWith("UnrealBuildTool_", StringComparison.OrdinalIgnoreCase))
    .ToDictionary(x => x.Key.ToString()!, x => x.Value?.ToString());
void Check(string name, Action action)
{
    try { action(); results.Add(new { name, passed = true, detail = "" }); Console.WriteLine($"PASS {name}"); }
    catch (Exception ex) { ++failed; results.Add(new { name, passed = false, detail = ex.ToString() }); Console.WriteLine($"FAIL {name}: {ex.Message}"); }
}
void Require(bool value, string message) { if (!value) throw new InvalidOperationException(message); }
void Reject(Action action) { try { action(); } catch (ArgumentException) { return; } catch (NotSupportedException) { return; } throw new InvalidOperationException("Unsupported invocation was accepted."); }
SeedForgeBuildPlan Make(string target = "UnrealEditor", string type = "Editor", string platform = "Win64",
    string config = "Development", string? plugin = null, string? project = null,
    string? additional = null, string[]? command = null)
    => SeedForgeBuildArguments.Create(root, project ?? hostProject, plugin ?? hostPlugin, target, type, platform, config, additional ?? extra, command ?? []);

foreach (var spec in new[] { ("UnrealEditor", "Editor", "Development"), ("UnrealGame", "Game", "Development"), ("UnrealGame", "Game", "Shipping") })
{
    Check($"stock target {spec.Item1} {spec.Item3}", () =>
    {
        var plan = Make(spec.Item1, spec.Item2, config: spec.Item3);
        string manifest = Path.Combine(Path.GetDirectoryName(hostProject)!, "Saved", $"Manifest-{spec.Item1}-Win64-{spec.Item3}.xml");
        Require(plan.ManifestPath == manifest, "Manifest identity differs.");
        Require(plan.Arguments.Contains($"-plugin=\"{hostPlugin}\"", StringComparison.Ordinal), "Plugin path lost its quoting.");
        Require(plan.Arguments.Contains($"-manifest=\"{manifest}\"", StringComparison.Ordinal), "Manifest path lost its quoting.");
        Require(plan.Arguments.Contains("-noubtmakefiles", StringComparison.Ordinal) && plan.Arguments.Contains("-nohotreload", StringComparison.Ordinal), "Stock build flags lost.");
        Require(plan.Arguments.Contains(extra, StringComparison.Ordinal), "Additional arguments were rewritten.");
        Require(plan.Arguments.Contains("-architecture=x64", StringComparison.Ordinal), "Win64 architecture is not explicit.");
        Require(plan.Arguments.EndsWith(" -UBADisableRemote", StringComparison.Ordinal), "Scoped disable flag missing.");
    });
}
Check("explicit x64 forwarded", () => Require(Make(command: ["-Architecture_Win64=x64"]).Arguments.Contains("-architecture=x64", StringComparison.Ordinal), "Explicit x64 lost."));
foreach (string architecture in new[] { "arm64", "x64+arm64", "", "x86" })
    Check($"reject architecture '{architecture}'", () => Reject(() => Make(command: [$"-Architecture_Win64={architecture}"])));
Check("reject duplicate architecture requests", () => Reject(() => Make(command: ["-Architecture_Win64=x64", "-Architecture_Win64=x64"])));
Check("reject architecture flag missing value", () => Reject(() => Make(command: ["-Architecture_Win64"])));
Check("ignore architecture-looking quoted unrelated field", () => Require(Make(command: ["-Note=literal -Architecture_Win64=arm64"]).Arguments.EndsWith(" -UBADisableRemote", StringComparison.Ordinal), "Unrelated field interpreted as architecture."));
Check("reject other plugin", () => Reject(() => Make(plugin: hostPlugin.Replace("SeedForge.uplugin", "Other.uplugin", StringComparison.Ordinal))));
Check("reject plugin outside host", () => Reject(() => Make(plugin: Path.Combine(fixture, "SeedForge.uplugin"))));
Check("reject host outside Artifacts", () => Reject(() => Make(project: Path.Combine(root, ".cache", "HostProject", "HostProject.uproject"))));
Check("reject relative host", () => Reject(() => Make(project: "HostProject/HostProject.uproject")));
Check("reject wrong host filename", () => Reject(() => Make(project: hostProject.Replace("HostProject.uproject", "Other.uproject", StringComparison.Ordinal))));
Check("reject Linux", () => Reject(() => Make(platform: "Linux")));
Check("reject other target", () => Reject(() => Make(target: "SeedForgeEditor")));
Check("reject wrong target type", () => Reject(() => Make(type: "Game")));
Check("reject Editor Shipping", () => Reject(() => Make(config: "Shipping")));
Check("reject Game Debug", () => Reject(() => Make("UnrealGame", "Game", config: "Debug")));
foreach (string argument in new[] { "-architecture=arm64", "-Mode=WriteMetadata", "-Project=other", "-plugin=other", "-manifest=other", "-log=other", "-UBADisableRemote=false", "-UBADisableRemote=true", "-UBAForceRemote", "--", "@args.rsp" })
    Check($"reject conflicting additional {argument}", () => Reject(() => Make(additional: argument)));
Check("reject unmatched additional quote", () => Reject(() => Make(additional: "-Define=\"unfinished")));
Check("direct switch is not duplicated", () => Require(Make(additional: "-NoPCH -UBADisableRemote").Arguments.Split("-UBADisableRemote", StringSplitOptions.None).Length == 2, "Disable flag duplicated."));
Check("inherited environment remains unchanged", () =>
{
    _ = Make();
    var after = Environment.GetEnvironmentVariables().Cast<System.Collections.DictionaryEntry>()
        .Where(x => x.Key.ToString() == "UBT_EXTRA_ARGS" || x.Key.ToString()!.StartsWith("UnrealBuildTool_", StringComparison.OrdinalIgnoreCase))
        .ToDictionary(x => x.Key.ToString()!, x => x.Value?.ToString());
    Require(beforeEnvironment.Count == after.Count && beforeEnvironment.All(x => after.TryGetValue(x.Key, out var value) && value == x.Value), "Inherited UBT environment mutated.");
});
Check("real adapter type and legacy overload reject without UBT execution", () =>
{
    string tool = Path.Combine(engine, "Engine", "Binaries", "DotNET", "AutomationTool");
    var paths = new[] {
        Path.Combine(tool,"AutomationScripts","Scripts","net10.0","AutomationScripts.Automation.dll"),
        Path.Combine(tool,"AutomationUtils","net10.0","AutomationUtils.Automation.dll"),
        Path.Combine(tool,"EpicGames.Core.dll"), Path.Combine(tool,"EpicGames.Build.dll"),
        Path.Combine(engine,"Engine","Binaries","DotNET","UnrealBuildTool","UnrealBuildTool.dll"),
        Path.Combine(tool,"Microsoft.Extensions.Logging.Abstractions.dll") };
    AssemblyLoadContext.Default.Resolving += (_, name) =>
    {
        string? path = paths.FirstOrDefault(x => Path.GetFileNameWithoutExtension(x) == name.Name && File.Exists(x));
        return path is null ? null : AssemblyLoadContext.Default.LoadFromAssemblyPath(path);
    };
    Assembly assembly = AssemblyLoadContext.Default.LoadFromAssemblyPath(adapterDll);
    Type type = assembly.GetType("SeedForge.Automation.BuildPlugin_Win64", throwOnError: true)!;
    Require(type.Name == "BuildPlugin_Win64" && type.BaseType!.FullName == "BuildPlugin+TargetPlatform", "Stock extension seam mismatch.");
    MethodInfo legacy = type.GetMethods().Single(x => x.Name == "CompilePluginWithUBT" && x.GetParameters()[0].ParameterType == typeof(string));
    object?[] values = legacy.GetParameters().Select(x => x.ParameterType.IsValueType ? Activator.CreateInstance(x.ParameterType) : null).ToArray();
    try { legacy.Invoke(Activator.CreateInstance(type), values); }
    catch (TargetInvocationException ex) when (ex.InnerException is NotSupportedException) { return; }
    throw new InvalidOperationException("Legacy adapter call did not explicitly reject.");
});
File.WriteAllText(Path.Combine(fixture,"argument-results.json"), JsonSerializer.Serialize(new { purpose="Synthetic argument contract; no UBT/UAT execution", total=results.Count, passed=results.Count-failed, failed, results },new JsonSerializerOptions{WriteIndented=true}));
Console.WriteLine($"AUTOMATION_EXTENSION total={results.Count} passed={results.Count-failed} failed={failed}");
return failed == 0 ? 0 : 1;

namespace SeedForge.Automation;

public sealed record SeedForgeBuildPlan(string ManifestPath, string Arguments);

public static class SeedForgeBuildArguments
{
    public static SeedForgeBuildPlan Create(string repositoryRoot, string hostProject, string hostPlugin,
        string target, string targetType, string platform, string configuration,
        string additionalArguments, IReadOnlyList<string> uatArguments)
    {
        ArgumentNullException.ThrowIfNull(additionalArguments);
        ArgumentNullException.ThrowIfNull(uatArguments);
        string root = FullPath(repositoryRoot).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        string project = FullPath(hostProject);
        string plugin = FullPath(hostPlugin);
        string artifactPrefix = Path.Combine(root, "Artifacts") + Path.DirectorySeparatorChar;
        if (!project.StartsWith(artifactPrefix, StringComparison.OrdinalIgnoreCase)
            || !Path.GetFileName(project).Equals("HostProject.uproject", StringComparison.OrdinalIgnoreCase)
            || !Path.GetFileName(Path.GetDirectoryName(project)!).Equals("HostProject", StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException("Adapter requires an owned Artifacts/.../HostProject/HostProject.uproject.");
        }
        string expectedPlugin = Path.Combine(Path.GetDirectoryName(project)!, "Plugins", "SeedForge", "SeedForge.uplugin");
        if (!plugin.Equals(expectedPlugin, StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException("Adapter supports only the SeedForge plugin in this exact HostProject.");
        }
        bool supportedTarget = (target == "UnrealEditor" && targetType == "Editor" && configuration == "Development")
            || (target == "UnrealGame" && targetType == "Game" && configuration is "Development" or "Shipping");
        if (platform != "Win64" || !supportedTarget)
        {
            throw new NotSupportedException("Adapter supports only stock Win64 UnrealEditor Development and UnrealGame Development/Shipping.");
        }

        // Stock BuildPlugin keeps Architecture_<Platform> in a private map.
        // Read its public invocation, reject ambiguity, and forward x64 explicitly.
        const string architecturePrefix = "-Architecture_Win64=";
        string[] requestedArchitectures = uatArguments.Where(value => value.StartsWith(architecturePrefix, StringComparison.OrdinalIgnoreCase)
            || value.Equals("-Architecture_Win64", StringComparison.OrdinalIgnoreCase)).ToArray();
        if (requestedArchitectures.Length > 1 || (requestedArchitectures.Length == 1
            && !requestedArchitectures[0].Equals(architecturePrefix + "x64", StringComparison.OrdinalIgnoreCase)))
        {
            throw new NotSupportedException("Only default or explicit Win64 x64 architecture is supported; no architecture request is discarded.");
        }
        string[] tokens = SplitArguments(additionalArguments);
        int disableCount = 0;
        foreach (string token in tokens)
        {
            if (token.Equals("-UBADisableRemote", StringComparison.OrdinalIgnoreCase)) { ++disableCount; continue; }
            string key = token.Split('=', 2)[0];
            if (token == "--" || token.StartsWith('@') || new[] { "-architecture", "-Mode", "-Project", "-plugin", "-manifest", "-log", "-UBADisableRemote", "-UBAForceRemote" }
                .Contains(key, StringComparer.OrdinalIgnoreCase))
            {
                throw new NotSupportedException($"Additional argument '{key}' conflicts with the owned build-only invocation.");
            }
        }
        if (disableCount > 1) { throw new ArgumentException("Additional arguments repeat the remote-disable switch."); }
        string manifest = Path.Combine(Path.GetDirectoryName(project)!, "Saved", $"Manifest-{target}-{platform}-{configuration}.xml");
        string arguments = $"-plugin=\"{plugin}\" -noubtmakefiles -manifest=\"{manifest}\" -nohotreload -architecture=x64";
        if (additionalArguments.Length != 0) { arguments += " " + additionalArguments; }
        if (disableCount == 0) { arguments += " -UBADisableRemote"; }
        return new SeedForgeBuildPlan(manifest, arguments);
    }

    private static string FullPath(string value)
    {
        if (string.IsNullOrWhiteSpace(value) || !Path.IsPathFullyQualified(value) || value.IndexOfAny(['"', '\r', '\n']) >= 0)
        {
            throw new ArgumentException("Build paths must be absolute and may not inject command-line quotes or lines.");
        }
        return Path.GetFullPath(value);
    }

    private static string[] SplitArguments(string value)
    {
        var tokens = new List<string>();
        var token = new System.Text.StringBuilder();
        bool quoted = false;
        foreach (char character in value)
        {
            if (character == '"') { quoted = !quoted; }
            else if (char.IsWhiteSpace(character) && !quoted)
            {
                if (token.Length > 0) { tokens.Add(token.ToString()); token.Clear(); }
            }
            else { token.Append(character); }
        }
        if (quoted) { throw new ArgumentException("Additional arguments have unmatched quotes."); }
        if (token.Length > 0) { tokens.Add(token.ToString()); }
        return tokens.ToArray();
    }
}

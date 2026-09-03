# Final handoff

## Recommended artifact

After final verification, run `Scripts/FinalizeRelease.ps1`. The newest complete delivery directory is recorded in:

```text
Artifacts/Final/LATEST.txt
```

The delivery contains:

- a source ZIP created by `git archive` from the verified revision;
- a standalone SeedForge plugin ZIP and adjacent SHA-256;
- a packaged Win64 demo ZIP and adjacent SHA-256;
- the latest UE Automation JSON/HTML report;
- build, smoke, package, and runtime logs;
- Editor and packaged screenshots;
- architecture, limitations, acceptance, AI disclosure, code walkthrough, interview guide, and rollback documents;
- `DELIVERY_MANIFEST.json` with path, size, and SHA-256 for every included file.

## Run the demo

Extract the demo archive and launch:

```text
Windows/SeedForge.exe
```

Use W/A/S/D and mouse look; Space/Ctrl or E/Q move vertically. A custom seed may be provided as `-SeedForgeSeed=<unsigned integer>`.

## Verify an archive

From PowerShell:

```powershell
Get-FileHash -Algorithm SHA256 .\SeedForgeDemo-Win64-*.zip
Get-Content .\SeedForgeDemo-Win64-*.zip.sha256
```

The values must match. The finalizer also verifies both release checksums before assembling the handoff.

## Rebuild everything

From a clean checkout with UE 5.8 installed:

```powershell
.\Scripts\VerifyAll.ps1 -EngineRoot 'D:\program\UnrealEngine\Epic Games\UE_5.8'
```

Do not use the project as evidence of independently hand-written C++ work. Read `AI_ASSISTANCE.md` and complete a personally authored, test-first change before presenting technical ownership.

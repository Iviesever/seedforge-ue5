# Final handoff

## Delivery location

After a clean `VerifyAll.ps1`, run `FinalizeRelease.ps1`. The selected delivery directory is written to:

```text
Artifacts/Final/LATEST.txt
```

The 0.2.0 delivery contains:

- source ZIP produced by `git archive` from the verified revision;
- independent plugin ZIP and SHA-256;
- packaged Win64 demo ZIP and SHA-256;
- 41-test UE Automation JSON/HTML report;
- two canonical documents, structural diff, 10k benchmark, per-command logs, and revision-aware report summary;
- build, smoke, capture, package, and packaged-run logs;
- Editor runtime, packaged runtime, and Inspector PNG evidence;
- architecture, schema, benchmark, limitations, acceptance, AI disclosure, walkthrough, interview, rollback, and release documents;
- Phase 1 and Phase 2 issue/plan/evidence journals;
- `DELIVERY_MANIFEST.json` for every payload and an adjacent manifest SHA-256.

The finalizer refuses to assemble if verification, report, plugin manifest, demo manifest, version, or Git revision disagree. It finishes by invoking `AuditDelivery.ps1` to re-enumerate and rehash the delivery independently.

## Re-audit

```powershell
$delivery = (Get-Content .\Artifacts\Final\LATEST.txt).Trim()
.\Scripts\AuditDelivery.ps1 -DeliveryRoot $delivery
```

## Run the demo

Extract `SeedForgeDemo-Win64-0.2.0-*.zip`, launch `Windows/SeedForge.exe`, and use W/A/S/D plus mouse look; Space/Ctrl or E/Q moves vertically. Use `-SeedForgeSeed=<uint64>` for another layout.

## Use the plugin

Extract `SeedForgePlugin-0.2.0-*.zip` to a UE 5.8 project's `Plugins/SeedForge` directory and rebuild. Runtime APIs live in `SeedForgeRuntime`; Commandlet and Inspector features are Editor-only.

## Interview warning

Do not present the repository as independently hand-written C++. Read `AI_ASSISTANCE.md`, reproduce the evidence, explain the architecture without notes, and complete a personally authored test-first change before claiming technical ownership.

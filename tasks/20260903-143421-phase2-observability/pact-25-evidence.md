# PACT-25 Release Evidence Contract

## Version and preservation

- Project/plugin version: 0.2.0.
- Generator version remains 1; the five 0.1.0 golden hashes are unchanged.
- Annotated `v0.1.0` remains fixed at `720ba4a8abc726add137d71c49572315f969f097`.
- Timestamped 0.1.0 final deliveries remain untouched.

## Pre-final verified milestones

- Canonical document focused/full tests: `pact-21-evidence.md`.
- Structural diff focused/full tests: `pact-22-evidence.md`.
- 10,000-seed report chain and 40-test regression: `pact-23-evidence.md`.
- Inspector entry gate, registration RED/GREEN, 41-test regression, and visual inspection: `pact-24-evidence.md`.
- A pre-Inspector 0.2.0 BuildPlugin passed Editor Development, Game Development, and Game Shipping.
- A pre-Inspector 0.2.0 Win64 BuildCookRun and packaged EXE smoke passed.

## Final authoritative evidence

The final release is authoritative only after a clean-revision `Scripts/VerifyAll.ps1` writes all of the following with the same revision/version:

- `Artifacts/Reports/verification-last.json`
- latest `Artifacts/Reports/Phase2/*/report-summary.json`
- `Artifacts/Plugin/last-plugin-package.json`
- `Artifacts/Package/last-package.json`

`Scripts/FinalizeRelease.ps1` then reads exact paths from those manifests, assembles the source/plugin/demo/docs/reports/logs/images payload, writes `DELIVERY_MANIFEST.json` plus its SHA-256, and invokes `Scripts/AuditDelivery.ps1` to re-enumerate and rehash every payload file.

The ignored `Artifacts/Final/LATEST.txt` is the pointer to the final delivery; its manifest is the source of truth for the final Git revision, version, file set, sizes, and checksums.

# Inspector shutdown EOS warning

## Reproduction and failed final candidate

Clean candidate `f395d603960ed64af1c2a01e6787c6b3f0904c6a` passed build/load, 119/119 complete Automation, all focused groups, 10k report, Editor ordinary input/gameplay and four negatives. `VerifyPhase3.ps1` then stopped at Inspector log validation. No BuildPlugin/package/final summary was produced for this candidate.

Failed log: `Artifacts/Logs/capture-inspector-20260904-202717-25893458c8614e7c8a764b1a37fc8153.log`. Inspector itself saved its expected image and requested exit 0 at lines 1862-1864. Editor shutdown began; an EOS product SDK config update completed later, then line 1921 emitted the only unexpected warning: `LogEOSSDK: Warning: LogHttp: ... request was cancelled`. Strict validation correctly rejected it.

The prior clean `5a91934` Inspector log (`201624`) loaded the same modules, but the EOS requests happened to complete before EOS shutdown and emitted no warning. That timing difference proves the warning is a nondeterministic external-request race, not a safe stable text to allow-list. The failed run remains failed.

## Ownership and minimum fix

Installed Engine descriptors are read-only. `Fab.uplugin` and `MetaHumanSDK.uplugin` are both EnabledByDefault Editor plugins and each enables `EOSShared` for Editor. EOSShared itself defaults false but its PostConfigInit runtime module initializes EOS SDK when either parent is enabled. The failed log mounts/loads Fab, MetaHumanSDK and EOSShared and starts online SDK config requests. SeedForge has no dependency on any of them; repository source contains no Fab/MetaHuman/EOS API use. Marketplace/MetaHuman/online services are outside the vertical slice's goals.

Do not silence the cancellation warning, delay exit for an unrelated network request, retry Inspector until timing is favorable, modify Engine descriptors or global plugin preferences. The project descriptor should explicitly disable the unused default roots Fab and Bridge plus their MetaHumanSDK dependency. Keep SeedForge/PythonScriptPlugin enabled; Python remains the deterministic map-rebuild tool. Disabling only EOSShared would conflict while a required parent remained enabled; disabling only Fab would leave Bridge -> MetaHumanSDK -> EOSShared. EOSShared itself is already default false and has no other default-true parent in this Engine, so an extra project entry is unnecessary.

Acceptance before implementation: a source harness requires exactly one explicit boolean policy for SeedForge/Python plus the three root/dependency plugins and no owned source dependency on disabled tooling. It failed first for missing Fab and then for missing default-enabled Bridge. An intermediate fixture additionally demanded explicit EOSShared=false; source/reverse-dependency review proved that unnecessary constraint, so it was removed before commit rather than adding unrelated configuration. After the minimal text change, build and Inspector must pass with no Fab/Bridge/MetaHumanSDK/EOSShared mount/load or EOS SDK request/shutdown lines, then the complete all-16 candidate must rerun on the new clean SHA.

This removes an unused Editor/network boundary; it does not add online functionality or claim every Engine plugin disabled. Packaged gameplay was not affected, because these dependencies are Editor-target-only. Review exact source/log ownership before committing.

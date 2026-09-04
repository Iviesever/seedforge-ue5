# Nested package evidence boundary

Contract: each successfully validated child process in PackageGameplay must immediately freeze its returned proof graph and raw summary/observation files. Subsequent input/gameplay/negative steps must not be able to change those bytes and have the outer manifest silently accept a new hash. Preserve the complete input/gameplay objects as well as PackageDemo's object. Before publishing the outer manifest, recheck every frozen child record; retain those expectations for the final aggregate.

Inspection found that only VerifyPhase3's top-level steps were snapshotted immediately. PackageGameplay retained PackageDemo's nested proof but reduced input and gameplay results to file paths. In particular, child summary and negative observation JSON paths had no producer digest and could be given a new baseline much later. The new harness runs the actual PackageGameplay script and shared contract against synthetic child producers, in artificial Git sentinel directories under Artifacts only. It does not contain a UE project or runtime and does not claim runtime evidence.

The eight tamper modes modify one already-returned package/input/gameplay/negative payload during a later child step. The unchanged control must publish once; each tamper must reject with a digest/size mismatch before publishing. Production orchestration changes follow an observed RED, then both PowerShell versions and existing shared/preflight regressions.

## Observed RED and GREEN

- Initial fixture launch was interrupted because its `Git` helper shadowed the native command (PowerShell names are case-insensitive). Renamed it `Invoke-FixtureGit`; no UE was launched, and that incomplete fixture is not counted as RED.
- Original-script RED at `PackageEvidenceSealing/20260904-183706-46a75a24d88641a4b3eb61da67c3cf78`: control passed; all eight tampered payloads were incorrectly accepted and published, 1/9.
- First post-fix fixtures used the Artifacts root itself as packageDirectory; the stricter graph correctly rejected that synthetic shape. Corrected it to the actual supported child-directory shape without changing the production path guard. Those 0/9 runs (`183819`, `183829`) are fixture failures, not GREEN or target-issue evidence.
- Re-ran the original unmodified PackageGameplay script retained in the initial fixture with the corrected fixture: `20260904-183919-af03683dd2ed4ce49b30765ede439ce8`, again 1/9. This independently confirms the same eight wrong acceptances, with the same valid directory shape as GREEN.
- Final PowerShell 7.6 GREEN: `20260904-183929-e046e63c5e394bab9d4da25c7602bbff`, 9/9.
- Final Windows PowerShell 5.1 GREEN: `20260904-183939-3b228301700f4108811ccaa74216a992`, 9/9.

The minimum caller fix wraps each existing child step with the unchanged source guard and an immediate evidence snapshot, rechecks frozen records before publishing, retains them in the outer manifest, and preserves nested input/gameplay proof objects. No native UE behavior, child validator, or warning policy was changed. Actual packaged execution and independent review remain required.

Read-only independent review found no P2 in the nested snapshot caller or its harness. The unchanged shared contract regression also passed 92/92 at `VerificationContract/20260904-184024-d9129d66b7d34b1a9b19f396c11622c8`. This closes the reproduced caller-level tamper gap locally; actual packaged execution remains pending.

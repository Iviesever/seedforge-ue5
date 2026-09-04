# SeedForge Rules and Workflow

## Scope boundary

- All SeedForge source code, assets, scripts, logs, reports, build outputs, planning documents, and temporary project files must remain under `D:\program\SeedForge`.
- User-approved exception (2026-09-04): UE/UBT's own `Trace*.uba` diagnostic and backup files may use `C:\Users\Iviesever\AppData\Local\UnrealBuildTool`. This does not permit moving project artifacts outside the repository or modifying Engine/global folder settings.
- Do not modify sibling directories under `D:\program` as part of SeedForge work.
- The installed Unreal Engine at `D:\program\UnrealEngine\Epic Games\UE_5.8` is read-only project infrastructure. Do not modify Engine files.

## Collaboration limit

- Use exactly one primary agent.
- The primary agent may use at most two sub-agents at any time.
- Sub-agents are optional and must receive concrete, non-overlapping tasks.
- Only one process may run UnrealBuildTool, AutomationTool, UnrealEditor, Cook, or Package against the integration checkout at a time.

## Development rules

- Lock an acceptance contract before each implementation change.
- Keep core behavior in reviewable C++ and text configuration.
- Minimize project-owned binary assets and never add unrelated Marketplace content.
- Preserve a last-known-good Git revision before risky integration work.
- Do not claim completion without fresh build, test, and smoke evidence.
- Record AI assistance accurately; do not describe generated code as independently hand-written by the user.

## Delivery workflow

1. Approve the technical blueprint and non-goals.
2. Create the repository and complete the minimal UE 5.8 C++ smoke build.
3. Implement one acceptance contract at a time using small, reviewable diffs.
4. Run focused tests after each contract and full Build/Test/Package gates at milestones.
5. Freeze features before the final release candidate.
6. Deliver source, packaged artifacts, verification evidence, known limitations, rollback information, and an interview study guide.

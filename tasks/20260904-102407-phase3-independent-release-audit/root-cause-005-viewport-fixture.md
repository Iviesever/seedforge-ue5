# Capture component fixture: discovery and Outer ownership

## Observations and exclusions

- The initial six pure lifecycle/receipt tests passed at `automation-20260904-121834`. The real component adapter compiled at `build-editor-20260904-122621.log`.
- Adding two component tests was initially missed by the cached build graph: `build-editor-20260904-122916.log` compiled only the Runtime change and the report still had six tests. No eight-test claim was made. A supported `-NoUBTMakefiles` build regenerated the graph, compiled the new test source, and discovered all eight cases (`build-capture-freshgraph-20260904-123027.log`).
- The fresh-graph build exposed a deprecated FSceneViewport constructor warning; changing to the UE 5.8 static Create(TStrongPtrVariant) API removed that warning. This API correction was separate from the runtime crash root cause.
- Both `automation-20260904-123036.log` and `automation-20260904-123117.log` exited 3 during ComponentCancelAndEndPlayUnbind. They report an invalid Outer ensure before the access violation; these are fixture failures, not product RED evidence.

## Root cause

`UGameViewportClient` is declared `UCLASS(Within=Engine, ...)` in the read-only engine header. The fixture used `NewObject<UGameViewportClient>(Fixture.World)`. Engine viewport operations call GetOuterUEngine(), so a World Outer violates the class ownership contract and leads to invalid access. Replacing only the FSceneViewport constructor could not repair this unrelated owner error.

## Next correction and boundary

Create the test client under `GEngine`, retain the actual transient World association only in FWorldContext.GameViewport, and use the supported FSceneViewport static factory. Restore the previous context viewport and release roots/references after testing. Do not change Runtime ownership or suppress the ensure. These tests simulate native delegate dispatch for cancellation/EndPlay cleanup; actual GPU rendering remains independently certified by Editor/package capture runs.

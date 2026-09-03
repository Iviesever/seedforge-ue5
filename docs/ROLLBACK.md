# Rollback and recovery

## Source checkpoints

- `ed2c49c` — delivery contract only.
- `bc7ca69` — verified UE 5.8 scaffold.
- `179e6cd` — deterministic generation core.
- `8782938` — topology/property verification.
- `1486e39` — cancellation-safe asynchronous generation.
- `69f0a81` — generated graybox and Editor capture.
- `5ede7af` — verified plugin package and Win64 RC1 pipeline.

Use a new branch or worktree when inspecting an earlier revision. Do not destroy the current branch with a hard reset.

## Artifact fallback

RC1 is retained under `Artifacts/Release/` with adjacent `.sha256` files. `Artifacts/Package/last-package.json` records the executable, screenshot, log, archive, and hash used for its packaged smoke.

RC2 must never replace RC1 in place; scripts create timestamped directories and archives. If final verification fails, deliver the newest independently verified candidate and document the newer failure rather than overwriting the good artifact.


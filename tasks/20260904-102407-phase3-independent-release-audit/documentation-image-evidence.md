# Documentation screenshot materialization

The primary inspected the clean packaged a9f56254f11554316302936926211e75d86d7f4d start/combat/win triplet at original 1280x720 resolution after PackageGameplay passed. HUD labels, counts, controls and terminal text are complete; combat and win objects are sharp. Start exposure is darker but the player, room and HUD remain visible. No pixel editing, brightening or cropping was performed.

Source run: `Artifacts/Media/Gameplay/20260904-194036-01908f1a1d5c4d23a4073bd3e12307f2/`. The three exact PNGs were copied byte-for-byte to tracked `docs/images/`; source and destination SHA256 were compared after copying.

| Tracked file | Original token | Bytes | SHA256 |
|---|---|---:|---|
| phase3-start-24301.png | e6ef5b144f00586370ce98b353837c4a | 343234 | a076e05707bf11ff41375d1f4edea5e26718281f4484c4e27201c86c0fce6849 |
| phase3-combat-24301.png | 37785a964c910ae50b2be79c05018800 | 630050 | dd4abdc09ae39bbfa573f36edb3e7b9d3d48d8c54ab6a78f4819c2adfa14ddc9 |
| phase3-win-24301.png | 97dd1b45470d4d200e0048b886045063 | 539729 | 5dab1312a8dca3fd7fe66baa2e593d0c83be1f57042d6001adfa4c3274d059d9 |

Original Artifact files remain unchanged. The replaced older tracked screenshots remain recoverable in Git history and are not accepted as current visual evidence. The documentation commit changes the candidate SHA, so the next full verification must create and review its own six new images; these illustrative copied images do not substitute for that final run.

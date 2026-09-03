# Acceptance matrix

| Goal requirement | Authoritative check | Current evidence | Status |
|---|---|---|---|
| UE 5.8 Editor target builds | `Scripts/Build.ps1` exit 0; binaries present | `evidence.md` PACT-00 | Passed |
| Headless project/plugin load | bounded commandlet, success marker, no fatal marker | `evidence.md` PACT-00 | Passed |
| Runtime plugin packages independently | `RunUAT BuildPlugin` for Editor/Game Dev/Game Shipping | `evidence.md` PACT-05 | Passed |
| Deterministic room/corridor generation | core Automation tests and golden hashes | `evidence.md` PACT-01/02 | Passed |
| Canonical hash | five committed seed/hash pairs | `evidence.md` PACT-02 | Passed |
| Invalid config fails explicitly | typed error-code tests | 25-test aggregate report | Passed |
| At least 1,000 valid seeds | deterministic property sweep | 10,000 seeds in PACT-02 extension | Passed |
| Sync repeat 100 times | complete-layout comparisons | five seeds x 100 | Passed |
| Async repeat 100 times | distinct sequential task/apply cycles | `SeedForge.Async.HundredSequentialRepetitions` | Passed |
| Worker compute/Game Thread apply | thread-id and Game Thread assertions | async coordinator suite | Passed |
| Pre/in-flight cancellation | immediate and gated-worker tests | async coordinator suite | Passed |
| Stale result cannot overwrite latest | two-request newest-wins test | async coordinator suite | Passed |
| UObject/world lifetime protection | weak apply + subsystem deinitialize suppression | subsystem suite and packaged runtime | Passed |
| Pure C++ graybox | HISM actor and generated map | source + inspected screenshot | Passed |
| Automated screenshot | bounded capture script and PNG size check | Editor and packaged PNGs | Passed |
| Win64 candidate | BuildCookRun and packaged EXE smoke | RC1 archive/checksum/log | Passed |
| One aggregate verification entry point | Build/Test/Smoke/BuildPlugin/Package orchestration | `Scripts/VerifyAll.ps1` | Pending final run |
| README and architecture | repository documents | README + `docs/ARCHITECTURE.md` | Passed |
| Test/build evidence and limitations | evidence journal and limitation document | task evidence + `docs/KNOWN_LIMITATIONS.md` | Passed |
| AI assistance disclosure | explicit non-authorship record | `docs/AI_ASSISTANCE.md` | Passed |
| Code/interview learning material | walkthrough and Q&A | `docs/CODE_WALKTHROUGH.md`, `docs/INTERVIEW_GUIDE.md` | Passed |
| Clean-revision final RC and rollback | aggregate final run and release manifest | RC2 | Pending final run |

The matrix is not complete until every Pending row is replaced by fresh final evidence.


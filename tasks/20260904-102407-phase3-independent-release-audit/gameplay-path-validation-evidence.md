# External H7 path-proof validation

Contract is the additive H7 section of `task-008.md`. `GameplayPathValidation.ps1` checks the serialized proof independently of native success: exact uint64 strings/types, fixed seed goldens, canonical bounded walkable/simple cardinal route, cell-to-world waypoints, same-run/request/revision, exactly two retained first/latest moves, finite segment/speed bounds, at least two observed moves/20 units, completion-freeze aggregate, zero passive bindings, and path completion no later than the first start/combat/win capture request. Native remains responsible for recomputing the exact A* path/expanded-node result; this script does not claim to reimplement the generator or A*.

The caller must also retain the existing source/process, whole-log, storage and three PNG/native-receipt checks. This helper neither opens images nor certifies source identity. The real smoke is not accepted merely because these synthetic tests pass. H6 input code/helper is unchanged; compatible serialized path checks follow the same rules, plus H7's first-capture boundary. Raw path/capture uint64 frames stay strings.

## Synthetic RED/GREEN

73 cases, including four acceptance controls (complete shape, same-frame last move/first capture, first/latest nonadjacent aggregate and off-center/small samples). The fixtures are visibly synthetic and reuse only H6's test fixture values; no runtime JSON or PNG is fabricated.

- RED stub rejected all four acceptance controls: 69/73, `GameplayPathValidation/20260904-104326-2a234a0a46254ef69b1588c47568bd90`.
- PowerShell 7.6 GREEN: 73/73, `20260904-104449-ff07d522fc304f9b8343710dc78f766c`.
- Windows PowerShell 5.1 GREEN: 73/73, `20260904-104506-78d316b85b9b4f2f8a3c725d5e9f5453`.

Harness timestamps above are UTC. Negative cases cover missing/miscased/typed fields, uint overflow/noncanonical values, wrong goldens/ownership, invalid bounds/route/waypoints, stale or non-increasing observations, off-plane/off-segment/backward/overspeed moves, nonfinite and derived overflow, aggregate count/distance/time inconsistency, continuation beyond completion, missing/reordered/wrong-ID capture correlation and a move after the first request.

The helper is now integrated into TestGameplay before the unchanged capture/PNG validator, and its compact pathProof is retained in the summary and nested package proof. Native H7 focused/live RED has been observed; real Editor and packaged GREEN, final independent review and clean verification remain required.

## Independent review: waypoint arrival

Review reproduced an impossible two-observation proof: switch to waypoint 1 while still 190 units from waypoint 0. Since count equals two, there are no omitted intermediate observations to explain the switch. The initial external checks accepted it. The same copied serialized-path logic in H6 had the same gap, so both external helpers were tightened after their separate REDs. Production movement/proof code was not changed.

Exactly-two-sample paths now permit at most one waypoint advance, and only after the prior endpoint is within the production 4.0 + 0.1 tolerance. Count greater than two still means first/latest samples may omit intermediate waypoints. A positive control actually reaching the prior waypoint verifies this is not an unconditional ban on advancement.

- H7 RED: 74/76 at `GameplayPathValidation/20260904-105042-5af6e577566c42aba0ff697c0f2628bd`.
- H6 RED: 158/160 at `InputSelfTestValidation/20260904-105103-46bc76b76a0a44ca9ec0358185cfa6e3`.
- H7 GREEN PS7.6 / PS5.1: 76/76 at `20260904-105146-5d818c4de80b462f8fb00c2a6eef79b4` / `20260904-105200-4eca856a77c747838a08778dfa0f467d`.
- H6 GREEN PS7.6 / PS5.1: 160/160 at `20260904-105206-9c79633bd0494db38ca258a8ed647296` / `20260904-105233-0c79036e39d249c7a323e34159d55427`.

These UTC-stamped synthetic results do not replace a fresh ordinary-input process after the final native checkpoint.

## Independent review: omitted displacement/time

`root-cause-compressed-path-proof.md` defines the next distinct aggregate gap and exact correction. Both external validators now require the first-to-last omitted movement's straight-line distance lower bound and enough hidden delta time, while retaining nonadjacent/turning-path compatibility. H7 native GREEN receives the same counterexample and aligned tolerances.

- H7 RED: 76/78, `GameplayPathValidation/20260904-105952-932d1ce87cc44d1b97ee111904d235ac`.
- H6 RED: 160/162, `InputSelfTestValidation/20260904-110012-41a6184cdccf409484d420a44250aea5`.
- H7 GREEN PS7.6 / PS5.1: 78/78 at `20260904-110106-d2e7cc95f27243dab84125664a11c889` / `20260904-110133-c2fae038cb82405e8187f8d6f139f9dd`.
- H6 GREEN PS7.6 / PS5.1: 162/162 at `20260904-110136-872269c22bb74dfca42a3a234e0d2be8` / `20260904-110207-165fc336042c4a0cbfa4b63eeb0f284f`.

Read-only reparse of the prior real `InputSelfTest/20260904-182403-f3c129ef902948ed981db36ddb31adb8` trace still passes with 24 observations and 20.4186055064195 total units. This verifies compatibility with those original bytes, not a new runtime execution or new source certification.

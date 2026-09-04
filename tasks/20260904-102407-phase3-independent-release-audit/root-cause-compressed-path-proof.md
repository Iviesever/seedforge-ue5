# Compressed path-proof consistency

## Ownership and observations

Production enemy ticks produce each real movement observation; the passive proof validates each consumed waypoint/revision and bounds the data. To avoid unbounded telemetry, a completed trace retains only the first and last movement samples plus total count/distance/delta. External codecs must not mistake two retained samples for the complete movement history when count is greater than two.

Independent review identified two distinct missing consistency checks in the external H7 helper and the same H6 serialized-path logic. Neither requires changing movement, A*, capture, input injection or trace storage limits.

1. **Count equals two:** no observations were omitted. A record switched to waypoint 1 while still 190 units from waypoint 0. Merely requiring nondecreasing indices and matching the newly claimed target accepted an impossible transition. The REDs were H7 74/76 and H6 158/160; fixed by requiring at most one advance and the production 4.0 + 0.1 arrival bound. Actual-arrival advancement remains a positive control.
2. **Count greater than two:** checking only retained-distance sum does not cover the gap. The review counterexample retains first y1000->995 (5), last y850->840 (10), count3/totalDistance20/totalDelta.15. Its minimum actual distance is 5 + 145 + 10 = 160. A separate counterexample has a feasible 8-unit gap/distance23 but virtually no omitted movement time. Both were accepted before the aggregate correction. Existing nonadjacent positive control first5/gap8/last10 with dt.15 remains valid.

## Correction contract before implementation

Keep the observed-count distinction. For count greater than two, compute only constant-time necessary bounds:

- `Gap = Distance(first.to, last.from)`.
- `TotalDistance + 0.1 >= firstDistance + Gap + lastDistance` (triangle lower bound, independent of turns or omitted waypoint count).
- `HiddenDt = TotalDeltaSeconds - firstDelta - lastDelta >= -1e-6`.
- `Gap <= 260 * max(0, HiddenDt) + 0.1 * (count - 2) + 0.1`.

The tolerances are aligned with the native H7 validator and existing finite/speed tolerances. No intermediate sample is invented and no equality/adjacency is imposed on count>2. All derived values must be finite. Existing completion-freeze threshold and first/last sequence capacity checks remain in place.

The authoritative ordinary-input evidence still combines native independent model/A* validation with external serialized/process/source checks. The new H7 native codec receives the same triangle/time cases while being implemented after its separately observed RED. These checks establish necessary consistency, not a cryptographic proof of all omitted events or human playability.

Original failed logs/fixtures are retained. Re-run both full external harnesses, actual ordinary-input and H7 Editor/package paths before final certification; synthetic GREEN alone cannot close H.

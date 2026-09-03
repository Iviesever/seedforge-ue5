# Canonical layout format v1

## Identity

- Schema name: `seedforge.layout`
- Schema version: `1`
- Generator version: `1`
- Encoding: compact UTF-8 JSON without a BOM
- `seed` and `canonicalHash`: exact base-10 JSON strings in the full `uint64` range

Schema version describes the document contract. Generator version describes topology/hash semantics. Import rejects unsupported versions; 0.2.0 does not migrate them.

## Fixed field order

Canonical export writes top-level fields in this order:

1. `schema`
2. `schemaVersion`
3. `generatorVersion`
4. `seed`
5. `canonicalHash`
6. `config`
7. `layout`

Configuration order is grid width/height, room count, min/max room width, min/max room height, padding, then placement attempts. Layout order is entrance, exit, rooms, then corridor cells. Points are `[x,y]`; rooms are `{"min":[x,y],"size":[width,height]}`.

Rooms use the generator's canonical minimum-coordinate/size comparator. Corridor cells use Y/X order and are unique. Export normalizes these collections and recomputes the canonical hash.

## Import pipeline

1. Parse a JSON object.
2. Require every v1 field with its exact JSON type.
3. Validate schema and versions.
4. Parse unsigned decimal strings manually with overflow checks and no signs or leading zeros.
5. Validate generation configuration bounds before accepting topology.
6. Parse rooms, corridors, and endpoints as exact signed 32-bit integers.
7. Compute the order-dependent input hash, normalize collections, then recompute the canonical hash.
8. Reject a hash tied to non-canonical input order; reject every other mismatch.
9. Run the full layout validator, including BFS connectivity.

Unknown object fields are ignored so a future writer can add optional metadata without breaking a v1 reader. Missing required fields never receive silent defaults.

## Typed failures

`ESeedForgeDocumentErrorCode` distinguishes malformed JSON, missing field, wrong field type, unsupported schema, unsupported schema version, unsupported generator version, invalid unsigned integer, non-canonical data, hash mismatch, invalid configuration, and layout validation failure. Messages include a JSON-like path and actionable detail.

## Example

```json
{"schema":"seedforge.layout","schemaVersion":1,"generatorVersion":1,"seed":"24301","canonicalHash":"7425849530159566348","config":{"gridWidth":48,"gridHeight":48,"roomCount":10,"minRoomWidth":4,"maxRoomWidth":9,"minRoomHeight":4,"maxRoomHeight":9,"roomPadding":1,"maxPlacementAttempts":512},"layout":{"entrance":[7,5],"exit":[35,42],"rooms":[{"min":[4,2],"size":[7,7]}],"corridorCells":[[7,9]]}}
```

The abbreviated topology above illustrates shape only; because its rooms/corridors do not match the shown hash/configuration, strict import correctly rejects it. Generate valid documents with `Scripts/Report.ps1` or the Inspector.

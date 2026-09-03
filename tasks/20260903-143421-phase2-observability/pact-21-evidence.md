# PACT-21 Evidence

## RED

- Contract commit: `85dcd3d`
- Filter: `SeedForge.Document`
- Result: 0 passed, 6 failed against the deliberate codec stub.
- Log: `Artifacts/Logs/automation-20260903-151228.log`

## GREEN

- Editor build: passed with UE 5.8 / MSVC 14.44.
- Focused filter: 6 of 6 document tests passed.
- Focused log: `Artifacts/Logs/automation-20260903-152016.log`
- Full regression: 31 of 31 SeedForge tests passed with zero warnings.
- Full log: `Artifacts/Logs/automation-20260903-152213.log`

The codec preserves exact unsigned 64-bit decimal strings, emits fixed-order compact JSON, normalizes ordered collections before integrity checks, rejects order-dependent hashes, validates configuration and topology, and ignores unknown fields.

# SheetsAnalyzer

`LoadChart` parses SUS with ChartConverter's XLAIR mode. `compilation/` converts
absolute beats to audio samples and preserves the game's curve subdivision,
hold judgement generation, and combo calculation.

Source interpretation follows ChartConverter, including overlapping side carriers,
`TIL` tick positions, and omission of unsupported notation. The former game-only
`xlair_directional_mode` request is no longer interpreted. Hold paths use the note's
speed group; the shared model has no per-point speed groups. Unsupported kinds that
reach the game adapter are rejected.

The game offset comes from `ChartLoadOptions::offset_seconds`; source `WAVEOFFSET`
is not added. FFI views remain local to loading and no borrowed pointer escapes.

For a local ChartConverter checkout, configure CMake with
`-DFETCHCONTENT_SOURCE_DIR_CHARTCONVERTER=/absolute/path/to/ChartConverter`.

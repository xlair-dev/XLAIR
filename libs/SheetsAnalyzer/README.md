# SheetsAnalyzer

`LoadChart` reads SUS bytes through ChartConverter's C API in XLAIR mode, then
compiles the shared chart representation into the game's sample-based `Chart`.
The public game model and `LoadChart` API remain independent of the FFI.

## Responsibilities

- ChartConverter owns text decoding, SUS syntax, measure lengths, note kinds,
  side-button mapping, parent relations, and source speed groups.
- `compilation/` converts rational absolute beats to audio samples, resolves
  scroll timelines, subdivides slider curves, generates hold judgement points,
  and computes the game's combo count. It does not parse SUS commands.
- `ChartLoadOptions::offset_seconds` remains the offset supplied by song metadata.
  Source `WAVEOFFSET` is not added, preserving the existing game clock contract.
- The Rust result is owned by `LoadChart` until compilation completes. No borrowed
  FFI pointer escapes into the game model; concurrent loads own separate results.

## Interpretation changes

SUS interpretation now follows ChartConverter's XLAIR mode. In particular, side
carriers can match overlapping lanes, and unsupported source notation may be omitted
and reported by ChartConverter rather than rejected by the old parser. The former
game-only `xlair_directional_mode` request is not interpreted by the game anymore.
`TIL` tick positions also follow the converter's XLAIR-mode interpretation rather
than the former game's beat-based tick conversion.
Source parser messages, including line numbers when available, are returned as
load errors with the source path.

Notation that reaches the game adapter but cannot be represented by its model
(such as EX/AIR variants outside side X-tap relations or gradual scroll changes)
is rejected. Hold paths use the note's speed group supplied by ChartConverter;
the shared model does not assign different speed groups to individual path points.

## Build

CMake fetches pinned ChartConverter and Corrosion revisions. Rust nightly via
rustup is required; Corrosion selects the target and native link dependencies.
The C header is generated from the Rust definitions while building the static
library. To work on a local ChartConverter checkout, configure with
`-DFETCHCONTENT_SOURCE_DIR_CHARTCONVERTER=/absolute/path/to/ChartConverter`.

The source-format parser tests live in ChartConverter. Game-facing fixture tests
remain under `tests/public/`, and curve/judgement generation tests under
`tests/compilation/`.

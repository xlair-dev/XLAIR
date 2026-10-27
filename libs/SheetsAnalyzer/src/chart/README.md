# Chart loading

SUS and UGC are parsed by the pinned ChartConverter C API package in
`cmake/ChartConverter.cmake`. No Rust toolchain or SUS conversion is needed at build
or runtime. The public `LoadChart` API and runtime `sheets::Chart` stay independent
of the C ABI.

`Loader` owns each C API result with RAII. `Compiler` borrows its IR only while
copying notes, paths, and timelines into the runtime chart; no borrowed pointers
escape. Structured parser diagnostics retain the source path and line.

## XLAIR interpretation

- The frontend runs in XLAIR mode. SUS defaults to the exact Tap 1 carrier policy
  (`xlair_directional_mode consume_tap`), prepended in memory. An explicit request
  in the file overrides it, and diagnostic lines exclude the synthetic prefix.
- A diagonal AIR child of a simultaneous XTap becomes one side XTap. Its parent is
  not emitted as a main-slider note. The ChartConverter IR itself is unchanged.
- Absolute rational beats are integrated across tempo changes before rounding to
  audio samples. Audio-file time is chart time plus `ChartLoadOptions::offset_seconds`
  minus source `WAVEOFFSET` / `BGMOFS` (zero when absent). Positive source offsets
  delay audio relative to the chart, while the additional metadata
  `music_offset_seconds` keeps its existing additive note-offset behavior. The
  combined offset applies once to notes, tempos, scrolls, and hold judgements.
- Global speed steps multiply the selected speed group. Group zero is a real
  group; unassigned notes use a separate default timeline. Point-level group
  selections are retained, including groups without scroll events.
- ChartConverter expands SUS Bezier handles before exposing the shared IR.
  Every IR point is on the path: `Control` and `Invisible` become hidden runtime
  points and are joined linearly, just like authored UGC controls. XLAIR never
  evaluates or resamples a Bezier curve again.
- Hold judgement generation remains an XLAIR responsibility. Judgement lanes use
  the same piecewise-linear path as rendering. Only musical intervals and visible
  checkpoints add judgements or combo; hidden points retain their own timelines.

## Supported subset

The runtime supports Tap, XTap / ExTap, Flick, slider Slide paths (including
Invisible and Control points), side taps / XTaps, and side Hold paths. ExTap and
Flick directions do not change XLAIR's existing judgement rules.

Unsupported note kinds (for example Mine, Tap 4–6, EX long notes and AIR long
notes), duration-based scrolls, and lane/note-scoped scrolls return errors rather
than being downgraded. Parser warnings currently describe omitted information,
so a warning also prevents a partial chart from being played.

UGC support does not mean every UGC feature is playable: speed fields such as
`SPDDEF` / `SPDFLD` are rejected by ChartConverter, and SUS `MEASUREHS` is not
represented in the current IR. Visual note attributes (height, roll, priority)
are not used by XLAIR's renderer.

Source syntax regression tests live in ChartConverter. Tests here cover public
file loading, XLAIR-specific mapping, timing, speed composition, normalized paths,
and judgement/combo generation.

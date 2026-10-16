# Judgement

The audio playback sample position is the chart clock. Input is judged at that
position minus the sum of `input.latency_offset_seconds` in `config.toml` and
the player's `judgment_offset_ms`. A positive offset compensates late input;
it does not move the rendered notes.

Tap, side-note, and hold-start timing windows are approximately ±33 ms
(Perfect), ±67 ms (Great), and ±100 ms (Good). XTap is Perfect within the Good
window. Hold checkpoints currently award Perfect while the relevant slider
lane or side button is held in the Great window, and Miss after the Good
window. These checkpoint grades and the score/gauge formula are provisional.

Flick requires a contact near its chart position followed by horizontal
movement across at least two lanes. Its contact and movement windows are
provisional and need validation with the physical controller.

An input is assigned to the closest pending note in its timing window. Nearby
overlapping notes retain a midpoint boundary even after one has been judged.

# SUS (Sliding Universal Score) format parser and analyzer

Spec: https://gist.github.com/kb10uy/c171c175ba913dc40a73c6ce69da9859

## XLAIR directional-note mode

Some SUS editors require every directional note (`#mmm5x`) to overlap a short note. By default, XLAIR treats an
overlapping Tap 1 at the same position, lane, width, and hispeed timeline as an editor-generated carrier. The carrier
is omitted from the compiled slider notes, so the pair contributes only the directional side note and one combo.

A matching Tap 2 produces a side XTap instead. It is also omitted from the slider notes. Carrier matching happens
before sample rounding and requires the same position, lane, width, and hispeed timeline. Tap 1 carriers take
precedence when both kinds are present. Multiple directional notes may share a Tap 2 carrier.

The default can be stated explicitly:

```sus
#REQUEST "xlair_directional_mode consume_tap"
```

Use the following XLAIR-specific request when the overlapping Tap 1 and directional note must remain two independent
notes (this also applies to Tap 2):

```sus
#REQUEST "xlair_directional_mode independent"
```

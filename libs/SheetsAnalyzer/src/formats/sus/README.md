# SUS (Sliding Universal Score) format parser and analyzer

Spec: https://gist.github.com/kb10uy/c171c175ba913dc40a73c6ce69da9859

## XLAIR directional-note mode

Some SUS editors require every directional note (`#mmm5x`) to overlap a short note. By default, XLAIR consumes an
overlapping Tap 1 or Tap 2 at the same position, lane, width, and hispeed timeline. Tap 1 with a direction becomes a
regular side note; Tap 2 (X-Tap) with a direction becomes a side X-Tap. The consumed short note is omitted from the
compiled slider notes, so the pair contributes one combo. If both kinds match, Tap 2 takes precedence.

The default can be stated explicitly:

```sus
#REQUEST "xlair_directional_mode consume_tap"
```

Use the following XLAIR-specific request to keep the short note and directional note independent. In this mode, the
directional note remains a regular side note:

```sus
#REQUEST "xlair_directional_mode independent"
```

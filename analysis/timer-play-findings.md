# TIMER / PLAY blinking pair

User supplied captures/manual/timer-play-blinking.sal and identified the TIMER
and timer PLAY symbols (not the transport play triangle). The source is unchanged;
exported CSV and84 complete frames per bus are in the sibling *-raw directory.
No player command was sent.

Numeric byte0 has four values:

| Byte0 | Count | TIMER/PLAY candidate bits (mask0C) |
|---|---:|---|
|01|40|00|
|05|3|04|
|8D|39|0C|
|89|2|08|

SLEEP bit0 stays on independently. Bit7 is the previously observed regular
phase bit. Bits2/3 are newly active in this labelled timer/play recording and
follow the visible blink, with each bit's transitions separated by~0.51s.
The rising/falling edges are staggered one~31.9ms frame:04 precedes0C and08
precedes00. This recording alone does not assign each bit to an individual
physical TIMER versus PLAY segment. It also does not map timer REC or clock.

Firmware exposes symbols.timer_play_raw (byte0&0C) and a combined timer_play
boolean (either bit set). The UI lights/dims TIMER and timer PLAY as the observed
pair. It does not claim individually isolated bit assignments; the one-frame
edge separation is coalesced into the combined pair. Both raw edge states remain
available for future comparison. No autonomous CSS blink timer is used; the
normal250ms polling follows received states, with scheduling-dependent phase.

Native replay checks all84 numeric frames, four raw-mask states, combined on/off,
and unchanged SLEEP state. Browser checks cover pair-on/pair-off and existing
SLEEP/source/MD/radio/volume states. A timer REC or a separately isolated TIMER
reference would help assign individual segment bits later.

## Separate TIMER, PLAY and REC bits

The user subsequently supplied `captures/manual2/timer-and-play-flashing.sal`
and `captures/manual2/timer-and-rec-flashing.sal`. The first capture has byte0
masked by `0E` cycling through `00`, `04`, `08`, `0C`; the second cycles through
`00`, `02`, `08`, `0A`. The common bit `08` is TIMER, the play-only bit `04`
is timer PLAY, and the record-only bit `02` is timer REC. At edges they change
one 31.9 ms frame apart. The original combined `timer_play` value has been
replaced with the PLAY bit, and independent `timer` and `timer_rec` values are
exposed; `timer_play_raw` retains the original byte0 mask `0C` for diagnostics.
The web symbols follow their individual bits without an independent blink timer.

A third user-labelled capture `TIMER-PLAY-is-lit-also-CLOCK-icon-is-lit.sal`
has byte0 masked to `1C`; bit `10` is the clock icon candidate. Among the
exported captures scanned, this is the only capture with bit `10` set. Other
state also differs from the flashing captures, so the bit has not been
independently isolated. The API and UI expose it as an inferred clock icon,
with dashed candidate styling. A same-state clock-off reference is still needed
to confirm it.

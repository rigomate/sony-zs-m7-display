# Display symbol inventory

Reference: images/IMG_6467.jpeg. The web UI represents the visible faceplate
symbols with text, inline SVG disc/clock drawings and a position scale. This
is a readable reconstruction, not an exact rendering of the original segments.
No external fonts, images or scripts are needed by the MCU-hosted page.

## Connected to decoded state

- MHz and kHz: selected by the observed tuner layout; adjacent to the number.
- PRESET: byte4 bit1 in tuner frames. Byte7 bit5 also changes in the reference
  pair; its independent purpose remains unknown.
- MD, CD, RADIO and LINE source text labels: numeric byte3 bits0..3,
  isolated by the source-cycle recording with MD removed.
- MD graphic command00 is fully off independently of selected source.
- MD play/pause and disc visual: observed playback/pause command patterns;
  playback wedge timing is reconstructed, paused wedges are all lit.
- TIMER / timer PLAY: paired byte0 mask0C; individual bit assignments remain
  unisolated (raw mask retained).
- OVER20: inferred numeric byte5 mask04 from20/21-disc comparisons; dashed
  candidate styling and tooltip, no visual confirmation yet.
- STEP / TRACK labels: numeric byte5 masks80/01, supported by labelled
  program-entry state and earlier normal/date/source layouts.
- PGM: numeric byte5 mask08 from both captured flashing phases.
- SHUFFLE: numeric byte5 mask10 from the user-labelled capture.
- REPEAT and1: numeric byte5 masks40/20 from paired user references.
- FM: observed numeric C0 pattern (FM/MHz bits not yet individually isolated).
  Radio prefix glyphs identify FM/MW/LW; clock AM/PM stay unmapped.
- DATE: numeric byte3 bit6, isolated from MD source with paired captures.
- SLEEP: numeric byte0 bit0 from user-confirmed on/off captures.
- Twelve text cells, numeric fields and indicators1–20 retain existing decoding.
  In radio mode the numbered indicators remain visible but dotted/unmapped.

Mapped symbols use on/off classes and their signal becomes stale with numeric
bus freshness. With no recognised mode their state is unknown rather than off.

## Visual placeholders pending protocol mapping

CD play and pause, CD disc graphic, CD-to-MD arrow;
DIGITAL, REC, TOC EDIT; STEREO, MONO;
clock and timer REC; AUTO; clock AM, PM, TV, colon;
the REC prefix beside DATE, REMAIN; POSITION,0%,100%, scale and four pointers.

These have dotted styling and unmapped tooltips/accessible labels. They are
not presented as live off bits, and the position bar has no invented progress.
FM/AM/PM in the photo are left unmapped as individual physical segments even
though the tuner band is known: their exact bit and mode roles need isolation.
The visible numeric fields already provide track, time, frequency and decimal
punctuation; STEP/calendar modes still need decoding.

Display controls are passive. No UI switch forces an indicator or transmits
commands to the player. Future mapping can update each named symbol separately.

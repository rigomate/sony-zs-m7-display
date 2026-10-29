# Constant-title disc: track, time, indicators

## Evidence and scope

The user inserted a 20-track disc whose track titles are all TEST. Captures
are in `captures/test20/`: one 140-second startup/playback recording and
nineteen 16-second recordings containing one track_next command each.
Native .sal and raw transition CSV files are retained. `run.jsonl` records
capture IDs, commands, adapter replies, and save/export completion.
Only play and track_next were sent. The original display is dead, so none
of the findings claim visual confirmation of lit segments.

## Sampling correction

The prior falling-edge/MSB-first interpretation sampled data near its change
and produced frame-dependent numeric values. Sampling the **rising edge** and
packing the first bit as bit 0 yields clean ASCII on SDI2 and stable numeric
fields on SDT1. With idle-high clock this corresponds to mode 3, LSB first.
The frame source now emits these logical bytes. Old printed hex values are
not directly comparable without accounting for both changes.

This was checked against saved data, without changing analyzer wiring. The
corrected interpretation also passes every prior capture framing test and
all twelve title regression cases. Merely reversing the old bytes does not
repair samples taken at the wrong edge.

## Field map (zero-based logical byte offsets)

| Bus | Bytes | Meaning in measured normal MD playback |
|---|---|---|
| SPI-2 SDI2 | 2-13 | Twelve ASCII character cells; 0xff unused suffix |
| SPI-1 SDT1 | 11 | Current track, packed BCD (0x10 = track 10) |
| SPI-1 SDT1 | 13 | Elapsed minutes, packed BCD |
| SPI-1 SDT1 | 14 | Elapsed seconds, packed BCD, 00-59 |
| SPI-1 SDT1 | 7, 8, low nibble of 9 | Twenty instantaneous track indicator bits |

Indicator bits map in ascending order: byte 7 bit 0 = track 1, through byte 7
bit 7 = track 8; byte 8 covers tracks 9-16; byte 9 bits 0-3 cover tracks 17-20.
In this experiment, preceding-track bits are clear, future-track bits are set,
and the current-track bit remains set. Thus the bitmask conveys current and
remaining tracks in this run. Byte 11 supplies the current track directly.
A separate flag toggles, but its relationship to visual blinking is unknown. Do not interpret the whole
byte 9 as indicator bits; its high nibble contains other flags.

The long capture includes consecutive second increments and both
00:59 -> 01:00 and 01:59 -> 02:00. The elapsed value tracks capture time within
one second of phase quantization after playback startup. Each sweep capture
begins with the previous track before changing to the next BCD number and
resetting elapsed time. These independent patterns support the field semantics;
IR ACK count alone is not the basis for decoding.

## Decoder boundary

`decode_numeric_display` exposes track, minutes, seconds, and the instantaneous
20-bit mask. It rejects incomplete frames, invalid BCD, seconds above 59,
and tracks outside the tested 1-20 range. It is validated for normal MD playback,
not tuner/CD/editing/remaining-time modes. Valid BCD alone cannot prove mode.
The host can combine this with `decode_title_window` for an OLED display.

No integrity/checksum rule or playback-mode flag has been assigned. Other
header/trailer fields remain unknown. Minutes above 02, tracks above 20,
disc-total/remaining time, and the complete long-title scroll require further
evidence. Physical signal voltage and driver ownership must be established
before choosing a replacement-controller electrical interface.

## Reproduction

Build and run `ctest --test-dir build --output-on-failure`.
`captured_numeric_display` checks every settled frame in all twenty recordings:
BCD track agrees with sequence, time advances with capture time, indicator bits
have the predicted pattern, and TEST appears repeatedly.
The long recording must contain two minute rollovers. This is a cross-capture
behavioral test, not a synthetic copy of the decoder logic alone.

`analysis/extract_frames.py <digital.csv> ...` optionally writes logical frames
as JSON next to a raw CSV. The raw CSV is the primary retained evidence.

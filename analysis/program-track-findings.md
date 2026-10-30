# Program-entry selected track

User supplied captures/manual/program-track-01-to-20-to-01.sal, describing the
three-digit field above TRACK. Although the message mentioned01..10..01, the
capture contains the full sequence01,02,...,20,01, matching its filename.
The original file is unchanged. Raw export and extracted JSON are in the
sibling *-raw directory.590 complete numeric and590 complete text frames were
extracted. No player command was sent.

## Selected-track encoding

Numeric byte11 is packed BCD:01..09,10..19,20,then01. Across visible phases
byte12=00. Blanked selection phases use byte11=AA and byte12=0A. There are532
visible numeric frames and58 blank frames. These are deliberate received
blank phases rather than missing bus traffic. Byte5 bit0(TRACK label) is also
clear during those phases; STEP remains on and PGM continues its own blink.

Byte10 stays01 throughout this capture, plausibly the program-step index, but
the recording does not increment that field, so no step-index decoder is claimed.
No track above20 or separate hundreds digit was exercised. The current decoder
accepts valid low-two-digit BCD only while byte12=00, so it must not be claimed
to support three-digit values100..999. Three-digit UI padding is presentation,
not a recovered general hundreds-digit map.

## Firmware

A separate program object carries selected_track and track_blank, detected in
the observed MD STEP/program-entry layout. It does not reuse MD playback numeric
fields. The UI marks MD PROGRAM, labels the field Program track and formats
visible values001..020, matching its three-cell role. Elapsed time is hidden;
received AA/0A blank phases blank the selection. No local blink timer or program
playlist is inferred. Program-step count and programmed-list indicators remain
unmapped. Leaving the observed STEP layout clears the program object.

Native replay verifies all590 frames, the full1..20..1 sequence, blank phases
and a controlled exit to ordinary MD display. Existing MD/PGM/state checks and
browser selected1/20/blank/exit cases provide compatibility coverage.

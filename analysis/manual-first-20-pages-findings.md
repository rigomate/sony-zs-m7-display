# Display behavior from the first 20 PDF pages

Source: `ZS-M7.pdf`, PDF pages 1–20 only. This is a scanned PDF; the
display diagrams and page text were inspected in addition to OCR. The scan's
PDF page 6 is blank, so the first steps of whole-CD recording cannot be
recovered from this range. Page 7 continues that operation. The PDF's page
numbers match the printed numbers for the cited pages below.

This file separates what the manual says about visible behavior from bus
encodings established by captures. The manual alone cannot determine a byte,
bit, or packet layout.

| Screen or event | Manual evidence and expected display | Current coverage / gap |
|---|---|---|
| CD playback, stop | pp. 4–5, 12: playing shows track, time and position pointer; stop shows total tracks, total time, position pointer and calendar. | The compact display currently falls back to CD source selected; CD track/time/calendar and pointer need captures. |
| CD DISPLAY cycle | p. 13: current track remaining; tracks left plus disc remaining; back to current track elapsed. Disc remaining / tracks left is unavailable in repeat, program and shuffle modes. | No complete capture-backed CD cycle or semantic classification yet. |
| MD stop | pp. 8, 13: disc name, total track count, total playing time and music calendar; DISPLAY shows remaining recording time and the position pointer. | Track/time fields are decoded for observed stopped-MD frames, but the compact page calls the count “Track”; the remaining-recording-time variant and pointer need validation. |
| MD playback DISPLAY cycle | p. 14: current-track remaining and pointer; tracks left, disc remaining and disc name; recording date/time; return to current-track elapsed and title. Disc remaining / tracks left is unavailable in repeat, program and shuffle modes. | The user-provided cycle now covers all four screens and is replay-tested; see `analysis/md-playback-display-cycling.md`. The pointer encoding and a populated recording date/time remain unconfirmed. |
| OVER 20 | pp. 12–13: the mark appears when a CD or MD has more than 20 tracks, not only when track 21 is selected. | The bit is inferred, not visually isolated; the compact page does not expose the mark. |
| Track 100+ | p. 15 explicitly gives the MD remote operation for track numbers of 100 or more. | Track and program decoders currently reject the unverified hundreds field. Do not invent its encoding. |
| Program entry | p. 17: up to 20 programmed tracks; CD and MD examples show playing order, track number, total programmed time and the programmed-track calendar simultaneously. p. 18 says total programmed time is not shown during playback; PGM Check steps through stored tracks. | Observed MD selected-track field and PGM/STEP flags are partly decoded; program order, total time, calendar semantics and CD programming need captures. |
| Shuffle / repeat | pp. 16, 19: single/all repeat and combinations with shuffle or program; shuffle cannot go to the previous track. | Repeat, one, shuffle and PGM flags have captures; combinations and transitions still need replay evidence. |
| Whole-CD recording | p. 7: pointer represents CD playback and MD recording positions; MD track number and remaining recording time are shown. TOC EDIT flashes after stopping; a whole-CD recording cannot be paused. `CD>MD OK?` can alternate with time when the MD lacks space. | Only the specific observed radio-to-MD recording layout is decoded; CD recording, both position markers, TOC EDIT and this message are not established. |
| Radio / presets | pp. 10–11, 20: band and frequency, FM stereo/mono, and AUTO PRESET flashing while auto-presetting. Up to 12 stations per band. | Frequency and an inferred preset flag have captures; STEREO, MONO, AUTO, preset selection and AUTO PRESET blink require further isolation. |

## Recommended capture sequences

Keep the player and disc constant within each sequence, capture both display
buses, and record the visible state and each button action. Replay raw Saleae
transitions through the existing native decoder; add both positive and
negative checks rather than assuming the printed display design specifies the
wire format.

1. **MD DISPLAY cycle:** capture normal elapsed playback and each of the four
   DISPLAY presses in order, including multiple seconds per state and the
   transition back. Note current track, tracks remaining, disc/track title and
   whether the physical REMAIN / DATE / REC marks are lit. Repeat while stopped
   for remaining recording time.
2. **CD baseline and DISPLAY cycle:** capture stopped and playing CD screens,
   each DISPLAY press, and pause. Include a CD with more than 20 tracks if
   available; record whether OVER 20 is visible while playing a track below 21.
3. **Program entry:** separately capture MD and CD stopped, enter PGM, add
   several known nonconsecutive tracks one by one, run PGM Check, start
   playback and stop. Record order, selected track, total programmed time,
   calendar and all blink phases.
4. **Three-digit track:** if a suitable MD is available, compare tracks 99,
   100 and 101 with their displayed titles. Keep source and mode identical.
5. **Indicators:** compare AUTO PRESET during its flashing phase against the
   normal tuner screen; isolate TOC EDIT and both position pointers using
   user-controlled captures when practical.

Clock and timer sections are outside the first 20 PDF pages and are not used as
evidence in this report. The capture plans above do not transmit any command or
alter the player.

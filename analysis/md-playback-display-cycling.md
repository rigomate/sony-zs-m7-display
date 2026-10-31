# MiniDisc DISPLAY cycle

Source: `captures/manual2/md-playback-display-cycling.sal`, exported to
`md-playback-display-cycling/raw/digital.csv` and replayed from the raw
Saleae transitions. The user reported playback while pressing DISPLAY. The
sequence matches page 14 of the manual; semantic names below follow that
sequence, rather than a visual reading of the dim physical display.

| Approx. time | SPI-1 significant bytes | Observed fields / interpretation |
|---|---|---|
| 0–1.1 s | `b3=01 b4=00 b11=01 b12=00` | Track 1, elapsed time about 00:16–00:17, track title. |
| 1.2–3.5 s | `b3=91 b4=00 b11=01 b12=00` | Track 1 remaining time about 04:38–04:35. SPI-2 mode `02` replaces ordinary title text; the pointer encoding remains unknown. |
| 3.6–5.7 s | `b3=91 b4=10 b11=17 b12=00` | 17 tracks left, disc remaining about 73:24–73:22; SPI-2 scrolls the disc name. `17` is a count, not the current track. |
| 5.8–8.0 s | `b3=61 b4=00 b11=AA b12=0A b13=FF b14=FF` | Recording-date screen: SPI-2 has `--y`, a zero/blank cell, `--m --d`; time bytes are unavailable. Do not invent a date or time. |
| 8.1–10.7 s | `b3=01 b4=00` | Returns to track 1 elapsed time and title. The capture then repeats the first two remaining screens. |

Playback frames use `b1=20`, disc animation byte `b2=01`, and TIMER PLAY
was armed. The distinction `b3=91,b4=10` versus `b3=91,b4=00` is a captured
combination; this does not independently prove the name of any one flag in
all other player states. Similarly, although mode `02` appears in the
current-track remaining screen where the manual shows a position pointer,
the position graphic and its parameters are not decoded.

The API `md_display.view` reports `elapsed`, `track_remaining`,
`disc_remaining`, or `recording_date` for these playback signatures.
`tracks_left`, remaining minutes/seconds, calendar bits, and validated
recording date text are separate fields. When a recording time's bytes are
invalid or blank, its hour/minute are null; a real stamped date/time still
needs an independently labelled capture. The legacy `numeric.track` and
`track_display.number` are suppressed for the two non-track screens to avoid
misidentifying a count as the playing track. The compact calendar retains
the received bits without highlighting a current track in the disc view.

`captured_md_display_cycle` exercises all four views, the countdown and return
to elapsed from the actual capture, including the unset date. Browser tests
cover the disc remaining and recording-date screens. Confirmed elapsed,
track-remaining, radio recording and timer capture regressions remain in the
suite.

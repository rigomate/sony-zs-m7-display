# Wake timer On hour setting, live capture

Passive capture: `captures/manual2/live-timer-hour/capture.sal`, with exported
`raw/digital.csv` and `raw/frames.json`. The user said the hour was selected
and blinking. All 189 complete numeric frames show MD source, timer PLAY on,
minutes `31` in byte14, and byte13 alternating `16` and `AA`. The alphanumeric
text is `On` throughout. The observed time was therefore 16:31, with byte13
blanking for the selected hour. Numeric byte0 bit08 (TIMER) alternates; bit04
(timer PLAY) stays on.

Hour transitions repeat every approximately 1.0198 s, with about 0.5099 s in
each phase, based on the full capture. TIMER follows the hour blanking edge one
frame later, approximately 31.87 ms. The capture's numeric layout includes
`b1=00,b2=07,b3=01,b4=00,b5=02,b6=07,b7=00,b8=00,b9=80,b10=AA,b11=AA,b12=0A`;
byte6 includes volume and should not be assumed fixed. This is a wake timer
setting with text `On`; it is not the previously captured DATE clock setting.
The C3 exposes a separate `timer_setting` for this observed numeric layout when
the fresh text bus displays `On`. It retains the latest hour through the blank
phase; the webpage shows the actual hour blink. A timer `Off` setting capture
would establish the contextual differences. No player commands were sent.

## Timer On minute setting

The user selected minutes and a second passive capture was saved to
`captures/manual2/live-timer-minute/`. The same numeric signature and `On` text
were present in 188 frames. Byte13 remained `16`; byte14 alternated `31` and
`AA`, confirming minute selection and its blank phase. Each phase lasted about
0.510 s. The decoder retains both values and identifies edit focus from which
pair blanks; it waits for a blank phase before claiming the selected field. The
webpage follows the observed minute blanking as well as hour blanking. Timer
Off setting remains unverified.


## Live On/Off entry and timer volume, 2026-10-30

User-guided passive Saleae captures were collected for On hour and minute,
Off hour and minute, and timer volume. No IR/player commands were transmitted.
The first file in serial-infra was provisionally named timer-off-hour-live;
its actual text was On, confirmed by the user and wire. It is retained here
as captures/manual2/live-timer-hour-layout0. Additional evidence is in
live-timer-on-minute-layout0, live-timer-off-hour, live-timer-off-minute,
and live-timer-volume under captures/manual2. Each includes a native .sal,
raw digital CSV, parsed frames and sampled ESP32 API state.

On time is 16:31, Off time 17:31. The newly captured numeric layout has byte2=00
rather than the older byte2=07; this caused timer_setting=null in the old API.
Both variants are now accepted with the remaining framing guards intact.
Hour and minute selection are identified by AA blanking in bytes13/14.
The fresh exact text On or Off supplies phase=on/off. Retained time and focus
are cleared on phase changes so start values cannot leak into stop entry.
The GUI labels them Timer start and Timer stop, retaining values but hiding
the received blank digit pair. The same raw time layout does not establish
clock versus timer context without its text signature.

Timer volume is 21/31 in this capture. The numeric body is:
 b1=00 b2=00 b3=01 b4=00 b5=00 b6=15/35 b7=00 b8=00 b9=80
 b10=AA b11=AA b12=0A b13=AA b14=AA.
Byte6 low five bits carry level21 and bit20 follows the visible phase. The
text bus alternates mode05/level15 with space-filled tail and ordinary blank
text. The existing ordinary volume parser expected zero-filled tails and did
not recognize this command. A separately guarded timer_volume_setting is now
exposed with level, blank and fresh; the GUI labels Timer volume and follows
its blink phase. This avoids treating timer volume as an hour/minute value.
The full min-to-max timer-volume range has not been swept.

Validation: all 27 prior CTest cases passed for the timer-time change. Updated
time regression then passed all six old/new hour/minute captures, and the new
timer-volume replay passed both phases and exit guard. Focused Chromium checks
passed clock entry, start/stop time, and timer-volume visible/blank rendering.
The full dashboard script has an unrelated paused-symbol fixture mismatch
when given the newly generated API fixture; focused timer checks were run
separately. ESP32-C3 firmware build succeeded. Flash/live verification is
recorded in firmware/esp32/TENSTAR-C3-VALIDATION.md.


Final live checks: the user briefly moved to source selection while testing.
That screen can overlap the volume numeric shape in its source-lit phase.
The volume API now additionally requires a recent mode05/level/space-tail
text command, clearing that context when another numeric layout arrives.
A negative source-selection regression prevents this false classification.
The user then confirmed volume was blinking; a new passive capture at level23
(live-timer-volume-23) yielded 35 blank and33 visible sampled API states, all
fresh and with level23. This is independent confirmation at another level.

The full dashboard script subsequently passed when supplied its expected
legacy fixture shape (the real capture's numeric/title data with optional
symbols and track_display omitted so synthetic overrides remain consistent).
Clock/time focused checks and both timer-volume blink phases also passed.
The previous paused-symbol failure was a fixture conflict, not a UI regression.


## Timer On hour with retained track indicators

User reported On-hour blinking physically but not in the GUI after returning
to timer play setup. Passive capture live-timer-on-hour-retained-tracks records
16:31, exact On text, byte13 alternating16/AA, and timer PLAY. The pre-fix API
reported timer_setting=null for all68 sampled states (clock_display and
clock_setting were also null). The cause was a rejected numeric layout, not
an active normal-clock view: bytes7–9 are FF FF83 rather than the prior000080.
Byte2 is07. The clock readout had preserved a track-indicator tail into timer
entry. Whether playback/history always explains this tail remains unverified.

Timer time decoding now accepts both captured tails independently of the
already observed byte2 variants00/07. Other shape checks, timer flags, fresh
On/Off context, BCD ranges and blink retention remain unchanged. New replay
coverage requires both visible and blank On-hour phases and retained16:31.
The same capture is a negative fixture for normal clock_readout classification,
using actual capture timestamps. All four focused clock/time/volume regression
suites passed, and the ESP32-C3 build succeeded. No IR commands were sent.

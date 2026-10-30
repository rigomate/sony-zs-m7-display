# Clock readout and colon blink — 2026-10-30

User reported steady hour/minute digits and a blinking colon. Passive capture
captures/manual2/live-clock-colon contains native Saleae, raw digital CSV,
parsed complete frames, and simultaneous pre-fix API samples. No IR/player
commands were sent. Text is 26y 10m 30d; numeric clock reads18:55.

The376 complete numeric frames contain two body patterns. Bytes13/14 remain
18/55 (BCD), while byte5 alternates00/02 and byte0 bit80 co-varies. The other
observed fields: b1=00,b2=07,b3=41,b4=00,b6=07,b7=FF,b8=FF,b9=83,
b10=AA,b11=AA,b12=0A. Each colon phase lasts0.5099s, full period1.0198s.
The user identifies the blinking segment as colon. Exact bit isolation and
physical phase polarity have not been independently verified because byte0
bit80 and byte5 bit02 co-vary. API colon_inferred=true records that limitation;
colon_on follows byte5 bit02 rather than an independent CSS animation.

This is a date/clock readout while MD is selected, not a timer On/Off editing
layout. The existing clock-setting decoder requires volume byte6=0A, so this
readout at7 was previously missed. Clock-setting captures can share portions
of the layout, and also contain date text. A separate clock_display is therefore
only activated after both digit pairs stay present through a full blink cycle
(>=1100ms with colon state change). Blanking either pair resets that detector.
This avoids displaying normal Clock during the captured hour/minute edits.
Only the captured MD-stopped/date layout is recognized; unobserved standby,
radio and playing clock layouts are not asserted supported.

GUI shows Clock with steady HH/MM, hiding only the colon's span during its
blank phase. The clock view takes priority over a potential clock_setting
candidate after the stable-clock detector confirms it. Stale bus handling and
other timer/volume views are preserved.

Validation: clock-readout replay verifies18:55 and both colon phases; old
hour/minute edit captures are replayed with timestamps to ensure they never
become clock_display. Clock-setting, timer-setting and timer-volume regressions
also pass. Focused Chromium checks pass steady-clock colon-on/off markup and
all clock/timer entry views. ESP32-C3 build passed; upload/live validation is
recorded in firmware/esp32/TENSTAR-C3-VALIDATION.md.


## Standby, TIMER PLAY and solid MiniDisc graphic

The user reported standby with time19:07 and supplied an image of the lit
rectangular MiniDisc graphic. Passive capture live-standby-clock-md-icon was
made after the minute advanced to19:08. All377 complete numeric frames retain
BCD19/08; byte5 alternates00/02 and byte0 alternates0C/8C. Byte2 is07, byte3
(source) is00, track-tail bytes7–9 are000000, byte6 is07, and bytes10–12 are
AA AA0A. Text input is mode01 with an all-zero message payload; returned ASCII
is blank. TIMER and timer PLAY bits are steady on; other source logos are off.

A separately guarded standby signature now feeds clock_display with
standby=true. The existing full-cycle confirmation and editing exclusions
remain. The solid graphic is recognized for byte2=07 in this captured standby
signature independently of a selected MD source. It does not imply playback
or animation. The GUI shows STANDBY, Standby clock, blank alphanumeric cells,
and the solid MiniDisc symbol; it avoids the unknown-numeric warning for a
recognized clock. Other unobserved standby signatures remain unsupported.

Replay verifies19:08, both colon phases, standby=true and md_disc=solid, while
retaining normal clock and negative editing fixtures. Timer entry and volume
regressions pass; Chromium checks include the standby clock and static icon.

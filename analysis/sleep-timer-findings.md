# Sleep timer display

Reference: user-labelled
`captures/manual2/SLEEP-blinking-with-10-20-30-60-90-120-on-display.sal`.
The6s Saleae recording has355 complete frames per bus; exported raw CSV and
paired frames are in the sibling `raw/` folder.

The recording begins in ordinary MD playback, then enters sleep-duration
selection. Numeric bytes11/12 show the selected timer as packed BCD: byte11
holds the lower two digits, byte12=`0A` blanks the hundreds digit, and
byte12=`01` adds the leading hundred. Thus the observed values decode as:

| Minutes | byte12 | byte11 |
|---:|---:|---:|
|10|0A|10|
|20|0A|20|
|30|0A|30|
|60|0A|60|
|90|0A|90|
|120|01|20|

The time digits in bytes13/14 are `AA/AA` while this menu is active. The timer
number itself alternates with the blank pair byte11=`AA`, byte12=`0A`; the UI
retains the last selected value through these phases. These blank phases occur
independently of the SLEEP lamp blink (numeric byte0 bit0). The API exposes a
separate `sleep_timer` object; the page labels the large numeric field “Sleep
timer”, appends “min”, hides elapsed time and track indicators, and restores
normal track display when the numeric frame leaves this narrowly matched
sleep-setting layout.

Mode match is constrained to the observed MD/play layout and stable surrounding
bytes, to avoid interpreting ordinary track/time fields as the sleep duration.
The capture demonstrates the displayed choices and digit positions, but does
not prove timer countdown after confirmation or automatic player power-off.
No remote command was transmitted.

## Live radio-mode fix

After the first implementation, the user reported that the page remained blank
when pressing SLEEP. A passive live capture showed the player was in RADIO mode;
the timer digits were present, but the SPI-1 envelope differs from the MD
playback capture. In the radio layout, bytes0..10 are
`00/81 00 00 04 02 00 0A 04 00 80 AA`; the same byte11/12 timer values and
byte13/14 `AA/AA` blank time fields follow. The earlier decoder only accepted
the MD envelope, so it returned no timer state despite the live SLEEP lamp.

The decoder now recognizes both captured MD and radio envelopes. The user's
live 10-second capture is saved at
`captures/manual2/live-sleep-debug/capture.sal`, with raw frames and API samples
alongside it. Its complete numeric frames repeat the same six choices and
blink blanking. The browser now has an explicit radio-mode timer case. No
player commands were sent.

# New remote controls and radio/clock indicators — 2026-10-30

The user explicitly requested testing the remote's new features and finding the
FM/AM and clock AM/PM indicators. Serial help/status/list on the Hades2001 USB
adapter confirmed zs-m7 and the added radio_band, tune_up, tune_down and date
commands. Only those four player commands were used. No stop, recording, eject,
power or timer-setting command was sent. Captures, acknowledgments and snapshots
are under captures/remote-indicators; analysis/capture_remote_indicators.py is an
explicit hardware runner with a restricted command whitelist.

## Controlled transitions

| Experiment | Result | Numeric byte4 | Text prefix bytes2/3 |
|---|---|---|---|
|01 radio_band from MD|FM87.50MHz|C0|8D 8B|
|02 radio_band|MW531kHz|20|8B 8C|
|03 tune_up|MW540kHz|20|8B 8C|
|04 radio_band|LW153kHz|20|8A 8C|
|05 date|DATE on, unset clock/date|00|ASCII --y --m --d |
|06 date|Return from DATE view|see exported trace|see exported trace|
|07 radio_band|FM87.50MHz|C0|8D 8B|
|08 tune_up|FM87.55MHz|C0|8D 8B|
|09 tune_down|FM87.50MHz|C0|8D 8B|

IR ACK functions: radio_band address100/function111, tune_up100/115,
tune_down100/116; date address68/function99. All are SONY15. ACK proves
transmission; captured before/after changes independently verify their effect.
The player is left on FM87.50MHz. MW's tuned value changed531->540 during testing;
no stored presets were edited.

Some saved captures/exported traces are shorter than the requested6s and the
Saleae close operation returned ReadTimeout. Their existing raw exports were
retained and parsed. Experiment06 has incomplete before/after metadata because
close raised before it was saved; the command was not repeated. The runner now
saves metadata before closing. Other command ACKs and snapshots are preserved.

## What is established

FM's observed numeric flag pattern isC0 (bits7+6); MW and LW use20 (bit5).
Bit20 accompanies kHz. The two FM bits always change together in these tests,
so this evidence cannot individually assign bit80 to FM and bit40 to MHz.
The UI's FM visual follows the observed C0 pair; it does not claim an isolated
single-bit assignment. No analogous radio-AM indicator pattern appears in these
European-band captures.

The radio text prefixes identify custom glyphs8D=F,8B=M,8C=W,8A=L in this
specific display format. Firmware now reports radio.display_band FM/MW/LW
(falling back to AM if the AM sub-band is not identified). The web renderer
translates only these recognised radio-prefix layouts and their blank parameters;
it retains the existing handling of arbitrary MD text/control values.

## Clock AM/PM limits

The user checked the clock/date view and confirmed DATE lit. Captured text is
--y --m --d with numeric FF/FF, consistent with an unset clock. No clock-AM or
clock-PM lit reference was obtained. They remain unmapped; the UI does not light
AM merely because the receiver selects MW or LW.

Sony's European instructions explicitly specify a24-hour clock (printedp42),
while the US instructions specify12-hour time. The observed FM/MW/LW band set
matches the European variant, so 12-hour AM/PM states may not be produced by
this unit. No clock settings or model/region configuration were changed.
Primary sources:
https://www.sony.com/electronics/support/res/manuals/3861/38614452M.pdf
https://www.sony.com/electronics/support/res/manuals/3861/38614451M.pdf

## Validation boundary

Native replay checks all controlled band/tune/date experiments, the FM visual
flag and final FM/MW/LW identification, alongside existing tuner/source/state
checks. Browser fixtures check FM on, MW/LW glyphs and leaving clock AM unknown.
These do not establish separate numeric FM/MHz bit roles or dormant AM/PM bits.

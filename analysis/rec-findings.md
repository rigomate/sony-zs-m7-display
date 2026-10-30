# Main REC-on reference

User reported REC lamp on. Passive6s capture:
captures/manual/rec-on/capture.sal.187 complete numeric and187 complete title
frames were exported; current selected source is FM87.50MHz. No player command
was transmitted, including no record, play, pause or stop action.

Numeric byte1 is70 throughout, versus00 in the settled FM87.50MHz portion of
captures/remote-indicators/07-return-fm. Previously labelled playback/pause
references identify byte1 bit5(20) as the MD play triangle and bit4(10) as pause.
The remaining bit6(40) is the REC illumination flag.
Other settled control bytes1–10 match except byte1. Timer-field bits are clear;
no automatic TIMER REC mapping is inferred. The user confirmed this is the main
boxed REC below DIGITAL. TIMER REC and the REC prefix beside DATE remain
unmapped.

Play and pause indicators can be lit independently of selected source: this
reference is on RADIO while MD transport flags20/10 are set, consistent with
record-pause/standby. The capture does not prove media is actively being written;
record-enabled/armed/paused distinctions must not be inferred from REC alone.
Raw time/frequency fields remain FM87.50MHz. Firmware maps the main REC visual
to byte1 bit6 and browser tests cover on/off. The mapping was included in the
verified C3 upload recorded in `firmware/esp32/TENSTAR-C3-VALIDATION.md`.

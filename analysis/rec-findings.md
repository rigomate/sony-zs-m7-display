# REC-on reference (label location pending)

User reported REC lamp on. Passive6s capture:
captures/manual/rec-on/capture.sal.187 complete numeric and187 complete title
frames were exported; current selected source is FM87.50MHz. No player command
was transmitted, including no record, play, pause or stop action.

Numeric byte1 is70 throughout, versus00 in the settled FM87.50MHz portion of
captures/remote-indicators/07-return-fm. Previously labelled playback/pause
references identify byte1 bit5(20) as the MD play triangle and bit4(10) as pause.
The remaining bit6(40) is therefore the candidate REC illumination flag.
Other settled control bytes1–10 match except byte1. Timer-field bits are clear;
no automatic TIMER REC mapping is inferred. The user was asked to distinguish
main REC below DIGITAL from TIMER REC or the REC prefix beside DATE before
assigning the visual.

Play and pause indicators can be lit independently of selected source: this
reference is on RADIO while MD transport flags20/10 are set, consistent with
record-pause/standby. The capture does not prove media is actively being written;
record-enabled/armed/paused distinctions must not be inferred from REC alone.
Raw time/frequency fields remain FM87.50MHz. No firmware mapping was installed
before the physical label location is confirmed.

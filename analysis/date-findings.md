# DATE icon on reference

User confirmed DATE is lit, with arrow pointing to the DATE part of REC DATE.
Passive6s recording: captures/manual/date-on/capture.sal. API snapshot, reference,
CSV and extracted JSON are alongside it.192 complete frames per bus were extracted.
No player commands were sent and firmware was not changed for this reference.

The twelve text cells read `26y 10m 29d `, consistent with2026-10-29. Numeric
bytes13/14 are21/04 (packed BCD), consistent with a clock reading21:04, although
the user has not separately confirmed that clock value. Numeric byte3=41:
MD source bit01 remains set and a new bit40 is set. In prior ordinary MD, source
cycle, SLEEP and timer/play references this upper source-byte bit was clear.
It is a strong candidate DATE indicator/mode bit, pending an isolated off reference.
The source decoder already masks low four bits, so the date flag cannot turn
MD into an unknown source.

Other observations: byte0=01/81 (SLEEP on and regular phase), byte2=07 (solid MD
graphic), byte5 alternates00/02, and byte9=03. Do not assign these changes to
DATE itself: clock rendering/colon and other active settings may explain them.
The visible REC portion has not been independently confirmed and must not be
assumed to share the DATE bit merely because the existing UI groups REC DATE.

Next: user-confirmed DATE-off capture without changing other settings to verify
byte3 bit6. A future implementation should split REC and DATE labels if their
bits prove independent. Date parsing and clock/timer numeric mode need separate
validation rather than treating these values as MD track/elapsed fields.

## Confirmed DATE-off comparison

User confirmed DATE off. The paired date-off capture has190 complete numeric
and190 complete text frames. Numeric byte3=01 throughout, compared with41 in
the192-frame on reference. MD source bit01 remains set; bit40 is present only
in the confirmed DATE-on capture. Text returns to the ordinary title and numeric
fields return to the disc summary layout. Other mode-dependent fields also
change; no unrelated bit is attributed to DATE.

Firmware now exposes symbols.date from numeric byte3 bit6, and the webpage
splits the former REC DATE visual into an unmapped REC prefix and a mapped DATE
label. DATE follows received state and freshness. There is no claim that REC
shares that flag, nor a new clock/date numeric decoder. Source logos remain
independent of the upper date flag.

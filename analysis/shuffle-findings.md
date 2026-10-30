# SHUFFLE indicator

User confirmed SHUFFLE lit. Passive recording:
captures/manual/shuffle-on/capture.sal;188 complete numeric and188 complete
text frames are extracted in the sibling raw directory. No player command sent.

Numeric control bytes1–10 match the previous REPEAT captures except byte5:
SHUFFLE13, REPEAT43, REPEAT1=63. The earlier normal mode has03. Thus numeric
byte5 mask10 identifies SHUFFLE, independently of REPEAT40 and repeat-one20.
All188 numeric frames consistently contain13. The low03 fields remain unchanged.

API symbols.shuffle and the SHUFFLE visual now follow that received bit and
numeric freshness. Native capture replay checks shuffle true with both repeat
flags false, plus a controlled cleared-bit frame. Browser fixtures check on/off
and independent REPEAT state. No PGM bit or shuffle-play ordering is inferred.
A dedicated user-labelled SHUFFLE-off capture was not requested: off uses the
existing normal-mode baseline and a controlled flag-clear fixture.

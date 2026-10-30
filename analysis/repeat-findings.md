# REPEAT and REPEAT 1

User confirmed REPEAT1 lit, then switched to REPEAT alone without other requested
changes. Passive recordings:
- captures/manual/repeat-one/capture.sal:78 complete numeric frames.
- captures/manual/repeat-all/capture.sal:188 complete numeric frames.

All numeric control bytes1–10 match between these two recordings except byte5:
63 for REPEAT1 versus43 for REPEAT. The earlier normal-mode snapshot in
captures/remote-indicators/01-band/experiment.json has byte5=03 with the same
source/volume/track-indicator layout. Other prior normal MD captures also use03.

Zero-based numeric byte5:
- bit6(40): REPEAT, set in both new references.
- bit5(20): the1 modifier, set only in REPEAT1.

The low03 bits are independent and must be preserved. REPEAT1's one-bit difference
is isolated directly by the user-labelled pair. REPEAT's40 assignment also uses
the earlier normal-mode baseline; no new user-labelled off capture was requested.
The short repeat-one export was retained despite a Saleae close ReadTimeout;
its78 complete frames consistently show63. The repeat-all capture has188 valid
frames, all43. No player commands were sent for either capture.

API symbols.repeat and symbols.repeat_one now expose these bits separately. The
webpage's REPEAT and1 visuals follow received state/freshness with no invented
mode cycling. Native replay checks both recordings; a controlled cleared-bit
fixture checks off behavior. Browser fixtures cover all three visible states.

# Independent source logos and MD disc graphic

User reference: MD removed, then remote selected MD -> CD -> RADIO -> LINE IN.
Only source text logos illuminated. Native capture:
`captures/manual/cycle-md-cd-radio-linein.sal`; original unchanged. Export,
318 complete numeric frames, extracted JSON and source-run summary are in the
sibling *-raw directory. The recording ends back in MD mode.

Numeric byte3, zero-based, isolates the four source text labels:

| Source | Byte3 | Mask |
|---|---|---|
|MD|01|01|
|CD|02|02|
|RADIO|04|04|
|LINE|08|08|

Other numeric body bytes remain fixed in the supplied recording. In particular,
byte2=00 throughout, confirming that the MD disc graphic can be off independently
of the source selection. Earlier user-confirmed playback/pause references had
byte2=01/07. These graphic values must not be part of the radio-layout signature.
No CD graphic bit mapping is inferred from this recording.

## Bug and firmware fix

The previous tuner decoder required byte2=07 in addition to source/band/frequency
fields. That accidentally coupled radio frequency reception to a fully lit MD
graphic. The dependency is removed. Native checks feed known FM and AM fields
with byte2=00 (plus01/07 for FM), verifying identical frequency results. Those
missing-disc frequency cases are controlled regression fixtures; the supplied
source-cycle recording itself has blank frequency fields.

API source and symbols now expose independent source text flags and MD graphic
state. With a recognised source but blank numeric fields, mode/source remain
known rather than falling back to MINIDISC or reporting invalid MD playback.
CD and LINE are represented explicitly. CD numeric playback is still unmapped
and no longer silently interpreted as MD track/time.

The MD graphic is completely hidden when its observed command is00, even if the
MD source label is lit. Command07 displays all wedges independently of selected
source. The previously confirmed MD-play command01 animates only in the matched
MD playback state. Unknown graphics commands remain unmapped. MD play/pause
indicators use the independently mapped byte1 flags and can be off with no disc.

Source logos and disc appearance retain freshness behavior. A known source also
allows the persistent numeric volume field to remain available when its digits
are blank. No source or player commands were sent during analysis/verification.

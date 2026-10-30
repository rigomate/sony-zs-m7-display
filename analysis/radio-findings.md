# Radio display: first reference capture

User confirmed tuning to105.9 on2026-10-29. FM is inferred from the frequency.
Passive6s Saleae capture: `captures/radio/01-fm-1059/capture.sal`;
raw CSV, extracted frames, MCU API snapshot and user reference are beside it.
No IR/player commands were sent.

The replay reports190 transactions per bus, all with expected lengths (16/15
bytes). The frame-extraction script retains189 per bus because it omits the
first event, printed without a timestamp by the replay formatter.

Representative numeric payload (zero-based byte offsets):

```
80 02 07 04 C0 00 4C 00 00 C0 AA AA 0A 05 90 66
```

Bytes1–14 stayed constant. Byte0 toggled00/80; byte15 varied. Relative to known
MD playback, several early fields differ and may identify source/band/icons.
Bytes12–14 (`0A 05 90`) are a candidate frequency region; the trailing05/90
is suggestive for105.9 but one reference cannot establish encoding or scale.
Do not implement a frequency decoder or mode mask from this single example.

The title bus body remained:

```
[sequence] 00 8D 8B 20 00 00 20 20 20 20 20 20 20 [trailer]
```

8D/8B are not printable ASCII and their radio meaning is unknown. Do not label
these codes as a station name or discard them as capture corruption.

Next: capture one adjacent known frequency, ideally105.8, with band, mono/stereo
setting and presets unchanged. Compare byte/nibble differences, then obtain a
reference crossing a digit boundary (e.g.105.9 to106.0). Later capture another
band and mono/stereo changes separately to distinguish source flags and icons.

## Second reference:103.9MHz

User then confirmed103.9. Capture: `captures/radio/02-fm-1039/capture.sal`.
The6s recording contains190 complete frames per bus plus two boundary partial
transactions per bus. Complete-frame representative numeric payload:

```
80 00 07 04 C0 00 50 00 00 80 AA AA 0A 03 90 C0
```

Comparison with105.9 (zero-based offsets):

| Byte |105.9|103.9|Interpretation |
|---|---|---|---|
|1|02|00|Unknown; may depend on reception/settings |
|6|4C|50|Unknown |
|9|C0|80|Unknown indicator bit6 changed |
|12|0A|0A|Frequency-region candidate, unchanged |
|13|05|03|Strong evidence for the MHz units digit |
|14|90|90|Consistent with unchanged fractional .9, not yet independently varied |

Byte15 also has a different rolling value set; no checksum interpretation is
established. Title-body bytes1–13 are identical in both recordings. Sequence
and trailer value sets differ because the captures start at different times.
The other numeric-byte changes must not be attributed solely to frequency:
reception/indicator state can also change when tuning a different station.

Next useful reference:103.8MHz, changing only the fractional digit. A subsequent
reference below100MHz (e.g.99.9) is needed to distinguish how the leading digits
and blank positions are encoded. Do not hard-code the leading100 from two
references that both start10x.

## Third reference:102.5MHz

User confirmed102.5. Capture: `captures/radio/03-fm-1025/capture.sal`.
Replay reports189 complete16-byte numeric frames and189 complete15-byte title
frames, plus an initial partial on each bus. Representative numeric payload:

```
80 02 07 04 C0 00 4D 00 00 C0 AA AA 0A 02 50 E8
```

The frequency candidate is constant throughout each reference:

| MHz | Byte12 | Byte13 | Byte14 |
|---|---|---|---|
|105.9|0A|05|90|
|103.9|0A|03|90|
|102.5|0A|02|50|

Evidence now strongly supports byte13's low nibble as the MHz units digit and
byte14's high nibble as the tenths digit. Its low nibble is0 in all observations;
a hundredths position is a hypothesis, not independently demonstrated.
The leading10x encoding remains unresolved. Next useful setting:99.5MHz,
keeping the fractional digit at5 while crossing below100. Do not deploy a
full FM decoder before resolving the leading positions and a reliable mode tag.

Other fields: byte1 and byte9 match the105.9 reference (02/C0), while byte6
is4D. These could be stereo/tuning/reception fields but are not mapped.
The title body is unchanged. No commands were transmitted.

## Fourth reference:98.7MHz

User confirmed98.7. Capture: `captures/radio/04-fm-0987/capture.sal`.
Replay reports191 complete numeric frames and190 complete title frames, with
boundary partials. Representative numeric payload:

```
00 02 07 04 C0 00 0A 00 00 80 AA AA 0A 98 70 B0
```

Bytes12–14 are`0A 98 70`. Byte13 therefore contains packed-BCD tens and units
of MHz, modulo100. Byte14's high nibble contains the tenths digit. Byte12
remains0A above and below100, so it does not independently distinguish the
hundreds digit in these recordings.

A new candidate for the hundreds indicator is byte6 bit6 (mask0x40):

| MHz | Byte6 | Byte13 | Byte14 |
|---|---|---|---|
|105.9|4C|05|90|
|103.9|50|03|90|
|102.5|4D|02|50|
|98.7|0A|98|70|

`100*(bool(byte6 & 0x40)) + BCD(byte13) + (byte14 >> 4)/10`
fits all four known reference frequencies. This is a candidate interpretation,
not a proven field map: byte6's other bits also change between references,
and there is only one below100 reference. Source/band identification still
needs an independent comparison. Do not infer that the remaining byte6 bits
are frequency digits, preset number or signal strength without another test.

Best next experiment:99.9MHz followed by100.0MHz, with other settings unchanged,
to observe the hundreds boundary. All captures so far are passive, with no IR
commands or firmware changes. Title body remains the same in all four cases.

## Experimental firmware implementation

At the user's request, the candidate frequency formula is now implemented in
`include/zs_m7/display_state.hpp` and consumed by LiveDisplay. It is restricted
to the observed numeric signature (bytes2/3/4=07/04/C0,10/11=AA/AA,12=0A),
valid BCD positions and87.5–108.0MHz range. This signature is provisional and
has not been validated against AM or all other player modes.

The API reports mode fm and radio.frequency_khz with experimental=true; MD
numeric fields are null in FM. The dashboard shows MHz and hides MD track/time
and track indicators. Native replay checks all four captured frequencies,
invalid BCD rejection, and clearing the radio state after switching to an MD
frame. Captured-radio, live-state, and all20 MD numeric replay checks pass.
The high-level radio display remains experimental; no presets, stereo or
other radio icons are decoded. Further feedback can use the user's available
stations rather than requiring arbitrary tuning.

## Hundredths digit:98.75MHz

User suspected98.75 after tuning again. The live C3 numeric raw payload was:

```
00 02 07 04 C0 00 0A 00 00 80 AA AA 0A 98 75 2C
```

This adds the first nonzero low nibble in byte14, consistent with a50kHz
change. The initial experimental firmware wrongly required that nibble to be
zero and formatted one decimal place. It now accepts both BCD fractional
nibbles, adds low_nibble*10kHz, and renders two decimal places. Candidate
frequency_khz =1000*(hundreds_flag*100+BCD(byte13))+10*BCD(byte14).
Tests cover all four capture references, a nonzero hundredths digit and invalid
fractional BCD. Full mode and icon mapping remain provisional.

## User-provided AM and isolated PRESET references

Six .sal recordings were exported to sibling *-raw directories under
captures/radio-manual. Original .sal files are unchanged. Machine-readable
stable-field comparison: captures/radio-manual/field-comparison.json.

| File label | Confirmed frequency | Numeric bytes13/14 | Byte6 |
|---|---|---|---|
|am-287khz|1287kHz|28 7A|4A|
|am-323khz|1323kHz|32 3A|4A|
|am-422khz|1422kHz|42 2A|4A|
|am-575khz|1575kHz|57 5A|4A|

The user explicitly confirmed the faint leading1 after the byte analysis.
Original filenames are retained as supplied; their three digits omit this1.
Byte6 bit6 supplies the separate leading1 in both units layouts:100MHz for FM
and1000kHz for AM. AM frequency_khz =1000*bool(byte6&0x40)+10*BCD(byte13)
+(byte14>>4), with low nibble of byte14=A (blank). The previous unlabelled AM
capture has no leading1 and yields531kHz; that frequency lacks user confirmation.

Byte4 is20 in the AM references, C0 in FM without PRESET, and C2 in FM with
PRESET. Exact meaning of each units/band bit remains unisolated; the observed
patterns reliably distinguish these recordings. The decoder masks bit1 before
matching either band pattern, then selects the respective frequency layout.

At104.00MHz, both PRESET recordings have frequency bytes0A 04 00 and byte6=4A.
Two stable numeric bits change together:

| Field | PRESET off | PRESET on |
|---|---|---|
|byte4 bit1|0|1|
|byte7 bit5|0|1|

The UI uses byte4 bit1 as its PRESET state. This pair cannot independently
assign both bits to physical segments, nor establish a preset number. No preset
number is inferred from byte7. The title body is identical in both captures.

Experimental firmware now supports AM/kHz, FM/MHz with two decimals, and PRESET.
Replay tests cover all four labelled AM recordings and both isolated PRESET
captures, in addition to the previous FM references. MD live-state regression
checks still pass. Other radio symbols and AM/FM states outside the observed
signatures remain unmapped. No IR or player commands were transmitted.

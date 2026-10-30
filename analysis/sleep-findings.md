# Steady SLEEP icon reference

User confirmed SLEEP is fully lit, not blinking. Passive6s recording:
`captures/manual/sleep-steady-on/capture.sal`. API snapshot, user reference,
CSV, extracted frames and summary are saved alongside it. No player commands
were transmitted and firmware was not changed.

190 complete numeric and190 complete title frames were extracted. The player
is in MD playback, track3, advancing from1:59 to2:05. Numeric byte9 is03
throughout, including the low track-indicator nibble3. Its upper bit7 (80) is
clear. Previous MD playback and pause captures had byte9=83 for the same18-track
indicator capacity; other previous source recordings typically had bit7 set.
This makes byte9 bit7 a candidate active-low SLEEP field. The regular byte0
blink bit still toggles and is not a constant-on SLEEP candidate by itself.

The comparison spans sessions, tracks, volume and user actions. A matching
SLEEP-off recording without changing source/transport/other settings is needed
to confirm the bit and its polarity before mapping the UI symbol. No timer
countdown, duration or power-off behavior is inferred from this reference.

## Confirmed off comparison and implementation

User confirmed SLEEP off; captures/manual/sleep-off/capture.sal records6s,
with190 complete numeric frames. All other control bytes1–8/10–12 match the
on reference. Track3 continued playing; time advanced normally between captures.

| Field | SLEEP steady on | SLEEP off |
|---|---|---|
|byte0 bit0|1 (01/81)|0 (00/80)|
|byte9 bit7|0 (byte03)|1 (byte83)|

Byte0 bit7 keeps toggling in both states. The earlier first-pass candidate
analysis focused on byte9 but missed byte0's low bit. Byte0 bit0 is a clear,
steady positive SLEEP-state marker across all380 labelled numeric frames.
Both changed fields remain correlated; their separate internal roles are not
isolated. The UI uses byte0 bit0, without assigning byte9 bit7 another symbol.

The API symbols object now includes sleep. The SLEEP visual follows that flag
without animation and uses existing numeric-bus freshness behavior. Native
paired-capture replay and browser checks cover both states. No duration or
countdown encoding is claimed. No player commands were transmitted.

## Precisely timed blinking reference

User supplied captures/manual/sleepisblinking.sal. Its one-second export has
32 complete numeric frames:16 with byte0=00 and16 with byte0=81, with the
observed transition0.509872s after the first sample. The larger preceding
sleep-blinking recording also contains these phases and intervening returns
to normal playback display. User timing identifies the short recording as the
clean blinking reference.

Crucially, byte9 remains83 throughout both phases. This rules out the earlier
byte9-bit7 active-low hypothesis as the direct SLEEP illumination flag. Byte0
bit0 consistently agrees with steady-on, off, and blinking references. The
webpage follows that received boolean directly; no CSS blink timer is used.
The standard250ms polling is faster than the measured~510ms blink phase, but
HTTP scheduling can still affect the apparent blink timing.

During this reference the ordinary time fields areAA/AA, byte5=00 and byte11
alternatesAA/10. The title body is blank zero codes. These are additional display
mode parameters; no timer-duration mapping is asserted from them alone.

Native tests now replay all three precisely labelled references, including both
blink phases while byte9 stays fixed. The already-passing browser checks verify
SLEEP on/off classes and inherited stale/disconnect behavior.

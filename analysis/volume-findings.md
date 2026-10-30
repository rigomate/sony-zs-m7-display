# Volume sweep decoding

Reference: user recorded minimum volume and repeatedly pressed volume-up until
maximum. Original: captures/manual/volume-from-min-to-max.sal. Export, complete
frames, summary and step timeline are in the sibling *-raw directory.
Reproduce analysis with `python3 analysis/analyze_volume_sweep.py`.
No player command was transmitted or firmware changed for this analysis.

440 complete frames from each bus cover approximately14s. The first69 paired
frames show normal text (Red (Taylor') and numeric byte6=00. At2.196388s the
text bus enters mode05 and the volume sequence visits every integer0..31 in
order. Level31 first appears at11.469719s and remains until the recording ends.

Zero-based fields:

| Field | Meaning in this recording |
|---|---|
|SDI2 byte1=05|Volume display command/mode |
|SDI2 byte2|Binary volume value0..31 |
|SDI2 bytes3..13|All00 in this command mode |
|SDT1 byte6 bits0..4|Same binary volume value |
|SDT1 byte6 bit5 (20)|Set during the volume display command |

Examples: volume0 -> numeric byte6=20, volume15 ->2F, volume31 ->3F.
The entire371-frame overlay interval agrees exactly across both buses;
all32 values occur. Values are binary, not packed BCD:0A means10,1F means31.
These are device step values; no dB scale or acoustic gain mapping is established.
The return-to-title timeout is not captured because the recording ends while
the volume command is still active.

This helps split numeric byte6 into independent roles. Earlier radio captures
associated bit6 (40) with the leading1 frequency segment. It must not be
included when extracting volume. Remaining bit7 is unmapped. Volume mapping
is directly validated in this MD pause sweep; its behavior across other source
modes and overlay transitions still needs comparison.

The MD is paused throughout this sweep: byte1=30,byte2=07 and time0:48 with
intentional blank phases. Other numeric control fields remain steady apart
from the known time blinking and rolling/timing fields. Changing volume does
not change the confirmed MD transport flags.

SDI2 mode05 payloads are parameterized display commands, not twelve ASCII
characters or corrupted title data. The current firmware will show unknown
cells for them. A future volume display should decode the mode first and retain
normal title decoding for mode00, without treating volume parameters as text.

## Firmware implementation

At the user's request, the C3 now publishes volume.level(0..31), max(31),
overlay and fresh in /api/state. Known numeric layouts provide the persistent
level from byte6's low five bits. A validated SPI-2 mode05 command overrides
that level while active and is never decoded as ASCII. Its payload is checked
for range0..31 and zero parameter padding. Partial frames preserve prior state.
The last ordinary text window is retained while a valid volume command is active.

The webpage adds a read-only volume level and step-fraction meter. During mode05
it reconstructs VOLUME nn in the twelve-cell window; normal title display resumes
when ordinary text arrives. No invented timeout hides a still-received command.
The meter uses received steps, not a dB scale or measured loudness. Stale and
unreachable-device states dim the meter along with other received data.

Native replay covers every captured level and command/ASCII separation, then
feeds a normal text frame to verify title restoration and persistent volume.
MD transport, radio, live-state and title regressions passed. Browser checks
cover minimum, maximum and returning to ordinary title display.

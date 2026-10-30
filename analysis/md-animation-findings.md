# MD playback disc animation

## Photo evidence

images/md-playback/IMG_6470.jpg, IMG_6471.jpg and IMG_6472.jpg show a fixed
rectangular MD outline containing illuminated opposing wedge segments. At
least two orientations are visible: top/bottom, and a diagonal pair. MD source
and play triangle remain illuminated. Three stills cannot establish the entire
phase sequence, direction or timing. IMG_6471/6472 appear to share a phase.

## Passive playback capture

captures/md-animation/01-playback/capture.sal records10s of live MD playback;
exported CSV and extracted complete frames are in raw/. Summary JSON sits
beside the capture. No IR/player commands were transmitted.

317 extracted complete numeric frames have constant bytes1–12, except time
fields start at13. Control region (bytes1–10):

```
20 01 01 00 03 0B FF FF 83 AA
```

Byte0 alternates00/80 about every0.509872s. A similar toggle occurs in previous
MD and radio recordings, so it is not sufficient to identify a disc-specific
animation phase. Byte15 and title-frame sequence/trailers also vary in other
modes. No disc-specific cyclic control-byte pattern is established.

Possible explanations include a fixed animation-enable command interpreted by
the display controller, or use of a shared timing/sequence field for animation.
The trace does not distinguish them. Do not attach a free-running CSS rotation
to any frame that happens to decode as an MD track/time display.

The older03-track01-stopped capture has control bytes1–6:
`00 00 01 00 00 0B`, versus current playback`20 01 01 00 03 0B`.
These are different sessions/discs and their remote command receipt was not
independently confirmed at the time. This is candidate evidence only for
source/play/disc-control fields; it does not independently map each bit.

Next: a user-confirmed paused-state capture, then resumed playback. Compare
source/play/pause bits and whether the apparent animation command stays enabled.
Existing UI wedge/disc graphics remain unmapped until the control is isolated.

## User-confirmed pause and photo

User confirmed paused playback and supplied images/md-playback/pause.jpg.
All six wedges are illuminated; both play triangle and pause bars are lit.
The passive02-held-time capture therefore becomes the confirmed pause reference.
Track2 time0:26 holds; both numeric-time fields alternate between00/26 andAA/AA.
Byte5 follows03/01 during that blink. This is deliberate blanking, not corruption.

| Numeric byte | Playback | Paused |
|---|---|---|
|1|20|30|
|2|01|07|
|3|01|01|
|4|00|00|
|5|03|03/01|

Byte1 bit4 correlates with the visible pause bars, while bit5 stays on with
the play triangle. Byte2's01/07 change correlates with animated/all-lit disc
states; individual animation phase bits are still not established. The frames
and photos establish these two states, not all stop/seek/record configurations.

Firmware now exposes a narrowly matched MD transport object, independent of
whether time digits are currently blank. Known pause blanking preserves the
last numeric value only for the same track; the UI blanks the time deliberately
while retaining MD mode and track. Play/pause visuals follow the matched state.

The MD icon now has six filled wedges in opposing pairs. In the observed
playback state the webpage reconstructs a three-pair animation with a1.5s
cycle; that phase order/timing is approximate and not claimed to be decoded
from the wire. In the observed pause state all wedges remain on. Animation
freezes when numeric input becomes stale or the device disconnects. Other
transport states remain unmapped instead of automatically animating any frame
with plausible track/time values.

Native replay checks both physical captures, pause flags and blank-time
retention; browser fixtures check animated versus solid icon and play/pause
indicators alongside existing radio and stale/disconnect states.

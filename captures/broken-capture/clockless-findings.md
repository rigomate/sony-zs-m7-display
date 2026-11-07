# Clockless sampling experiment

The user's hypothesis is partly supported: channels 6 and 7 retain structured
serial information that can be sampled with a reconstructed timing schedule.
Counting their transitions as if they were clocks was not sufficient to decide
whether data remained recoverable. Their physical wire identity still requires
confirmation; decoding structure does not establish an electrical connection.

## Method and cross-check

The original capture is preserved. `raw/digital.csv` was exported by the Saleae
application from `something-is-broken.sal`, at its recorded 8 MS/s rate. Thirty-eight
whole active-low channel-0 windows each contain 128 channel-1 rising edges.

The healthy recording `captures/title-test-01-0123456789AB-raw/digital.csv`
provides the SPI2 sampling schedule: 15 bursts of eight bits, LSB first. Adjacent
bits within a byte are approximately 25.54 us apart; byte starts are approximately
817.1 us apart. Thus there is a substantial inter-byte pause, not a uniform
120-pulse clock. The calibrated virtual schedule reproduces all 191 complete
healthy SPI2 packets byte-for-byte when anchored to their actual first clock
edge, with sampling six microseconds into the stable part of each bit.

For the broken capture, use the first surviving channel-1 rising edge in each
channel-0 window as an anchor, plus the same six-microsecond phase margin.
Both channels 6 and 7 recover these first packets:

```
E2 00 00 00 00 00 00 00 00 00 00 00 00 00 E4
E3 00 00 00 00 00 00 00 00 00 00 00 00 00 E5
E4 00 00 00 00 00 00 00 00 00 00 00 00 00 E6
```

Across all 38 packets, byte 0 increments by one on every transition, wrapping
from FF to 00. Byte 1 is 00 throughout. Bytes 2–13 are all zero. Byte 14 is
byte 0 plus two modulo 256. This is packet-like structure, but the trailer has
not been established as a CRC and no checksum-validity claim is made.
The same header progression survives several nearby sample phases, so it is
not dependent on one finely chosen sample instant.

## What is and is not recovered

The twelve text cells contain no readable title/date in this recording.
This is not simply a bad sampling phase: on both channels, the entire interval
from first channel-1 clock +1.65 ms through +10.8 ms is continuously low in all
38 frames, with no transitions between sample points. Whether zero-filled
content was intended or is the result of the electrical fault is unknown.
A capture of a known nonblank title/date is needed to test useful payload recovery.

A fixed first sample at CS1 falling +3.012 ms does not recover the counter
progression in this file. CS1 falling +3.32825 ms does, with the burst schedule
above. The measured first surviving clock is 3.321–3.323625 ms after CS1 falling;
the chosen input sample is six microseconds later. Those are capture-specific
observations, not guaranteed Sony timing. Healthy reference frames have first
SPI2 clocks 3.564125–5.430125 ms after CS1 falling, so a single absolute delay is
not established across operating states.

An internal timer-triggered input sampler is therefore a viable experiment,
but this recording does not demonstrate reliable restoration of arbitrary text.
It should keep Sony nets input-only, not drive a replacement clock into shorted
Sony outputs. This experiment changes neither STM32 firmware nor Sony hardware.

## Reproduce

From the repository root:

```sh
python3 analysis/recover_clockless_spi.py
```

The output is `clockless-recovery.json`, containing calibrated sample offsets,
every recovered packet, phase-sweep results, fixed-delay trials, and limits.

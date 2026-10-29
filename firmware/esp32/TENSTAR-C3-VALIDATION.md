# Physical Tenstar C3 validation — 2026-10-29

## Installed firmware

Tenstar Robot ESP32-C3 Super Mini, chip rev0.4, 4MB XMC flash,
MAC18:8b:0e:2e:2d:04. Firmware SHA256: `ccfe1e9492d8119ecde0897dc4e6495173751e27594dbce20b87b5a307183acb`.
The ROM uploader verified flash hashes and completed successfully. The RAM
upload stub had crashed; the configured no-stub path avoids that failure.
Wiring and build instructions: [TENSTAR-C3.md](TENSTAR-C3.md).

Both buses use filtered GPIO interrupts, not hardware SPI. The attempted C3
SPI-slave path produced incorrect lengths and is not part of this validation.
The input output-enable masks were zero on both buses. No player command was
sent during these tests.

## Independent wire comparison

`captures/tenstar-c3/04-payload-comparison.sal` and its exported CSV record20s
of Sony traffic at8MS/s while the host polls the physical C3 API every~250ms
and periodically downloads its webpage. The reproducible passive test is
`analysis/validate_c3_live.py`; API samples and summary are beside the capture.

68 API polls were collected. Excluding boundary samples and partial raw frames,
all51 numeric and49 title payloads matched complete frames from the simultaneous
Saleae recording byte-for-byte. No unmatched complete payload was observed.
This samples received frames; it does not prove every received bit is correct.
The API advanced from track3 time4:14 to track4 time0:15 during automatic track
advance. All20 test-disc tracks have title TEST; title transition/control windows
also occur on the wire, so requiring every poll to equal TEST is invalid.

During the~19s API window each bus reported594 frames:

| Bus | Complete | Rejected | Rejected fraction |
|---|---:|---:|---:|
| Numeric |551|43|7.2%|
| Title |557|37|6.2%|

Queue drops did not increase during this test. The lifetime counter already
contained4 earlier drops, which the webpage continues to report. Rejected
partial frames preserve the most recent complete decoded values; freshness
expires after1.5s without another complete frame. Both buses remain live.

The earlier `03-gpio-live` independent12s capture also decoded TEST and track3,
time1:15 through1:27. The first strict polling script stopped at a title ST
transition; this was an invalid constant-title assertion, not evidence by itself
of corrupted reception. The later full-payload comparison handles transitions.

## Webpage and native checks

A headless Chromium loaded the physical C3 webpage and rendered TEST, track03,
1:14 and the corresponding track indicator. Snapshot: `captures/tenstar-c3/dashboard.png`.
That snapshot precedes the20s comparison; its time is not the comparison endpoint.
The host's existing ZS-M7-Display Wi-Fi profile was preserved.

Native fixed-buffer replay recovers captured 0123456789AB and TEST and checks
buffer overflow and edge filtering. Live-display tests verify partial frames
retain the last complete fields and cannot keep stale values fresh. Both scoped
native tests passed after the final capture/state changes. The separate M5
firmware target also built during development; it is disconnected from the Sony.

## Remaining limits

This is a working prototype, not lossless capture. Wi-Fi/interrupt load still
causes incomplete frames. There is no decoded checksum to reject a complete
frame containing a wrong bit. Higher-confidence replacement hardware should
reduce capture loss and validate any checksum before relying on every frame.
Unknown title control codes remain visible through raw diagnostics rather than
being invented as printable characters. Voltage compatibility still requires
measurement; a zero output-enable mask does not establish electrical safety.

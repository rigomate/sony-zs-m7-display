# Verification record

**Current live-wiring status: not working yet.** G0 has no detected clock edges;
numeric transactions are zero-bit, and text reception is corrupted. The initial
web-only checks below preceded wiring. See [the active investigation](../../analysis/m5-gpio0-findings.md).


- Target queried over USB: ESP32-PICO-D4 revision 1.0, MAC d4:d4:da:96:be:c4.
- PlatformIO espressif32 6.12.0 / Arduino-ESP32 2.0.17 builds successfully.
  Static RAM ~54 KB; application flash ~1.065 MB, within the default partition.
- Five CTest suites pass: shared-CS payload equivalence across all twenty TEST
  captures; live-state JSON/escaping/staleness/millis-wrap; numeric playback;
  title windows; prior capture framing.
- Chromium checks pass for live TEST / track20 / 0:14, stale input, unknown title
  controls, and disconnected device. These browser cases use recorded API fixtures.
- Electrical capture from the M5Stick connected to the Sony is not yet validated.
  Confirm signal voltage, connect the documented wires, and compare both raw
  receive streams with Saleae. No Sony commands were sent during firmware work.

## Physical-board results

- Uploaded successfully at 115200 baud; esptool verified the written image hash.
- USB diagnostics confirm both SPI receivers initialized with no capture error.
- AP `ZS-M7-Display` is visible and accepts a Wi-Fi connection.
- `GET http://192.168.4.1/` returns byte-for-byte identical HTML to the source.
- `GET /api/state` returns valid JSON, no capture error, zero dropped frames,
  and null numeric/title fields with no display frames received.
- The browser renders the physical device's page and polls its API successfully.
- Temporary host Wi-Fi test configuration was removed after verification.
- No backup was made, per the user's instruction. Existing board firmware was replaced.

Application image SHA-256: `34a4577fd958e15567c54ed3529e1737e4d818e6f30fc42bbff47f442888bb78`.

Logs and physical-device API snapshot are in ignored `build/m5stick-*` files.
These checks verify deployment and serving; Sony-connected SPI reception still
requires the live wiring test described above.

Current diagnostic firmware SHA-256: `6513fe294ac49ea8b3a2a9fc037b80d8cad0e17bd24ddea51dbf71b2d714774b`. Startup GPIO0 ADC minimum ~1.935 V, 4,168 samples, high values saturated; hardware numeric clock counter remains essentially zero. Capture error and application queue drops are zero. These are diagnostic results, not a successful live decoder test.

# Handoff: Sony ZS-M7 replacement display

## Current target: working Tenstar C3 prototype

User switched to the Tenstar Robot ESP32-C3 Super Mini, MAC18:8b:0e:2e:2d:04,
USB `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_18:8B:0E:2E:2D:04-if00`.
The M5Stick is fully disconnected from the Sony. Firmware target `tenstar-c3`
in firmware/esp32/platformio.ini uses ROM upload (no stub), DIO 40 MHz, full
watchdog reset. The upload stub crashed on this board; the ROM path succeeds.

Wiring: SDT1->7, SCK1->6, CS1->10, SDI2->4, SCK2->5, CS2->3, SGND->GND.
No GPIO0, no shared CS, no return-data output. Verify voltage levels.
Both buses now use filtered, IRAM GPIO sampling. The attempted hardware SPI
path returned incorrect lengths; it is not used by the working C3 source.
GPIO callbacks with clock low are discarded, and edges less than18us apart
are rejected using esp_timer (not CPU cycle counter). Frame flush occurs after
CS inactive50us without masking interrupts mid-frame; vectors are reused.
Bad frames count as rejected but do not erase the last good display; stale
status is based on the most recent complete frame, not malformed traffic.

Live C3 snapshots recover TEST, current track and advancing elapsed time, with
zero output-enable masks. All100 sampled complete payloads matched simultaneous
Saleae wire frames; track3 advanced automatically to4. Around6–7% of incoming
frames were incomplete and rejected. No additional queue drops occurred in the
20s comparison, but the lifetime counter already contained4. A real browser
renders the physical page. Live polling and Saleae verification are recorded under
captures/tenstar-c3 and firmware/esp32/TENSTAR-C3-VALIDATION.md.
Same AP: ZS-M7-Display / minidisc-test / http://192.168.4.1/.
Two nearby APs may share that name if the old M5Stick remains powered; verify
receiver backend/pins or C3 BSSID18:8b:0e:2e:2d:05 when testing.

POST /api/capture/pause stops only capture and leaves inputs floating;
POST /api/capture/restart reboots the C3 to resume. No player commands.

## Active G0 investigation

The user connected the M5Stick to the Sony, but numeric reception has zero-bit
transactions and title bytes are corrupt. A diagnostic build stops the zero-bit
queue flood and exposes GPIO sample counts, hardware pulse counts and SPI routes.
G0 stayed HIGH in all samples (not low) and a hardware pulse counter detected
zero rising edges while the text clock counted thousands. The user is building
a parallel Saleae header; until then no parallel probing is available.

Official M5 schematic R12 pulls G0 up to 3.3 V with 1 kOhm. Sony SCK1 path R448
is also 1 kOhm, so the tap location may create a divider that prevents a valid
low voltage. Startup ADC evidence now supports this: 4,168 samples, minimum ~1.935 V,
maximum saturated. The true high is unknown; default-reference ADC readings are
approximate. A valid ESP32 low must be <=~0.825 V at 3.3 V. Firmware adds
a short startup ADC measurement before Wi-Fi/SPI, restoring G0 to digital input.
See `analysis/m5-gpio0-findings.md` and `captures/m5-debug/`. Do not claim the
wired receiver works. No Sony commands were sent during this diagnosis.

## MCU web prototype

User selected **M5StickC PLUS 1.1 / ESP32-PICO-D4** instead of the initially
connected ESP32-C3 Super Mini. Firmware and wiring instructions are in
`firmware/esp32/README.md`. It uses both hardware SPI slave receivers and hosts
an AP dashboard at http://192.168.4.1/ (SSID ZS-M7-Display, password minidisc-test).
Built-in TFT unused. Default GPIOs: SDT1=26, SCK1=0, shared CS1=36,
SDI2=32, SCK2=33. Do not tie Sony CS1 and CS2 together: only CS1 is wired.
All twenty captures validate identical complete payloads when CS1 frames both.
GPIO0 is a strap pin; verify voltage levels and follow firmware README before wiring.

The board was identified on
`/dev/serial/by-id/usb-Hades2001_M5stack_39522B0848-if00-port0`, MAC d4:d4:da:96:be:c4.
Do not assume ttyUSB0 is still the IR adapter: it currently belongs to this M5Stick.
User explicitly said **no flash backup is needed**; do not repeat backup work.
High-speed serial switches failed; use 115200 baud for this board.

Build with PlatformIO Core 6.x: `pio run -d firmware/esp32`.
This host has a working core at `/home/mester/.platformio/penv/bin/pio`;
`/usr/bin/pio` is an old broken install. Five CTest suites and headless browser
checks pass. The firmware was uploaded and the physical AP/page/API verified. The host Wi-Fi
test connection was removed afterward. Live electrical validation against the Sony
remains pending.
See `firmware/esp32/VALIDATION.md` for the latest physical-board test status.

## Goal and current result

Recreate MiniDisc text, current track, indicators 1-20, and elapsed playback
time on an MCU-connected OLED/TFT. The original display is dead; do not ask
for videos of lit values. Its physical layout is in images/IMG_6467.jpeg.

The core fields are decoded and implemented in portable C++17:

- SPI-2 SDI2 bytes 2-13: twelve ASCII cells, 0xff unused suffix.
- SPI-1 SDT1 byte 11: current track, packed BCD (tested 1-20).
- SPI-1 SDT1 byte 13: elapsed minutes, packed BCD (tested 00-02).
- SPI-1 SDT1 byte 14: elapsed seconds, packed BCD (tested 00-59).
- SPI-1 bytes 7,8 and low nibble of 9: indicator bits 1-20 in ascending order.
  In normal playback here, current and later tracks are set; earlier tracks clear.

**Important correction:** sample rising edges, LSB-first bytes, selects active
low, clock idle high (mode 3). Earlier falling-edge/mode-2/MSB interpretations
and their printed hex values are historical. The old reversed-seven-bit SDO2
text rule recovered stable text but is superseded by direct ASCII on SDI2.
SDI2 and SDO2 usually agree when settled but differ during updates.

## Hardware constraints

Only IR commands play, track_prev, track_next, digit_0 through digit_9,
and power_off are user-approved. **Never send stop or any unlisted command.**
Avoid power_off unless specifically requested. This run used only play and
track_next. Wait at least three seconds after each remote ACK; ACK confirms
transmission, not player receipt.

Serial adapter: /dev/ttyUSB0, 115200 baud, carriage-return terminated lines,
selected device zs-m7. `status` reads adapter configuration, not player state.
`send play\r` / `send track_next\r` transmit commands.

Saleae Logic device DAAD127308D7F0CD; MCP http://127.0.0.1:10530.
Some tool contexts cannot see hardware; host exec with approved escalation
reached both MCP and ttyUSB0. Use discovery before claiming disconnection.
`analysis/capture_test20.py` is the explicit hardware runner; inspect before
invoking because begin and sweep SEND COMMANDS. Do not rerun just to inspect.

Confirmed channels: 1=SDO2, 2=SDI2, 3=SCK2, 4=CS2,
5=SDT1, 6=SCK1, 7=CS1; channel0 unused. Ground=SGND.
Capture settings: digital0-7, 8 MS/s. Do not probe display power/VFD pins.
Signal voltage and ownership remain unverified for an ESP32 interface.

## New disc and capture set

User inserted a 20-track disc with all titles TEST. `captures/test20/`
contains 01-play-time (140 seconds starting before play) and 02-next through
20-next (16 seconds, each starting before one track_next). Each has a native
.sal and a -raw/digital.csv. run.jsonl records commands, ACKs and artifacts.
This run should leave the player playing track 20; do not send commands
merely to query it. Check run.jsonl for completion if resuming mid-run.

After settling, the BCD track fields agree with all sequence positions;
the long trace covers two minute rollovers and time advances with capture
clock. See analysis/test20-findings.md and the capture regression test.
No live visual/audible track confirmation was obtained.

Earlier title-test traces cover all letters/digits and selected punctuation.
Files10/11 are mislabeled: contents PLAY / TRACK 10. File12 starts with
0123456789 and only reaches LON entering the window before capture end.
Full long-title scrolling remains unvalidated. See analysis/title-findings.md.

## Code and checks

- include/zs_m7/spi_frame_source.hpp: generic logical frame interface.
- src/saleae_csv_source.cpp: rising-edge LSB-first CSV replay.
- include/zs_m7/title_decoder.hpp: current twelve-cell window, explicit unknown controls.
- include/zs_m7/display_state.hpp: BCD track/time and 20-bit indicator mask.
- src/replay_main.cpp: raw payloads plus readable text/track/time/mask.
- tests/display_state_test.cpp: every settled new frame, sequence/mask checks,
  elapsed-time continuity and clock agreement, two minute rollovers, TEST text.
- tests/title_test.cpp: all twelve recorded final text windows.
- tests/replay_test.cpp: 24 historical/title capture framing regressions.

Build: cmake -S . -B build && cmake --build build --parallel 2
Test: ctest --test-dir build --output-on-failure
Example: ./build/display_replay captures/test20/20-next-raw/digital.csv

## Remaining work

Core normal-MD playback fields are sufficient for a software display prototype.
Before MCU hardware, establish voltage levels and which board drives the data
lines; retaining versus replacing the display controller changes the interface.
The hardware ISpiFrameSource and web presentation are implemented for the M5Stick.
OLED/TFT presentation and live Sony signal validation remain.

Unknown: checksums/trailers, other flags (including byte0 bit7 toggling), playback
mode detection, remaining/total time, minutes above02, tracks above20, complete
long-title scroll, unsupported glyphs, and control-code rendering. Valid BCD
alone does not identify MD mode; do not claim the decoder covers tuner/CD/edit.

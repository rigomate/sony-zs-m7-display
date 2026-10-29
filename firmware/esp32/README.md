# ESP32 display receiver + web dashboard

The current target is the **[Tenstar ESP32-C3 Super Mini](TENSTAR-C3.md)**.
The M5Stick instructions below describe the older dual-SPI implementation and
its unresolved direct-input loading issue. Use the C3 wiring table for the Tenstar.

## M5StickC PLUS 1.1

Passively receives the Sony ZS-M7 display interface and hosts a live webpage.
The built-in M5Stick TFT is left unused. No M5 display/microphone libraries
are initialized, and no Sony control or response signals are transmitted.

Target: **M5StickC PLUS 1.1 / ESP32-PICO-D4**, Arduino-ESP32 2.0.17,
PlatformIO espressif32 6.12.0. The PlatformIO board identifier is `m5stick-c`;
it provides the matching ESP32-PICO-D4 build settings for this web-only use.
The C3 uses a separate hybrid receiver; see TENSTAR-C3.md.

## Wiring

**Verify the Sony signal voltage before wiring. ESP32 inputs are 3.3 V logic;
use appropriate input level conversion if the Sony outputs exceed that.**
Keep the original display-controller board connected for this listening test.
Power the M5Stick from USB. Connect to the labeled signal points, not VFD power.

| Sony signal | Analyzer channel | M5StickC PLUS GPIO | Connector |
|---|---:|---:|---|
| SDT1 (numbers) | 5 | 26 | HAT header |
| SCK1 | 6 | 0 | HAT header |
| CS1 | 7 | 36 | HAT header |
| SDI2 (text) | 2 | 32 | Grove yellow |
| SCK2 | 3 | 33 | Grove white |
| SGND | Ground | GND | HAT or Grove black |

**Leave CS2 and SDO2 unconnected to the M5Stick. Do not tie CS1 and CS2
on the Sony board together.** GPIO36 internally feeds both SPI CS inputs.
The two clocks and two data inputs stay separate. All twenty retained TEST-disc
captures produce identical complete payloads with CS1 framing both buses;
`shared_select_capture_equivalence` reproduces this check.

GPIO0 is a boot-strapping pin. It must be high when the ESP32 resets to boot
normally. SCK1 is normally idle high, but booting during a low clock phase can
enter the ROM loader. If needed, disconnect the signal wires, boot the board,
then reconnect them. GPIO36 has no internal pull-up; an external weak pull-up
to the M5Stick's 3.3 V keeps shared CS inactive when the Sony cable is unplugged.
Do not connect the Grove red 5 V wire to a display signal.

Default pin assignments can be overridden in `src/config.hpp`, copied from
`src/config.example.hpp`. Only the CS input may share a GPIO between buses.
No internal TFT pins are used by the default receiver wiring. This prototype
is tested on USB power; battery/power-button management is not implemented.

## Build and upload

Install PlatformIO Core 6.x (or use PlatformIO in VS Code):

```sh
pio run -d firmware/esp32
pio run -d firmware/esp32 -t upload --upload-port /dev/ttyUSB0
pio device monitor -p /dev/ttyUSB0 -b 115200
```

Choose the actual M5Stick port; a port called ttyUSB0 may instead be the IR
adapter on another connection. Identify the device before uploading. No player
commands are needed to build or upload. The generated application binary is
`firmware/esp32/.pio/build/m5stickc-plus/firmware.bin`; use PlatformIO upload to
place the bootloader, partition table, and application at the proper offsets.
No filesystem upload is needed: HTML, CSS, and JS are embedded in flash.

## Open the webpage

1. Join Wi-Fi **ZS-M7-Display**, password **minidisc-test**.
2. Open **http://192.168.4.1/**. The access point does not provide internet.
3. Play a MiniDisc on the Sony. Incoming fields update about every 250 ms.

Optional router connection: copy `src/config.example.hpp` to `src/config.hpp`
and set `ZS_WIFI_SSID` / `ZS_WIFI_PASSWORD`, then rebuild and upload. The device
keeps its own AP available. Serial prints a JSON diagnostic snapshot every five seconds and the router-assigned address; mDNS
also advertises `http://zs-m7.local/` where supported. Change the AP password
in the same local configuration file. `config.hpp` is ignored by Git.

## What the page shows

- The twelve currently transmitted text cells (not an accumulated long title).
- Current track 1-20 and elapsed minutes/seconds.
- The received 20-bit indicator mask, with current track highlighted separately.
- Per-bus frame counts, rejected lengths, age, raw bytes, and receiver queue drops.
- Stale/disconnected indicators; no fabricated time advancement between frames.

`GET /api/state` exposes the same data as JSON. Empty/invalid fields are null.
Unknown title controls are rendered as dots, and their raw byte codes remain
in the API. Frames older than 1.5 seconds are marked stale; the last captured
values stay visible but dim. If the device itself becomes unreachable, the page
shows a disconnected state. The HTTP API has no player-control endpoints.

The numeric mapping is validated for normal MD playback. No mode flag or frame
checksum is known, so valid-looking bytes alone cannot distinguish CD/radio/edit
modes. This firmware does not claim support for those modes.

## Receiver implementation

Two SPI slave peripherals receive mode 3, LSB-first data. MISO pins are disabled
and transmit buffers are null. DMA is disabled because SPI-2 frames are 15 bytes.
Each peripheral has eight queued receive buffers; a separate FreeRTOS task
returns completed frames to a bounded 32-frame application queue. HTTP handling
runs independently. Unexpected lengths are rejected, and application queue
overflow is reported. Hardware frames missed while a peripheral is not armed
cannot be counted exactly; compare the approximately 31.4 frames/s per bus with
Saleae during the first live-wiring test.

The board uses `ISpiFrameSource`, the same decoder headers as CSV replay, and
`LiveDisplay` for JSON snapshots. Capture timestamps approximate task receipt
time, rather than precise CS edges. No allocation or formatting is performed
inside custom interrupt callbacks; capture uses the ESP-IDF SPI driver.

## Validation and local preview

```sh
cmake -S . -B build
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/live_display_test captures/test20/20-next-raw/digital.csv > /tmp/zsm7-state.json
python3 scripts/preview_display.py --fixture /tmp/zsm7-state.json
python3 scripts/test_dashboard.py --fixture /tmp/zsm7-state.json
```

The preview explicitly serves recorded data and never accesses serial or SPI.
Browser checks require local Chromium. Native tests verify all twenty captures,
shared-CS payload equivalence, JSON escaping, stale data, millis rollover, and
partial-frame rejection. A compiled firmware image and a passing replay do not
replace a live electrical/timing check of the wired board.

References: [M5StickC PLUS pin map](https://docs.m5stack.com/en/core/StickC-Plus),
[ESP-IDF 4.4.7 SPI slave driver](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/peripherals/spi_slave.html).

## Live-wiring diagnostic status

Direct G0 clock reception is currently failing. The PLUS has a fixed 1 kOhm
pull-up on GPIO0, and the loaded startup ADC measurement never dropped below
approximately 1.935 V. The parallel header is needed to confirm the loaded versus
unloaded waveform. See ../../analysis/m5-gpio0-findings.md before treating the
original direct-wiring table as a working electrical interface.

The diagnostic build adds `receiver` fields (bit lengths, independent PCNT
clock-edge counts, sampled GPIO levels, and routing registers) and
`gpio0_voltage_at_boot` (approximate ADC range sampled before Wi-Fi). It filters
empty transactions, bounds receiver draining, and rearms recycled descriptors
only while CS is inactive, preventing the observed empty-frame queue flood.

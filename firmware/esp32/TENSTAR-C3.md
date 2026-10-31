# Tenstar Robot ESP32-C3 Super Mini

This build reuses the ZS-M7 decoder and web dashboard. Both buses use IRAM-resident GPIO interrupts on the
rising clock edge plus CS framing. The first hardware-SPI implementation
reported incorrect lengths; the working prototype uses GPIO capture for both. There are no player-facing output pins.

## Wiring

| Sony signal | Tenstar GPIO |
|---|---:|
| SDT1 (numeric data) | 7 |
| SCK1 | 6 |
| CS1 | 10 |
| SDI2 (title data) | 4 |
| SCK2 | 5 |
| CS2 | 3 |
| SGND | GND |

Leave SDO2 unconnected. This target uses separate selects; do not copy the
M5Stick shared-CS wiring. GPIO0, boot pins 2/8/9, onboard LED8, and USB pins18/19
are unused. Pin numbers refer to GPIO labels, not connector positions.

Inputs must be 3.3 V compatible. Verify the Sony high/low levels and use suitable
level conversion if needed. Removing the M5Stick's strong pull-up from the circuit
does not establish that the Sony high level is safe for the C3.

## Build and upload

```sh
pio run -d firmware/esp32 -e tenstar-c3
pio run -d firmware/esp32 -e tenstar-c3 -t upload --upload-port /dev/ttyACM0
```

The target uses a 4 MB ESP32-C3, DIO flash at 40 MHz, and native USB Serial/JTAG.
The initial esptool RAM upload stub crashed during flash writing on this board.
The target therefore uploads with `--no-stub` at 115200 baud and uses a full
watchdog reset afterward to leave manually entered ROM download mode.

If USB disappears or automatic reset cannot enter download mode, hold BOOT,
tap RESET, then release BOOT. Upload again after the USB port reappears. Do not
hold BOOT after upload. No backup is needed per the user's instruction.

## Webpage

SSID **ZS-M7-Display**, password **minidisc-test**, URL **http://192.168.4.1/**.

The alternative **http://192.168.4.1/tft** page previews a 320 × 240 display.
Its Live player mode uses `/api/state`; its example selector shows sample
states without changing the player. The page is a browser prototype for a
future TFT and does not drive a physical display. Decoded states include MD,
radio, radio-to-MD recording, clock, timer, volume, audio adjustment and
program selection. Source or edit modes without decoded fields are shown
without invented values.
The AP has no internet access. The existing optional `config.hpp` router settings
also work; do not copy the M5-specific pin overrides from `config.example.hpp`.
Use `config.c3.example.hpp` if C3 wiring overrides are needed.

`/api/state` includes per-bus `receiver` diagnostics. `backend` is `gpio-isr` for both buses. Raw callback counts include
callbacks observed while the input was low; these do not count as sampled bits.
The ISR requires a high clock and at least 18 microseconds between accepted
edges (measured clock period ~25.5 microseconds). Timing uses esp_timer, not
the CPU performance counter. Expected frame lengths are 128 and 120 bits.

Frame publication waits at least 50 microseconds after select deasserts, allowing
a pending final clock interrupt to finish. A separate task copies completed
frames only after select is inactive. Routine receive buffers are reused to
avoid heap locks during capture. Partial frames increment rejection counters
and do not erase the most recent complete display; valid-data age still expires
after 1.5 seconds.

Diagnostic POST endpoints `/api/capture/pause` and `/api/capture/restart` pause
input capture or reboot the board. They do not send any command to the Sony.

## Verification boundary

The C3 firmware builds, and a native test feeds raw Saleae transitions through
the same fixed-storage bit accumulator, recovering 0123456789AB and TEST.
Overflow cannot wrap into a valid frame. Both GPIO handlers link into IRAM.
The M5 target still builds separately. Physical-board startup and reception
results are tracked in TENSTAR-C3-VALIDATION.md.

Live GPIO reception was compared under Wi-Fi load against Saleae: all100
sampled complete payloads matched exactly, with zero additional queue drops.
About6–7% of incoming frames had incomplete bit counts and were rejected;
see TENSTAR-C3-VALIDATION.md for measured results.
Wrong bit lengths and application queue drops are counted. A valid length alone
cannot prove that no data bit was sampled incorrectly. No checksum rule is known.

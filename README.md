# Sony ZS-M7 Display Interface Reverse Engineering

## Goal

Capture and decode the serial signals between the Sony ZS-M7 main board and its front display, then use the decoded display data to drive a replacement OLED.

## STM32 TFT and USB debugging

The `blackpill` STM32F411 firmware receives both Sony SPI streams, drives the
white-on-black LVGL display, and streams timestamped raw frames over its prepared
USB virtual COM port. See [wiring, build and USB trace instructions](blackpill/README.md).
Use [`scripts/capture_stm32_usb.py`](scripts/capture_stm32_usb.py) to save payload
traces on a PC without the logic analyzer.

## Files and references

- [`images/display-intreface.png`](images/display-intreface.png): connector/interface schematic. The filename contains the spelling `intreface`.
- [`images/personal_minidisc_system_zs_m7_1791802.jpg`](images/personal_minidisc_system_zs_m7_1791802.jpg): front-panel/display photo.
- [`images/IMG_6467.jpeg`](images/IMG_6467.jpeg): clearer display photo showing the 12-character area, track indicators 1-20, and position bar.
- [`sony_ZS-M7_service_manual.pdf`](sony_ZS-M7_service_manual.pdf): service manual.

## Interface clues

The display connector schematic shows IC401 (`CXP82832-005Q`, marked `DISPLAY DRIVE`) and two serial-looking signal groups:

| Group | Schematic labels |
|---|---|
| 1 | `CS1`, `SCK1`, `SDI1` |
| 2 | `CS2`, `SCK2`, `SDI2`, `SDO2` |

These labels suggest two SPI-like interfaces, but the protocol, directions, voltage levels, timing, and relationship between the groups have not yet been verified. `SDO2` may be a return line; confirm from captures rather than assuming.

Use `SGND` as the analyzer reference. Verify signal voltage levels before connecting probes. Do not connect logic-analyzer inputs to display power or VFD pins.

### Saleae channel wiring

The user confirmed this physical channel map:

| Saleae channel | Board signal | Role |
|---:|---|---|
| 0 | Unused | -- |
| 1 | `SDO2` | SPI-2 return/MISO |
| 2 | `SDI2` | SPI-2 data/MOSI |
| 3 | `SCK2` | SPI-2 clock |
| 4 | `CS2` | SPI-2 active-low enable |
| 5 | `SDT1` | SPI-1 data/MOSI |
| 6 | `SCK1` | SPI-1 clock |
| 7 | `CS1` | SPI-1 active-low enable |

Both buses decode MSB first, 8 bits per transfer, CPOL=1, CPHA=0 (SPI mode 2), with active-low enable. Connect Saleae ground to `SGND`; do not connect analyzer inputs to display power or VFD pins.

## Saleae MCP server

The local Saleae Logic 2 MCP endpoint previously responded at `http://127.0.0.1:10530` and reported physical device ID `DAAD127308D7F0CD` (Logic). It can be configured in VS Code with:

```json
{
  "servers": {
    "saleae-logic2": {
      "type": "http",
      "url": "http://127.0.0.1:10530"
    }
  }
}
```

The MCP exposes capture, native `.sal` save, raw-data export, SPI analyzer, and decoded-table export tools. The first attempt was made with unconnected probes and yielded only constant-high channels; later captures below were made after the probes were connected.

## IR remote serial adapter

The remote controller is on `/dev/ttyUSB0` at 115200 baud. Its help response indicates that commands are line-based and terminated with carriage return (`\r`):

```text
help
status
sensor [status]
sensor threshold <on_adc>
list
select mds-s40|zs-m7
send <command>
sendcmd <1-based scan index>
scan start <all|12|15|20> [offset]
scan play
scan short
scan status
scan stop
```

The remote was selected as `zs-m7`. `status` reported:

```text
OK device=zs-m7 protocol=SONY15 address=100
```

`help\n` returned `ERR unknown request; use help`; `help\r` worked. The following transmissions were accepted:

| Remote command | Reported SONY function |
|---|---:|
| `send play` | 66 |
| `send track_next` | 68 |
| `send track_prev` | 67 |

Other available commands reported by the remote include `play_cd`, `power_off`, `source_cycle`, `stop`, `fast_forward`, `rewind`, `eject_cd`, `eject_md`, and `digit_0` through `digit_9`. Avoid power/eject commands unless specifically needed.

## Test MiniDisc

The inserted disc has 12 ungrouped SP tracks, with distinct titles and varied durations:

| Track | Title | Duration |
|---:|---|---:|
| 1 | `8` | 00:05:44 |
| 2 | `1` | 00:07:17 |
| 3 | `2` | 00:05:59 |
| 4 | `5` | 00:02:16 |
| 5 | `0` | 00:02:26 |
| 6 | `A` | 00:02:52 |
| 7 | `b` | 00:02:27 |
| 8 | `H` | 00:03:20 |
| 9 | `PLAY` | 00:00:45 |
| 10 | `TRACK 10` | 00:05:15 |
| 11 | `0123456789` | 00:01:31 |
| 12 | `LONG TITLE SCROLL 0123456789 ABCDEFGHIJKLMNOP` | 00:00:28 |

This set is useful for observing digit/letter glyphs, multi-character titles, a two-digit track number, title truncation/scrolling, and track transitions. Keep playback mode normal (no shuffle, program, or groups) for repeatable observations.

## Current protocol result

The constant-title 20-track experiment resolves the main MD playback fields.
Use **rising-edge sampling, LSB first (mode 3)**. This corrects the historical
mode-2/MSB-first settings elsewhere in the early investigation notes.

| Stream | Logical byte offsets (zero-based) | Field |
|---|---|---|
| SDI2 | 2-13 | Twelve ASCII cells; 0xff padding |
| SDT1 | 11 | Current track, packed BCD |
| SDT1 | 13, 14 | Elapsed minutes, seconds, packed BCD |
| SDT1 | 7, 8, low nibble of 9 | Track indicators 1-20 |

The replay prints text, current track, elapsed time, and the indicator mask.
See [capture evidence and limits](analysis/test20-findings.md) and
[corrected title findings](analysis/title-findings.md). The original display
is dead; reference values come from controlled disc contents and protocol
consistency. Unknown modes and other flags remain unassigned.

## Captures and measurements

All successful capture artifacts are under [`captures/`](captures/). Each native `.sal` file can be reopened in Logic 2. Raw digital transition CSVs and SPI decoded-table CSVs are saved alongside it.

| Capture | Contents | Artifacts |
|---|---|---|
| Stopped baseline | 2 s, 8 MS/s, channels 0-7; player stopped | [`00-stopped-baseline.sal`](captures/00-stopped-baseline.sal), [`00-stopped-baseline-csv/digital.csv`](captures/00-stopped-baseline-csv/digital.csv), [`00-stopped-baseline-decoded.csv`](captures/00-stopped-baseline-decoded.csv) |
| Play/navigation (historical) | 16 s, 8 MS/s; includes `stop` | [`01-play-and-navigation.sal`](captures/01-play-and-navigation.sal), [`01-play-and-navigation-raw/digital.csv`](captures/01-play-and-navigation-raw/digital.csv), [`01-play-and-navigation-decoded.csv`](captures/01-play-and-navigation-decoded.csv), [`01-play-and-navigation-commands.txt`](captures/01-play-and-navigation-commands.txt) |
| Explicit track sweep (historical) | 20 s, 8 MS/s; includes `stop` | [`02-track-sweep.sal`](captures/02-track-sweep.sal), [`02-track-sweep-raw/digital.csv`](captures/02-track-sweep-raw/digital.csv), [`02-track-sweep-decoded.csv`](captures/02-track-sweep-decoded.csv), [`02-track-sweep-commands.txt`](captures/02-track-sweep-commands.txt) |
| Corrected analyzer assignment | Re-analysis of the same sweep with channel 2 as SPI-2 MISO | [`02-track-sweep-wired-decode.sal`](captures/02-track-sweep-wired-decode.sal), [`02-track-sweep-wired-decoded.csv`](captures/02-track-sweep-wired-decoded.csv) |
| Isolated track 1 | 3 s, stopped, expected track 1 | [`03-track01-stopped.sal`](captures/03-track01-stopped.sal), [`03-track01-stopped-raw/digital.csv`](captures/03-track01-stopped-raw/digital.csv), [`03-track01-stopped-decoded.csv`](captures/03-track01-stopped-decoded.csv) |
| Isolated track 10 | 3 s, stopped, expected track 10 | [`04-track10-stopped.sal`](captures/04-track10-stopped.sal), [`04-track10-stopped-raw/digital.csv`](captures/04-track10-stopped-raw/digital.csv), [`04-track10-stopped-decoded.csv`](captures/04-track10-stopped-decoded.csv) |
| Isolated track 12 | 5 s, stopped, expected track 12 | [`05-track12-stopped.sal`](captures/05-track12-stopped.sal), [`05-track12-stopped-raw/digital.csv`](captures/05-track12-stopped-raw/digital.csv), [`05-track12-stopped-decoded.csv`](captures/05-track12-stopped-decoded.csv) |

The track-state command log is [`03-isolated-track-states-commands.txt`](captures/03-isolated-track-states-commands.txt). The track labels are inferred from the known starting position and acknowledged step counts, not read from the player.

### Safe per-track run

The latest run created one 12-second native capture and raw digital CSV per track in [`captures/per-track/`](captures/per-track/). Each capture was started before the corresponding command and retained at least 10 seconds after the remote acknowledgment. The sequence used `play` for track 1 and one `track_next` per following track; it sent no `stop` or `power_off`. See [`track-run-log.txt`](captures/per-track/track-run-log.txt) for track/file mapping. Track numbers are still inferred from command order because the permitted remote command set has no read-only track-status command.

The three `00-...` captures in that directory are failed/pre-correction diagnostic runs, not track data. In particular, `digit_1` was rejected and one earlier run sent `stop` before the user prohibited it.

### Title-test run

The current disc has the requested alphanumeric test strings on tracks 1-6 and reference titles on tracks 7-12. Each expected track has a unique 6-second, 8 MS/s native capture and raw CSV under `captures/`. The remote sequence used only `play` on track 1 and `track_next` for each following track, with at least 3 seconds after every acknowledgment. No `stop` or `power_off` was sent. See [`captures/title-test-run-log.txt`](captures/title-test-run-log.txt) for the full file map and limitations.

Track 11's Saleae `wait_capture` call timed out, but save and raw export succeeded. Its CSV covers 4.7232 seconds and contains 147 complete frames per bus; the other captures are approximately 6 seconds with 188-192 complete frames.

### Observations so far

- Channel 0 stayed high throughout the stopped baseline and is not assigned to either SPI analyzer.
- The stopped baseline already contains continuous bus traffic; the display interface does not go idle when the player is stopped.
- The two enable signals pulse together. In the baseline, each has about 65 active-low pulses in 2.078 s. In the 16 s navigation capture, both analyzers report 504 frames with a median period of 31.866 ms (about 31.4 frames/s).
- With the user's physical wiring and mode-2 settings, SPI-2 (`SDI2`/`SDO2`) carries 15 bytes per frame and SPI-1 (`SDT1`) carries 16 bytes per frame. The two active-low chip selects pulse together at approximately 31.4 frames/s.
- The raw replay parser observes 376-379 complete frames per 12-second per-track capture. Both SPI-2 data directions vary; SPI-1 also varies. Per-track settled-tail signatures differ, especially on SPI-2, but track positions are inferred from command order.
- Working hypothesis: SPI-2 bytes 2-13 are a 12-cell display-data area. The known short titles populate fewer leading bytes, while `TRACK 10`, `0123456789`, and the long title populate progressively more cells; unused cells often read as `0xFF`. Bytes 0-1 and 14 may be control/trailer fields, but that is not established. Per-frame bit changes and ambiguous character values mean this is not yet a glyph decoder.
- The title-test run has 188-192 complete frames per bus (track 11 is the shortened exception). SPI-2 `SDO2` return bytes 2-13 correlate strongly with the six 12-character test titles, providing candidate one-byte-per-character code samples. This does not yet prove that `SDO2` is the authoritative text source; compare it against `SDI2` and `SDT1` before assigning protocol semantics.
- No continuous elapsed-time trace was captured, and the current disc has only 12 tracks. Track-number indicators 13-20 remain unmeasured.
- The isolated stopped captures for expected tracks 1, 10, and 12 have very similar per-byte MOSI value distributions. No title or track-number field has been identified yet; the acknowledged IR transmissions and assumed track positions are not proof that the player received the commands.
- The IR adapter's sensor read `PLAY_LED_OFF` before and after a `play`/`stop` check. Remote transmission was acknowledged by the adapter, but the player's receipt was not independently confirmed.
- These are preliminary decodes, not a complete protocol reverse engineering. Bit/byte alignment, exact wire mapping, semantic fields, and the meaning of the two streams remain to be established. The analyzer settings are the ones shown in the provided screenshots; test alternative settings only against saved traces.
- The initial unconnected-probe trace was written under `/tmp` and is not part of this repository because it contains no useful signal data.

### Remote command sequence for the captures

The earlier 16 s and 20 s traces include `stop` and are retained only as historical evidence. Do not use those command sequences for a new hardware run.

The 20 s trace began stopped on track 11. It sent `track_prev` ten times to reach track 1, `track_next` nine times to reach track 10, `track_next` twice to reach track 12, `track_prev` to return to track 11, then `stop`. Commands were spaced by approximately 0.65 s; the remote acknowledged each. The command log's timestamps are relative to the first command, not the Saleae capture start.

### Next analysis steps

1. Establish ground-truth track/title state through audible confirmation; the player has no allowed read-only track query.
1. Compare `SDO2`, `SDI2`, and `SDT1` in the title-test traces to isolate title bytes and validate a character table.
2. Capture a known track for at least 60 seconds after the 3-second seek delay to identify elapsed-time fields.
3. Use a 20-track disc with a constant title to map all track indicators 1-20 independently from title bytes.
4. Save any additional native trace and raw CSV under `captures/`, with command and wait details recorded in the run log.

## C++ replay tool

The C++17 code separates the protocol consumer from its source with `ISpiFrameSource`. A platform-specific receiver can implement that interface; `SaleaeCsvSource` replays raw Saleae transition CSVs using the confirmed channel map, rising-edge sampling, LSB-first bytes, and active-low chip selects. The replay reports complete/partial frames and the latest raw `SDT1`, `SDI2`, and `SDO2` bytes. The decoders expose the twelve-cell text window and numeric track/time fields; the MCU receiver and OLED renderer remain to be implemented.

Build and run the captured-data replay:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/display_replay captures/per-track/track-12-next-raw/digital.csv
```

The regression suite uses Python 3 as well as the native compiler. In addition
to raw Saleae replays, `captured_c3_snapshots` feeds saved C3 JSONL payloads
through the native decoder and checks clock and timer blink phases. These are
periodic snapshots of the latest bus frames, so they do not replace a
synchronized logic-analyzer capture. Keep the referenced recordings under
`captures/` when sharing or committing the tests.

Append `--dump` to print every frame and its bytes. The `captured_track_replay` CMake test replays the prior twelve-track set and the twelve title-test raw CSVs, checking frame lengths, return-data presence, and within-capture payload variation. The test executable can also be run directly with the raw CSV paths.

The `captured_numeric_display` test replays all twenty constant-title captures and checks track, elapsed time, minute rollovers, and indicator masks.

## MCU web-display prototype

The [M5StickC PLUS 1.1 firmware](firmware/esp32/README.md) listens to both buses
and hosts a live Wi-Fi dashboard. It uses five signal GPIOs by feeding CS1 into
both hardware SPI receivers; all twenty captures validate that framing.
The built-in TFT is unused. See the firmware README for the exact wiring, voltage
requirements, build/upload commands, Wi-Fi access, and verification limits.

The active hardware target is now the [Tenstar ESP32-C3 Super Mini](firmware/esp32/TENSTAR-C3.md), using filtered GPIO interrupts for both buses. The M5Stick G0 input was electrically loaded by its fixed pull-up.

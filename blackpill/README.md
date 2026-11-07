# STM32 Sony display receiver

The firmware now boots into **live reception**, using `MX_SPI1_Init()` and
`MX_SPI2_Init()`. It displays `No signal` until complete numeric frames arrive.
The original white-on-black demo remains available through `sony_demo_start()`;
pause calls to `sony_live_tick()` when explicitly running that demo.

## Wiring

Use the existing SPI1 and SPI2 pins from this STM32F411 project:

| Sony signal | STM32 pin | Function |
|---|---|---|
| CS1 | PA4 | SPI1 hardware NSS and shared frame end for both receivers |
| SCK1 | PA5 | SPI1 clock |
| SDT1 (numeric data; SDI1 on schematic) | PA7 | SPI1 receive data |
| CS2 | Unconnected | Damaged line; SPI2 is framed using CS1 internally |
| SCK2 | PB13 | SPI2 clock |
| SDI2 (text / commands) | PB14 | SPI2 bidirectional **receive-only** data |
| SGND | GND | Common signal reference |

Leave **PA6, PB12, PB15, Sony CS2 and Sony SDO2 unconnected**. Do not join the Sony CS1 and CS2
lines. SPI2 deliberately retains the project's one-line slave pin PB14, which
is the bidirectional data input in this configuration; it is not transmitting
MISO data. SPI1 uses two-line receive-only mode and leaves PA6 as a GPIO input.
No SPI transmit operation is used. The TFT remains on its existing SPI3 bus.

Use compatible input voltage levels for the STM32 pins and a shared signal
ground; this firmware does not establish the Sony's electrical voltage levels.
For the first listening test, leave the Sony's original display controller
connected: return traffic on SDO2 is neither captured nor emulated here.

Both receivers use **mode 3 (CPOL high, CPHA second edge), 8-bit, LSB first**,
matching the repository's corrected capture decoder. This replaces the initial
Cube-generated mode-0/MSB-first values.

## Build / flash

Build the existing STM32CubeIDE **Release** configuration and flash
`Release/blackpill.elf` with the project's usual ST-Link setup. Or, with the
STM32 GNU tools on PATH:

```sh
make -C blackpill/Release -j4 all
```

`makefile.defs` is the CubeIDE make hook for `sony/sony_spi.c` and
`sony/sony_live.cpp`, kept outside the generated source roots. It builds the
bridge as C++17 with exceptions/RTTI disabled, links the C++ runtime, and reuses
`../include/zs_m7` from this repository. Keep the entire repository layout.
No changes to ignored generated Release/Debug makefiles are required.
The stack reservation is increased to 8 KiB for decoding/rendering.

## Reception / display

SPI1 RX uses DMA2 stream 0, channel 3. SPI2 RX uses DMA1 stream 3, channel 0.
These do not conflict with the existing SPI3 LCD DMA1 streams 0 and 5.
At each CS1 rising edge, the ISR handles both receivers: it stops DMA, checks received byte count and
DMA/overrun errors, queues a fixed-size raw record including error flags, resets
the SPI to discard partial shift state, then rearms for the next frame. The first
transaction is excluded from display decoding
to synchronize even if startup occurs midway through a transfer. Expected lengths
are 16 bytes for SDT1 and 15 bytes for SDI2. RX DMA has 32-byte capacity to detect
extra complete bytes. SPI hardware does not expose a trailing partial-bit count;
a frame with the expected whole-byte count plus extra fractional bits cannot be
identified solely from DMA NDTR.

Each bus has an eight-record queue. The main loop consumes both queues in timestamp
order, calls the shared `LiveDisplay` decoder, and presents its typed snapshot to
LVGL. Capture interrupts do no allocation, protocol decoding or LVGL work.
EXTI has priority 0, LCD DMA priority 1 and USB priority 2. Rendering is limited to
roughly 30 Hz and unchanged presentations do not trigger LVGL updates. Timers,
clock blinks, and volume commands use the same decoder/history as the webpage;
readout values do not advance between received frames.

After 1.5 seconds without a valid SDT1 frame, the screen shows `No signal`.
If only SDI2 stops, stale title/command data is hidden and views with a text footer
report `SPI2 / text unavailable`. Reception recovers on subsequent valid frames.
Use debugger watch `sony_spi_stats[0]` / `[1]`: each has `accepted`, `rejected`, and
`dropped` counters. One initial rejected frame per bus is expected. Increasing
rejections indicate byte-count, DMA or overrun errors; increasing drops indicate
the main loop cannot keep up. No reception will leave all counters at zero until
an NSS rising edge is observed.

## Validation

The Release firmware builds successfully: 514,664 bytes text, 536 bytes data,
118,872 bytes BSS (including the stack/heap reservations), within 512 KiB flash
and 128 KiB RAM. The linker emits the existing toolchain GNU-stack metadata
warning. All 37 CTest tests pass, including `stm32_live_capture_replay`, which
feeds saved Sony payloads through the STM32 bridge and checks playback, FM,
disc messages, clock, stale-data handling, tick wrap and unchanged-state redraws.
The USB tests additionally verify DTR/session handling, disconnected-device
safety, asynchronous transmit-buffer ownership, slow-host backpressure, raw
rejected-frame output, queue drops, statistics, and reconnect behavior.
These are software checks. The new DMA/EXTI receiver has not yet been flashed
or validated against the physical Sony clocks, NSS timing, or red filter.

## USB virtual COM capture

The prepared USB CDC port now streams both SPI buses while the display continues
running. Open the STM32's USB virtual COM port with **DTR asserted** (normal serial
terminal behavior). Use 115200 / 8N1 in the terminal; this is a USB device, so the
baud selection does not determine capture speed. Closing the port disables logging.
There are no commands to type and no extra serial adapter is needed.

To save a capture from the repository root:

```sh
python3 -m pip install pyserial
python3 scripts/capture_stm32_usb.py --list
python3 scripts/capture_stm32_usb.py --port /dev/ttyACM0 --output captures/sony-usb-trace.csv --seconds 60
```

On Windows use the reported port, for example `--port COM5`. Omit `--seconds` to
record until Ctrl-C. The script creates a new file and refuses to overwrite an
existing one. It saves the device's bytes verbatim after a PC UTC start comment;
its final summary reports the frame counts and latest device statistics.

The ASCII stream starts with `#ZS-M7 SPI TRACE v1` and column comments:

```text
F,bus,sequence,ms,flags_hex,data_hex
S,ms,spi1_valid,spi1_rejected,spi1_dropped,spi2_valid,spi2_rejected,spi2_dropped,usb_dropped
```

`F` is a raw frame record. Bus 1 is SDT1, bus 2 is SDI2. Sequence numbers count
all NSS rising edges separately for each bus, beginning at 1 and wrapping at
32 bits. Missing numbers within a session expose dropped records. `ms` is the
NSS-end timestamp from `HAL_GetTick()` (milliseconds since STM32 boot, wrapping
at 32 bits), **not** the PC arrival time. Hex bytes already have the captured
LSB-first bit order normalized to logical bytes; do not reverse them again.
Payload length is half the hex string length, including zero-byte records.

Flags are a hexadecimal bit mask:

| Flag | Meaning |
|---|---|
| `00` | Valid byte count; synchronized; no recorded DMA/overrun error |
| `01` | First transaction after capture startup; synchronization uncertain |
| `02` | Unexpected complete-byte count (normally 16 on SPI1, 15 on SPI2) |
| `04` | SPI overrun; bytes may have been lost |
| `08` | DMA error; captured bytes may be incomplete |

For example `F,2,7,123,02,00AAFF` reports three raw bytes on SPI2 at millisecond
123, flagged for incorrect length. Flags can be combined. Rejected transactions
are included for debugging but never reach the display decoder. The first frame
on each bus is intentionally marked unsynchronized. Captures exceeding the
32-byte DMA capacity are truncated and flagged for length.

`S` reports cumulative device counters at session start and approximately once
per second. Valid/rejected count observed transactions; `spi*_dropped` counts
records lost before reaching the main loop, including otherwise valid frames.
`usb_dropped` counts records discarded by the separate 32-record USB queue when
the PC is too slow. These counters reset on MCU reboot, not on serial-port close.
USB writes never wait or retry in a loop. The CDC driver retains its own transmit
buffer until completion; a busy PC can lose debug records without blocking SPI
capture or the GUI. Reopening clears queued old-session records and sends a new
header; a USB transfer already submitted before port close may still complete.

This replaces the logic analyzer for **SPI payload / protocol debugging**. It
records received bytes and frame-end timestamps, not clock/data waveforms or
individual edge timing. It does not capture SDO2, voltage levels, or trailing
partial-bit counts; those checks still require external measurement.


## Remembering scrolling track titles

For the current MD track, the decoder joins confirmed overlapping 12-character
windows, up to 160 characters. It waits for repeated windows, a held initial
window, and the observed `0xFF` end padding before publishing a complete title.
Unknown/control glyphs are excluded, and incompatible windows start a fresh
assembly instead of being concatenated. Up to three shifted cells can be matched
when an intermediate window was missed. Titles first observed midway through a
scroll may need another cycle to identify their beginning.

Once complete, the title stays still for that track while the Sony continues
scrolling. Long completed titles use two lines of white 14-point text above the
track/time readouts, with an ellipsis if necessary; short titles retain the
larger font. See [rendered full-title example](title-preview.png). The cache lives
in RAM and is cleared on a confirmed track change, another source, disc-removal
message, or a numeric-bus timeout. Volume commands do not erase it. There are no
flash writes or persistence across power cycles. Freshness rules still apply.

The shared snapshot exposes the completed text as `title` with
`title_complete=true`. JSON keeps the received twelve-cell `title.text` and
`title.codes` intact and adds `title.full_text` for the reconstructed result.
The `sony-usb4.csv` regression recovers `We Are Never Ever Getting Back Together
(Taylor's Version)` and verifies retention across the scroll restart. This is
reconstruction from observed windows, not a protocol-level title-length field;
undetected printable corruption can still affect results.


Timer mode selection also recognizes the playback layout captured in
`captures/sony-usb5.csv`: the source and numeric digits are blank, but PLAY,
disc-animation state and track-calendar bits remain present. This now selects
the `TIMER / SETUP` screen with PLAY/REC blink handling instead of falling through
to `Waiting for display data`. Regression tests cover the real capture and both
PLAY/REC choices with retained playback state.

Timer On/Off time entry also accepts the retained MD playback layout from
`captures/sony-usb6.csv`. The capture shows 12:03 with hour editing; the timer
screen now displays those digits and follows the received hour blanking while
retaining both values in the decoder. Tests also exercise minute blanking, Off
context and rejection of invalid hour/minute values in this layout.

Timer volume setup during MD playback is confirmed by `captures/sony-usb7.csv`:
level 4 alternates visible/blank while transport and calendar bits remain set.
The display now shows `TIMER PLAY / SET VOLUME`, the received value and its bar.
Sony sends mode-05 text commands only briefly on entry, so their timer context
is retained as long as fresh numeric frames match the volume layout. Leaving
that layout clears it; loss of numeric frames still triggers the usual timeout.
The recording regression checks both blink phases through the STM32 renderer,
and a separate workflow test checks retention beyond the command timeout.

Radio timer REC setup with retained playback flags is confirmed by
`captures/sony-usb8.csv` (12:03, hour editing) and `captures/sony-usb9.csv`
(volume 4). The shared retained-playback check now covers both MD timer PLAY
and radio timer REC for time and volume entry. The display uses `TIMER REC`
for these captures and follows the received blinking. Regression replay covers
all eight USB setup/title/date recordings, with synthetic checks for REC minute/Off
entry, volume-context retention, invalid values and unsupported source flags.


## Date display

`captures/sony-usb10.csv` confirms the DATE screen while the Sony is in standby.
The text bus carries `12y  1m  1d ` and the numeric bus carries 22:01. The TFT
now shows `DATE`, a `Date / YY-MM-DD` heading, `12-01-01`, and the clock below it
with its received colon blink. The two-digit year is preserved; no century or
PC date is substituted. See [date preview](date-preview.png).

The same date layout also appears while the player is active, so it is not used
to infer a power-state change. A typed `CalendarDate` is exposed as `snapshot.date`
only with fresh numeric DATE/clock state, fresh text and valid date markers and
numeric ranges. Missing/stale text, invalid date fields, and a cleared DATE bit
cannot leave an old date visible. Tests cover the USB trace through the actual
STM32 renderer and preserve the earlier active-clock/date capture behavior.


`captures/sony-usb11.csv` captures clock-hour editing after the calendar date
has been set: 22:05 alternates with a blank hour while the armed timer flags
remain set. This now shows `CLOCK / SETTING`, `Set clock`, and `Adjust hour`
with the received blanking. The armed-timer variant retains the last valid hour
and minute during their dark phases and also supports subsequent minute editing.
Tests distinguish it from the steady date readout, timer setup, and malformed
time digits; the real USB trace is replayed through the STM32 presentation code.


## Stable clock positions

Five-character time readouts now use fixed-width character cells with the
existing Montserrat fonts. The colon and any edited digit pair can be blanked
without changing the positions of the other digits. This applies to the large
clock/timer/playback readouts and the small clock under the date. Ordinary text
keeps its existing spacing; no additional font is required.

[Colon on/off preview](clock-blink-preview.png). Native LVGL framebuffer checks
confirmed identical minute pixels across colon and hour blanking for both font
sizes and both alignments, and checked that time cells disappear when switching
to ordinary values. The Release firmware build passes.


## Shared CS1 for SPI1 and SPI2

The damaged SPI2 select connection is bypassed. Connect Sony CS1 only to PA4;
**disconnect Sony CS2 from PB12 and leave PB12 unused**. Do not electrically tie
the damaged CS2 line to CS1. SCK2 remains PB13 and SDI2 remains PB14.
`MX_SPI2_Init()` now selects software NSS, and the capture driver explicitly
sets SSM=1 / SSI=0 (internally selected slave, receive-only). SPI2's own clock
still controls sampling; its DMA packet is finalized and rearmed on the same
PA4 rising edge as SPI1. There is no dependency on PB12 or EXTI12.

SPI2 remains selected between frame boundaries, so any stray clocks in that
interval also count toward its next packet. Frame lengths and DMA/overrun flags
continue to detect incomplete/oversized transactions. The shared-select replay
test checks saved Sony traces against the original independent-CS framing;
live timing on the repaired setup still needs checking with a fresh USB log.
USB records retain their existing format and separate bus counters.

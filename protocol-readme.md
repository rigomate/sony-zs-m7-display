# Sony ZS-M7 Display Protocol

This document records the currently validated protocol for the ZS-M7 display while playing a normal MiniDisc. It supersedes the earlier exploratory interpretation that used falling-edge sampling and MSB-first bytes. Those settings produced unstable numeric values and are historical only.

## Bus wiring and framing

The Saleae channel map used for the captures is:

| Saleae channel | Signal | Direction at display connector |
|---:|---|---|
| 1 | `SDO2` | SPI-2 return/readback |
| 2 | `SDI2` | SPI-2 data into display |
| 3 | `SCK2` | SPI-2 clock |
| 4 | `CS2` | SPI-2 chip select |
| 5 | `SDT1` | SPI-1 data |
| 6 | `SCK1` | SPI-1 clock |
| 7 | `CS1` | SPI-1 chip select |
| 0 | Unused | -- |

Ground reference is `SGND`. Do not connect logic inputs to display power or VFD pins. Signal voltage and driver ownership have not been established for direct MCU attachment; measure levels and electrical direction before connecting replacement hardware.

Both links use 8-bit transfers, active-low chip select, an idle-high clock, and sample on the rising clock edge. Bits arrive least-significant bit first. This is equivalent to SPI mode 3 (CPOL=1, CPHA=1) with LSB-first byte packing. Logical byte indices below refer to bytes produced by that corrected parser, not the earlier mode-2/MSB-first exports.

- SPI-1 (`SDT1`, selected by `CS1`): 16 bytes per complete frame.
- SPI-2 (`SDI2`/`SDO2`, selected by `CS2`): 15 bytes per complete frame.
- Both links refresh at about 31.4 frames/s in the captures.

## Validated normal-MD fields

### Title window

SPI-2 `SDI2`, bytes 2-13 inclusive, carry a 12-character display window as ordinary ASCII. An unused suffix is padded with `0xFF`. The title decoder returns printable text only when all non-padding codes are printable ASCII and padding occurs only at the end. It preserves raw codes instead of inventing a glyph for control/non-ASCII values.

The window is not a reconstruction of the entire disc title. Long-title scrolling has not been fully characterized. `SDO2` often agrees with the title stream after it settles, but it can differ during updates; use `SDI2` as the observed title input.

### Track number and elapsed time

SPI-1 `SDT1` contains packed BCD values:

| Byte offset | Meaning | Example |
|---:|---|---|
| 11 | Current track, range 1-20 in the test | `0x10` = track 10 |
| 13 | Elapsed minutes | `0x02` = 2 minutes |
| 14 | Elapsed seconds, validated range 00-59 | `0x59` = 59 seconds |

Each nibble is a decimal digit. The decoder rejects invalid BCD, seconds above 59, incomplete frames, and track numbers outside the tested 1-20 range. The 140-second capture shows elapsed time advancing with capture time, including `00:59 -> 01:00` and `01:59 -> 02:00` rollovers. This is elapsed playback time; remaining time, disc total time, and minutes beyond the tested range are not decoded.

### Track indicators 1-20

The instantaneous indicator mask is assembled from SPI-1 bytes 7-9:

```text
mask = byte[7] | (byte[8] << 8) | ((byte[9] & 0x0F) << 16)
```

Bits map in ascending order: mask bit 0 is track 1, bit 1 is track 2, through bit 19 for track 20. The high nibble of byte 9 is not part of this mask.

On the constant-title 20-track disc, the current and later track bits were set while earlier track bits were clear. This behavior was checked against all 20 track positions. The mask is an observed instantaneous state; the meaning of a separately toggling flag and any visual blinking behavior are unknown.

## Evidence and implementation

The principal evidence is the constant-title 20-track run in [`captures/test20/`](captures/test20/): one 140-second play capture followed by a 16-second capture for each `track_next` position. [`captures/test20/run.jsonl`](captures/test20/run.jsonl) records command acknowledgments and artifact completion. The title test set and corrected character analysis are documented in [`analysis/title-findings.md`](analysis/title-findings.md); numeric-field validation is in [`analysis/test20-findings.md`](analysis/test20-findings.md).

The portable C++ decoding boundary is implemented in:

- [`include/zs_m7/spi_frame_source.hpp`](include/zs_m7/spi_frame_source.hpp): logical frame and source interface.
- [`src/saleae_csv_source.cpp`](src/saleae_csv_source.cpp): corrected rising-edge, LSB-first parser for raw Saleae CSV.
- [`include/zs_m7/title_decoder.hpp`](include/zs_m7/title_decoder.hpp): 12-cell ASCII title window.
- [`include/zs_m7/display_state.hpp`](include/zs_m7/display_state.hpp): packed-BCD track/time and 20-bit indicator decoders.

The `captured_numeric_display` regression checks all 20 track captures, the expected track and indicator mask, advancing time, title `TEST`, and both minute rollovers. Title regression tests check the retained title captures. Rebuild and run the project tests with:

```sh
cmake -S . -B build
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

## Scope and open questions

The field map is validated for normal MiniDisc playback only. A valid BCD value alone does not establish that the stream is in MD mode. Radio/FM currently has a separate experimental decoder and evidence set; do not generalize this MD layout to CD, tuner, edit, or other modes.

Not yet assigned: checksums/trailers, all header/control flags, playback-mode detection, remaining/total time, minutes above 02, track numbers above 20, every character range, complete long-title scrolling, and the display behavior of control codes. The currently decoded ASCII window and numeric values are sufficient to prototype the main MD screen, but electrical compatibility and physical signal ownership must be verified before wiring a replacement MCU/display controller.

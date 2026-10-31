# Sony ZS-M7 Front-Display Serial Protocol

Specification of the two serial links between the ZS-M7 main board and its front display (IC401 `CXP82832-005Q`, "DISPLAY DRIVE").

**Source of truth:** the reverse-engineering work in <https://github.com/rigomate/sony-zs-m7-display> (`protocol-readme.md`, `display_state.hpp`, `title_decoder.hpp`, `TENSTAR-C3.md`), which is based on logic-analyzer captures. The user manual (ZS-M7.pdf) could not be read in this session, so feature names below come from the repository's decoders, not the manual.

## 0. Confidence legend

| Tag | Meaning |
|---|---|
| **[V]** | Validated by controlled captures (regression-tested in the repo) |
| **[O]** | Observed in captures/photos and used by a decoder, but only partially isolated |
| **[I]** | Inferred by me from decoder constants/comments, not directly proven |
| **[?]** | Unknown / not decoded. A new interface must pass these bits through or ignore them |

Important limits (stated by the repo author):

- The field map is **fully validated only for normal MiniDisc playback**. Other modes are decoded by *layout signatures*.
- **No checksum is known.** Unknown bits/bytes are not assigned.
- **Electrical levels and driver ownership have not been established.** Measure before connecting anything.
- The original display is dead, so there is no ground-truth reference display. Values come from controlled disc contents.

---

## 1. Physical layer

Two independent, write-only (from the main board) SPI-like links. They are always active, even when the player is stopped.

| Link | Name here | Signals | Frame size |
|---|---|---|---|
| SPI-1 | **STATUS bus** ("Display1") | `CS1`, `SCK1`, `SDT1` | **16 bytes = 128 bits** |
| SPI-2 | **TEXT/CMD bus** ("Display2") | `CS2`, `SCK2`, `SDI2` (+ `SDO2` return) | **15 bytes = 120 bits** |

Ground reference: `SGND`. Do **not** connect to display power or VFD pins.

### 1.1 Bit-level framing **[V]**

| Parameter | Value |
|---|---|
| Chip select | Active **low**; frame = one CS-low interval |
| Clock idle level | **High** (CPOL = 1) |
| Sampling edge | **Rising** clock edge (this is SPI mode 3, CPOL=1/CPHA=1) |
| Bit order | **LSB first** within each byte |
| Word size | 8 bits, bytes transmitted in index order 0, 1, 2, … |
| Clock period | about 25.5 µs (about 39 kHz); the C3 receiver ignores edges closer than 18 µs |
| Frame rate | about 31.4 frames/s (median period 31.866 ms) on both links |
| CS relationship | `CS1` and `CS2` pulse together, once per frame period |

Frames are only valid with exactly 128 (SPI-1) or 120 (SPI-2) clock edges while CS is low. Frames with other lengths must be discarded. The reference ESP32-C3 receiver sees about 6–7 % short frames under Wi-Fi load and rejects them; a correct bit count does not prove each bit was sampled correctly, since there is no checksum.

Receiver tip from the repo: publish a frame only ≥ 50 µs after CS deasserts, so a pending last clock interrupt finishes. Treat the display as blanked if no valid frame arrives for 1.5 s.

### 1.2 Bit-assembly pseudocode

```c
// On each rising SCK edge while CS is low:
byte_idx = bit_count >> 3;
bit_pos  = bit_count & 7;          // 0 = LSB
if (data_pin) frame[byte_idx] |= (1u << bit_pos);
bit_count++;
// On CS rising: accept only if bit_count == 128 (SPI-1) or 120 (SPI-2).
```

### 1.3 `SDO2`

`SDO2` is a return/readback line on SPI-2. It often echoes the text window after it settles but can differ during updates. Its true semantics are **[?]**. Use `SDI2` as the authoritative text/command input. A replacement display may leave `SDO2` unconnected, as the repo's C3 receiver does; whether the main board needs a response from a replacement display has not been tested.

---

## 2. SPI-1 "STATUS" frame (16 bytes)

All byte indices are zero-based after the LSB-first assembly of §1.2. Byte 15 is never used by any decoder (**[?]**).

### 2.1 Byte map

| Byte | Content | Status |
|---:|---|---|
| 0 | Mode/timer flags (§2.2) | **[O]** |
| 1 | Transport flags (§2.3) | **[O]** |
| 2 | Layout/sub-state code (§2.3) | **[O]** |
| 3 | Source + icon flags (§2.4) | **[O]** |
| 4 | Radio band + PRESET flag (§2.5) | **[O]** |
| 5 | Display-mode flags (§2.6) | **[O]** |
| 6 | Value 0–31 + flags (§2.7) | **[O]** |
| 7 | Track indicators 1–8 (bit0 = track 1) | **[V]** |
| 8 | Track indicators 9–16 (bit0 = track 9) | **[V]** |
| 9 | bits 0–3: track indicators 17–20; bits 4–7 see §2.8 | low nibble **[V]**, high nibble **[?]** |
| 10 | Digit pair, usually `0xAA` (blank) | **[O]** |
| 11 | Track number (packed BCD), or minutes-low/frequency etc. | **[V]** for MD |
| 12 | Hundreds digit (low nibble), high nibble 0 | **[O]** |
| 13 | Minutes / hour / frequency digits | **[V]** for MD |
| 14 | Seconds / minutes / frequency digits | **[V]** for MD |
| 15 | not decoded | **[?]** |

### 2.2 Byte 0: mode / timer flags **[O]**

| Bits | Meaning |
|---|---|
| bit 0 (`0x01`) | **SLEEP indicator lit** **[I]** (decoder comment: "unless the SLEEP indicator is lit") |
| bit 1 (`0x02`) | TIMER **REC** selected (`(b0&0x06)==0x02`) |
| bit 2 (`0x04`) | TIMER **PLAY** selected (`(b0&0x06)==0x04`) |
| bit 3 (`0x08`) | Set together with bit 1 or 2 in the `0x0A`/`0x0C` variants (timer-armed/blink phase) **[?]** |
| bits 4–6 | 0 in all decoded timer/clock screens (`(b0&0x70)==0`) |
| bit 7 (`0x80`) | Toggles in clock-setting screens (observed `0x00`/`0x80`); decoders mask it. Likely blink phase **[?]** |

Observed values used by decoders (bit 7 masked): `0x00` (clock date, radio sleep), `0x02`/`0x0A` (REC timer), `0x04`/`0x0C` (PLAY timer).

### 2.3 Bytes 1–2: transport and layout

**Byte 1 (MD transport) [O]:**

| Bit / value | Meaning |
|---|---|
| bit 5 (`0x20`) | **Playing** |
| bit 4 (`0x10`) | **Paused** (`b1 = 0x30`) |
| `0x00` | Stopped / non-MD screens (clock, timer, source select) |

**Byte 2 [O]** (meaning of the codes not isolated):

| `b1` | Observed `b2` values |
|---|---|
| `0x20` (playing) | `1`, `2`, `3` |
| `0x30` (paused) | `7` |
| `0x00` | `7` (stopped, clock, timer screens), `0` (timer-play volume layout) |

Treat `b2` as a layout selector, not a numeric field.

### 2.4 Byte 3: source and icon flags **[O]**

| Bits | Meaning |
|---|---|
| bits 0–3 (`b3 & 0x0F`) | Source, one-hot: `0x1` = **MD**, `0x2` = **CD**, `0x4` = **RADIO**, `0x8` = **LINE**, `0x0` = none/selection |
| bit 4 (`0x10`) | Set in `0x91` (MD playback with **REMAIN** shown); individual meaning not isolated **[?]** |
| bit 6 (`0x40`) | Set in clock/date screens (`0x40`, `0x41`) **[?]** |
| bit 7 (`0x80`) | Set in `0x91` (see bit 4) **[?]** |

The source logo is independent of disc graphics and the numeric content. Note that the radio frequency decoder requires `b3 == 0x04` exactly.

### 2.5 Byte 4: radio band + PRESET **[O]**

| Value / bit | Meaning |
|---|---|
| `b4 & ~0x02 == 0xC0` | **FM** |
| `b4 & ~0x02 == 0x20` | **AM** |
| bit 1 (`0x02`) | **PRESET** indicator |
| `0x00` | non-radio screens (MD/CD/LINE and most others) |

(The radio-recording and radio-sleep screens also show other values; `b4 == 2` was seen in the radio sleep layout.)

### 2.6 Byte 5: display-mode flags **[O]**

| Bit | Meaning |
|---|---|
| bit 0 (`0x01`) | **MD track/time display valid** (track digits shown); required for MD track and radio-recording decoders |
| bit 1 (`0x02`) | **Clock colon on** (blink phase) in clock screens; `b5 == 2` in timer-time entry |
| bit 7 (`0x80`) | **PROGRAM entry** layout (clear for normal track display) |
| other bits | 0 in all decoded screens **[?]** |

### 2.7 Byte 6: 5-bit value + flags **[O]**

| Bits | Meaning |
|---|---|
| bits 0–4 | Value 0–31. In the timer-volume setting screen this is the **volume level**; in other screens a value ≤ 31 is required (e.g. `0x0C` and `0x0A` are seen in sleep layouts). That it tracks the current volume everywhere is **[I]** |
| bit 5 (`0x20`) | Volume digits visible (clear = blank phase) in the timer-volume screen |
| bit 6 (`0x40`) | **Radio frequency leading "1"**: FM → +100 MHz (i.e. 1xx.xx), AM → +1000 kHz |
| bit 7 | 0 in decoded screens |

### 2.8 Bytes 7–9: track indicators **[V]**

```
mask = b7 | (b8 << 8) | ((b9 & 0x0F) << 16);   // 20 bits
// bit 0 = track 1 ... bit 19 = track 20
```

Observed behaviour on a 20-track disc during playback: the **current and all later tracks are set, earlier ones are clear**. The mask is the instantaneous state; blink behaviour is not characterised.

`b9` high nibble: bit 7 (`0x80`) is set in many MD-related screens (`0x80`, `0x83`) even with no track bits, and clear in "no tracks" layouts (`b7=b8=b9=0`). Its meaning (disc-present / bar graphics) is **[?]**. Bits 4–6 are **[?]**. The position bar and anything beyond 20 tracks are **not decoded**.

### 2.9 Bytes 10–14: digit area **[O]**

Nibble encoding: `0–9` = decimal digit, **`0xA` = blank digit**. Values are packed BCD (high nibble = tens).

| Screen | b10 | b11 | b12 | b13 | b14 |
|---|---|---|---|---|---|
| **MD play/stop/pause** **[V]** | any (`0xAA` in rec) | track (BCD, 01–20 tested) | `0x00` | elapsed **minutes** (BCD) | elapsed **seconds** (BCD, ≤ 59) |
| **MD program entry** **[O]** | – | selected track BCD; blank = `0xAA` | `0x00` (blank: `0x0A`) | – | – |
| **Clock / timer time** **[O]** | `0xAA` | `0xAA` | `0x0A` | **hour** (BCD; tens may be `0xA_` = blank leading zero) | **minute** (BCD) |
| **FM radio** **[O]** | `0xAA` | `0xAA` | `0x0A` | BCD of MHz (2 low digits) | high nibble = 0.1 MHz digit, low nibble = 0.01 MHz digit |
| **AM radio** **[O]** | `0xAA` | `0xAA` | `0x0A` | BCD → tens of kHz | high nibble = units digit of tens-of-kHz, low nibble = `0xA` |
| **Sleep timer** **[O]** | `0xAA` | minutes low 2 digits (BCD) | `0x01` = +100 min, else `0x0A` | `0xAA` | `0xAA` |
| **Timer volume** **[O]** | `0xAA` | `0xAA` | `0x0A` | `0xAA` | `0xAA` |
| **Radio → MD recording** **[O]** | `0xAA` | track BCD | `0x00` | rec/remain minutes | seconds |

Formulas:

```
FM kHz = ((b6.bit6 ? 100 : 0) + BCD(b13)) * 1000 + (b14>>4)*100 + (b14&15)*10   // 87500..108000
AM kHz = (b6.bit6 ? 1000 : 0) + BCD(b13)*10 + (b14>>4)                          // 100..2000, requires (b14&15)==0xA
Sleep  = BCD(b11) + (b12==1 ? 100 : 0)    // valid: 10,20,30,60,90,120; b11=0xAA & b12=0x0A = blank phase
Hour   = ((b13>>4)==0xA) ? (b13&15) : BCD(b13)        // 0..23
```

A BCD decoder must reject nibbles > 9 (except `0xA` where blanking is allowed) and seconds/minutes > 59.

### 2.10 Screen / layout signatures

The status frame has no mode byte. The screen type is recognised from constant combinations. These are the signatures the repo's decoders use (all comparisons on the assembled bytes; "×" = don't care):

| Screen | Required constants |
|---|---|
| **MD playing/paused** | `b3 ∈ {0x01,0x91}`, `b4=0`, `b1=0x20` with `b2∈{1,2,3}`, or `b1=0x30` with `b2=7`. Time blank when `b13=b14=0xAA` |
| **MD track display** | `(b3&0x0F)=1`, `b5.bit0=1`, `b5.bit7=0`, `b12=0`, `BCD(b11)≥1` |
| **MD program entry** | `(b3&0x0F)=1`, `b4=0`, `b5.bit7=1` |
| **Radio recording (REC+REMAIN)** | `b1=0x60`, `b2=0x41`, `b3=0xB4`, `b4=0`, `b5.bit0=1`, `b5.bit7=0`, `b10=0xAA`, `b12=0` |
| **Sleep timer edit** | `b13=b14=0xAA`; one of: MD play (`(b0&0x7E)=0x0C, b1=0x20, b2=1, (b3&15)=1, b4=b5=0, b6=0x0C, b7=b8=0xFF, b9=0x83, b10=0xAA`), MD stopped (`b1=0, b2=7, b3=1, b6≤31, b7=b8=0xFF, b9=0x83, b10=0xAA`), radio (`(b0&0x7E)=0, b1=b2=0, b3=4, b4=2, b6=0x0A, b7=4, b8=0, b9=0x80, b10=0xAA`) |
| **Timer volume setting** | `b1=0`, `b4=b5=0`, `(b6&0xC0)=0`, `b10=b11=0xAA`, `b12=0x0A`, `b13=b14=0xAA`; `b0&0x7F ∈ {4,12}` (PLAY) or `{2,10}` (REC) |
| **Timer mode selection (blank)** | `(b0&0x70)=0`, `(b0&0x06)≠0x06`, `b1=0`, `b2=7`, `b3=0`, `b4=b5=0`, `b6≤31`, `b7=b8=0xFF`, `b9=0x83`, `b10=b11=0xAA`, `b12=0x0A`, `b13=b14=0xAA` |
| **Timer source selection** | `b1=0`, `b2=7`, `b5=b4=0`, `b3 ∈ {0,4,8}` for REC or `{0,1,2,4,8}` for PLAY, `b10=b11=0xAA`, `b12=0x0A`, `b13=b14=0xAA` |
| **Clock readout** | `b1=0`, `b2=7`, `b4=0`, `b5∈{0,2}`, `b6≤31`, `b10=b11=0xAA`, `b12=0x0A`; date layout `(b0&0x7F)=0, b3=0x41, b7=b8=0xFF, b9=0x83`; timer-date `b0&0x7F∈{0x0C,0x0A}, b3=0x40, b7=b8=b9=0`; standby `b0&0x7F∈{0x0C,0x0A}, b3=0, b7=b8=b9=0` |
| **Clock/timer time entry** | like the clock readout but `b5=2` (colon on); either `b9=0x80`/`b7=b8=0` or retained tracks `b7=b8=0xFF,b9=0x83` |
| **Radio tuner** | `b3=0x04`, band from `b4` (§2.5), `b10=b11=0xAA`, `b12=0x0A`, `b14>>4 ≤ 9` |

If you only need MD playback, you can skip this table: use the `b3` source nibble, `b5.bit0`, and the fields in §2.9.

---

## 3. SPI-2 "TEXT / COMMAND" frame (15 bytes, `SDI2`)

### 3.1 Byte map

| Byte | Content | Status |
|---:|---|---|
| 0 | not decoded | **[?]** |
| 1 | Frame type / command selector | **[O]** |
| 2–13 | 12 payload bytes (12 character cells for text frames) | **[V]** for text |
| 14 | not decoded (possible trailer) | **[?]** |

### 3.2 Text frame **[V]**

A frame is a **text frame** when `b1 ∉ {0x05, 0x06, 0x07}` and it is not a predefined message (§3.3).

- `b2…b13` = 12 character cells, left to right, **one ASCII byte per cell** (`0x20`–`0x7E`).
- Unused trailing cells are padded with **`0xFF`**. Padding appears only at the end of the window.
- Codes `< 0x20` or `> 0x7E` (other than padding) are **[?]** (animation/control codes). Preserve them; do not invent glyphs.
- The window is a **display window, not the whole title**. Long-title scrolling is only partly characterised: the main board sends successive 12-char windows. Just redraw whatever arrives.

Example, title `TEST`: `54 45 53 54 FF FF FF FF FF FF FF FF` in `b2…b13`.

### 3.3 Predefined messages (`b1 == 0x01`) **[O]**

`b2` selects a message; the display renders the text itself.

| `b2` | Message | Other bytes |
|---|---|---|
| `0x84` | "Program On" | `b3…b13 = 0x00` |
| `0x85` | "Program Off" | `b3…b13 = 0x00` |
| `0x1E` | "Blank Disc" | `b3…b6 = 0x00`, `b7…b13 = 0x20` |
| `0x05` | "No Disc" | `b3…b6 = 0x00`, `b7…b13 = 0x20` |

(The strings are labels chosen by the repo author; the original on-screen wording is not captured.)

### 3.4 Parameter commands (non-text) **[O]**

For all of these, `b3…b13 = 0x00` and `b14` is not checked.

| `b1` | Command | `b2` range |
|---|---|---|
| `0x05` | **Volume** | 0–31 |
| `0x06` | **BASS** adjustment | 0–12 (12-position display value, no known dB scale) |
| `0x07` | **TREBLE** adjustment | 0–12 |

---

## 4. Feature → bit quick reference

| Display feature | Where |
|---|---|
| 12-char text | SPI-2 `b2…b13` |
| "Program On/Off", "Blank Disc", "No Disc" | SPI-2 `b1=1`, `b2` code |
| Volume (momentary) | SPI-2 `b1=5`, `b2` (0–31) |
| BASS / TREBLE | SPI-2 `b1=6/7`, `b2` (0–12) |
| Source logo MD/CD/RADIO/LINE | SPI-1 `b3 & 0x0F` (1/2/4/8) |
| Play / Pause | SPI-1 `b1.bit5` / `b1.bit4` |
| Track number | SPI-1 `b11` (BCD) |
| Elapsed (or remain) time | SPI-1 `b13:b14` (mm:ss BCD) |
| Track indicators 1–20 | SPI-1 `b7`, `b8`, `b9[3:0]` |
| PROGRAM entry | SPI-1 `b5.bit7` |
| SLEEP | SPI-1 `b0.bit0` **[I]**; minutes in `b11/b12` |
| TIMER PLAY / REC | SPI-1 `b0` bits 2 / 1 |
| Clock hh:mm and colon | SPI-1 `b13`, `b14`, `b5.bit1` |
| FM/AM frequency, PRESET | SPI-1 `b3=4`, `b4`, `b6.bit6`, `b13`, `b14` |
| REMAIN / REC icons, position bar, other icons | partly in `b3` bits 4/7, `b9` bits 4–7 **[?]** |

---

## 5. Building a replacement display: practical guidance

1. **Receiver:** 5 signals suffice (CS1, SCK1, SDT1, CS2, SCK2, SDI2 + GND; leave SDO2 open, as the reference ESP32-C3 does; it uses GPIO ISRs on the rising SCK edge, not hardware SPI, because hardware SPI reported wrong lengths).
2. **Check voltage levels first.** The Sony high level has not been measured by the repo author; use level shifting if your MCU is 3.3 V and the lines are 5 V.
3. **Frame validation:** length (128/120 bits), BCD validity, seconds/minutes ≤ 59, hour ≤ 23, track ≥ 1. No checksum exists.
4. **Persistence:** keep the last valid complete frame; timeout after 1.5 s.
5. **Minimum viable MD screen:** text (SPI-2 `b2…b13`), track (`b11`), time (`b13:b14`), indicators (`b7:b9`), source (`b3&15`), play/pause (`b1`).
6. **Blink handling:** blinking elements (sleep minutes, clock colon, timer fields) are encoded by alternating frames (e.g. blank digits `0xAA`), so the receiver should render each frame as received and not add its own blink.
7. **Unknown bits:** pass them through to your own debug output so gaps can be closed with further captures.

## 6. Known gaps (not defined by the source material)

- SPI-1 byte 15; SPI-2 bytes 0 and 14; any checksum.
- SPI-1 `b0` bits 3 and 7, `b3` bits 4/6/7, `b9` bits 4–7, `b2` value meanings, `b6` bit 5 outside timer-volume.
- Remaining/total time, minutes beyond 02 in long captures, track numbers above 20, tuner/CD time fields beyond the signatures above.
- Control/animation text codes, full long-title scroll behaviour, and what `SDO2` carries.
- CD, edit, record-menu and other modes beyond those listed in §2.10.

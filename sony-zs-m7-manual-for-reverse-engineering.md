# Sony ZS-M7 Personal MiniDisc System: Operating Instructions
## Structured transcription for display reverse engineering (humans and agents)

**Source:** `ZS-M7.pdf` (Sony Operating Instructions, 3-861-445-11, (c) 1998). This is a re-organised, condensed transcription of the pages in the supplied scan, not a verbatim copy. Quoted display strings are exact as printed in the manual.

**Purpose:** list everything the front display can show (text, numbers, indicators, messages) and which button or state causes it, so each can be matched to bits on the display bus described in `sony-zs-m7-display-protocol.md`.

## 0. Coverage and gaps of the supplied scan

| Item | Status |
|---|---|
| Printed pages present | 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 41, 42, 43, 44, 45, 46, 47 (page 5/7 etc. partly cropped; page numbers from the footer, some digits are cut off) |
| Not in the scan | pages 1-3, 6 (steps 1-3 of "Recording a whole CD"), 40 (blank scan), **48-66**: sleep timer (48), setup (49-53), **audio emphasis / BASS-TREBLE (54)**, precautions, system limitations (57), "Getting to know the MD" (59), troubleshooting (61), **error messages (64)**, specifications, index |
| Consequence | The **message list on page 64** and the **sleep timer and bass/treble pages** would be the most useful for decoding `b1=0x01` message codes and `b1=0x06/0x07`. Please supply them if available. |

Legend for the **Protocol link** columns: `known` = already decoded in the protocol spec; `candidate` = a plausible match I suggest testing; `unknown` = nothing decoded yet.

---

## 1. Device overview

- **Sources:** MD, CD, RADIO (FM and AM), LINE (external input).
- **Radio:** 12 presets per band, 24 in total. Station labels up to 12 characters.
- **MD:** play, record (from CD, radio or line), edit (erase, divide, combine, move, name). More than 20 tracks on a disc show "OVER 20"; MDs can hold 100 or more tracks (track number can have 3 digits).
- **Timer:** clock (12-hour system), TIMER PLAY (wake up to music), TIMER REC (timer recording), SLEEP.
- **Audio emphasis:** BASS/TREBLE adjustment (page 54, not in scan).
- Remote control exists with number buttons, `>10`, `0/10`, MODE, SLEEP, BASS/TREBLE, a second tone button (label cut off), VOL +/-, transport keys for MD and CD, RADIO BAND and TUNE +/-.

### 1.1 Front-panel controls named in the manual

| Control | Where used |
|---|---|
| POWER | all (direct power-on by pressing a source key) |
| CD OPEN/CLOSE (eject symbol), MD EJECT | tray / disc |
| CD play/pause, CD stop, MD play/pause, MD stop | transport |
| Jog dial (turn = select, also next/previous track), ENTER/YES, CANCEL/NO | selection |
| DISPLAY | cycles display content (see section 4) |
| EDIT | edit menu; held 2 s for station label |
| arrow-left, arrow-right (EDIT arrow) , DELETE, INSERT | text entry |
| SHUF/PGM, REPEAT | play modes |
| BAND, TUNE +/-, AUTO PRESET, MONO/ST | radio |
| REC, DISC ALL, TO TOP, TO END | recording |
| CLOCK, TIMER, STANDBY | clock and timers |
| VOLUME +/- | volume |

---

## 2. Display elements inventory

Derived from the display drawings in the manual. Names in quotes are printed text; others are my descriptions.

| Element | Description | Protocol link |
|---|---|---|
| **Text window** | 12-character alphanumeric area. Shows disc name, track name, station label, menu and prompt messages (section 3). Labels can be up to 100 characters (MD) or 12 (station), so long text must scroll or be sent in windows. | known: SPI-2 b2..b13 (ASCII, 0xFF padding) |
| **Source indication** | Boxed "MD" or "CD" mark next to a disc symbol; "FM" circle and "AM" for radio; "LINE" for line input | known: SPI-1 b3 low nibble (1 MD, 2 CD, 4 RADIO, 8 LINE) |
| **Track number** | Digits under the label "TRACK". **Three digits** are possible (examples show `001`, `004`; the manual describes MDs with 100+ tracks and `>10` pressed twice). | known: SPI-1 b11 (BCD) and b12 (hundreds) |
| **Time digits** | mm:ss style. Shows playing time, remaining time, total time or recording remaining time ("REC REMAIN" small labels). Also shows clock time and timer time. | known: SPI-1 b13, b14 |
| **Position pointer** | Drawn **in the 12-cell dot-matrix text row** as a bar graph (the glass has the printed scale `POSITION 0% ... 100%` directly under those cells, see section 2.1). Shows where on the track the player is playing, and the recording position on the MD. | unknown: probably special glyph codes in SPI-2 `b2..b13` |
| **Music calendar** | Row of 20 small boxes numbered 1-20. Shows tracks present; played tracks disappear during play. Programmed tracks appear here in program mode. "OVER 20" for longer discs. | known: SPI-1 b7..b9 (20 bits) |
| **STEP / playing order** | Small "STEP" label with a number: order within the program | unknown |
| **PGM** | "PGM" indicator (program play) | unknown |
| **SHUFFLE** | "SHUFFLE" indicator | unknown |
| **REPEAT, REPEAT 1** | "REPEAT" and "REPEAT 1" indicators; combinations "REPEAT SHUFFLE" and "REPEAT PGM" | unknown |
| **TOC EDIT** | Boxed "TOC EDIT" indicator. **Flashes after recording or editing stops**, until the table of contents is written. Do not remove power while flashing. | unknown (candidate: SPI-1 b0, b3 high bits or b9 high bits) |
| **TIMER PLAY / TIMER REC** | Indicators for timer standby. Flash while selecting, light steadily in standby. A clock symbol is lit while in timer mode. | candidate: SPI-1 b0 bits 1/2 (REC/PLAY) |
| **SLEEP** | Sleep timer indicator | candidate: SPI-1 b0 bit 0 |
| **STEREO / MONO** | "STEREO" indicator for FM stereo; "MONO" when forced mono | unknown |
| **FM / AM, PRESET** | Band indicators and preset marker | known: SPI-1 b4 |
| **Clock/date** | "98y 2m 14d" style date and a time; separate `AM` and `PM` labels (shared `AM` segment with the radio band), `DATE` label; the clock is a **12-hour system** | partly known: see sections 2.1 and 6 |
| **VOL bar** | "VOL" with a block bar and a flashing number in timer volume setting | partly known: SPI-1 b6, SPI-2 b1=0x05 |


### 2.1 Photo-verified glass layout (from the supplied display photo `IMG_6467.jpeg`)

The photo shows the unlit VFD, so every segment group printed on the glass is visible. Inventory, left to right, top to bottom. "Separate" means the photo suggests an independently drivable segment.

| Area | Elements on the glass |
|---|---|
| Source column (left) | Boxed `MD`, `CD`, `RADIO`, `LINE` (four separate source boxes) |
| Transport icons | `MD`: play arrow and pause bars, plus an MD disc symbol. `CD`: play arrow and pause bars, plus a CD disc symbol. A bent arrow from the CD disc to the MD disc (record direction) |
| Record / edit | `DIGITAL` (above) with a boxed `REC`; boxed `TOC EDIT` |
| Play-mode words | `REPEAT` with `1`, `SHUFFLE`, `PGM` |
| Radio | `STEREO`, `MONO` (yellow), `AUTO` / `PRESET` stacked label, `FM`, `AM`, `MHz`, `kHz` |
| Timer / clock | Clock-face symbol; boxed `TIMER` containing `REC` and `PLAY`; `SLEEP`; `AM`, `PM` |
| Text row | **12** dot-matrix character cells (5x7 style). Printed scale below them: `POSITION`, `0%` under cell 2, small triangles under cells 4, 6, 8, 10, `100%` under cell 12 |
| Seven-segment digits (as I read the photo) | **2 digits** above the label `STEP`; **3 digits** at the label `TRACK`; a small partial digit near a `TV` label (probably the leading "1" for frequencies); **4 digits with a colon** over the labels `REC`, `DATE`, `REMAIN` |
| Track calendar | Boxed numbers `1` to `20`, then `OVER 20` |

Confirmations of earlier assumptions:

- **12-character text window:** confirmed (12 dot-matrix cells).
- **3-digit track number:** confirmed (3 digits under `TRACK`).
- **20 track boxes plus `OVER 20`:** confirmed.
- **12-hour clock:** confirmed by the `AM` and `PM` labels.
- **Four source boxes:** confirmed (matches the one-hot nibble in SPI-1 `b3`).

New elements the manual text did not describe: `DIGITAL REC`, `DATE`, `REC`, `REMAIN`, `AM`, `PM`, `TV`, `MHz`, `kHz`, separate `STEP` digits, `POSITION 0%..100%` scale, separate play/pause icons for MD and CD, record-direction arrow.

---

## 3. Message catalog

Every on-screen message in the supplied pages. `Context` is the situation. `Code` is blank where nothing has been captured yet.

| Message (as printed) | Context | Page | Bus code seen | Notes |
|---|---|---|---|---|
| `TOC Reading` | After inserting an MD, before the disc name | 8 | - | |
| `SELECTION` | Example **disc name** shown while stopped (also on page 13) | 8, 13 | - | example label from the manual figure |
| `LOVE SONG` | Example **track name** while playing | 8 | - | example label |
| `OVER 20` | Disc has more than 20 tracks | 12, 13 | - | |
| `PGM` | Program mode on | 17 | - | |
| `PGM Check` | Press ENTER/YES before starting program play to check order | 18 | - | |
| `SHUFFLE` | Shuffle play | 19 | - | |
| `REPEAT 1`, `REPEAT`, `REPEAT SHUFFLE`, `REPEAT PGM` | Repeat modes | 16 | - | |
| `AUTO PRESET` | Flashes while auto-presetting radio stations | 20 | - | |
| `CD>MD OK?` | Not enough room on MD for whole CD; **alternates with the time display** | 7, 25, 28 | - | alternating display: expect blink-style alternation |
| `Protected` | MD record-protect tab is in the protected position | 31 | - | |
| `Track Erase` | EDIT menu item while a track is playing | 31 | - | |
| `All Erase` | EDIT menu item while stopped | 32 | - | candidate for `b2=0x7D/0x7E` |
| `Erase OK?` | Confirmation after ENTER/YES on an erase item | 31, 32 | - | one-track repeat play starts for Track Erase |
| `Complete` | Edit finished; shown a few seconds | 31, 35, 37, 38 | - | |
| `Blank Disc` | After "All Erase", or an empty MD | 32 | `b1=1,b2=0x1E` | known |
| `Divide` | EDIT menu item (player paused in a track) | 34 | - | |
| `Divide OK?` | Confirmation | 34 | - | |
| `Sorry` | Cannot divide or combine (system limit) | 34, 36 | - | |
| `Combine` | EDIT menu item | 36 | - | |
| `Combine OK?` | Confirmation; player enters pause | 36 | - | |
| `Cannot Edit` | Combine attempted on the first track | 36 | - | |
| `Track Move` | EDIT menu item | 38 | - | |
| arrow + `Track 003?` | Move destination prompt (the arrow glyph precedes "Track"); jog dial changes the number | 38 | - | contains a non-ASCII arrow glyph |
| `Disc Name` | EDIT menu item (stopped MD): name entry | 39 | - | candidate for `b2=0x7D/0x7E` |
| `MegaBass On` / `MegaBass Off` | (not in this scan) | - | `b1=1,b2=0x84/0x85` | known from repo |
| `No Disc` | (not in this scan) | - | `b1=1,b2=0x05` | known from repo |
| `- -y - -m - -d`, `- -:- -` | Clock not yet set | 42 | - | blank date/time placeholders |
| `TIMER PLAY`, `TIMER REC` | Timer type selection (flash) | 44, 46 | - | |
| `MD`, `CD`, `RADIO`, `LINE` | Timer source selection (flash) | 44, 46 | - | source logos |
| `On` / `Off` style time prompts | Timer start/stop time entry (shown as "On" and hour/minute) | 45, 46 | - | the "Off" text is inferred from step 6; only "On" is clearly printed |
| `VOL` | Timer volume setting | 45, 47 | - | block bar plus flashing number |

### 3.1 EDIT menu order (what repeated EDIT presses cycle through)

The manual names these items but does not give the full cycle order. What it says:

| Player state | Items reached with EDIT (page) |
|---|---|
| Playing a track | `Track Erase` (31), `Track Move` (38), `Combine` (36) |
| Paused inside a track | `Divide` (34) |
| Stopped | `All Erase` (32), `Disc Name` (39) |
| Playing (naming a track) | track name entry (39-41) |
| EDIT held 2 s on a station | station label entry (21) |
| During recording | EDIT starts track-name entry (23) |

The capture the user made (EDIT pressed with a stopped or inserted disc, message codes `0x7D` and `0x7E`) is consistent with two items from the stopped list, most likely `All Erase` and `Disc Name`. This is a **hypothesis**: confirm by photographing the display at each press.

---

## 4. DISPLAY button behaviour (what changes in the text window)

### 4.1 CD, stopped
Total track count and total playing time; music calendar; position pointer; "OVER 20" if more than 20 tracks.

### 4.2 CD, playing (each press of DISPLAY)
| Presses | Shows |
|---|---|
| 1 | current track number and remaining time of current track |
| 2 | number of tracks left and remaining time on the CD |
| 3 | current track number and playing (elapsed) time |

During repeat, program or shuffle play the "tracks left / remaining time on the CD" display is not available.

### 4.3 MD, stopped
Text window shows the **disc name** if the MD was labelled (otherwise blank or default); track count and total playing time. DISPLAY also shows the remaining **recording** time on the MD and the position pointer.

### 4.4 MD, playing (each press of DISPLAY)
| Presses | Shows |
|---|---|
| 1 | current track number, remaining time of current track, position pointer |
| 2 | number of tracks left, remaining time on the MD, and the **disc name** in the text window |
| 3 | **recording date and time** (needs the clock to be set; stamped at recording) |
| 4 | current track number, playing time, and the **track name** in the text window (the default view) |

The text window therefore changes between: track name, disc name, date/time. A capture should record which one is shown.

---

## 5. Feature walkthroughs with expected display states

Each block is a sequence to reproduce while logging both display buses. Page numbers refer to the manual.

### 5.1 Playing a CD (p4-5)
1. Press CD OPEN/CLOSE (direct power-on), place disc, press CD play.
2. Tray closes. Display: CD indication, position pointer, track number (`001`), elapsed time. All tracks play once.
Controls: volume, stop, pause, next/previous via jog dial (clockwise = next), eject, POWER.

### 5.2 Recording a whole CD, "Synchronized recording" (p7, p25)
Step 4 only is present: press DISC ALL. Display shows position pointer (CD playing position and MD recording position), MD track number and **remaining recording time of the MD** ("REC REMAIN"). `TOC EDIT` flashes after stopping. If `CD>MD OK?` alternates with the time display the MD is too small; ENTER/YES records as much as fits, CANCEL/NO stops.

### 5.3 Playing an MD (p8-9)
1. Insert MD (direct power-on). `TOC Reading` appears, then the disc name (if labelled) with the total track number and total time.
2. Press MD play: track number, playing time, and track name (if labelled).
Controls as for CD; MD EJECT; DISPLAY (section 4.4).

### 5.4 Radio (p10-11, p20-22)
- BAND selects FM, AM; display shows "FM" or "AM" and the frequency (example `89.2`, with FM marker). "STEREO" indicator for stereo FM. Hold TUNE +/- to auto-scan.
- MONO/ST toggles "MONO".
- **AUTO PRESET** (hold): "AUTO PRESET" flashes, stations stored from the low end, preset 1 first. 12 per band.
- Manual preset: tune with TUNE +/-, hold ENTER/YES 2 s, choose preset number with the jog dial, press ENTER/YES.
- Play preset: BAND then jog dial shows preset numbers. On the remote: number buttons (`0/10` for 10, `>10` then 1 or 2 for 11, 12).
- **Label a station** (p21): tune in, hold EDIT 2 s, enter up to 12 characters (arrow keys move cursor, jog dial selects character, DELETE, INSERT), press ENTER/YES.

### 5.5 Locating tracks (p15)
Number buttons choose a track directly. `>10` first for tracks above 10 (`>10` twice for 100 or more). Not possible while "REPEAT PGM", "REPEAT SHUFFLE", "PGM" or "SHUFFLE" is lit.

### 5.6 Repeat, program, shuffle (p16-19)
- **Repeat:** REPEAT cycles "REPEAT 1" -> "REPEAT" (all) and, if shuffle/program is active, "REPEAT SHUFFLE" / "REPEAT PGM".
- **Program:** stopped, SHUF/PGM until "PGM"; jog dial selects track, ENTER/YES stores (up to 20). Programmed tracks show in the music calendar; STEP (order) and total programmed time are shown. ENTER/YES before play shows "PGM Check" and steps through the order. CANCEL/NO erases the last entry.
- **Shuffle:** SHUF/PGM until "SHUFFLE".

### 5.7 Recording (p23-28)
- **REC IT:** press TO TOP or TO END while a CD plays; the track is recorded from its start.
- Naming during recording: EDIT, DISPLAY to choose character type, jog dial, arrow, repeat, ENTER/YES.
- **Programmed recording:** stopped CD, SHUF/PGM -> "PGM", choose tracks, DISC ALL.
- **Recording from radio or CD:** REC puts the player in recording standby; MD play/pause starts it; MD stop ends it; MD play/pause pauses (adds a track mark). `TOC EDIT` flashes after stop.
- Recording on a recorded MD: appends from the last recorded position; "record over from a specific point" is possible after a pause.

### 5.8 Editing (p29-41)
Track marks sit between tracks. The edit functions all end with "Complete" shown for a few seconds, then `TOC EDIT` flashes while the table of contents is written.

| Function | Steps and displayed messages |
|---|---|
| Erase single track | play track; EDIT until `Track Erase`; ENTER/YES; `Erase OK?` (one-track repeat play); ENTER/YES; `Complete`. If `Protected` appears the disc tab is in the protect position. |
| Erase all | stopped; EDIT until `All Erase`; ENTER/YES; `Erase OK?`; ENTER/YES; `TOC EDIT` disappears; `Blank Disc` |
| Erase part of a track | combination of Divide, Erase, Combine |
| Divide | play, pause at the point; EDIT until `Divide`; ENTER/YES; `Divide OK?`; ENTER/YES; `Complete` (may show `Sorry`) |
| Combine | play the later track of the pair; EDIT until `Combine`; ENTER/YES; `Combine OK?` (pause mode); ENTER/YES; `Complete` (may show `Sorry` or `Cannot Edit`) |
| Move | play the track; EDIT until `Track Move`; ENTER/YES; arrow + `Track 003?`; jog dial selects new position; ENTER/YES; `Complete` |
| Name (disc) | stopped; EDIT until `Disc Name`; ENTER/YES; DISPLAY selects character type; jog dial selects character; arrow moves cursor; ENTER/YES |

### 5.9 Character set for names (p39, p41)
Each label: up to 100 characters; about 1,700 per MD.

| Type | Characters (as printed) |
|---|---|
| Uppercase | `ABCD...WXYZ` `'` `,` `/` `:` and space |
| Lowercase | `abcd...wxyz` `'` `,` `/` `:` and space |
| Numbers and symbols | `0123456789` `!` `"` `#` `$` `%` `&` `(` `)` `*` `.` `;` `<` `=` `>` `?` `@` `_` `` ` `` `+` `-` `'` `,` `/` `:` and space |

DISPLAY cycles the type: uppercase -> lowercase -> numbers and symbols. This is consistent with a plain ASCII text window (0x20 to 0x7E). A character such as the move-arrow in `Track Move` is outside this set.

### 5.10 Clock (p42-43)
- Until set, the display shows `- -y - -m - -d` and `- -:- -`.
- Hold CLOCK until the year digits flash; jog dial and ENTER/YES for year, month, day, hour, minute. After the minutes, ENTER/YES starts the clock from 00 seconds.
- Example display: `98y 2m 14d` with a time such as `3:15`. The **clock is a 12-hour system** (an AM/PM marker is shown to the left of the time).
- Pressing CLOCK shows the time; it also works with the power off (standby clock).

### 5.11 Timer play and timer record (p44-47)
Preconditions: clock set and neither "TIMER PLAY" nor "TIMER REC" displayed (press STANDBY to clear).
1. Prepare the source (CD, MD, radio station, or LINE equipment).
2. Press TIMER: "TIMER PLAY" or "TIMER REC" flashes.
3. Jog dial to the wanted one; ENTER/YES. The source (`MD`, `CD`, `RADIO`, `LINE`; for REC only RADIO or LINE) flashes: jog dial then ENTER/YES.
4. Set start hour and minute ("On" prompt), then stop hour and minute.
5. Set the volume ("VOL" bar with a flashing number) with the jog dial; ENTER/YES.
6. Press STANDBY: the timer indicator lights and the clock symbol is on.
To check settings: TIMER then ENTER/YES repeatedly steps through stored settings. To cancel: STANDBY.

---

## 6. Observations linking the manual to the bus protocol

These are suggestions for the protocol spec and new captures, not facts.

1. **Three-digit track number:** the manual shows `001`/`004`-style numbers and 100+ tracks, so the track field is probably three BCD digits across `b11` and `b12` (hundreds in the low nibble of `b12`), matching what the sleep decoder does with `b12` for 100 minutes.
2. **12-hour clock:** confirmed by the separate `AM` and `PM` labels on the glass. The repo decoder accepts hours 0-23 in `b13`; the clock-setting screens show `b0` = `0x00` or `0x80`, so **`b0` bit 7 is a good PM candidate** (the AM segment may be derived from it or sit in another bit).
3. **Date display (`98y 2m 14d`):** year, month and day need three more numeric fields. The layouts with `b3 = 0x40/0x41` in the repo's clock decoder are candidates for the date screen. No decode exists yet.
4. **DISPLAY button views:** the text window content (track name, disc name, date) and the numeric fields (elapsed vs remaining vs tracks left) change per DISPLAY press. Record which view is active when capturing, and compare the `REMAIN`/`0x91` flag in SPI-1 `b3` bits 4 and 7 with the "remaining time" views.
5. **Alternating messages:** "CD>MD OK?" alternates with the time display. That is the same kind of alternation seen with `0x7D`/`0x7E` message codes. When both buses are captured together, look for `b1=0x01` frames interleaved with normal text frames.
6. **TOC EDIT flashing:** a dedicated indicator bit with a blink pattern; best captured after stopping a recording (or after Complete). Candidate bits: SPI-1 `b0` bits 3 and 7, `b3` bits 4/6/7, `b9` bits 4-6.
7. **Indicators with no decoded bit yet:** PGM, SHUFFLE, REPEAT, REPEAT 1, STEREO, MONO, STEP, REC/REMAIN labels, position pointer segments, TIMER PLAY/REC and SLEEP (partly). Each can be isolated by toggling it on the player and diffing consecutive SPI-1 frames.
8. **Message candidates to test:** produce each manual message once and record the `b2` value of the SPI-2 `b1 = 0x01` frame: `Track Erase`, `All Erase`, `Disc Name`, `Erase OK?`, `Complete`, `Divide`, `Divide OK?`, `Combine`, `Combine OK?`, `Cannot Edit`, `Sorry`, `Track Move`, `Protected`, `TOC Reading`, `PGM Check`, `CD>MD OK?`.

### 6.1 Suggested capture plan (both buses at the same time)

| # | Action (start capture first) | What to log |
|---|---|---|
| 1 | Insert labelled MD, wait for `TOC Reading` | text window, `b1` values, SPI-1 flags |
| 2 | Press DISPLAY 1-4 times while playing | per-press text/numeric fields |
| 3 | EDIT presses, stopped, one press at a time, photo each | `b2` for each menu item |
| 4 | EDIT presses, playing | `Track Erase`, `Track Move`, `Combine` codes |
| 5 | Toggle PGM, SHUFFLE, REPEAT cycle | SPI-1 flag bits |
| 6 | Radio: FM/AM, MONO, STEREO, PRESET | SPI-1 b3, b4, b6 bits |
| 7 | Record a short piece, stop, watch `TOC EDIT` | SPI-1 blink bit |
| 8 | Set the clock through year, month, day, hour, minute | date fields and AM/PM bit |
| 9 | TIMER PLAY and TIMER REC setup | timer bits and source blink |

---

## 7. Machine-readable appendix

```yaml
device: Sony ZS-M7
manual: ZS-M7.pdf (1998, 3-861-445-11)
pages_present: [4,5,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,41,42,43,44,45,46,47]
pages_missing_important: [6, 40, 48, 54, 57, 59, 61, 64]
display:
  text_window: {chars: 12, label_max_md: 100, label_max_station: 12}
  track_number_digits: 3
  music_calendar_slots: 20
  clock_system: 12-hour
sources: [MD, CD, RADIO, LINE]
messages:
  - {text: "TOC Reading", context: "MD inserted", page: 8}
  - {text: "OVER 20", context: ">20 tracks", page: 12}
  - {text: "PGM", context: "program mode", page: 17}
  - {text: "PGM Check", context: "check program order", page: 18}
  - {text: "SHUFFLE", context: "shuffle mode", page: 19}
  - {text: "REPEAT 1", context: "repeat single", page: 16}
  - {text: "REPEAT", context: "repeat all", page: 16}
  - {text: "REPEAT SHUFFLE", context: "repeat shuffle", page: 16}
  - {text: "REPEAT PGM", context: "repeat program", page: 16}
  - {text: "AUTO PRESET", context: "flashes while presetting", page: 20}
  - {text: "CD>MD OK?", context: "MD too small; alternates with time", page: 25}
  - {text: "Protected", context: "record-protect tab", page: 31}
  - {text: "Track Erase", context: "EDIT menu while playing", page: 31}
  - {text: "All Erase", context: "EDIT menu while stopped", page: 32}
  - {text: "Erase OK?", context: "erase confirm", page: 31}
  - {text: "Complete", context: "edit done", page: 31}
  - {text: "Blank Disc", context: "after All Erase / empty MD", page: 32, bus: {b1: 1, b2: 0x1E}}
  - {text: "Divide", context: "EDIT menu paused", page: 34}
  - {text: "Divide OK?", context: "divide confirm", page: 34}
  - {text: "Sorry", context: "cannot divide/combine", page: 34}
  - {text: "Combine", context: "EDIT menu", page: 36}
  - {text: "Combine OK?", context: "combine confirm", page: 36}
  - {text: "Cannot Edit", context: "combine on first track", page: 36}
  - {text: "Track Move", context: "EDIT menu", page: 38}
  - {text: "->Track 003?", context: "move destination prompt (arrow glyph)", page: 38}
  - {text: "Disc Name", context: "EDIT menu stopped", page: 39}
  - {text: "MegaBass On", bus: {b1: 1, b2: 0x84}, source: repo}
  - {text: "MegaBass Off", bus: {b1: 1, b2: 0x85}, source: repo}
  - {text: "No Disc", bus: {b1: 1, b2: 0x05}, source: repo}
indicators:
  - {name: "TOC EDIT", behavior: "flashes after recording/editing stop"}
  - {name: "TIMER PLAY", behavior: "flashes during setup, solid in standby"}
  - {name: "TIMER REC", behavior: "flashes during setup, solid in standby"}
  - {name: "PGM"}
  - {name: "SHUFFLE"}
  - {name: "REPEAT"}
  - {name: "REPEAT 1"}
  - {name: "STEP", meaning: "program playing order"}
  - {name: "STEREO"}
  - {name: "MONO"}
  - {name: "FM"}
  - {name: "AM"}
  - {name: "REC"}
  - {name: "REMAIN"}
  - {name: "SLEEP", page: 48, in_scan: false}
display_button_cycle:
  cd_playing: [track+remaining_track, tracks_left+remaining_cd, track+elapsed]
  md_playing: [track+remaining_track+pointer, tracks_left+remaining_md+disc_name, rec_date_time, track+elapsed+track_name]
edit_menu:
  stopped: ["All Erase", "Disc Name"]
  playing: ["Track Erase", "Track Move", "Combine"]
  paused: ["Divide"]
```

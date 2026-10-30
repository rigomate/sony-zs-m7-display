# Handoff: Sony ZS-M7 replacement display

## Stopped digits / track21 time / inferred OVER20

User explicitly requested remote play and track21 tests. captures/track21 has
stopped baseline, play, >10+2+0 selecting20, >10+2+1 selecting21, and later
name display. Commands acknowledged and actual bus frames confirmed selection.
No STOP sent; it was observed stopped before the name command (cause not recorded).
Stopped selected01 hasAA/AA time, later stopped summary21/56:24; playback21
has advancing time. Fixed <=20 decoder limit, now lowBCD1..99 with byte12zero.
New track_display separates track/calendar from time; stopped blank time no
longer hides digits. Program selection remains independent.
OVER20 is inferred byte5 mask04, shown dashed with unconfirmed tooltip/APIflag.
User could not see it. Earlier byte9 mask40 candidate is ruled out by the
20-track normal summary having it set. Current21-disc flag04 stays set at1,
20,21 and stopped. Final post-flash remote play/>10,2,1 verification showed track21 and advancing
01:01..01:03; Chromium rendered21/01:04. Player left playing21 at last check.
See analysis/track21-findings.md for proof and limits.

## Program On/Off messages resolved

User-labelled program-on.sal contains23 complete mode01/code84 messages,
confirming Program On. Earlier paired84/85 recording had confirmed ON/OFF
endings, so85 is the OFF member (prefix inferred from the paired ON wording).
Firmware reconstructs Program On/Off for exact zero-parameter mode01 packets;
API message preserves wire mode/code, title contains printable reconstructed
text. Numeric program selection and PGM state remain independent. Native and
browser checks cover messages and ASCII return. No IR sent.

## Earlier ON/OFF investigation (superseded)

program-on_program_off.sal has130 complete frames per bus. SPI-2 mode01,
byte2 alternates84/85, remaining parameters zero. No literal ASCII strings.
User confirmed only endings ON/OFF; prefix Program is uncertain. Which code
means ON is also unconfirmed. Numeric PGM/STEP/REPEAT remain active, so do not
infer global program-enabled state. No firmware strings were assigned.
Need a one-message visually labelled ON capture or synchronized visual/code
reference, then readable prefix. See analysis/program-message-findings.md.

## Program selected-track number

User capture program-track-01-to-20-to-01 actually walks all1..20 then1.
590 numeric frames: byte11 packedBCD selected track, byte12=00 when visible;
AA/0A blanks selection (58 frames). New program object separates this from
playback numeric fields; UI MD PROGRAM, Program track, three-digit formatting,
no elapsed time. Track>99/hundreds and numeric step index are not decoded.
Native replay and browser cases cover selection/blank/exit. No IR sent.
See analysis/program-track-findings.md and latest validation entry.

## STEP and TRACK labels mapped

User confirmed STEP and TRACK steady with flashing PGM; pgm-step-track passive
capture has188 numeric frames, byte5=83/8B. STEP mask80 and TRACK mask01 are
supported by this positive reference and earlier normal/date/source-only layouts.
API symbols.step/track drive labels independently of PGM mask08. No program-step
numeric decoder was added. Native and browser checks cover both PGM phases and
cleared label states. See analysis/pgm-findings.md for reference limitations.

## PGM blink mapped

User-labelled pgm-blinking capture has253 complete numeric frames: byte5=83
or8B, with~510ms phases. Mask08 is PGM illumination; the later STEP/TRACK reference assigns80 to STEP. API symbols.pgm and UI follow received phases
without a local timer, independent of SHUFFLE/REPEAT bits. Native replay and
browser lit/dark checks added. No IR command sent. See analysis/pgm-findings.md.

## SHUFFLE mapped

User-labelled shuffle-on capture has188 complete numeric frames, byte5=13.
Controls match the earlier normal/repeat captures aside from mode bits:10 is
SHUFFLE,40 REPEAT,20 repeat-one. API symbols.shuffle and UI are independent;
captured mode replay and browser on/off checks added. No IR command sent.
See analysis/shuffle-findings.md and latest physical validation entry.

## REPEAT / REPEAT 1 mapped

User-labelled repeat-one and repeat-all captures differ only in numeric byte5
among control bytes1–10:63 versus43. Earlier normal baseline03 supplies off.
Byte5 mask40=REPEAT,20=1 modifier. API symbols.repeat/repeat_one and UI visuals
are independent. Captured/native and browser three-state checks pass. Evidence
and baseline distinction: analysis/repeat-findings.md. No IR commands sent.

## New remote controls / FM visual (2026-10-30)

User explicitly authorised trying new remote features to investigate radio and
clock indicators. Tested radio_band100/111, tune_up100/115, tune_down100/116,
and date68/99 (SONY15), with captures/remote-indicators before/after evidence.
FM87.50->MW531->MW540->LW153->DATE unset->FM87.50->87.55->87.50.
Player leftFM87.50; MW tuning changed to540, presets not edited. No stop/eject/
record/power/timer-setting command was sent. Current hardware runner has a
restricted whitelist; do not import/run it just to inspect files.
FM observed byte4=C0 versus20 onMW/LW. FM/MHz bits co-vary, not individually
isolated. API symbols.fm and radio.display_band, plus FM/MW/LW custom glyph
rendering, are added. ClockAM/PM remain unmapped. User's DATE view currently
shows an unset clock; European manual says24h versusUS12h. Do not set AM from
AM-radio mode. See analysis/remote-radio-indicators.md for limits and evidence.

## DATE indicator confirmed

Paired user-labelled captures/manual/date-on and date-off isolate numeric
byte3 bit6(40):41 with MD+DATE,01 with MD only. API symbols.date and the separate
DATE visual now follow that flag; the adjacent REC prefix remains unmapped.
On-state text is26y10m29d (spaced) and numeric time21/04, but parsing a new
clock/date numeric mode is outside this change. Source masking stays low4bits.
See analysis/date-findings.md and physical validation entry.

## TIMER/PLAY blinking pair

User-labelled timer-play-blinking.sal isolates byte0 mask0C. Values04/08
occur for one frame at edges;00/0C are main dark/lit phases. Individual TIMER
versus PLAY bit roles remain unisolated. API symbols.timer_play_raw preserves
the mask and timer_play is either bit set; the UI follows that observed pair
without an independent blink timer. Timer REC and clock remain unmapped.
See analysis/timer-play-findings.md and latest validation entry.

## SLEEP visual decoded

User-confirmed paired captures/manual/sleep-steady-on and sleep-off isolate
byte0 bit0 as a steady positive SLEEP flag (01/81 on,00/80 off). Byte9 bit7
changed inversely in the first pair, but the user-supplied sleepisblinking.sal
keeps it fixed across both blink phases, ruling it out as the illumination bit. API symbols.sleep and
steady UI visual are implemented. The UI follows the received bit0 directly, including blinking; no CSS timer.
A32-frame short capture has16 on/16 off frames at~510ms per phase.
See analysis/sleep-findings.md for the paired380-frame reference and tests.

## Independent source logos / missing-MD radio fix

User capture captures/manual/cycle-md-cd-radio-linein.sal isolates numeric
byte3 logos01 MD,02 CD,04 RADIO,08 LINE. Byte2=00 with MD removed across all
sources. Removed erroneous byte2=07 requirement from tuner decoding. Source
and symbols JSON now independent of numeric interpretation, so CD/LINE/empty
MD/RADIO stay correctly identified. MD graphic off hides its SVG completely;
07 solid and matched01 playback animate independently of source labels.
CD numeric playback remains unknown rather than being interpreted as MD.
Native cycle replay and missing-disc FM/AM regression fixtures pass, along with
MD/radio/volume/state checks. Browser checks include all empty source states,
FM with MD graphic off, and RADIO before frequency digits arrive.
See analysis/source-findings.md and latest physical validation entry.

## Volume display implemented

User supplied captures/manual/volume-from-min-to-max.sal. Exported paired
frames establish SDT1 byte6 &1F as binary volume0..31 and bit5(20) as volume
command active. SDI2 byte1=05 switches to parameterized volume mode; byte2 is
the same value, bytes3..13 zero. All371 overlay frame pairs agree exactly,
visiting every value0..31. Firmware now exposes volume level/max/overlay/fresh, separating mode05 commands
from ASCII. The page has a read-only volume meter and VOLUME nn overlay, retaining
the last normal title for restoration. Native sweep and browser min/max/title
restoration checks pass. See the latest physical validation entry for flashing.
See analysis/volume-findings.md and analysis/analyze_volume_sweep.py.

## MD playback/pause animation

User supplied images/md-playback/IMG_6470..6472.jpg and pause.jpg, confirming
opposing wedge pairs animate during playback and all six wedges light in pause.
Passive captures/md-animation/01-playback and02-held-time record both states;
user confirmed the held-time state is pause. Numeric byte1=20 play,30 pause;
byte2=01 animated,07 solid. Source matching remains limited to these patterns.
The UI maps play/pause/disc states and reconstructs a three-pair animation with
approximate1.5s timing (not wire-phase synchronised). Stale data freezes it.
Pause deliberately blanks time bytes13/14 toAA/AA, correlated with byte5=01/03.
LiveDisplay retains the same track's last numeric value while exposing blanking
through the independent md object, so mode does not oscillate to unknown.
Native playback/pause capture replay and browser animation/pause checks pass.
See analysis/md-animation-findings.md for measured controls and limits.

## Original faceplate visuals

The user requested every visual from images/IMG_6467.jpeg on the hosted page.
Named symbol placeholders, disc/clock SVG graphics and the position scale are
added; see analysis/display-symbol-inventory.md. MHz/kHz/PRESET and known source
labels follow decoded state; unknown symbols remain dotted/unmapped rather
than falsely off. The20 numbered indicators remain visible in radio mode with
unknown mapping. No additional protocol bits or player controls are invented.
Browser checks cover unit/PRESET on/off, stale signals and return to MD.

## AM/kHz and PRESET extension

Six user-provided .sal files in captures/radio-manual are exported and analysed
in analysis/radio-findings.md. AM filenames omit a faint leading1, which the
user subsequently confirmed:1287,1323,1422,1575kHz. Byte6 bit6 is the shared
leading1 segment (100MHz or1000kHz). AM byte13 is BCD hundreds/tens; byte14
high nibble units, low nibble A blank. Byte4 patterns20=AM,C0=FM,C2=FM/PRESET.
PRESET pair also changes byte7 bit5; exact individual segment roles unisolated.
Firmware/API/UI support AM band, integer kHz and PRESET label; old FM behavior
retained. Native replay covers all manual references and MD state regression.

## Experimental FM display added

User requested applying the frequency theory to firmware without requiring
arbitrary tuning. Four user-labelled Saleae references under captures/radio:
105.9,103.9,102.5,98.7MHz. Candidate uses byte6 bit6 for100MHz, byte13 packed
BCD for tens/units, byte14 high nibble for tenths. Observed signature and range
guard distinguish this provisional FM layout; other modes remain unverified.
API mode/radio fields and webpage MHz view are implemented; MD track/time and
indicator grid are hidden in radio mode. See analysis/radio-findings.md.

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

Original capture restrictions permitted play, track_prev, track_next, digits
and power_off. On2026-10-30 the user explicitly authorised testing new remote
features for radio/clock indicators; radio_band, tune_up/down and date were used.
Never send stop. Other unrelated commands still require applicable task scope.
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

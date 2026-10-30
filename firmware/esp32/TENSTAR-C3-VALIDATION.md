# Physical Tenstar C3 validation — 2026-10-29

## Installed firmware

Tenstar Robot ESP32-C3 Super Mini, chip rev0.4, 4MB XMC flash,
MAC18:8b:0e:2e:2d:04. Firmware SHA256: `ccfe1e9492d8119ecde0897dc4e6495173751e27594dbce20b87b5a307183acb`.
The ROM uploader verified flash hashes and completed successfully. The RAM
upload stub had crashed; the configured no-stub path avoids that failure.
Wiring and build instructions: [TENSTAR-C3.md](TENSTAR-C3.md).

Both buses use filtered GPIO interrupts, not hardware SPI. The attempted C3
SPI-slave path produced incorrect lengths and is not part of this validation.
The input output-enable masks were zero on both buses. No player command was
sent during these tests.

## Independent wire comparison

`captures/tenstar-c3/04-payload-comparison.sal` and its exported CSV record20s
of Sony traffic at8MS/s while the host polls the physical C3 API every~250ms
and periodically downloads its webpage. The reproducible passive test is
`analysis/validate_c3_live.py`; API samples and summary are beside the capture.

68 API polls were collected. Excluding boundary samples and partial raw frames,
all51 numeric and49 title payloads matched complete frames from the simultaneous
Saleae recording byte-for-byte. No unmatched complete payload was observed.
This samples received frames; it does not prove every received bit is correct.
The API advanced from track3 time4:14 to track4 time0:15 during automatic track
advance. All20 test-disc tracks have title TEST; title transition/control windows
also occur on the wire, so requiring every poll to equal TEST is invalid.

During the~19s API window each bus reported594 frames:

| Bus | Complete | Rejected | Rejected fraction |
|---|---:|---:|---:|
| Numeric |551|43|7.2%|
| Title |557|37|6.2%|

Queue drops did not increase during this test. The lifetime counter already
contained4 earlier drops, which the webpage continues to report. Rejected
partial frames preserve the most recent complete decoded values; freshness
expires after1.5s without another complete frame. Both buses remain live.

The earlier `03-gpio-live` independent12s capture also decoded TEST and track3,
time1:15 through1:27. The first strict polling script stopped at a title ST
transition; this was an invalid constant-title assertion, not evidence by itself
of corrupted reception. The later full-payload comparison handles transitions.

## Webpage and native checks

A headless Chromium loaded the physical C3 webpage and rendered TEST, track03,
1:14 and the corresponding track indicator. Snapshot: `captures/tenstar-c3/dashboard.png`.
That snapshot precedes the20s comparison; its time is not the comparison endpoint.
The host's existing ZS-M7-Display Wi-Fi profile was preserved.

Native fixed-buffer replay recovers captured 0123456789AB and TEST and checks
buffer overflow and edge filtering. Live-display tests verify partial frames
retain the last complete fields and cannot keep stale values fresh. Both scoped
native tests passed after the final capture/state changes. The separate M5
firmware target also built during development; it is disconnected from the Sony.

## Remaining limits

This is a working prototype, not lossless capture. Wi-Fi/interrupt load still
causes incomplete frames. There is no decoded checksum to reject a complete
frame containing a wrong bit. Higher-confidence replacement hardware should
reduce capture loss and validate any checksum before relying on every frame.
Unknown title control codes remain visible through raw diagnostics rather than
being invented as printable characters. Voltage compatibility still requires
measurement; a zero output-enable mask does not establish electrical safety.

## Experimental FM update

The FM-enabled firmware was built and flashed successfully (ROM hash verified).
The physical API reported mode fm, frequency_khz98700, experimental=true,
null MD numeric fields and both buses fresh. Chromium rendered FM RADIO and
98.7MHz with the elapsed-time group hidden. Evidence: captures/radio/04-fm-0987/
flashed-api-state.json and dashboard.png. The lifetime drop count was2 after
this reboot; this spot check does not establish lossless capture.
Four captured FM references and MD compatibility native checks passed.
The source signature and hundreds-bit interpretation remain provisional.

FM firmware SHA256: `a85c6440e8a492d1972bf0379b763d618e3c923abae1444b569d4554be2cb098`.

## AM/kHz and PRESET update

All six user-supplied manual captures replay successfully with AM/FM unit
selection, their frequency fields and PRESET state. The user confirmed that
the four AM filenames omit the faint leading1; decoded values are1287,1323,
1422 and1575kHz. Original filenames and recordings were preserved.

Firmware upload completed successfully with ROM flash hash verification.
The physical API then reported AM1503kHz, preset=false, both buses fresh.
Chromium rendered AM RADIO, Frequency · kHz and1503, with MD fields hidden.
Evidence: captures/radio-manual/flashed-am-api.json and dashboard-am.png.
The live frequency is a different setting from the supplied manual recordings;
its displayed value has not yet been independently confirmed by the user.
PRESET was checked against the supplied off/on recording pair and browser
fixtures; no remote command was sent to toggle the physical player.

AM/PRESET firmware SHA256: `18984d764f074703668a9e80b20c7076f8e4e78c4ebeeaf5c2c1f2e2f32a0cab`.

## Full faceplate visual inventory

Added39 named symbol elements plus the position scale and pointers, based on
images/IMG_6467.jpeg. Units and PRESET follow decoded tuner state; unmapped
symbols have dotted styling and accessible unknown-state labels. Numbered
indicators remain available visually in radio mode without guessing their bits.
Inventory: analysis/display-symbol-inventory.md.

Browser fixtures passed MD, FM/MHz/PRESET-on, AM/kHz/PRESET-off, stale, unknown
text and disconnection states. The C3 was flashed successfully and its physical
page rendered all39 elements with live104.00MHz, PRESET on, MHz glowing and
kHz dim. Screenshot: captures/radio-manual/dashboard-symbols.png.

Symbols firmware SHA256: `c8b16d3dae7ac62fce98539defcabc9ac8bd19195e64c7849494f891abcb76ae`.

## MD animation and pause update

User-confirmed photos and new playback/pause captures establish observed
transport states. Byte1 changes20->30 and byte2 changes01->07. The UI now
shows six solid wedges in pause and reconstructs opposing wedge-pair animation
in playback. The animation's phase/timing are approximate, not wire-decoded.

Captured MD transport replay, existing radio replay and live-state tests passed.
Browser fixtures passed play animation, paused solid wedges, FM/AM/PRESET,
stale and disconnection checks. Firmware was flashed and the physical API
maintained paused MD mode and numeric values through both time-blanking phases.
The physical browser showed play+pause lit with a solid MD disc, matching
pause.jpg. Evidence: captures/md-animation/02-held-time/flashed-api.jsonl and
dashboard.png. No IR/player command was sent. Live resumed playback was not
requested or forced after the pause capture; the playback visual was exercised
through captured-state replay and browser fixtures.

MD animation firmware SHA256: `2f013d14f722101ea29253dc09a71069923c927edc233bb58b6348f4559d3db2`.

## Volume decoding update

Added persistent0–31 volume readout and a reconstructed VOLUME nn window when
SPI-2 mode05 commands arrive. All32 captured levels and371 overlay frames are
covered by native sweep validation. Additional MD, radio, live-state and title
checks passed. Browser checks passed minimum, maximum, title restoration and
existing display states. Native title restoration is a synthetic transition;
the original sweep recording ends before the volume command expires.

The C3 upload completed with flash hash verification. Its physical API reported
level5/max31, overlay=false, fresh=true, agreeing with the low five bits of the
numeric raw field. The physical browser rendered5 /31 and a live meter alongside
the normal title. Evidence: captures/manual/volume-flashed-api.json and
volume-dashboard.png. Live hardware overlay transitions were not forced;
those are covered by captured-frame replay and browser fixtures. No player
command was sent, and this is a read-only meter rather than a volume control.

Volume firmware SHA256: `f789f1d185a19a0e9a3aeacbe5dea39692f05e35332e08111adf20344ecab018`.

## Source text logos and independent MD graphic

The user-recorded empty-drive source cycle isolates byte3 masks01/02/04/08
for MD/CD/RADIO/LINE. Native replay checks all318 complete numeric frames,
source identification, blank numeric data and MD graphic off. Controlled FM
and AM regression fixtures vary/remove only the independent MD-graphic field
and retain the same frequencies. This checks the removal of the accidental
byte2=07 requirement without claiming that the source-only capture contains
valid radio-frequency digits.

MD/volume/radio/live-state regressions pass. Browser checks pass all four empty
source states, FM with the MD graphic off, and the radio-before-frequency state.
A discovered null-radio access in PRESET rendering was fixed before the final
upload; the full browser suite then passed. The final ROM upload verified hashes.

Physical API verification then reported RADIO source and AM531kHz, with only
the RADIO source text flag true, matching numeric byte3. Its current MD graphic
command was07 (solid); actual no-MD frequency reception was therefore covered
by regression fixtures rather than this live spot check. The physical browser
rendered without error. Evidence: captures/manual/source-flashed-api.json and
source-dashboard.png. No player commands were transmitted.

Source-fix firmware SHA256: `616092cdf164bfa4511bf32b7c7fb260ee9d152cd21efa0e03c135b39e8c60c4`.

## SLEEP steady/off/blinking update

Three user-labelled references isolate numeric byte0 bit0 as the SLEEP
illumination state. The precisely timed sleepisblinking.sal has16 lit and16
unlit frames, with~510ms phase spacing. Byte9 bit7 stays fixed in that capture,
ruling out the earlier active-low candidate as the direct illumination flag.
The UI follows received symbols.sleep directly without a local blink timer.
Native paired-reference/blink replay and live-state checks passed. The browser
suite passed SLEEP on/off along with existing display modes.

The C3 firmware uploaded successfully. Eight physical API snapshots agreed
with the received raw illumination bit; observed states in this spot check:
[True]. The physical browser rendered a decoded SLEEP state.
Live blink-phase transitions were not forced after flashing; the capture replay
covers both phases. Evidence: captures/manual/sleep-flashed-api.jsonl and
sleep-dashboard.png. No player commands were transmitted.

SLEEP firmware SHA256: `1ab2f29a90947c6f2a87751628cf180d6bc5c40ceb018e07c717b6d650051956`.

## TIMER/PLAY paired visual update

User-labelled timer-play-blinking.sal has84 complete numeric frames, including
raw byte0 mask0C states00/04/08/0C. The intermediate masks occur for one frame
at staggered edges. Individual physical TIMER versus timer PLAY bit assignment
remains unisolated. API timer_play_raw preserves the mask; timer_play combines
either bit on. The webpage displays the observed pair together using received
states, with no independent CSS blinking. REC and clock remain unmapped.

Native timer replay, SLEEP references and live-state checks passed. Browser
checks passed both pair phases and all existing indicator states. The C3 upload
verified flash hashes. Physical API snapshots matched the complete raw-frame
flags; observed timer masks in the final spot check:
[0]. The physical browser displayed TIMER and timer PLAY consistently.
Evidence: captures/manual/timer-flashed-api.jsonl and timer-dashboard.png.
No player commands were sent. Replay covers blink phases even if the live spot
check has returned to a steady/off state.

Timer firmware SHA256: `8a060d3419842336975a04e9c3bd757e0312a75db656e3abde401d381322b1e1`.

## DATE label update

User-labelled date-on/off references isolate numeric byte3 bit6(40), with MD
source bit01 preserved. Native replay covers all382 numeric frames; source and
live-state regression checks passed. Browser fixtures passed DATE on/off and
existing states. The former REC DATE visual was split so only DATE is mapped;
the adjacent REC prefix remains unknown. No new date/clock numeric decoder was
introduced.

The C3 upload verified flash hashes. The physical API reported date=false and
source=md, matching the user's off reference; the physical browser rendered the
separate DATE symbol off. Evidence: captures/manual/date-flashed-api.json and
date-dashboard.png. No player command was sent.

DATE firmware SHA256: `152a17d53c144c029c3f19db9d0e2e6d0db3c632a4f99ab722eda7c8b12f838d`.

## New remote feature / FM indicator update (2026-10-30)

At the user's request, controlled IR experiments verified radio_band, tune_up,
tune_down and date. Independent saved traces show FM87.50MHz, MW531->540kHz,
LW153kHz, unset date/clock view, and FM87.50->87.55->87.50MHz. No stop/eject/
record/power/timer-setting command was sent and no presets were edited. Some
exports were short and Saleae close returned ReadTimeout; existing exports
and available metadata are retained, with limits documented in
analysis/remote-radio-indicators.md.

FM and MHz numeric flags co-vary as byte4=C0. The FM visual uses this observed
pattern, without assigning bit80/40 individually. Text custom-glyph prefixes
identify FM/MW/LW and are rendered in their recognised format. AM clock is not
lit merely for MW/LW. No lit clockAM/PM reference was obtained; Sony's European
manual specifies24h, and the current clock/date view is unset. AM/PM remain
unmapped rather than guessed.

Controlled remote capture replay, existing radio/source/live-state checks and
browser FM/MW/LW fixtures passed. A stale temporary fixture was replaced with
fresh captured MD replay data before browser validation. The C3 upload verified
flash hashes. Its live API reported FM87.50MHz/display_bandFM/symbols.fm=true.
The physical browser showed FM lit and decoded FM text, with AM/PM unmapped.
Evidence: captures/remote-indicators/flashed-api.json and dashboard.png.
The unit is left on FM87.50MHz, DATE view off; MW was tuned from531 to540kHz
while testing. No claim is made to identify dormant clockAM/PM bit positions.

FM indicator firmware SHA256: `5688c3a14934c978c1decbc3b4d11382e078d1320b6a58be28c3c734e24046fa`.

## REPEAT and REPEAT1 update

User-labelled REPEAT1 and REPEAT-only captures isolate numeric byte5=63/43,
with the earlier normal baseline03. API symbols.repeat/repeat_one and the two
visuals use masks40/20. Native replay verifies266 captured numeric frames and
a controlled cleared-bit state. A test initially reused the trailing partial
frame for clearing; it now uses the last complete numeric frame and passes.
Live-state regression and browser all/one/off checks passed.

The C3 upload verified flash hashes. The physical API then reported repeat=False and repeat_one=False, matching its complete raw frame.
The physical browser rendered both decoded indicator states. Live on-states
were not forced after flashing; recorded replay and browser fixtures cover them.
Evidence: captures/manual/repeat-flashed-api.json and repeat-dashboard.png.
No player commands were transmitted during this capture/update task.

Repeat firmware SHA256: `0b75a4ea40b0d61d4c1eddb00800550e2fa7fe8044f6637f9b97edab815b2b76`.

## SHUFFLE update

User-labelled shuffle-on capture has188 complete numeric frames, all with
byte5=13 compared with normal03 and REPEAT43/63. API symbols.shuffle uses
byte5 mask10 independently of repeat flags. Native replay and live-state
checks passed, plus browser on/off and existing display checks.

The C3 upload verified hashes. Its physical API reported shuffle=True, matching the live complete raw field; the physical browser rendered the
decoded state. Evidence: captures/manual/shuffle-flashed-api.json and
shuffle-dashboard.png. Off is tested against the earlier normal baseline and
a controlled cleared-bit fixture. No player command was sent.

SHUFFLE firmware SHA256: `2a8383c4d0419784da8ee5ffabb02fb3458f31911dc234ef785204e164a7e2b9`.

## PGM flashing and STEP/TRACK steady labels

User-labelled pgm-blinking and pgm-step-track references establish received
PGM illumination bit08 in numeric byte5, with~510ms phases. STEP80 and TRACK01
are supported by the user's steady positive labels and earlier normal/date/
source-only layouts. No numeric program-step decoder was added. Native replay
covers253 PGM-only plus188 explicitly labelled STEP/TRACK frames and controlled
clearing. Browser checks cover both PGM phases with STEP/TRACK steady and off.

Both the PGM-only upload and the later STEP/TRACK upload verified flash hashes.
After the PGM-only upload, physical API snapshots observed both PGM phases.
The final physical API raw-flag spot check observed:
{'step': [True], 'track': [True], 'pgm': [False, True]}. Physical Chromium rendered all three decoded indicators.
Evidence: captures/manual/pgm-flashed-api.jsonl, step-track-flashed-api.jsonl,
and step-track-dashboard.png. No player commands were sent. Individual STEP/
TRACK off states use earlier references and controlled tests rather than a new
user-labelled one-at-a-time toggle experiment.

STEP/TRACK firmware SHA256: `b591f7db4e935ff749b2e04ad3c950ab9b2ab3b15e19785ee740bb3eb47e0e64`.

## Program-entry selected track update

The user-supplied program-track-01-to-20-to-01 trace contains590 complete
numeric frames:532 visible and58 deliberate AA/0A blank phases. Byte11 packed
BCD visits1..20 then1. New program state separates this selection from MD
playback numbers and hides elapsed time; three-digit padding is UI formatting,
not proof of a general hundreds-digit protocol. Numeric step index is not mapped.

Native full-sequence/blank/exit checks and existing MD/PGM/live-state checks
passed. Browser cases passed001,020, blank selection and returning to playback.
The C3 upload verified flash hashes. Physical API snapshots reported selected
track1 and both blank/unblank phases, matching raw complete frames. Physical
Chromium rendered MD PROGRAM and001 with elapsed time hidden. Evidence:
captures/manual/program-track-flashed-api.jsonl and program-track-dashboard.png.
No player commands were transmitted. Program playlist indicators remain unmapped.

Program-track firmware SHA256: `964e0b85a2cb5a7a351b82e8c1d7dbbb7475293922cd951fdd1fac1982f52579`.

## Predefined Program On/Off text messages

The user-labelled program-on.sal has23 complete text frames: mode01/code84,
zero parameters, explicitly identified as Program On. The earlier paired
ON/OFF capture uses84/85, identifying85 as the OFF member; the Program prefix
on that OFF string is inferred from the pair rather than a separate OFF-only
labelled reference. The decoder reconstructs the two strings only for exact
known zero-parameter commands. The API message object preserves wire mode/code;
numeric program selection and PGM flags remain independent.

Native ON/paired-message replay, unknown-parameter rejection, ASCII return,
volume/title/state regressions and browser on/off/title-return cases passed.
The C3 upload verified hashes; physical API exposes the new message field and
the webpage renders live. At the final spot check message was null, so transient
ON/OFF text is validated by captured replay/browser fixtures rather than a
forced live player action. Evidence: captures/manual/program-message-flashed-api.json
and program-message-dashboard.png. No player commands were sent.

Program-message firmware SHA256: `48b3a6bf0e2818a607b0ed8aec547c25f10da0421df225edf547d93727d853ed`.

## Stopped digits and track21/MM:SS fixes

User-requested remote experiments independently verified play and direct20/21
selection using >10,2,0/1, with acknowledgments and complete Saleae frames. A
stopped selected01 was captured with timeAA/AA; a later stopped summary showed
21/56:24. The stop transition was not recorded and no STOP was sent. Numeric
decoding now accepts the observed lowBCD track21 field (up to99 with byte12zero),
and track_display independently preserves digits/calendar when time is blank.
Playback time renders four digits MM:SS; stopped time is labelled generically.

The candidate OVER20 mapping is byte5 mask04, dashed and explicitly unconfirmed.
User could not see the physical icon. Earlier byte9 mask40 was ruled out by an
older20-track summary having that bit set. Current21-disc flag04 remains set
across stopped/playing1/20/21; this is comparative inference, not a visual label.

Native new captures and previous1..20/program/state regressions passed, plus
browser stopped/21/time and inferred-flag cases. The upload verified flash hashes.
A second remote play/>10,2,1 test after flashing produced valid21 playback time;
physical API snapshots advanced01:01..01:03 and Chromium displayed21/01:04.
Evidence: captures/track21/flashed-api.jsonl and dashboard.png. The player was
left playing21 at the final check. Volume/presets/settings were not changed.

Track21 firmware SHA256: `82f072d9883e28f77757e88fb3c6a9b7aca9f084439372276d7dfc32db720b30`.

# Timer recording setup

The user selected TIMER REC, RADIO, then the start and stop time and volume.
The C3 API was sampled passively; timestamped snapshots are in
`captures/manual2/live-timer-rec-setup/`, `live-timer-rec-start/`,
`live-timer-rec-start-minute/`, `live-timer-rec-stop/`,
`live-timer-rec-stop-minute/`, and `live-timer-rec-volume/`. These are C3
snapshots rather than Saleae captures and may contain occasional incomplete
frames; the observations below use complete, consistent frames.

- Initial TIMER REC selection uses the separately established blinking bits
  byte0 TIMER `08` and REC `02`.
- In the RADIO source selection screen, REC bit `02` remains on and source
  byte3 alternates `04` (RADIO) and `00` (dark); numeric bytes7..9 are
  `FF FF 83` and bytes10..14 `AA AA 0A AA AA`. The GUI retains RADIO through
  its blink. Only the RADIO recording source was captured; LINE remains
  unverified.
- Start hour/minute displayed alphanumeric `On` with numeric `19:09`; stop
  hour/minute displayed `Off` with `19:12`. Numeric byte0 alternates `02/0A`,
  byte3 remains `04`, byte5 is `02`, bytes7..9 are `FF FF 83`, and byte13 or
  byte14 blinks to `AA` for the selected field. These signatures are now
  supported alongside the previously captured timer PLAY layouts.
- At timer volume level20, byte0 alternated `02/0A`, byte3 stayed `04`,
  byte5 was `00`, byte6 alternated `34/14`, and bytes7..14 were
  `FF FF 83 AA AA 0A AA AA`. The text line sent mode `05` with value `14` and
  space parameters during the visible phase. Byte6 bit `20` indicates the
  volume's visible phase, and its low five bits carry level20.

The compact page identifies TIMER REC and uses separate layouts for radio
source, recording start/stop and volume. Timer REC on other sources and the
recording standby clock remain unverified until labelled captures are obtained.

## Wake timer RADIO source selection

A subsequent user-confirmed wake timer RADIO source selection was sampled into
`captures/manual2/live-timer-play-radio-source/state.jsonl`. PLAY remains set
in byte0 bit04; byte3 alternates `04` and `00` as RADIO blinks. The numeric
signature uses bytes7..9 `00 00 00`, unlike the recorded timer REC source
selection's `FF FF 83`; bytes10..14 are `AA AA 0A AA AA`. The decoder now
recognizes these two RADIO selection signatures independently, retains the
selected source across its dark phase and exposes `mode` play or rec plus the
current blink state. It tolerates up to200ms of nonmatching received frames
before dropping a confirmed selection, while a new On/Off timer title ends it
immediately. The compact screen shows TIMER PLAY / Wake-up source / RADIO with
manual dial and ENTER guidance. Other wake sources remain unverified.

## Wake timer CD and MD source selection

The user selected CD and MD in the same wake timer source menu. C3 snapshots
in `captures/manual2/live-timer-play-cd-confirmed/` and
`captures/manual2/live-timer-play-md-confirmed/` show the same PLAY source
selection layout as RADIO. Byte3 alternates `02/00` for CD and `01/00` for
MD. The decoder now retains the selected source across the dark phase and
switches immediately when a different lit source arrives; the compact display
shows the same Wake-up source screen for all three. The recording source
matcher still accepts only the separately observed RADIO recording layout.

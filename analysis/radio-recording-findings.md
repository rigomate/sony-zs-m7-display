# Radio recording to MiniDisc

The user recorded these two captures while recording radio to MD:

- `captures/manual2/REC-is-going-on-timer-counting-back-REMAIN-icon-on-REC-icon-next-to-REMAIN-also-on-track1-is-blinking.sal`
- `captures/manual2/REC-is-going-on-timer-counting-back-REMAIN-icon-on-REC-icon-next-to-REMAIN-also-on-track2-is-blinking-track1-is-on.sal`

In each, the source label is RADIO and the first 15 numeric bytes have the
pattern `00/80 60 41 B4 00 03 8A xx 00 80 AA tt 00 mm ss`.
Byte11 is the track in BCD; bytes13 and14 are remaining minutes and seconds.
They count down 74:58 to 74:51 in the first capture, 70:19 to 70:16 in the
second. The byte7 low bits are instantaneous indicators: capture 1 alternates
00/01 for track 1; capture 2 alternates 01/03, with track 1 steadily on and
track 2 blinking. The existing calendar layout places tracks 1..20 in bytes7,
8 and the low nibble of9. The user confirmed these are the physical numbered
indicators, not the three-digit track display.

The independent `REC-DATE-icons-lit-itstheREC-next-to-DATE.sal` reference has
byte3=`61` (MD source `01`, DATE `40`, and REC `20`), corroborating `20` for the
REC prefix beside DATE. The user confirmed this is the REC beside DATE, not the
main boxed REC, which remains byte1 bit40.

The later user-confirmed MD playback REMAIN capture has byte3 `91` and counts
down. Like recording, it has both `10` and `80` set, so it confirms the
combined signature but still does not isolate either individual bit.

Byte3 in the radio-recording captures is `B4`: RADIO `04`, REC `20`, and two
additional bits `10` and `80`. REMAIN is visibly lit, but these two bits are
always on together in the available references, so its individual bit cannot
be isolated. The UI exposes REMAIN as inferred for the observed radio
recording and MD playback signatures; it stays unmapped outside those and
confirmed normal MD playback. Do not assume the two bits both mean REMAIN.
No other recording sources or recording
layouts have been established.

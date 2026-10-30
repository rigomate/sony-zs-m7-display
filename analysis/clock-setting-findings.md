# Clock setting digits

User-labelled captures:

- `captures/manual2/DATE-blinking-4numerical-hour-from-0-to-24-and-0.sal`
- `captures/manual2/DATE-blinking-4numerical-minute-from-0-to-59-to-0-again.sal`

Both capture the DATE/clock-setting display with MD selected. The alphanumeric
bus continues to show the date string. Numeric byte13 carries the two hour
digits and byte14 the two minute digits. In the hour sweep, byte14 stays00;
byte13 advances through the full 24-hour range and wraps from23 to00. Hours
0..9 use `A0`..`A9`, where high nibble `A` blanks the leading tens digit; 10..23
use packed BCD. During the edit blink, byte13 is `AA` and byte14 remains00.

In the minute sweep, byte13 remains `A0` (hour00 with its leading zero blanked)
and byte14 advances from00 through59 and wraps to00. During the edit blink,
byte14 is `AA`. Thus the selected field is identified by which digit byte
blanks. The decoder retains the last valid value for that pair through `AA`
phases and remembers the edit focus from the blanked pair.

The API now exposes a separate `clock_setting` object with hour, minute,
editing field, and freshness. The webpage presents `HH:MM` under “Clock setting
· hour/minute”, hides track indicators and elapsed playback time, and returns
to normal track interpretation after the special clock-setting frame layout
ends. It does not substitute system time or assume a configured timezone.

The capture names mention “0 to24 and0”; the wire values show00..23 then00,
which is the expected24-hour cycle. No player commands were sent.

## Clock setting while MD playback is active

The live capture `captures/manual2/live-clock-debug/raw/digital.csv` shows a
second clock-setting packet layout when changing the hour during MD playback.
Its header is `b1=20,b2=01` (instead of the stopped-player `00,07`), and its
trailer is `FF FF 83` at bytes7..9 (instead of `03 00 80`). The remaining
signature and digit positions are consistent. In the captured `06:52` state,
byte13 is the hour pair and alternates between its value and `AA` during the
hour blink; byte14 carries minute52. This explains why the earlier decoder
missed the clock whenever the MD was playing. Both layouts are now recognized.

During the user's live check, another combination appeared: the stopped-player
header `00 00 07` with the playback trailer `FF FF 83`. The current frame
`80 00 07 41 00 02 0A FF FF 83 AA AA 0A A4 54 DC` decodes to04:54 with the
hour selected. The decoder now accepts the header and trailer variants
independently. After flashing, the physical API reported `clock_setting` hour4,
minute54, editing hour, with fresh bus data.

In a later live minute edit at06:48, byte14 alternated between `48` and `AA`
in step with the DATE indicator. The API now reports `hour_blank` and
`minute_blank`, and the webpage hides the corresponding digits for the actual
received blank phase while retaining the numeric value for the lit phase.

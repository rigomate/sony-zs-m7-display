# MegaBass ON/OFF predefined messages

The user corrected the displayed wording: **MegaBass On / MegaBass Off**,
not the earlier Program On / Program Off interpretation. The observed mapping is:

| SPI-2 byte 1 | Byte 2 | Bytes 3–13 | Display text |
|---|---|---|---|
| `01` | `84` | all zero | MegaBass On |
| `01` | `85` | all zero | MegaBass Off |

The historical captures retain their original filenames:
`captures/manual/program-on-raw/digital.csv` contains 23 ON frames, and
`captures/manual/program-on_program_off-raw/digital.csv` contains 130 paired
ON/OFF frames. Those names reflect the earlier mistaken prefix and are not
proof of a relationship to program playback. Raw recordings are unchanged.

These are predefined-message commands, not ASCII payloads. The decoder supplies
the corrected strings only for these exact mode/code/zero-parameter forms.
`MegaBass Off` occupies all 12 text cells; `MegaBass On` uses 11 plus padding.
The API preserves the original message code, and raw USB diagnostics retain the
packet bytes. No program-enabled or PGM state is inferred from these messages.

Native replay checks both strings and their title-window representations,
unknown-parameter rejection, API output, and return to ordinary ASCII. Browser
fixtures use the same corrected wording and twelve-cell lengths.

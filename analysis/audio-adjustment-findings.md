# BASS and TRE adjustment display

User-labelled passive Saleae captures:

- `captures/manual2/BASS-setting-goting-from-min-to-max.sal`
- `captures/manual2/TRE-setting-goting-from-min-to-max.sal`

Each capture was loaded and exported through the local Logic Analyzer MCP.
Complete paired SPI frame dumps are in each capture's `raw/frames.json`;
the raw Saleae CSV is beside it.

The BASS file has170 complete frames per bus; TRE has137 complete frames plus
one initial partial frame. BASS SDI2 command byte1 is `06`; TRE uses `07`.
In both, byte2 ranges from0 through12 and bytes3..13 are zero. Thus the
command byte identifies which setting is being adjusted.

The twelve values are consistent with a displayed adjustment position/level.
The user-described min-to-max sweeps reach12. They do not establish an audio
gain or frequency response scale. Numeric SPI byte0 alternates between `0C`
and `8C` in both recordings. Numeric byte15 varies in both but is not a stable
single-valued encoding of the text-side setting; it appears to include changing
frame/timing data. Do not decode it as a level without stronger evidence.

Firmware now recognizes SDI2 mode06 as BASS and mode07 as TRE, with the
validated payload shape. It publishes
`audio_adjustment: {position, steps:12, setting, fresh}` and excludes both
modes from ASCII title decoding. The page shows twelve position cells and the
selected BASS/TRE name. It does not render a fabricated dB scale on the main
display.

No player commands were sent and no hardware firmware was flashed in this
analysis pass.

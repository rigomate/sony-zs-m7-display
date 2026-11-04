# Sony ZS-M7 LVGL demo

The default firmware now receives live Sony SPI data. See [receiver setup](README.md).
To run this original demo, pause `sony_live_tick()` and call `sony_demo_start()`
after `sony_live_init()`. The rest of this document describes the demo mode.

Build the existing `blackpill` project in STM32CubeIDE (Release), then
flash the resulting ELF using the usual ST-Link configuration. The demo starts
automatically after the existing LCD initialization. No touch input is needed.

The 320 x 240 display uses white text, outlines and level bars on pure black.
It retains the compact webpage's source/status header, large readouts and
20-track calendar. The selected track has a white outline; unavailable tracks
are hidden. There are no colored status cues or dim gray secondary labels.
Font antialiasing can produce neutral gray edge pixels.

The demo cycles through 28 examples every five seconds (140 seconds per loop):
MD playing/paused/no disc, CD, line input, FM/MW radio, radio-to-MD recording,
standby clock, clock setting, timer PLAY and REC setup including source,
start/stop and volume, sleep, program, volume, bass and no signal. Only the
playing example advances its seconds. All values are simulated, and the bottom
line identifies the demo. Setup instructions illustrate future Sony controls;
they do not accept input in this demo.

Implementation: `eez/screens.c`, `eez/screens.h`, `eez/ui.c`. These are now
hand-maintained; exporting the old EEZ project into this directory will overwrite
them. The original EEZ color-test project has not been converted. Additional
Montserrat 24 and 40 fonts are enabled in `Core/Inc/lv_conf.h`.

## Validation

The Release build succeeds with the installed STM32 GNU toolchain: 490,232 bytes
text, 440 bytes data and 107,648 bytes BSS, within the F411CE's 512 KiB flash
and 128 KiB RAM. The linker reports a toolchain GNU-stack metadata warning.
A native LVGL framebuffer run rendered all 28 scenes, checked neutral RGB pixels
and exercised the external state update. See [rendered examples](demo-preview.png).
The firmware has not been flashed or checked through the physical red filter.

## Connecting live data later

After `ui_init()`, call `sony_display_show(&state)` from the main GUI loop to stop
the demo, hide its label, and render a `sony_display_state_t`. Strings are copied
by LVGL. Do not call LVGL from SPI/DMA interrupts: collect received data there,
then decode and submit a stable state from the main loop. `sony_demo_start()`
restarts the examples. Track availability uses bit 0 for track 1 through bit 19
for track 20, independently of any future wire-protocol encoding.

```c
sony_display_state_t state = {
    .view = SONY_VIEW_TRACK,
    .source = "MINIDISC", .status = "PLAY",
    .heading = "Midnight Drive", .value = "02:37",
    .detail = "Elapsed", .footer = "",
    .track = 3, .tracks = 0xfffffU
};
sony_display_show(&state);
```

The existing LCD transport and Sony SPI1/SPI2 configuration are unchanged.
This step does not receive or decode Sony bus traffic.

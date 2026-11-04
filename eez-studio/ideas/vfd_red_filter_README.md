# VFD Red Filter Test — EEZ Studio / ILI9341 / LVGL 9

Open `vfd_red_filter_320x240.eez-project` in EEZ Studio.

Resolution: 320 x 240 (landscape).
Project type: LVGL (without EEZ Flow), target LVGL 9.x.

The single editable screen contains:
- Full-brightness WHITE (#FFFFFF), RED (#FF0000), and YELLOW (#FFFF00) comparison patches.
- Five white RGB input levels: 100%, 75%, 50%, 25%, 10%.
- Pure-black (#000000) background.

Use EEZ Studio's Build command to generate source code in `src/ui`.
For your existing STM32 + LVGL integration, ensure the generated files are compiled and call `ui_init()` after LVGL display initialization. Call `ui_tick()` from your main loop as applicable.

Keep the LCD backlight at its rated full brightness during optical comparisons.
The grayscale percentages are input code levels, NOT calibrated light output percentages.
Test patches through the actual physical red filter. The red and white patches may have comparable perceived brightness with narrowband red filters.

`vfd_red_filter_preview.png` is a visual layout reference, not an EEZ Studio screenshot.

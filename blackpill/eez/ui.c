#include "ui.h"
static int current_screen = -1;
void loadScreen(enum ScreensEnum id) {
    if (id != SCREEN_ID_MAIN) return;
    current_screen = 0;
    lv_screen_load(objects.main);
}
void ui_init(void) {
    create_screens();
    loadScreen(SCREEN_ID_MAIN);
}
void ui_tick(void) { tick_screen(current_screen); }

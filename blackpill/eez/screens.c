/* Hand-maintained Sony demo. EEZ export will overwrite this file: see DEMO.md. */
#include "screens.h"
#include "fonts.h"
#include <string.h>

objects_t objects;
ext_font_desc_t fonts[] = {
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
    { "MONTSERRAT_40", &lv_font_montserrat_40 }
};
static lv_obj_t *source, *status, *heading, *value, *detail, *footer;
static lv_obj_t *value_cells[5], *detail_cells[5];
static int32_t value_cell_width, detail_cell_width;
static lv_obj_t *track_label, *track_value, *calendar[20], *level_bar, *demo_label;
static uint32_t demo_started, last_second = UINT32_MAX;
static unsigned last_scene = UINT32_MAX;
static bool demo_running;

#define TRACKS 0xfffffU
/* Same example states as the compact webpage. Every value here is simulated. */
static const sony_display_state_t scenes[] = {
    { SONY_VIEW_TRACK, "MINIDISC", LV_SYMBOL_PLAY " PLAY", "Midnight Drive", "02:37", "Elapsed", "", 3, TRACKS },
    { SONY_VIEW_TRACK, "MINIDISC", LV_SYMBOL_PAUSE " PAUSE", "Midnight Drive", "02:37", "Time", "", 3, TRACKS },
    { SONY_VIEW_MESSAGE, "MINIDISC", "STOP", "Disc status", "No Disc", "Insert a MiniDisc", "MiniDisc" },
    { SONY_VIEW_MESSAGE, "CD", "SELECTED", "Compact disc", "CD", "Source selected", "Waiting for disc data" },
    { SONY_VIEW_MESSAGE, "LINE", "INPUT", "External source", "Line input", "Audio input", "External audio" },
    { SONY_VIEW_VALUE, "FM RADIO", "PRESET", "Frequency / MHz", "104.00", "Preset station", "FM radio" },
    { SONY_VIEW_VALUE, "MW RADIO", "TUNING", "Frequency / kHz", "1287", "Manual tuning", "AM radio" },
    { SONY_VIEW_TRACK, "RADIO > MD", "REC", "Recording", "70:19", "Remaining", "", 2, 3 },
    { SONY_VIEW_VALUE, "STANDBY", "TIMER PLAY", "Standby clock", "18:55", "", "Player in standby" },
    { SONY_VIEW_VALUE, "CLOCK", "SETTING", "Set clock", "06:48", "Adjust minute", "Clock setting" },
    { SONY_VIEW_MESSAGE, "TIMER", "SETUP", "Timer setup", "PLAY", "Choose PLAY or REC", "Turn dial / Press ENTER" },
    { SONY_VIEW_MESSAGE, "TIMER PLAY", "SETUP", "Wake-up source", "RADIO", "Select input", "Turn dial / Press ENTER" },
    { SONY_VIEW_MESSAGE, "TIMER PLAY", "SETUP", "Wake-up source", "CD", "Select input", "Turn dial / Press ENTER" },
    { SONY_VIEW_MESSAGE, "TIMER PLAY", "SETUP", "Wake-up source", "MD", "Select input", "Turn dial / Press ENTER" },
    { SONY_VIEW_MESSAGE, "TIMER PLAY", "SETUP", "Wake-up source", "LINE", "Select input", "Turn dial / Press ENTER" },
    { SONY_VIEW_VALUE, "TIMER PLAY", "SETTING", "Timer start", "07:00", "Adjust hour", "Wake timer" },
    { SONY_VIEW_VALUE, "TIMER PLAY", "SETTING", "Timer stop", "08:00", "Adjust minute", "Wake timer" },
    { SONY_VIEW_LEVEL, "TIMER PLAY", "SET VOLUME", "Timer volume", "18", "", "Level / 0-31", 0, 0, 18, 31 },
    { SONY_VIEW_MESSAGE, "TIMER", "SETUP", "Timer setup", "REC", "Choose PLAY or REC", "Turn dial / Press ENTER" },
    { SONY_VIEW_MESSAGE, "TIMER REC", "SETUP", "Recording source", "RADIO", "Select input", "Turn dial / Press ENTER" },
    { SONY_VIEW_VALUE, "TIMER REC", "SETTING", "Record start", "19:00", "Adjust hour", "Recording timer" },
    { SONY_VIEW_VALUE, "TIMER REC", "SETTING", "Record stop", "20:00", "Adjust minute", "Recording timer" },
    { SONY_VIEW_LEVEL, "TIMER REC", "SET VOLUME", "Timer volume", "12", "", "Level / 0-31", 0, 0, 12, 31 },
    { SONY_VIEW_VALUE, "SLEEP", "SETTING", "Sleep timer / min", "30", "Automatic power off", "Sleep duration" },
    { SONY_VIEW_VALUE, "MD PROGRAM", "PROGRAM", "Program track", "03", "Midnight Drive", "Program play" },
    { SONY_VIEW_LEVEL, "SOUND", "VOLUME", "Volume", "18", "", "Level / 0-31", 0, 0, 18, 31 },
    { SONY_VIEW_LEVEL, "SOUND", "ADJUST", "Bass", "8", "", "Sound adjustment / 0-12", 0, 0, 8, 12 },
    { SONY_VIEW_MESSAGE, "ZS-M7", "OFFLINE", "Receiver status", "No signal", "Waiting for the player", "Check receiver connection" }
};
#define SCENE_COUNT (sizeof(scenes) / sizeof(scenes[0]))
#define SCENE_MS 5000U

static lv_obj_t *label(int x, int y, int w, const lv_font_t *font) {
    lv_obj_t *obj = lv_label_create(objects.main);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, w);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_label_set_text(obj, "");
    return obj;
}
static void visible(lv_obj_t *obj, bool show) {
    if (show) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
/* Give all five time characters fixed cells. A blank colon or an editing
 * digit pair then changes visibility without changing any other position. */
static int32_t time_cell_width(const lv_font_t *font) {
    int32_t width = lv_font_get_glyph_width(font, ':', 0);
    for (char c = '0'; c <= '9'; ++c) {
        int32_t digit = lv_font_get_glyph_width(font, c, 0);
        if (digit > width) width = digit;
    }
    int32_t dash = lv_font_get_glyph_width(font, '-', 0);
    return dash > width ? dash : width;
}
static bool is_time_text(const char *text) {
    if (!text || strlen(text) != 5 || (text[2] != ':' && text[2] != ' ')) return false;
    for (unsigned i = 0; i < 5; ++i) {
        if (i == 2) continue;
        if ((text[i] < '0' || text[i] > '9') && text[i] != '-' && text[i] != ' ') return false;
    }
    return true;
}
static void render_time(lv_obj_t *base, lv_obj_t **cells, const char *text,
                        int32_t cell_width, int32_t x, int32_t y,
                        int32_t width, bool right_align) {
    bool fixed = is_time_text(text);
    visible(base, !fixed);
    if (right_align) x += width - 5 * cell_width;
    for (unsigned i = 0; i < 5; ++i) {
        visible(cells[i], fixed);
        if (!fixed) continue;
        char digit[2] = { text[i] == ' ' ? '\0' : text[i], '\0' };
        lv_obj_set_pos(cells[i], x + i * cell_width, y);
        lv_label_set_text(cells[i], digit);
    }
}
static void render(const sony_display_state_t *s) {
    const bool track = s->view == SONY_VIEW_TRACK;
    const bool level = s->view == SONY_VIEW_LEVEL;
    lv_label_set_text(source, s->source ? s->source : "");
    lv_label_set_text(status, s->status ? s->status : "");
    const bool expanded = track && s->expanded_title;
    lv_obj_set_style_text_font(heading, expanded ? &lv_font_montserrat_14 : &lv_font_montserrat_24, 0);
    lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(heading, 12, expanded ? 44 : 51);
    lv_obj_set_height(heading, expanded ? 36 : 29);
    lv_label_set_text(heading, s->heading ? s->heading : "");
    lv_label_set_text(value, s->value ? s->value : "");
    lv_label_set_text(detail, s->detail ? s->detail : "");
    lv_label_set_text(footer, s->footer ? s->footer : "");
    lv_obj_set_pos(value, track ? 111 : 12, track ? 113 : 82);
    lv_obj_set_width(value, track ? 197 : 296);
    lv_obj_set_style_text_align(value, track ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(detail, track ? 111 : 12, track ? 92 : 139);
    lv_obj_set_style_text_align(detail, track ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(detail, track ? 197 : 296);
    render_time(value, value_cells, s->value, value_cell_width,
                track ? 111 : 12, track ? 113 : 82, track ? 197 : 296, track);
    render_time(detail, detail_cells, s->detail, detail_cell_width,
                track ? 111 : 12, track ? 92 : 139, track ? 197 : 296, track);
    visible(footer, !track);
    visible(track_label, track);
    visible(track_value, track);
    lv_label_set_text_fmt(track_value, "%02u", s->track);
    visible(level_bar, level);
    lv_bar_set_range(level_bar, 0, s->maximum ? (int32_t)s->maximum : 1);
    lv_bar_set_value(level_bar, (int32_t)s->level, LV_ANIM_OFF);
    for (unsigned i = 0; i < 20; ++i) {
        visible(calendar[i], track && (s->tracks & (1UL << i)));
        lv_obj_set_style_border_width(calendar[i], s->track == i + 1 ? 1 : 0, 0);
    }
}
void sony_display_show(const sony_display_state_t *state) {
    if (!state) return;
    demo_running = false;
    visible(demo_label, false);
    render(state);
}
void sony_demo_start(void) {
    demo_started = lv_tick_get();
    last_scene = UINT32_MAX;
    last_second = UINT32_MAX;
    demo_running = true;
    visible(demo_label, true);
    tick_screen_main();
}
void create_screen_main(void) {
    objects.main = lv_obj_create(NULL);
    lv_obj_remove_style_all(objects.main);
    lv_obj_set_size(objects.main, 320, 240);
    lv_obj_remove_flag(objects.main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(objects.main, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(objects.main, LV_OPA_COVER, 0);
    source = label(12, 10, 155, &lv_font_montserrat_14);
    status = label(165, 10, 143, &lv_font_montserrat_14);
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *rule = lv_obj_create(objects.main);
    lv_obj_remove_style_all(rule);
    lv_obj_set_pos(rule, 12, 35);
    lv_obj_set_size(rule, 296, 1);
    lv_obj_set_style_bg_color(rule, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    heading = label(12, 51, 296, &lv_font_montserrat_24);
    value = label(12, 82, 296, &lv_font_montserrat_40);
    detail = label(12, 139, 296, &lv_font_montserrat_14);
    value_cell_width = time_cell_width(&lv_font_montserrat_40);
    detail_cell_width = time_cell_width(&lv_font_montserrat_14);
    for (unsigned i = 0; i < 5; ++i) {
        value_cells[i] = label(0, 0, value_cell_width, &lv_font_montserrat_40);
        detail_cells[i] = label(0, 0, detail_cell_width, &lv_font_montserrat_14);
        lv_obj_set_style_text_align(value_cells[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_align(detail_cells[i], LV_TEXT_ALIGN_CENTER, 0);
    }
    track_label = label(12, 92, 90, &lv_font_montserrat_14);
    lv_label_set_text(track_label, "Track");
    track_value = label(12, 113, 90, &lv_font_montserrat_40);
    footer = label(12, 185, 296, &lv_font_montserrat_14);
    for (unsigned i = 0; i < 20; ++i) {
        calendar[i] = label(12 + (i % 10) * 30, 173 + (i / 10) * 23, 26, &lv_font_montserrat_14);
        lv_obj_set_height(calendar[i], 22);
        lv_obj_set_style_text_align(calendar[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_pad_top(calendar[i], 2, 0);
        lv_obj_set_style_border_color(calendar[i], lv_color_white(), 0);
        lv_label_set_text_fmt(calendar[i], "%u", i + 1);
    }
    level_bar = lv_bar_create(objects.main);
    lv_obj_remove_style_all(level_bar);
    lv_obj_set_pos(level_bar, 12, 148);
    lv_obj_set_size(level_bar, 296, 12);
    lv_obj_set_style_border_width(level_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(level_bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(level_bar, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(level_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    demo_label = label(12, 222, 296, &lv_font_montserrat_14);
    lv_label_set_text(demo_label, "DEMO / SIMULATED DATA");
    sony_demo_start();
}
void tick_screen_main(void) {
    if (!demo_running) return;
    uint32_t elapsed = lv_tick_get() - demo_started;
    unsigned scene = (elapsed / SCENE_MS) % SCENE_COUNT;
    unsigned second = (elapsed % SCENE_MS) / 1000;
    if (scene != last_scene) {
        render(&scenes[scene]);
        last_scene = scene;
        last_second = UINT32_MAX;
    }
    if (second != last_second) {
        /* Only the playing example advances. Paused and settings stay fixed. */
        if (scene == 0) {
            lv_label_set_text_fmt(value, "02:%02u", 37 + second);
            render_time(value, value_cells, lv_label_get_text(value), value_cell_width,
                        111, 113, 197, true);
        }
        last_second = second;
    }
}
void tick_screen(int screen_index) { if (screen_index == 0) tick_screen_main(); }
void tick_screen_by_id(enum ScreensEnum id) { tick_screen(id - 1); }
void create_screens(void) { create_screen_main(); }

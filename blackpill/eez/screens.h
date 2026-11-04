#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif
enum ScreensEnum { _SCREEN_ID_FIRST = 1, SCREEN_ID_MAIN = 1, _SCREEN_ID_LAST = 1 };
typedef struct { lv_obj_t *main; } objects_t;
extern objects_t objects;
/* Text is copied by LVGL. Call from the GUI thread, never from a SPI ISR. */
typedef enum { SONY_VIEW_MESSAGE, SONY_VIEW_TRACK, SONY_VIEW_VALUE, SONY_VIEW_LEVEL } sony_view_t;
typedef struct {
    sony_view_t view;
    const char *source, *status, *heading, *value, *detail, *footer;
    unsigned track;             /* 1..20; zero means no selected track */
    uint32_t tracks;            /* bit 0 = track 1; bit 19 = track 20 */
    unsigned level, maximum;
} sony_display_state_t;
/* Stops the automatic demo and displays an application-supplied state. */
void sony_display_show(const sony_display_state_t *state);
void sony_demo_start(void);
void create_screen_main(void);
void tick_screen_main(void);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);
void create_screens(void);
#ifdef __cplusplus
}
#endif
#endif

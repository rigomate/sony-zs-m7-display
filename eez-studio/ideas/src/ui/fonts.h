#ifndef EEZ_LVGL_UI_FONTS_H
#define EEZ_LVGL_UI_FONTS_H

#include <lvgl.h>

#ifndef EXT_FONT_DESC_T
#define EXT_FONT_DESC_T
typedef struct _ext_font_desc_t {
    const char *name;
    const void *font_ptr;
} ext_font_desc_t;
#endif

extern ext_font_desc_t fonts[];

#endif /*EEZ_LVGL_UI_FONTS_H*/
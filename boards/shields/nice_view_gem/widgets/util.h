#pragma once

#include <lvgl.h>
#include <zmk/endpoints.h>

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define LVGL_BACKGROUND lv_color_black()
#define LVGL_FOREGROUND lv_color_white()

/*
 * LVGL 9 canvas buffers for LV_COLOR_FORMAT_I1 start with the palette
 * (2 entries x 4 bytes) followed by the packed 1bpp pixel rows.
 */
#define SCREEN_BUF_SIZE                                                                              \
    (LV_COLOR_INDEXED_PALETTE_SIZE(LV_COLOR_FORMAT_I1) * sizeof(lv_color32_t) +                      \
     LV_CANVAS_BUF_SIZE(SCREEN_WIDTH, SCREEN_HEIGHT, 1, LV_DRAW_BUF_STRIDE_ALIGN))

struct status_state {
    uint8_t battery;
    uint8_t battery_p;
    bool charging;
    bool charging_p;
    uint8_t wpm;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t layer_index;
    const char *layer_label;
#else
    bool connected;
#endif
};

void to_uppercase(char *str);
void fill_background(lv_obj_t *canvas);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);
void draw_rect_on_canvas(lv_obj_t *canvas, int32_t x, int32_t y, int32_t w, int32_t h,
                         lv_draw_rect_dsc_t *rect_dsc);
void draw_text_on_canvas(lv_obj_t *canvas, int32_t x, int32_t y, int32_t max_w,
                         lv_draw_label_dsc_t *label_dsc, const char *text);
void draw_image_on_canvas(lv_obj_t *canvas, int32_t x, int32_t y, const lv_image_dsc_t *img,
                          lv_draw_image_dsc_t *draw_dsc);

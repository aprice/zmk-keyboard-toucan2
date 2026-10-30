#include <zephyr/kernel.h>
#include "util.h"
#include <ctype.h>


void to_uppercase(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

void fill_background(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    draw_rect_on_canvas(canvas, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, &rect_black_dsc);
}

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align) {
    lv_draw_label_dsc_init(label_dsc);
    label_dsc->color = color;
    label_dsc->font = font;
    label_dsc->align = align;
}

void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color) {
    lv_draw_rect_dsc_init(rect_dsc);
    rect_dsc->bg_color = bg_color;
}

void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width) {
    lv_draw_line_dsc_init(line_dsc);
    line_dsc->color = color;
    line_dsc->width = width;
}

void draw_rect_on_canvas(lv_obj_t *canvas, int32_t x, int32_t y, int32_t w, int32_t h,
                         lv_draw_rect_dsc_t *rect_dsc) {
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_area_t coords = {x, y, x + w - 1, y + h - 1};
    lv_draw_rect(&layer, rect_dsc, &coords);

    lv_canvas_finish_layer(canvas, &layer);
}

void draw_text_on_canvas(lv_obj_t *canvas, int32_t x, int32_t y, int32_t max_w,
                         lv_draw_label_dsc_t *label_dsc, const char *text) {
    label_dsc->text = text;

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_area_t coords = {x, y, x + max_w - 1, y + lv_font_get_line_height(label_dsc->font) - 1};
    lv_draw_label(&layer, label_dsc, &coords);

    lv_canvas_finish_layer(canvas, &layer);
}

void draw_image_on_canvas(lv_obj_t *canvas, int32_t x, int32_t y, const lv_image_dsc_t *img,
                          lv_draw_image_dsc_t *draw_dsc) {
    draw_dsc->src = img;

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    lv_area_t coords = {x, y, x + img->header.w - 1, y + img->header.h - 1};
    lv_draw_image(&layer, draw_dsc, &coords);

    lv_canvas_finish_layer(canvas, &layer);
}

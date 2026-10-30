# ZMK config for beekeeb Toucan2 Keyboard

[The beekeeb Toucan2 Keyboard](https://beekeeb.com/introducing-toucan2/) is a wireless split 42-key column‑stagger keyboard that a display and a trackpad, with an aggressive stagger on the pinky columns.

# Customizations

- **Keymap**: [config/toucan.keymap](config/toucan.keymap)
- **General configs**: [boards/shields/toucan/toucan_left.conf](boards/shields/toucan/toucan_left.conf) and [boards/shields/toucan/toucan_right.conf](boards/shields/toucan/toucan_right.conf)
- **Swipe shortcuts**: the `swipe_button_mapper` node in [boards/shields/toucan/toucan.dtsi](boards/shields/toucan/toucan.dtsi)
- **Invert scroll / trackpad settings**: the `tps43_trackpad` node in [boards/shields/toucan/toucan_right.overlay](boards/shields/toucan/toucan_right.overlay)

# ZMK main migration notes

The config tracks ZMK `main`, which moved to Zephyr 4.1 + LVGL 9. This required the following
changes relative to upstream; without them a clean build fails:

- **nice_view_gem ported from LVGL 8 to LVGL 9** ([boards/shields/nice_view_gem](boards/shields/nice_view_gem)):
    - Image descriptors: `LV_IMG_CF_INDEXED_1BIT` → `LV_COLOR_FORMAT_I1`, and the obsolete
      `always_zero`/`reserved` header fields were dropped.
    - The removed `lv_canvas_draw_rect/text/img` calls were replaced with the LVGL 9 canvas-layer
      API (`lv_canvas_init_layer` + `lv_draw_rect/label/image` + `lv_canvas_finish_layer`), wrapped
      in the helpers in [widgets/util.c](boards/shields/nice_view_gem/widgets/util.c).
    - `lv_draw_img_dsc_t` → `lv_draw_image_dsc_t`.
    - Canvas buffers changed from `lv_color_t` arrays to packed 1-bpp buffers with an 8-byte
      black/white palette (`SCREEN_BUF_SIZE`); with LVGL 9's 3-byte `lv_color_t` the old buffers
      would have cost ~217 KB of RAM.
    - `zmk_endpoints_selected()` → `zmk_endpoint_get_selected()` (ZMK API rename).
    - Kconfig: `select LV_USE_IMG` → `select LV_USE_IMAGE`; dropped the nonexistent
      `LV_USE_ANIMATION` and unused `LV_USE_ANIMIMG` selects.
- **Board target**: `-b xiao_ble` → `-b xiao_ble//zmk` in [build.sh](build.sh) and
  [build.yaml](build.yaml). ZMK main now extends the upstream Zephyr `xiao_ble` board with a
  `zmk` variant; the plain target resolves to the stock upstream board (missing ZMK's battery/
  QSPI/bootloader setup), and the `rgbled_adapter` shield's LED alias overlay only applies to the
  `zmk` variant, which the RGBLED widget hard-asserts on.

# License

The code in this repo is available under the MIT license.

The included shield nice_view_gem is modified from https://github.com/M165437/nice-view-gem licensed under the MIT License.

The linked trackpad module is based on https://github.com/geeksville/zmk_driver_azoteq

ZMK code snippets are taken from the ZMK documentation under the MIT license.

The embedded font QuinqueFive is designed by GGBotNet, licensed under under the SIL Open Font License, Version 1.1.

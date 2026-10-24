#!/bin/bash
set -e

rm -rf /ws/config && cp -r /src/config /ws/config
[ -d .west ] || west init -l /ws/config
west update
west zephyr-export

# left (with Studio)
west build -s zmk/app -d build/left -b seeeduino_xiao_ble \
	-S studio-rpc-usb-uart -- \
	-DSHIELD="toucan_left rgbled_adapter nice_view_gem" \
	-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src \
	-DCONFIG_ZMK_STUDIO=y

# right
west build -s zmk/app -d build/right -b seeeduino_xiao_ble -- \
	-DSHIELD="toucan_right rgbled_adapter" \
	-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src

# settings reset
west build -s zmk/app -d build/reset -b seeeduino_xiao_ble -- \
	-DSHIELD=settings_reset \
	-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src

for s in left right reset; do cp build/$s/zephyr/zmk.uf2 /out/$s.uf2; done

#!/bin/bash
set -e

rm -rf /ws/config && cp -r /src/config /ws/config
[ -d .west ] || west init -l /ws/config
west update
west zephyr-export

ts=$(date -u +%Y%m%dT%H%M%SZ)
sha=$(git -C /src rev-parse --short HEAD 2>/dev/null || echo nogit)
tag="$ts-$sha"
outdir="/out/$tag"
mkdir -p "$outdir"

pristine=""
if [ "$PRISTINE" = "1" ]; then
	pristine="--pristine"
fi

if [ "$DBG" = "1" ]; then
	# Debug build: USB logging + asserts, no Studio snippet (keeps CDC free for logs)
	extra=()
	builddir="build/left-dbg"
	if [ "$NOSCREEN" = "1" ]; then
		# Bisect: use ZMK's built-in (label-only) status screen instead of the gem's canvas one
		builddir="build/left-noscreen"
		extra+=(-DCONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN=y -DCONFIG_NICE_VIEW_WIDGET_STATUS=n)
	fi
	if [ "$NODISP" = "1" ]; then
		# Bisect: disable the display entirely to isolate display-related hangs
		builddir="build/left-nodisp"
		extra+=(-DCONFIG_ZMK_DISPLAY=n)
	fi
	west build $pristine -s zmk/app -d $builddir -b xiao_ble//zmk -S zmk-usb-logging -- \
		-DSHIELD="toucan_left rgbled_adapter nice_view_gem" \
		-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src \
		-DCONFIG_ASSERT=y \
		-DCONFIG_THREAD_NAME=y \
		-DCONFIG_LOG_MODE_IMMEDIATE=y \
		-DCONFIG_REBOOT=y \
		-DCONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y \
		-DCONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE=4096 \
		-DCONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=4096 \
		-DCONFIG_USB_WORKQUEUE_STACK_SIZE=4096 \
		-DCONFIG_ZMK_LOW_PRIORITY_THREAD_STACK_SIZE=4096 \
		"${extra[@]}"
	west build $pristine -s zmk/app -d build/reset -b xiao_ble//zmk -- \
		-DSHIELD=settings_reset \
		-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src
	cp $builddir/zephyr/zmk.uf2 "$outdir/left-dbg.uf2"
	cp build/reset/zephyr/zmk.uf2 "$outdir/reset.uf2"
	exit 0
fi

# left (with Studio)
west build $pristine -s zmk/app -d build/left -b xiao_ble//zmk \
	-S studio-rpc-usb-uart -- \
	-DSHIELD="toucan_left rgbled_adapter nice_view_gem" \
	-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src \
	-DCONFIG_ZMK_STUDIO=${STUDIO:-y}

# right
west build $pristine -s zmk/app -d build/right -b xiao_ble//zmk -- \
	-DSHIELD="toucan_right rgbled_adapter" \
	-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src

# settings reset
west build $pristine -s zmk/app -d build/reset -b xiao_ble//zmk -- \
	-DSHIELD=settings_reset \
	-DZMK_CONFIG=/ws/config -DZMK_EXTRA_MODULES=/src

for s in left right reset; do cp build/$s/zephyr/zmk.uf2 "$outdir/$s.uf2"; done

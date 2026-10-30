# Debugging notes — Toucan2 / ZMK main (Zephyr 4.1)

Persistent reference for debugging this repo. The TEMPORARY instrumentation
described here was removed after the boot-wedge was root-caused; this document
preserves the recipes so it can be recreated.

## Serial console: there are TWO USB CDC devices

The board enumerates two USB serial interfaces:

1. **First (alphabetically first `/dev/tty.usbmodem*`)** — mostly silent. This
   is the interface used for firmware flashing (UF2/bootloader side).
2. **Second** — the ZMK log console. This is where log output appears.

If tio on the first device looks dead, try the second device. Attach loop
(fish):

```fish
while true
    for p in (/bin/ls /dev/tty.usbmodem* 2>/dev/null)
        tio $p; and exit 0
    end
    sleep 1
end
```

Console wiring gotcha: the `zephyr,console` device is only created by the
**`zmk-usb-logging` snippet** (CDC node + chosen). `-DCONFIG_ZMK_USB_LOGGING=y`
alone enables the log backend with **no console device** → always build with
`-S zmk-usb-logging` when you want logs.

Also: `LOG_MODE_IMMEDIATE` drops all logs written before USB CDC enumerates
(~1s after boot), and immediate-mode log formatting (picolibc printf, ~1KB+ of
stack) inside workqueue-context listeners is itself a stack-overflow hazard.

## Build modes (build.sh)

- `DBG=1` — USB logging snippet + `CONFIG_ASSERT` + `CONFIG_THREAD_NAME` +
  `CONFIG_LOG_MODE_IMMEDIATE` + `CONFIG_REBOOT`, builds to `build/left-dbg`.
- `DBG=1 NOSCREEN=1` — bisect: ZMK built-in label status screen instead of gem.
- `DBG=1 NODISP=1` — bisect: display entirely disabled.
- `PRISTINE=1` — pristine west build.
- Board target must be `xiao_ble//zmk` (ZMK's HWMv2 extension), not plain
  `xiao_ble` — the LED aliases used by rgbled_adapter only exist on the former.

Symbol map for resolving crash PCs: `ws/build/<builddir>/zephyr/zmk.map`
(after a Zephyr 4.x build it's `zephyr_final.map`; PC values in fault dumps are
direct `.text` addresses, no extra app offset needed for this build).

## LED forensics rig (removed — recreate when needed)

A minimal rig lives in `boards/shields/toucan/heartbeat.c` (recreate it; it is
NOT in git history) registered via
`zephyr_library_sources(heartbeat.c)` in `boards/shields/toucan/CMakeLists.txt`
(that CMakeLists only existed for the rig — delete it again afterwards). It
needs `CONFIG_REBOOT=y` and, for full output, `CONFIG_THREAD_NAME=y` +
`CONFIG_ASSERT=y`.

Components:

1. **`.noinit` state struct** — magic `"ZDBG"`, boot_count, stage,
   fault_reason, fault_pc, faults, assert_file, assert_line, thread_name,
   stack_start, stack_size. Survives warm reset (`sys_reboot(SYS_REBOOT_WARM)`);
   only a power cycle clears it.
2. **Fatal hook** — override `k_sys_fatal_error_handler`: record reason/PC and
   `_current` thread name + `thread.stack_info`, blink the report, then
   `sys_reboot(SYS_REBOOT_WARM)`.
3. **Assert hook** — override `assert_post_action(const char *file, unsigned
   line)` (the correct symbol for `CONFIG_ASSERT` without `NO_FILE_INFO`,
   verified in `lib/os/assert.c`): record file pointer + line, then
   `k_panic()` so the fatal hook reports it.
4. **Heartbeat thread** — `K_THREAD_DEFINE` (prio 10, 2048 stack) logging every
   200ms, plus optional stage breadcrumbs set via `toucan_dbg_set_stage()` at
   points of interest in the code under test.

### LED report protocol

Green LED = `DT_ALIAS(led_green)` child of `DT_COMPAT_GET_ANY_STATUS_OKAY(gpio_leds)`
(gpio-leds, **not** PWM; same device the RGBLED widget uses).

- Boot: `(boot_count % 8) + 1` short (60ms) blinks.
- Fault: 1 long green (preamble) → `(reason & 0xF) + 1` blinks (200ms) → stage
  (1 nibble group) → PC low16 (4 nibble groups) → thread-name-ptr low16 (4
  groups; unreliable — gave garbage once) → stack-start low16 (4 groups) → if
  an assert was captured: 2 long greens + file-ptr low16 + line low16.
- Nibble group = value blinks of 200ms, 500ms gap between groups, a **1s gap
  encodes 0**.

### Zephyr fatal-reason enum

From `include/zephyr/fatal_types.h` (do NOT use the old kernel-era numbering):

| value | reason |
|-------|--------|
| 0 | `K_ERR_CPU_EXCEPTION` |
| 1 | `K_ERR_SPURIOUS_IRQ` |
| 2 | `K_ERR_STACK_CHK_FAIL` |
| 3 | `K_ERR_KERNEL_OOPS` |
| 4 | `K_ERR_KERNEL_PANIC` |
| 16 | `K_ERR_ARCH_START` |

### Stack-overflow signature

PC at the exact function-entry address of a leaf function, twice (entry +
fault dump), especially an entry-prologue push — classic stack overflow. If
`CONFIG_ASSERT` is on, an assert site PC is the other common signature.
Resolve the PC against `zmk.map`; a PC landing inside a thread's stack symbol
(e.g. `zmk_workqueue_lowprio_work_q`'s `lowprio_q_stack`) confirms it.

## Case history: the main-migration boot wedge (2026-10)

Migration to ZMK `main` (Zephyr `v4.1.0+zmk-fixes`, LVGL 9.3-dev) wedged the
left/central half on boot: LED frozen, no display, no BLE advertising, no USB
HID, zero serial output. Root cause was a **chain of stack overflows**, each
surfacing on a different thread as the previous one was fixed:

1. **Display render on the system workqueue** — ZMK main defaults sysworkq to
   ~2KB and the deep LVGL9 SW-render chain (gem arc-style screens) overflowed
   inside `lv_draw_sw_blend_image_to_i1`.
   Fix: `CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y` +
   `CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE=4096`.
2. **ZMK low-priority work queue (768 bytes default!)** — `battery.c`'s
   `battery_work` runs on `zmk_workqueue_lowprio_work_q()`; the event manager
   dispatches listeners inline in the raising thread's context, and the RGBLED
   widget listener logs at INF — with immediate mode that formats and writes
   via CDC and overflows (same family as community zmk#3460).
   Fix: `CONFIG_ZMK_LOW_PRIORITY_THREAD_STACK_SIZE=4096`.

Both fixes now live in `boards/shields/toucan/toucan_left.conf` /
`toucan_right.conf`. Note there is no fix yet (upstream open) for the display
workqueue need on boards without this config — if the wedge returns, check
stacks first.

Related memory sizing (kept): `LV_Z_MEM_POOL_SIZE=24576` in
`boards/shields/nice_view_gem/Kconfig.defconfig` — LVGL9 allocates a draw task
per primitive and 4KB was a hard crash. RAM budget after all bumps: ~120KB of
256KB on the nRF52840 (DBG build), so no exhaustion concern.

## Known upstream issues (as of 2026-10, ZMK main)

- **zmk#3522** — split central `peripheral_battery_levels[]` OOB write (source
  = `(uint8_t)-EINVAL` = 234) on non-peripheral disconnect; corrupts BT
  controller `preempt_req` on nRF52840/Zephyr 4.1. Open. Mitigation if it
  bites: `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=n` (costs the
  peripheral battery % on the display).
- **zmk#3459 / PR #3460** — display thread priority collision + undersized
  workqueue stacks (our lowprio overflow is the same family). Open.
- **zmk#3110** (fixed in 0.4.0) — split peripheral input-report system-workqueue
  deadlock (relevant: trackpad = input device on right half).
- **zmk#3207 / #3195** — central-role regressions on main/Zephyr 4.1
  (deep-sleep related).
- **zmk#3538/#3539/#3534** — recent split-central BLE robustness bugs on main.

## General gotchas

- `rg -r` is *replace*, not recursive — it rewrites matched text in output and
  caused phantom decodes once. Use `rg -n` / `rg -l -n`.
- `struct k_thread.name` exists only with `CONFIG_THREAD_NAME=y`.
- kconfiglib: typeless/prompt-less Kconfig symbols are not `-D`-assignable;
  give symbols a type and prompt (see `NICE_VIEW_WIDGET_STATUS`).

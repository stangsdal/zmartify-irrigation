# Display Settings Recommendations From Examples 13-16

## Scope

This document analyzes the exact display stack usage in the following example projects:

- `examples/13_LVGL_TRANSPLANT`
- `examples/14_LVGL_BTN`
- `examples/15_LVGL_SLIDER`
- `examples/16_LVGL_UI`

and proposes a consolidated settings profile for the main project.

## What Is Common Across All 4 Projects

All four examples target the same hardware/display path and share the same high-level initialization sequence:

1. GT911 touch init (`touch_gt911_init()`)
2. RGB panel init (`waveshare_esp32_s3_rgb_lcd_init()`)
3. Backlight on (`wavesahre_rgb_lcd_bl_on()`)
4. LVGL port init (`lvgl_port_init(panel_handle, tp_handle)`)

All four examples use:

- LVGL resolution: `1024 x 600`
- LVGL tick period: `2 ms`
- Anti-tearing enabled, mode `3` (RGB double-buffer + LVGL direct mode)
- Touch enabled and integrated with LVGL input device
- PSRAM enabled for frame-buffer-heavy usage
- RGB timing tuned for the Waveshare 7-inch panel

## Exact Display Configuration Used

### Panel And RGB Bus

From `components/rgb_lcd_port/rgb_lcd_port.h` and `.c` across the examples:

- Resolution: `1024 x 600`
- Pixel clock: `30.85 MHz`
- RGB data width: `16-bit`
- Color depth path: RGB565 (`16 bpp`)
- Frame buffers: `2`
- Bounce buffer size: `EXAMPLE_LCD_H_RES * 10` pixels
- PSRAM frame buffers: enabled (`fb_in_psram = 1`)

Timing values in panel config:

- `hsync_pulse_width = 162`
- `hsync_back_porch = 152`
- `hsync_front_porch = 48`
- `vsync_pulse_width = 45`
- `vsync_back_porch = 13`
- `vsync_front_porch = 3`
- `pclk_active_neg = 1`

### LVGL Port

From `components/lvgl_port/lvgl_port.h` and `.c`:

- `LVGL_PORT_H_RES = 1024`
- `LVGL_PORT_V_RES = 600`
- `LVGL_PORT_TICK_PERIOD_MS = 2`
- `LVGL_PORT_AVOID_TEAR_ENABLE = 1`
- `LVGL_PORT_AVOID_TEAR_MODE = 3`
- Rotation fixed at `0` degrees in all four projects
- LVGL task:
  - min delay `10 ms`
  - max delay `500 ms`
  - priority `2`
  - core affinity `-1` (no pinning)

LVGL task stack size differs:

- `13/14/15`: `6 * 1024`
- `16`: `8 * 1024`

### `sdkconfig.defaults` Display-Relevant Pattern

All examples include:

- `CONFIG_SPIRAM=y`
- `CONFIG_LV_COLOR_SCREEN_TRANSP=y`
- `CONFIG_LV_MEM_CUSTOM=y`
- `CONFIG_LV_USE_PERF_MONITOR=y`
- `CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y`
- Multiple Montserrat fonts enabled

Differences:

- SPIRAM speed:
  - `13`, `16`: `120M`
  - `14`, `15`: `80M`
- Larger font use:
  - `15`, `16` include `CONFIG_LV_FONT_MONTSERRAT_44=y`
- `16` enables `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y` because Wi-Fi is active in UI flows.

## Exact Per-Example Display Use

### 13_LVGL_TRANSPLANT

- Purpose: pure LVGL port validation on real panel/touch.
- UI load: LVGL official demo (`lv_demo_widgets()` by default).
- Display pressure: high widget variety, animation-capable, broad coverage of rendering paths.
- Takeaway: good baseline for correctness and anti-tearing behavior.

### 14_LVGL_BTN

- Purpose: simple interactive UI (single button + label).
- UI load: very light, mostly static with click events.
- Display pressure: low.
- Takeaway: confirms stack works for low-complexity HMI with minimal redraw pressure.

### 15_LVGL_SLIDER

- Purpose: interactive slider + frequent battery text updates.
- UI load: moderate due to repeated value/label changes.
- Display pressure: moderate from periodic text redraw and control interaction.
- Takeaway: validates responsiveness under regular small dirty-area updates.

### 16_LVGL_UI

- Purpose: full multi-screen application (Login/Create User/Main/RS485/CAN/PWM/Wi-Fi) generated with SquareLine and customized.
- UI load: high object count, many screen transitions, keyboard/text area interactions, dynamic status updates.
- Display pressure: highest among all examples.
- Additional evidence of heavier runtime:
  - LVGL task stack increased to `8 KB`.
  - RGB panel config uses `dma_burst_size = 64`.
  - LVGL color depth expectation is explicitly guarded as 16-bit in `ui.c`.

## Recommended Settings Profile

These recommendations are based on what already works across all four examples, with priority on stability for a production multi-screen UI.

### Keep (Strong Recommendation)

- Resolution: `1024x600`
- Color depth: `16-bit RGB565`
- Anti-tearing: enabled, mode `3` (direct mode + double buffer)
- LVGL tick: `2 ms`
- RGB frame buffers in PSRAM
- Keep current panel timing values unchanged unless panel vendor spec changes

### Prefer For Main Product Build

- Use `120 MHz` SPIRAM profile (as in examples `13` and `16`) for UI-heavy builds.
- Use LVGL task stack `8 KB` (not `6 KB`) to match complex UI behavior seen in example `16`.
- Keep `CONFIG_LV_FONT_MONTSERRAT_44=y` only if large text is actually used; otherwise disable to save flash/ram.
- Keep `CONFIG_LV_USE_PERF_MONITOR=y` in development builds, disable for release if you need lower overhead.

### Optional Hardening

- Explicitly keep LVGL color depth fixed at 16-bit across all build configs to avoid SquareLine/UI asset mismatch.
- If Wi-Fi and large UI run together, keep PSRAM allocation options aligned with example `16` (`CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y`).
- Keep VSYNC notification path enabled (`on_vsync`) and avoid mixing callback modes between targets.

## Suggested Baseline For This Repository

For a robust default that covers both simple and complex UI cases:

- Panel: current 1024x600 RGB timing and pin map (unchanged)
- LVGL: anti-tearing mode 3, 2 ms tick, direct mode path
- Buffers: RGB double buffer in PSRAM
- Tasking: LVGL stack `8 KB`, priority `2`
- SPIRAM: `120M` on ESP32-S3 if board stability confirms it
- Fonts: include only sizes used by active screens

This baseline is effectively the intersection of "works on simple demos" (14/15) and "works on production-like UI" (16), while preserving the proven panel driver setup from all four examples.

## Applied To Current Firmware (2026-06-29)

The following changes were applied in the main firmware to align with these recommendations.

### 1. Main Startup Wiring

- `main/main.c`: integrated `hmi_board_init()` in `app_main()` before runtime task startup.
- `main/main.c`: added explicit runtime status logging for panel/touch/backlight after init.
- `main/CMakeLists.txt`: added `hmi_board` to `REQUIRES`.

### 2. Compile-Time Display Profile Switch

- Added `components/hmi_board/Kconfig` with a profile choice:
  - `CONFIG_HMI_DISPLAY_PROFILE_LOW_RISK` (14 MHz)
  - `CONFIG_HMI_DISPLAY_PROFILE_HIGH_THROUGHPUT` (30.85 MHz)
- `components/hmi_board/src/hmi_7b_rgb.c` now uses the selected profile for `LCD_PIXEL_CLOCK_HZ`.
- Current selected profile in `sdkconfig`: `CONFIG_HMI_DISPLAY_PROFILE_HIGH_THROUGHPUT=y`.

### 3. Applied sdkconfig Key Delta

Set to recommendation-aligned values:

- `CONFIG_SPIRAM_SPEED_120M=y`
- `CONFIG_SPIRAM_SPEED=120`
- `CONFIG_LV_COLOR_SCREEN_TRANSP=y`
- `CONFIG_LV_MEM_CUSTOM=y`
- `CONFIG_LV_MEMCPY_MEMSET_STD=y`
- `CONFIG_LV_USE_PERF_MONITOR=y`
- `CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y`
- `CONFIG_LV_FONT_MONTSERRAT_12=y`
- `CONFIG_LV_FONT_MONTSERRAT_16=y`
- `CONFIG_LV_FONT_MONTSERRAT_20=y`
- `CONFIG_LV_FONT_MONTSERRAT_24=y`
- `CONFIG_LV_FONT_MONTSERRAT_44=y`
- `CONFIG_HMI_DISPLAY_PROFILE_HIGH_THROUGHPUT=y`
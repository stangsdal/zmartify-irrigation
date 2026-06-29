# Display Bring-Up Continuation Handoff

Date: 2026-06-29
Target: esp32s3

## Scope Completed Today (Point 1)

Goal executed: full reconfigure/build attempt to regenerate derived build/config artifacts and confirm current Kconfig resolution.

Actions performed:
- Ran ESP-IDF `fullClean`.
- Ran ESP-IDF `setTarget esp32s3` (this recreated `build/` and regenerated `sdkconfig`).
- Fixed main component dependency typo in `main/CMakeLists.txt` from `json` to `cjson`.
- Ran ESP-IDF `build`.

## Build Outcome

Compile blockers from the previous run have been cleared. Latest ESP-IDF build now produces fresh artifacts:

- `build/zmartify_irrigation.elf`
- `build/zmartify_irrigation.bin`
- `build/bootloader/bootloader.bin`

### Compile Blockers Cleared (What Was Fixed)

- `components/diagnostics_manager/src/diagnostics_manager.c`
   - added legacy alarm compatibility aliases/APIs in `components/alarm_manager/include/alarm_manager.h` and `components/alarm_manager/src/alarm_manager.c`:
      - `alarm_raise(...)`
      - `alarm_active_count()`
      - `alarm_active_count_by_severity(...)`
      - legacy constants mapped to current enum set.
   - updated `storage_manager_count` call site to pass explicit pointer (`NULL` for compatibility path).
- `components/mqtt_manager/src/mqtt_manager.c`
   - updated `storage_manager_count` call site to pass explicit pointer (`NULL`).
- `main/main.c`
   - fixed FreeRTOS include order (`FreeRTOS.h` included before `event_groups.h`).
- `main/CMakeLists.txt`
   - fixed component dependency typo: `json` -> `cjson`.
   - expanded `REQUIRES` list so headers included by `main/main.c` resolve from their owning components.

## Config State Preserved For Next Session

`setTarget` temporarily reset `sdkconfig` aggressively. To preserve intended tuning, `sdkconfig` was restored from `sdkconfig.old`.

Current key display/memory settings in `sdkconfig`:
- `CONFIG_SPIRAM=y`
- `CONFIG_SPIRAM_SPEED_120M=y`
- `CONFIG_SPIRAM_SPEED=120`
- `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y`
- `CONFIG_HMI_DISPLAY_PROFILE_HIGH_THROUGHPUT=y`
- `CONFIG_LV_COLOR_SCREEN_TRANSP=y`
- `CONFIG_LV_MEM_CUSTOM=y`
- `CONFIG_LV_USE_PERF_MONITOR=y`
- `CONFIG_LV_FONT_MONTSERRAT_12=y`
- `CONFIG_LV_FONT_MONTSERRAT_16=y`
- `CONFIG_LV_FONT_MONTSERRAT_20=y`
- `CONFIG_LV_FONT_MONTSERRAT_24=y`
- `CONFIG_LV_FONT_MONTSERRAT_44=y`

## Next-Day Resume Plan (When Board Is Connected)

1. Flash and monitor:
   - `flash`
   - `monitor`
2. Validate runtime logs:
   - HMI init status line in app startup
   - RGB panel log includes profile and pclk (`high-throughput`, `30850000`)
3. On-device checks:
   - backlight on/off behavior
   - touch responsiveness
   - visible tearing/flicker under UI activity
4. If unstable/noisy signal on current wiring:
   - switch to low-risk profile (`CONFIG_HMI_DISPLAY_PROFILE_LOW_RISK`), rebuild, reflash, compare.

## Board-Day Runbook (Command Sequence + Log Checks)

Use these in order from project root.

1. Verify serial device:
   - `ls /dev/cu.usbmodem*`
2. Select port in ESP-IDF extension:
   - command: `selectPort`
3. Build (sanity check before flash):
   - command: `build`
4. Flash firmware:
   - command: `flash`
5. Start monitor immediately after flash:
   - command: `monitor`

Expected success log patterns:
- App startup banner:
  - `Zmartify Irrigation Controller booting`
- HMI status line from `app_main`:
  - `HMI status: panel=1 touch=... backlight=...`
- RGB panel init line:
  - contains `RGB panel ready (1024x600, pclk=30850000, profile=high-throughput)`

Warning/failure log patterns to act on:
- If HMI init is degraded:
  - `HMI init reported degraded status`
- If panel allocation/timing fails:
  - look for `esp_lcd` errors and any `no mem` / panel init failure lines.
- If monitor stalls or serial is busy:
  - close any existing monitor session, then retry flash and monitor.

Fallback test path (if image unstable or tearing is severe):
1. Switch profile in config:
   - set `CONFIG_HMI_DISPLAY_PROFILE_LOW_RISK=y`
   - unset `CONFIG_HMI_DISPLAY_PROFILE_HIGH_THROUGHPUT`
2. Rebuild:
   - command: `build`
3. Reflash and monitor:
   - commands: `flash`, then `monitor`
4. Compare behavior versus high-throughput profile:
   - backlight bring-up reliability
   - touch stability
   - visible tearing/flicker under UI updates

## Files Changed For This Step

- `main/CMakeLists.txt` (fixed typo + expanded `REQUIRES`)
- `main/main.c` (FreeRTOS include-order fix + HMI startup integration from prior step)
- `components/alarm_manager/include/alarm_manager.h` (legacy API compatibility aliases/prototypes)
- `components/alarm_manager/src/alarm_manager.c` (legacy compatibility implementation)
- `components/diagnostics_manager/src/diagnostics_manager.c` (storage count signature fix)
- `components/mqtt_manager/src/mqtt_manager.c` (storage count signature fix)
- `sdkconfig` restored from `sdkconfig.old` after set-target regeneration
- this handoff file: `docs/DISPLAY_CONTINUATION_HANDOFF.md`

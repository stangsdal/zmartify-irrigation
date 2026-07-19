# HMI Workflows and Acceptance

## Product Baseline

The supported local interface is the Waveshare ESP32-S3 7B, 1024x600 RGB display with GT911
capacitive touch. The supported product scope is English with the light operational theme. Theme
switching and additional languages are not release features.

The HMI has three ownership layers:

1. `hmi_controller` owns navigation, action validation and confirmation state without depending on
   LVGL, FreeRTOS or hardware.
2. `hmi_lvgl_port` renders snapshots and converts touch events into controller requests.
3. `main` copies runtime state under `s_ctx_lock` and dispatches accepted actions. Irrigation
   actions enter the same runtime command queue as remote commands; alarm actions use the alarm
   manager under the same lock. No HMI source calls relay HAL or irrigation-engine control APIs.

## Supported Screens

| Screen | Required visible state | Local workflows | Acceptance |
|---|---|---|---|
| Dashboard | Controller, active zone, remaining time, weather, hydraulics, programs, rain delay and alarm count | Immediate Stop All | Live values refresh without resizing navigation; Stop All has no confirmation delay |
| Irrigation | Current run, selected zone/runtime, selected program availability, flow and pressure | Select and start zone; select and run enabled program; Stop All | Start/run requires confirmation and reaches the shared runtime queue; disabled programs are rejected |
| Weather | Temperature, humidity, rain, ET and current rain delay | Set 24-hour rain delay; clear delay | Both changes require confirmation and use runtime commands |
| Hydraulics | Sensor availability, readings and all uncleared alarms | Select alarm; acknowledge; clear resolved alarm | Alarm actions require confirmation; clear succeeds only after resolution and acknowledgment; other critical alarms retain lockout |
| Settings | Display baseline, interface scope, MQTT, time, storage and configuration health | Status inspection | Credentials and safety thresholds are not exposed; authenticated service tooling remains authoritative |

Alarm count or configuration safe mode is displayed persistently in the footer on every screen.
Uncleared active, acknowledged and resolved alarms remain in the view model until alarm policy
permits explicit clearing.

## Confirmation Policy

| Action | Confirmation | Execution boundary |
|---|:---:|---|
| Stop All | No | Shared runtime command queue |
| Start zone | Yes | Shared runtime command queue |
| Run program | Yes | Shared runtime command queue and scheduler state |
| Set or clear rain delay | Yes | Shared runtime command queue |
| Acknowledge alarm | Yes | Alarm manager under runtime lock |
| Clear resolved alarm | Yes | Alarm manager under runtime lock |

Only one confirmation may be pending. Cancel discards the request without dispatch. Program IDs,
zone IDs, runtimes, rain-delay values and alarm IDs are validated before a dialog is opened.

## Verification

- `test_hmi_controller` verifies navigation, validation, immediate Stop All, protected zone/program,
  weather and alarm actions, cancellation and single-pending-dialog behavior.
- `test_alarm_lifecycle` verifies that clearing one of multiple critical alarms does not release
  the remaining lockout.
- The complete host suite contains 15 tests and passes.
- The signed ESP32-S3 image builds at `0x151000` with 21 percent partition headroom.
- Device boot evidence verifies the 1024x600 RGB panel, GT911 touch, LVGL 8.4, validated schema v3
  configuration and stable repeated telemetry after HMI binding.

The current firmware has no framebuffer screenshot export or desktop LVGL simulator. Therefore,
navigation and dialog state are regression-tested at the controller layer, while physical render
and touch initialization are device-verified. Screenshot regression remains a documented Step 10
verification improvement rather than being claimed as completed evidence.
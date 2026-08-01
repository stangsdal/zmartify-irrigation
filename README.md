# Zmartify Irrigation Controller (ZIC)

ESP32-S3 based irrigation controller firmware built with ESP-IDF.

## Current Status

The controller runs autonomous persisted schedules, synchronizes program
configuration from the edge over MQTT v2, and reports runtime state and
outcomes back to the edge. The current execution limit is two simultaneous
zones in the same schedule group; later groups wait for all zones in the
current group to finish.

## Architecture Components

- `zone_manager`
- `irrigation_engine`
- `flow_manager`
- `pressure_manager`
- `weather_manager`
- `et_engine`
- `mqtt_manager`
- `storage_manager`
- `alarm_manager`
- `ota_manager`
- `ui_manager`

## Build

1. Install ESP-IDF.
2. Select target: `esp32s3`.
3. Build with `./scripts/build.sh`.

Firmware builds should use ESP-IDF's Ninja toolchain and Python environment. Avoid building the firmware with VS Code CMake Tools or standalone CMake presets; those can configure `build/` with a non-ESP-IDF generator or pick up another Python/esptool from `PATH`. Host tests are the exception and use their own `build-host-*` CMake directories.

## Documentation

- [MQTT Command Interface](docs/mqtt-command-interface.md)
- [Program and schedule audit (2026-07-29)](docs/PROGRAM-SCHEDULE-AUDIT-2026-07-29.md)
- [Program and schedule implementation checklist (2026-07-29)](docs/PROGRAM-SCHEDULE-IMPLEMENTATION-CHECKLIST-2026-07-29.md)
- [MQTT v2 program and schedule sync contract](docs/MQTT-V2-PROGRAM-SCHEDULE-SYNC-CONTRACT.md)
- [OTA security and provisioning](docs/OTA-SECURITY-PROVISIONING.md)
- [Operational logging](docs/OPERATIONAL-LOGGING.md)

## Host Tests

Configure host tests separately from ESP-IDF builds, then run the full suite:

```sh
cmake -S . -B build-host -DZIC_HOST_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

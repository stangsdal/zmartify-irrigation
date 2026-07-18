# Zmartify Irrigation Controller (ZIC)

ESP32-S3 based irrigation controller firmware built with ESP-IDF.

## Current Status

This repository is being implemented incrementally from the v3.0 documentation package.

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
3. Build with `idf.py build`.

## Documentation

- [MQTT Command Interface](docs/mqtt-command-interface.md)

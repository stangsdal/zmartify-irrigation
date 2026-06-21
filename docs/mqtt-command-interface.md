# MQTT Command Interface

## Overview

This document defines inbound MQTT command topics and payload formats currently supported by the firmware runtime.

Root topic:

- `zmartify/irrigation/controller_01`

Inbound command topics (QoS 1):

- `zmartify/irrigation/controller_01/command/start_zone`
- `zmartify/irrigation/controller_01/command/stop_zone`
- `zmartify/irrigation/controller_01/command/run_program` (stub, currently ignored)
- `zmartify/irrigation/controller_01/command/stop_all`
- `zmartify/irrigation/controller_01/command/rain_delay`

## Runtime Handling

1. MQTT transport subscribes to all command topics on connect.
2. Incoming MQTT messages are parsed in the message callback.
3. Valid commands are converted to internal runtime commands.
4. Commands are pushed onto the FreeRTOS command queue.
5. Control task applies commands to irrigation engine and state machine.

## Payload Formats

Commands support both legacy numeric payloads and JSON payloads.

### start_zone

Topic:

- `.../command/start_zone`

Accepted payloads:

- Numeric zone id: `3`
- JSON with zone only: `{ "zone": 3 }`
- JSON with zone id alias: `{ "zone_id": 3 }`
- JSON with runtime: `{ "zone": 3, "runtime_seconds": 600 }`
- JSON with runtime alias: `{ "zone": 3, "runtime": 600 }`

Rules:

- Zone range must be 1..15.
- If runtime is missing or 0, runtime defaults to 300 seconds.

### stop_zone

Topic:

- `.../command/stop_zone`

Accepted payloads:

- Numeric zone id: `3`
- JSON: `{ "zone": 3 }`
- JSON alias: `{ "zone_id": 3 }`

Rules:

- Zone range must be 1..15.

### stop_all

Topic:

- `.../command/stop_all`

Accepted payloads:

- Any payload is ignored. Command is accepted by topic.

### rain_delay

Topic:

- `.../command/rain_delay`

Accepted payloads:

- Numeric hours: `12`
- JSON: `{ "hours": 12 }`
- JSON alias: `{ "rain_delay_hours": 12 }`

Rules:

- Value `0` clears rain delay.
- Value `> 0` sets rain delay in hours.
- Current rain delay value is persisted in NVS key `rain_delay_h`.

### run_program

Topic:

- `.../command/run_program`

Status:

- Reserved for future implementation.
- Messages are currently ignored by runtime parser.

## Validation and Error Behavior

- Invalid payloads are rejected and logged as warnings.
- Unknown command topics are ignored.
- Out-of-range zone values are rejected.
- Queue send failures are not retried in current implementation.

## Telemetry Publishing

Runtime telemetry is currently published to:

- `zmartify/irrigation/controller_01/state` (QoS 0)

The current payload is a CSV line generated from the in-memory log entry.

## Implementation Notes

- Topic mapping is handled in MQTT topic helpers.
- Inbound parsing and queue dispatch are implemented in runtime app code.
- JSON parsing uses ESP-IDF `json` component (`cJSON`).

## Example MQTT Publish Commands

Start zone 4 for default runtime:

```sh
mosquitto_pub -h 192.168.10.2 -t zmartify/irrigation/controller_01/command/start_zone -m "4" -q 1
```

Start zone 4 for 15 minutes:

```sh
mosquitto_pub -h 192.168.10.2 -t zmartify/irrigation/controller_01/command/start_zone -m '{"zone":4,"runtime_seconds":900}' -q 1
```

Stop zone 4:

```sh
mosquitto_pub -h 192.168.10.2 -t zmartify/irrigation/controller_01/command/stop_zone -m '{"zone":4}' -q 1
```

Set 24-hour rain delay:

```sh
mosquitto_pub -h 192.168.10.2 -t zmartify/irrigation/controller_01/command/rain_delay -m '{"hours":24}' -q 1
```

Clear rain delay:

```sh
mosquitto_pub -h 192.168.10.2 -t zmartify/irrigation/controller_01/command/rain_delay -m "0" -q 1
```

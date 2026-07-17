# Zmartify Water Sensor Integration

**Status:** Active architecture decision  
**Applies to:** Zmartify Irrigation Controller v5.x and later  
**External device:** `stangsdal/zmartify-watersensor`

## 1. Purpose

The irrigation controller shall use the ESP32-S2 based Zmartify Water Sensor node as the preferred acquisition point for water-related sensors.

The Water Sensor node initially provides:

- up to four pulse-based flow-meter channels;
- temperature measurements, initially from externally powered DS18B20 sensors;
- persistent pulse and accumulated-volume counters;
- sensor validity, age and diagnostic flags;
- a versioned I2C interface for local controller access;
- independent MQTT telemetry to Zmartify Edge.

The architecture is intentionally extensible to soil moisture, pressure, leak detection, tank level, conductivity and related environmental measurements.

## 2. Responsibility boundary

### Irrigation controller remains responsible for

- valve and master-valve control;
- pump control where fitted;
- irrigation schedules and zone sequencing;
- safety-critical actuator interlocks;
- flow and pressure anomaly policies;
- hydraulic baseline learning;
- emergency shutdown decisions;
- alarms related to irrigation operation;
- determining whether a measurement is sufficiently fresh and valid for a control decision.

### Water Sensor node is responsible for

- electrical sensor interfacing;
- pulse capture and monotonic pulse counters;
- flow conversion and accumulated volume;
- temperature acquisition;
- per-sensor calibration;
- local filtering that does not alter accumulated totals;
- sensor presence, validity, age and fault diagnostics;
- persistence of accumulated flow totals;
- publishing coherent sensor snapshots over I2C and MQTT.

### Zmartify Edge remains responsible for

- device discovery and inventory;
- historical storage and dashboards;
- long-term analytics and notifications;
- fleet health, firmware and protocol visibility;
- non-safety-critical automation.

## 3. System context

```text
Flow meters ----\
Temperatures ----+--> Zmartify Water Sensor (ESP32-S2)
Future sensors --/          |
                            +-- I2C --> Irrigation Controller (ESP32-S3)
                            |
                            +-- Wi-Fi/MQTT --> Zmartify Edge

Irrigation Controller --> valves, master valve and pumps
```

The I2C connection is the preferred deterministic local path. MQTT is not a substitute for local hydraulic safety data.

## 4. Architectural changes

The following previous assumptions are superseded:

| Previous assumption | New design |
|---|---|
| Flow pulses are counted by ESP32-S3 PCNT | Pulses are counted by the Water Sensor node |
| Temperature is read directly from MCP9808 by the controller | Temperature is normally supplied by the Water Sensor node |
| Flow Manager owns raw acquisition and calibration | Flow Manager consumes validated snapshots and owns irrigation-specific analysis |
| Controller stores lifetime pulse totals | Water Sensor owns monotonic counters and persistent totals |
| Sensor absence may be represented as zero | Missing, invalid or stale values remain explicitly invalid |

Pressure may remain directly connected to the irrigation controller when it is required for a fast, safety-critical local interlock. A later pressure channel on the Water Sensor node may supplement monitoring, but shall not silently replace a required independent safety path.

## 5. Controller-side components

### WaterSensorClient

A new controller component shall:

- initialize and supervise the I2C link;
- read identity and capability information;
- negotiate or validate protocol major version;
- periodically read the complete sensor snapshot;
- verify frame length, CRC and sequence number;
- decode fixed-width little-endian fields;
- maintain the latest coherent snapshot;
- publish sensor update and communication fault events;
- expose measurement age and validity to consumers;
- avoid dynamic allocation in I2C callbacks or safety paths.

Suggested component location:

```text
components/watersensor_client/
  include/watersensor_client.h
  include/watersensor_protocol.h
  src/watersensor_client.c
  src/watersensor_protocol.c
  test/test_watersensor_protocol.c
```

### Flow Manager

Flow Manager shall no longer depend directly on a PCNT HAL driver. It shall consume flow records from `WaterSensorClient` and retain responsibility for:

- mapping Water Sensor flow channels to irrigation functions;
- checking validity and freshness;
- expected-versus-measured flow comparison;
- no-flow, low-flow and excess-flow detection;
- post-close flow detection;
- leak policy and severity;
- zone hydraulic baseline learning;
- program and zone consumption derived from monotonic counters;
- safety events to the Irrigation Engine and Alarm Manager.

### Temperature consumers

Temperature consumers shall use stable sensor IDs and configured roles rather than bus order. Example roles include:

- `cabinet_temperature`;
- `water_supply_temperature`;
- `soil_temperature_zone_01`;
- `greenhouse_temperature`.

A stale or invalid temperature shall not be converted to `0 °C`.

## 6. Sensor registry and mapping

The controller configuration shall map stable external sensor IDs to local semantic roles.

Example:

```json
{
  "watersensor_device_id": "watersensor-a1b2c3",
  "flow_channels": {
    "main_supply": 1,
    "irrigation_supply": 2
  },
  "temperature_roles": {
    "cabinet_temperature": "28-ff-64-a2-91-16-04-7c",
    "soil_temperature_zone_01": "28-ff-2a-91-44-16-03-5e"
  }
}
```

Unknown sensors may be reported diagnostically but shall not automatically acquire a control role.

## 7. Data quality rules

Every externally acquired measurement used by control logic shall include or derive:

- source device identity;
- sensor identity or channel;
- measurement type and unit;
- snapshot sequence;
- validity flag;
- measurement age;
- sensor fault flags;
- communication state.

Controller policy shall distinguish:

- valid zero;
- stale measurement;
- absent sensor;
- invalid measurement;
- communication failure;
- unsupported protocol or capability.

The controller shall enter the configured safe behavior when required safety measurements are unavailable beyond their timeout.

## 8. Timing and safety

Recommended initial polling policy:

| Data | Poll/refresh target | Maximum age for active irrigation |
|---|---:|---:|
| Flow snapshot | 250–500 ms | configurable, initially 1500 ms |
| Temperature | 1–5 s | role-dependent |
| Identity/capabilities | startup and reconnect | not applicable |
| Diagnostics | 5–30 s | not normally safety-critical |

Flow safety decisions remain local to the irrigation controller. Loss of Water Sensor communication during active irrigation shall raise a sensor/communication alarm and trigger the configured fail-safe action.

The Water Sensor node may continue measuring while the controller is offline, but it shall not independently operate irrigation valves.

## 9. Event model

Suggested controller events:

```text
EVT_WATERSENSOR_ONLINE
EVT_WATERSENSOR_OFFLINE
EVT_WATERSENSOR_PROTOCOL_ERROR
EVT_WATERSENSOR_SNAPSHOT
EVT_SENSOR_ADDED
EVT_SENSOR_REMOVED
EVT_SENSOR_VALUE_UPDATED
EVT_SENSOR_STALE
EVT_SENSOR_FAULT
```

Existing flow events remain valid and are produced by Flow Manager after policy evaluation:

```text
EVT_FLOW_UPDATED
EVT_FLOW_HIGH
EVT_FLOW_LOW
EVT_FLOW_NONE
EVT_FLOW_LEAK
EVT_FLOW_SENSOR_FAULT
```

Raw external sensor updates are telemetry facts. Safety and control events are derived by controller-owned policy components.

## 10. MQTT ownership

The Water Sensor node publishes its own raw and normalized sensor telemetry under its versioned topic tree.

The irrigation controller publishes:

- controller state;
- irrigation context;
- expected and measured flow correlation;
- hydraulic alarms and decisions;
- zone/program consumption;
- Water Sensor link health where useful.

The controller should not republish every raw Water Sensor field unless required by the Zmartify Edge ingestion model. Duplicate telemetry sources must have explicit ownership and stable semantics.

## 11. Migration plan

1. Implement `WaterSensorClient` and protocol decoder behind a mockable interface.
2. Add configuration for external device identity, flow-channel mapping and sensor roles.
3. Feed recorded or simulated snapshots into Flow Manager tests.
4. Change Flow Manager acquisition from PCNT HAL to the client snapshot interface.
5. Retain the old direct-flow path temporarily behind a build-time feature flag if needed.
6. Integrate temperature roles and remove direct temperature acquisition only after parity testing.
7. Validate communication-loss behavior during active irrigation.
8. Remove obsolete PCNT and local-temperature assumptions after hardware migration is complete.

## 12. Acceptance criteria

- Flow Manager can run entirely from simulated Water Sensor snapshots.
- Controller never resets or modifies Water Sensor lifetime counters.
- Counter deltas remain correct across controller restart and Water Sensor restart.
- Stale, missing and invalid measurements cannot be mistaken for valid zero values.
- I2C CRC, version and malformed-frame failures are detected.
- Active irrigation enters the configured safe state when required flow data becomes unavailable.
- Temperature sensors are selected by stable ID and semantic role.
- Water Sensor MQTT availability is not used as the sole local safety signal.
- Existing irrigation alarms and event contracts remain stable where their meaning has not changed.

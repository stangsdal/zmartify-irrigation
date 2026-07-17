# Architecture Addendum v5.1 — Zmartify Water Sensor

**Status:** Active engineering baseline  
**Supersedes:** Sensor-acquisition assumptions in `ARCHITECTURE.md` v5.0  
**Related:** [WATERSENSOR-INTEGRATION.md](WATERSENSOR-INTEGRATION.md)

## 1. Decision

The ESP32-S3 irrigation controller no longer acts as the primary acquisition device for flow-meter pulses or general temperature sensors. These measurements are acquired by the ESP32-S2 based `zmartify-watersensor` node and read locally by the irrigation controller over a versioned I2C protocol.

The irrigation controller remains the authority for irrigation decisions, actuator control and fail-safe behavior.

## 2. Revised architecture layers

```text
Zmartify Edge
    ^
    | MQTT
    |
+-------------------------+       I2C       +--------------------------+
| Zmartify Water Sensor   |---------------->| Irrigation Controller    |
| ESP32-S2                |                 | ESP32-S3                 |
|                         |                 |                          |
| - pulse acquisition     |                 | - schedules              |
| - flow calculation      |                 | - zone sequencing        |
| - accumulated totals    |                 | - flow safety policy     |
| - temperature sensing   |                 | - pressure safety        |
| - sensor diagnostics    |                 | - valves and pumps       |
+-------------------------+                 +--------------------------+
       ^                                             |
       |                                             v
flow, temperature and                        master valve, zones,
future water-related sensors                 pump and local alarms
```

## 3. Revised subsystem responsibilities

| Subsystem | Revised responsibility |
|---|---|
| `WaterSensorClient` | Acquire, validate and cache coherent sensor snapshots over I2C |
| Flow Manager | Analyse external flow measurements and make irrigation-specific safety decisions |
| Temperature consumers | Read measurements by stable sensor ID and configured semantic role |
| Pressure Manager | Continue direct local pressure acquisition when required for a safety interlock |
| Irrigation Engine | Own schedules, zone sequencing, shutdown decisions and actuator commands |
| Alarm Manager | Raise controller alarms from communication, validity and policy failures |
| MQTT Manager | Publish controller context and decisions; avoid unnecessary duplication of raw Water Sensor telemetry |

## 4. HAL changes

The HAL baseline is revised as follows:

- I2C becomes the primary local interface to the Water Sensor node.
- PCNT remains available only as a transitional or optional direct-flow implementation.
- Direct MCP9808 temperature acquisition is no longer the default architecture.
- ADS1115 pressure acquisition remains local until a separate safety review approves migration.
- Sensor values exposed above HAL must include validity and age; unavailable data must not be represented as a valid zero.

## 5. Revised task model

| Task | Priority guidance | Responsibility |
|---|---:|---|
| Water Sensor polling | 9–10 | Poll flow snapshot every 250–500 ms during active irrigation |
| Flow Guard | 14 | Evaluate freshness, expected flow and hydraulic faults |
| Pressure Guard | 14 | Evaluate locally acquired pressure |
| Relay Control | 12 | Operate valves and pumps |
| Irrigation Engine | 10 | Orchestrate programs and safe shutdown |
| Temperature update | 6–8 | Refresh non-critical temperature roles |
| Diagnostics | 4 | Track I2C errors, stale data and protocol health |

The polling task must not block Flow Guard or relay-control paths. Flow Guard evaluates the latest validated snapshot and its age.

## 6. Revised sensor inputs

| Measurement | Source | Controller interface | Typical refresh | Safety role |
|---|---|---|---:|---|
| Flow | Water Sensor pulse channels | I2C snapshot | 250–500 ms | Hydraulic safety |
| Temperature | Water Sensor DS18B20 providers | I2C snapshot | 1–5 s | Role dependent |
| Pressure | Local ADS1115/transducer | Local HAL | 10 Hz | Hydraulic safety |
| Future soil moisture | Water Sensor provider | I2C snapshot | configurable | Scheduling input, normally non-critical |

## 7. Failure behavior

During active irrigation:

1. CRC, protocol or I2C failures invalidate the affected snapshot.
2. The previous measurement may be used only while it remains within its configured maximum age.
3. Expired required flow data raises a communication/sensor fault.
4. The Irrigation Engine enters the configured hydraulically safe state.
5. MQTT availability is not used as proof that the local I2C measurement path is healthy.

During idle operation, loss of the Water Sensor node raises a diagnostic alarm but need not prevent unrelated controller administration.

## 8. Configuration additions

The controller configuration shall include:

- expected Water Sensor device identity;
- supported protocol major version;
- I2C address and polling intervals;
- maximum allowed measurement ages;
- flow-channel-to-system-role mapping;
- stable temperature-sensor-ID-to-role mapping;
- communication-loss policy;
- optional transitional direct-flow feature flag.

## 9. Documentation precedence

Where the v5.0 architecture states that:

- the ESP32-S3 PCNT peripheral is the normal flow source;
- Flow Manager counts raw pulses;
- MCP9808 is the normal temperature source;
- lifetime flow totals are owned by the irrigation controller;

this addendum and `WATERSENSOR-INTEGRATION.md` take precedence.

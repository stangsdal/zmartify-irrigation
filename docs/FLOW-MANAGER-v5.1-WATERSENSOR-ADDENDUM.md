# Flow Manager Addendum v5.1 — External Water Sensor Input

**Status:** Active engineering baseline  
**Supersedes:** Raw-acquisition sections of `Chapter 10 – Flow Manager Architecture` v5.0  
**Related:** [WATERSENSOR-INTEGRATION.md](WATERSENSOR-INTEGRATION.md)

## 1. Revised purpose

Flow Manager remains responsible for irrigation-specific flow supervision, anomaly detection, learning and safety events. It is no longer responsible for electrical pulse acquisition, pulse calibration or persistence of lifetime pulse totals.

The authoritative raw and normalized flow measurements are supplied by `zmartify-watersensor` through `WaterSensorClient`.

## 2. Revised architectural position

```text
Flow meters
    |
    v
Zmartify Water Sensor
    - pulse capture
    - calibration
    - flow calculation
    - monotonic counters
    - accumulated volume
    |
    | versioned I2C snapshot
    v
WaterSensorClient
    - CRC and version validation
    - freshness and communication state
    - coherent snapshot cache
    |
    v
Flow Manager
    - channel mapping
    - expected/measured comparison
    - learning and anomaly policy
    - irrigation consumption deltas
    - safety events
    |
    +--> Irrigation Engine
    +--> Alarm Manager
    +--> MQTT/UI/Diagnostics
```

## 3. Input contract

For every configured flow channel, Flow Manager consumes:

| Field | Meaning |
|---|---|
| `source_device_id` | Stable Water Sensor identity |
| `channel_id` | Stable flow channel identifier |
| `snapshot_sequence` | Sequence of the coherent source snapshot |
| `pulse_count_total` | Monotonic pulse counter owned by Water Sensor |
| `volume_total_ml` | Monotonic accumulated volume owned by Water Sensor |
| `flow_ml_min` | Current filtered flow |
| `last_pulse_age_ms` | Age of the most recent accepted pulse |
| `measurement_age_ms` | Age of the snapshot at the controller |
| `valid` | Whether the measurement can be interpreted |
| `fault_flags` | Source-side sensor and acquisition diagnostics |

Flow Manager must distinguish valid zero flow from missing, stale or invalid data.

## 4. Removed responsibilities

The following responsibilities move out of Flow Manager:

- ESP32-S3 PCNT initialization and pulse counting;
- electrical input filtering and debounce;
- pulses-per-liter conversion;
- sensor-side instantaneous-flow filtering;
- persistence of lifetime pulse and volume totals;
- direct flow-meter hardware diagnostics.

These belong to the Water Sensor node. Controller-side diagnostics still track link quality, freshness, mapping and policy state.

## 5. Retained responsibilities

Flow Manager continues to own:

- mapping source channels to irrigation supply roles;
- validation of required source identity and capabilities;
- active-zone expected flow;
- no-flow, low-flow and high-flow policy;
- unexpected flow while idle;
- flow after valve closure;
- learned per-zone hydraulic baselines;
- correlation of flow with pressure and valve state;
- program and zone water consumption;
- alarms, safety events and diagnostics;
- controller-specific telemetry.

## 6. Consumption accounting

Zone and program consumption shall be calculated from monotonic Water Sensor counters:

```text
consumption_delta_ml = end_volume_total_ml - start_volume_total_ml
```

Rules:

- Store the source device ID, channel ID and starting counter when irrigation begins.
- Use unsigned monotonic delta logic.
- Detect source restart or counter discontinuity using device identity, boot ID, sequence and status fields.
- Never write, reset or recalibrate the Water Sensor lifetime counter from Flow Manager.
- If a reliable delta cannot be established, mark consumption incomplete rather than inventing zero.

## 7. Freshness policy

Suggested initial values:

| State | Maximum snapshot age |
|---|---:|
| Active irrigation | 1500 ms |
| Valve opening verification | 1000 ms |
| Post-close leak verification | 1500 ms |
| Idle monitoring | 5000 ms |

Values shall be configurable and validated.

A stale required measurement is a sensor-availability fault, not a zero-flow reading.

## 8. Revised flow-learning sequence

```text
Validate Water Sensor link and mapped flow channel
    ↓
Record starting monotonic counter
    ↓
Open master valve and selected zone
    ↓
Wait for pressure and flow stabilization
    ↓
Collect only valid, fresh flow snapshots
    ↓
Reject communication gaps and hydraulic transients
    ↓
Calculate baseline statistics
    ↓
Store baseline with source identity and channel mapping
```

A baseline becomes invalid or requires review if its mapped Water Sensor device or channel changes.

## 9. Fault handling

Flow Manager shall map source and link conditions into controller policy:

| Condition | Controller interpretation |
|---|---|
| Valid flow value of zero | Hydraulic no-flow candidate |
| Snapshot stale | Measurement unavailable |
| I2C communication failure | Water Sensor link fault |
| CRC/protocol failure | Invalid external measurement |
| Source channel fault | Flow sensor fault |
| Device identity mismatch | Configuration/integration fault |
| Counter discontinuity | Accounting fault; do not infer consumption |

Safety shutdown remains a controller decision. The Water Sensor node does not operate valves.

## 10. Event behavior

`WaterSensorClient` publishes acquisition facts such as:

```text
EVT_WATERSENSOR_SNAPSHOT
EVT_WATERSENSOR_OFFLINE
EVT_WATERSENSOR_PROTOCOL_ERROR
EVT_SENSOR_STALE
EVT_SENSOR_FAULT
```

Flow Manager converts these facts and irrigation context into existing domain events:

```text
EVT_FLOW_UPDATED
EVT_FLOW_HIGH
EVT_FLOW_LOW
EVT_FLOW_NONE
EVT_FLOW_LEAK
EVT_FLOW_LEARNED
EVT_FLOW_SENSOR_FAULT
```

Existing subscribers should not need to know whether flow originated from PCNT or an external node.

## 11. Transitional compatibility

A temporary direct-PCNT provider may remain behind a build-time feature flag while hardware migration is validated. Both providers shall implement the same controller-side flow-source interface.

Production configuration must select exactly one authoritative source for each logical flow role. Measurements from two sources must not be silently combined.

## 12. Revised testing requirements

Automated tests shall cover:

- decoding valid and malformed Water Sensor frames;
- CRC and protocol-major rejection;
- valid zero versus stale/invalid flow;
- source restart and counter discontinuity;
- consumption deltas across controller restart;
- communication loss during active irrigation;
- expected-versus-measured flow policy using simulated snapshots;
- flow learning with missing samples and transient rejection;
- device/channel mapping changes;
- compatibility of existing flow events and alarms.

Hardware-in-the-loop tests shall disconnect I2C, reset either controller independently and inject known pulse trains while irrigation is active.

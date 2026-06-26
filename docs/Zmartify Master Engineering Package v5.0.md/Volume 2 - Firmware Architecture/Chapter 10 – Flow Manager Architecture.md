# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 10 – Flow Manager Architecture

---

# 10 Flow Manager

---

# 10.1 Purpose

The Flow Manager is responsible for measuring, validating, analysing and supervising all water flow through the irrigation system.

The Flow Manager is one of the most safety-critical software components in the controller because it provides the primary protection against:

* Pipe rupture
* Broken sprinklers
* Stuck valves
* Solenoid failure
* Dry-running irrigation
* Unexpected water consumption
* Hydraulic degradation

The Flow Manager shall continuously supervise water flow whenever the irrigation system is active and shall also monitor for unexpected flow while the system is idle.

The Flow Manager shall never directly control relays. Instead, it provides information and alarms to the Irrigation Engine and Safety Supervisor.

---

# 10.2 Design Objectives

The Flow Manager shall:

* Measure instantaneous flow
* Measure accumulated water consumption
* Detect abnormal flow
* Detect missing flow
* Detect excessive flow
* Detect leaks
* Learn hydraulic baselines
* Support predictive diagnostics
* Maintain historical statistics
* Publish telemetry via MQTT

---

# 10.3 Architectural Position

```text
                  Flow Sensor
                      │
                      ▼
                HAL Flow Driver
                      │
                      ▼
                 Flow Manager
                      │
        ┌─────────────┼──────────────┐
        ▼             ▼              ▼
 Irrigation      Alarm Manager   MQTT Manager
    Engine
```

The Flow Manager shall receive pulse information exclusively from the HAL.

---

# 10.4 Supported Hardware

Approved sensor:

**DN50 G2 Turbine Hall Effect Flow Meter**

Specifications:

| Parameter |        Value |
| --------- | -----------: |
| Pipe Size |         DN50 |
| Thread    |           G2 |
| Output    |   Hall Pulse |
| Range     | 10–200 L/min |
| Interface | Pulse Output |
| Input     |   ESP32 PCNT |

Future firmware shall support additional pulse-based flow meters through calibration.

---

# 10.5 Measurement Philosophy

The controller performs four independent flow calculations.

### Instantaneous Flow

Current flow rate.

Unit:

L/min

---

### Zone Flow

Current zone flow.

Used for:

* Leak detection
* Hydraulic verification

---

### Program Consumption

Total water consumed during one irrigation program.

---

### Lifetime Statistics

Permanent accumulated water consumption.

---

# 10.6 Data Acquisition

The ESP32 PCNT peripheral counts pulses continuously.

Sampling interval:

250 ms

Processing interval:

1 second

Flow calculations shall use moving averages to reduce noise.

---

# 10.7 Flow Calculation

```text
Pulse Count

↓

Calibration Constant

↓

Flow (L/min)

↓

Moving Average

↓

Filtering

↓

Validated Flow
```

Calibration constants shall be configurable.

---

# 10.8 Calibration

Each flow meter shall support:

* Factory calibration
* User calibration
* Automatic Flow Learning correction

Calibration parameters:

| Parameter                | Description         |
| ------------------------ | ------------------- |
| Pulses/Litre             | Primary calibration |
| Offset                   | Fine adjustment     |
| Temperature Compensation | Reserved            |
| Linearization            | Future              |

Calibration changes shall immediately affect new measurements.

---

# 10.9 Filtering

Measurements shall be filtered using:

* Median filter
* Moving average
* Spike rejection

Filtering shall not delay leak detection beyond specified safety limits.

---

# 10.10 Flow Learning

Flow Learning establishes hydraulic baselines for each irrigation zone.

Sequence:

```text
Select Zone

↓

Open Master Valve

↓

Pressure Stabilization

↓

Open Zone

↓

Ignore Initial Transient

↓

Measure Flow

↓

Calculate Average

↓

Store Baseline
```

Learning duration:

Default:

60 seconds

Configurable.

---

# 10.11 Stored Baseline

Each zone stores:

* Average Flow
* Minimum Flow
* Maximum Flow
* Standard Deviation
* Confidence Score
* Learning Date

Historical learning data shall be retained for trend analysis.

---

# 10.12 Flow Verification

Whenever irrigation begins:

```text
Zone Opens

↓

Expected Flow

↓

Measured Flow

↓

Tolerance Check

↓

Pass / Fail
```

Default tolerance:

±15%

User configurable.

---

# 10.13 Leak Detection

Leak detection operates continuously.

Detection conditions include:

* Unexpected flow while idle
* Flow above learned maximum
* Continuous low flow
* Flow after valve closure
* Flow with Master Valve closed

Leak severity levels:

| Level    | Description         |
| -------- | ------------------- |
| Minor    | Small abnormal flow |
| Major    | Significant leak    |
| Critical | Pipe rupture        |

---

# 10.14 Dry Flow Detection

Dry flow occurs when:

Master Valve

ON

Zone Valve

ON

Measured Flow

≈0

Possible causes:

* Closed isolation valve
* Empty water source
* Blocked filter
* Broken solenoid
* Flow sensor failure

The controller shall stop irrigation and raise an alarm if the condition persists beyond the configured timeout.

---

# 10.15 Excess Flow Detection

Possible causes:

* Broken pipe
* Missing sprinkler
* Valve failure
* Hydraulic damage

Sequence:

```text
Measured Flow

>

Maximum Allowed

↓

Confirm

↓

Alarm

↓

Safe Shutdown
```

Confirmation prevents nuisance alarms caused by hydraulic transients.

---

# 10.16 Water Consumption

Statistics shall include:

Per Zone

* Daily
* Weekly
* Monthly
* Seasonal
* Lifetime

Per Program

* Total Water
* Average Flow
* Runtime

System

* Daily Total
* Monthly Total
* Annual Total
* Lifetime Total

---

# 10.17 Predictive Diagnostics

Flow trends shall be analysed.

Examples:

Increasing flow

Possible pipe degradation.

Decreasing flow

Possible clogged filter.

Flow oscillation

Possible unstable pressure.

Future firmware may generate maintenance recommendations.

---

# 10.18 MQTT Integration

Topics:

```text
zmartify/flow/current

zmartify/flow/total

zmartify/flow/history

zmartify/zone/01/flow

zmartify/zone/01/water

zmartify/alarm/leak
```

All payloads use JSON defined in Volume 3.

---

# 10.19 Event Publication

The Flow Manager publishes:

```text
EVT_FLOW_UPDATED

EVT_FLOW_HIGH

EVT_FLOW_LOW

EVT_FLOW_NONE

EVT_FLOW_LEAK

EVT_FLOW_LEARNED

EVT_FLOW_CALIBRATION

EVT_FLOW_SENSOR_FAULT
```

Subscribers include:

* Irrigation Engine
* Alarm Manager
* Diagnostics Manager
* MQTT Manager
* UI Manager

---

# 10.20 Historical Database

Minimum retained data:

| Data                  | Retention |
| --------------------- | --------: |
| Minute Flow           |  24 hours |
| Hourly Flow           |   90 days |
| Daily Water           |  Lifetime |
| Monthly Water         |  Lifetime |
| Flow Learning History |  Lifetime |

Historical data supports analytics and predictive maintenance.

---

# 10.21 User Interface

The Flow Dashboard shall display:

Current:

* Flow Rate
* Water Today
* Active Zone
* Expected Flow
* Measured Flow

Historical:

* Daily graph
* Weekly graph
* Monthly graph
* Seasonal graph

Diagnostics:

* Calibration
* Baseline
* Leak Status
* Sensor Health

---

# 10.22 Public API

Example interface.

```c
flow_manager_init();

flow_manager_start();

flow_manager_stop();

flow_manager_update();

flow_manager_get_current();

flow_manager_get_zone();

flow_manager_get_statistics();

flow_manager_start_learning();

flow_manager_store_learning();

flow_manager_calibrate();
```

Application components shall never access the HAL directly.

---

# 10.23 Diagnostics

Diagnostic parameters include:

* Sensor status
* Pulse frequency
* Flow stability
* Calibration version
* Last learning date
* Baseline confidence
* Measurement latency
* Communication errors

Diagnostics shall be accessible through:

* LVGL
* MQTT
* Diagnostics Manager

---

# 10.24 Unit Testing

Automated testing shall verify:

* Pulse counting
* Flow calculation
* Calibration
* Learning
* Leak detection
* Dry flow detection
* Excess flow detection
* Water statistics
* MQTT publication
* Error handling

Minimum code coverage:

**95%**

---

# 10.25 Future Enhancements

The architecture supports future capabilities including:

* Dual redundant flow sensors
* Bidirectional flow measurement
* High-resolution industrial flow meters
* Automatic self-calibration
* AI-assisted leak detection
* Water quality monitoring
* Smart water cost optimization
* Distributed hydraulic monitoring

The Flow Manager API shall remain stable as these features are introduced.

---

# 10.26 Engineering Notes

For the approved **DN50 G2 Hall-effect flow meter**, the firmware should support a configurable pulse constant rather than hard-coding a value, as manufacturers often specify different calibration factors.

Flow Learning shall automatically refine the effective calibration for each irrigation zone, while preserving the original factory calibration for traceability.

Flow measurements shall always be correlated with pressure measurements before any hydraulic fault is declared, minimizing false alarms caused by transient hydraulic conditions.

---

# 10.27 Chapter Summary

The Flow Manager is one of the key intelligence and safety components of the Zmartify Irrigation Controller.

By combining real-time pulse measurement, hydraulic baseline learning, leak detection, predictive diagnostics and long-term water accounting, it transforms a simple flow sensor into a comprehensive hydraulic monitoring system.

Together with the Pressure Manager, it forms the core of the Zmartify Hydraulic Safety System (ZHSS), ensuring safe, efficient and intelligent irrigation operation while providing detailed analytics for optimization and maintenance.

---

# End of Chapter 10

**Next Chapter**

**Chapter 11 – Pressure Manager Architecture**


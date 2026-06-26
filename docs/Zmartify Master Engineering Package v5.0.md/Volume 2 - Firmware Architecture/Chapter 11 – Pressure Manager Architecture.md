# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 11 – Pressure Manager Architecture

---

# 11 Pressure Manager

---

# 11.1 Purpose

The Pressure Manager is responsible for continuously measuring, validating and supervising the hydraulic pressure of the irrigation system.

Together with the Flow Manager, it forms the **Zmartify Hydraulic Safety System (ZHSS)**, providing continuous protection against hydraulic faults while also supplying valuable diagnostic information for predictive maintenance and system optimization.

The Pressure Manager shall continuously monitor pressure whenever the controller is powered, regardless of whether irrigation is active.

---

# 11.2 Design Objectives

The Pressure Manager shall:

* Measure system pressure
* Validate pressure measurements
* Learn hydraulic baselines
* Detect abnormal pressure
* Detect pressure collapse
* Detect overpressure
* Detect unstable pressure
* Support hydraulic diagnostics
* Publish pressure telemetry
* Support predictive maintenance

The Pressure Manager shall never directly operate valves or relays.

---

# 11.3 Architectural Position

```text
          Pressure Sensor
                │
                ▼
         ADS1115 (16-bit ADC)
                │
                ▼
        HAL Pressure Driver
                │
                ▼
         Pressure Manager
                │
      ┌─────────┼─────────┐
      ▼         ▼         ▼
 Irrigation  Alarm     MQTT
   Engine    Manager   Manager
```

The Pressure Manager shall access the pressure sensor exclusively through the HAL.

---

# 11.4 Approved Hardware

Approved sensor:

Industrial Pressure Transmitter

Specifications:

| Parameter |              Value |
| --------- | -----------------: |
| Range     |           0–10 bar |
| Output    |        0.5–4.5 VDC |
| Accuracy  | ±0.5% FS (minimum) |
| Supply    |              5 VDC |
| Interface |            ADS1115 |

ADC:

ADS1115

* 16-bit resolution
* I²C interface
* Programmable gain
* Differential measurement capability

---

# 11.5 Measurement Philosophy

Pressure shall be treated as a continuously changing process variable.

Measurements shall be used for:

* Hydraulic verification
* Leak detection support
* Valve diagnostics
* Pump diagnostics (future)
* Flow correlation
* Predictive maintenance

The controller shall not rely on single pressure samples.

---

# 11.6 Measurement Cycle

Pressure acquisition sequence:

```text
Acquire ADC

↓

Convert Voltage

↓

Apply Calibration

↓

Digital Filtering

↓

Pressure (bar)

↓

Diagnostics

↓

Publish Event
```

Default update interval:

250 ms

---

# 11.7 Pressure Calibration

Calibration shall support:

Factory calibration

User calibration

Service calibration

Calibration consists of:

* Zero offset
* Span adjustment
* Sensor scaling

Calibration shall be stored in non-volatile memory.

---

# 11.8 Pressure Filtering

Measurements shall pass through multiple filtering stages.

Recommended filters:

* Moving average
* Median filter
* Spike rejection
* Low-pass filter

Filtering shall reduce sensor noise without masking genuine hydraulic events.

---

# 11.9 Pressure Learning

The controller shall automatically learn the normal operating pressure for each irrigation zone.

Learning sequence:

```text
Open Master Valve

↓

Stabilize

↓

Open Zone

↓

Wait

↓

Measure

↓

Average

↓

Store Baseline
```

Learning shall record:

* Average pressure
* Minimum pressure
* Maximum pressure
* Standard deviation
* Confidence level

---

# 11.10 Pressure Verification

Every irrigation cycle shall include pressure verification.

Sequence:

```text
Master Valve ON

↓

Pressure Rise?

↓

Zone Valve ON

↓

Pressure Stable?

↓

Continue Irrigation
```

Failure shall generate a hydraulic alarm.

---

# 11.11 Pressure States

Each zone maintains its own pressure state.

```text
Unknown

↓

Learning

↓

Normal

↓

Low

↓

High

↓

Critical
```

Transitions generate Event Bus notifications.

---

# 11.12 Pressure Collapse Detection

Pressure collapse indicates major hydraulic failure.

Possible causes:

* Pipe burst
* Empty water source
* Pump failure
* Main valve failure
* Broken fitting

Detection logic:

```text
Pressure

↓

Below Minimum

↓

Confirmation Timer

↓

Critical Alarm

↓

Shutdown
```

Default confirmation:

2 seconds

---

# 11.13 Overpressure Detection

Possible causes:

* Closed valve
* Pump malfunction
* Water hammer
* Incorrect regulator

Sequence:

```text
Pressure

>

Maximum

↓

Verify

↓

Warning

↓

Critical

↓

Shutdown
```

---

# 11.14 Pressure Oscillation

Rapid pressure variation may indicate:

* Valve chatter
* Air in pipework
* Pump instability
* Faulty pressure regulator

Oscillation shall be detected using rolling statistical analysis.

Future firmware may classify oscillation patterns automatically.

---

# 11.15 Pressure Baseline Database

Each irrigation zone stores:

* Average Pressure
* Minimum Pressure
* Maximum Pressure
* Pressure Variance
* Learning Date
* Confidence Score

These values form the hydraulic fingerprint of the zone.

---

# 11.16 Correlation with Flow

Pressure shall always be evaluated together with flow.

Examples:

| Flow        |    Pressure | Interpretation             |
| ----------- | ----------: | -------------------------- |
| Normal      |      Normal | Healthy                    |
| High        |         Low | Leak                       |
| Zero        |        High | Closed Valve               |
| Zero        |         Low | Water Supply Failure       |
| High        |        High | Sensor Fault / Restriction |
| Oscillating | Oscillating | Hydraulic Instability      |

Hydraulic decisions shall consider both parameters before generating Critical alarms whenever practical.

---

# 11.17 MQTT Integration

Topics:

```text
zmartify/pressure/current

zmartify/pressure/history

zmartify/pressure/status

zmartify/zone/01/pressure

zmartify/alarm/pressure
```

All payloads use JSON defined in Volume 3.

---

# 11.18 Published Events

The Pressure Manager publishes:

```text
EVT_PRESS_UPDATED

EVT_PRESS_NORMAL

EVT_PRESS_LOW

EVT_PRESS_HIGH

EVT_PRESS_COLLAPSE

EVT_PRESS_OSCILLATION

EVT_PRESS_SENSOR_FAULT

EVT_PRESS_LEARNED
```

Subscribers:

* Irrigation Engine
* Alarm Manager
* MQTT Manager
* Diagnostics Manager
* UI Manager

---

# 11.19 Historical Database

Minimum retained information:

| Data             | Retention |
| ---------------- | --------: |
| Minute Pressure  |  24 hours |
| Hourly Average   |   90 days |
| Daily Average    |  Lifetime |
| Learning History |  Lifetime |
| Alarm History    |  Lifetime |

---

# 11.20 User Interface

Pressure Dashboard:

Current:

* Pressure
* Zone
* Baseline
* Status

Historical:

* Pressure graph
* Pressure trends
* Daily average
* Weekly average

Diagnostics:

* Calibration
* Sensor health
* Stability index
* Learning confidence

The dashboard shall support synchronized display with Flow graphs to simplify hydraulic troubleshooting.

---

# 11.21 Public API

Example interface.

```c
pressure_manager_init();

pressure_manager_start();

pressure_manager_stop();

pressure_manager_update();

pressure_manager_get_current();

pressure_manager_get_zone();

pressure_manager_start_learning();

pressure_manager_store_learning();

pressure_manager_calibrate();

pressure_manager_status();
```

The Irrigation Engine shall obtain pressure information exclusively through this API.

---

# 11.22 Diagnostics

Diagnostic parameters include:

* Sensor status
* ADC communication
* I²C health
* Calibration version
* Pressure stability
* Baseline confidence
* Last learning date
* Measurement latency
* Sensor drift estimate

Diagnostics shall be available through:

* LVGL
* MQTT
* Diagnostics Manager

---

# 11.23 Unit Testing

Automated tests shall verify:

* ADC acquisition
* Voltage conversion
* Calibration
* Filtering
* Pressure Learning
* Low-pressure detection
* Overpressure detection
* Oscillation detection
* Historical storage
* MQTT publication
* Error recovery

Minimum code coverage:

**95%**

---

# 11.24 Future Enhancements

The architecture supports:

* Dual redundant pressure sensors
* Pressure sensors on multiple hydraulic branches
* Pump suction pressure monitoring
* Automatic pressure regulator tuning
* AI-assisted hydraulic diagnostics
* Water hammer detection
* Smart pump control
* Reservoir pressure monitoring

The public API shall remain stable as new capabilities are introduced.

---

# 11.25 Engineering Notes

The approved 0–10 bar pressure transmitter connected through the ADS1115 provides substantially higher measurement stability than the ESP32's internal ADC.

Pressure Learning shall establish a hydraulic fingerprint for each irrigation zone. Combined with Flow Learning, the controller will continuously refine its understanding of the irrigation network, allowing increasingly accurate fault detection over time.

The Pressure Manager and Flow Manager together constitute the **Zmartify Hydraulic Safety System (ZHSS)**. Neither subsystem should independently trigger a catastrophic shutdown unless the condition is unequivocal (for example, complete loss of pressure with the Master Valve open). In most cases, hydraulic decisions shall be based on correlated pressure and flow analysis to minimize false positives.

---

# 11.26 Chapter Summary

The Pressure Manager provides continuous supervision of the hydraulic health of the irrigation system.

By combining high-resolution pressure measurement, adaptive baseline learning, intelligent diagnostics and close integration with the Flow Manager, it enables the controller to detect leaks, blockages, supply failures and hydraulic degradation long before they become serious problems.

This subsystem is fundamental to the Zmartify philosophy of creating an autonomous, self-monitoring irrigation controller that not only waters efficiently but also continuously protects itself and the irrigation infrastructure.

---

# End of Chapter 11

**Next Chapter**

**Chapter 12 – Alarm Manager Architecture**

# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 6

# MQTT Topic Specification – Flow, Pressure & Hydraulic Monitoring

---

# 6.1 Purpose

This chapter specifies the MQTT interface for the **Zmartify Hydraulic Safety System (ZHSS)**.

The Hydraulic Safety System combines the functionality of:

* Flow Manager
* Pressure Manager
* Irrigation Engine
* Alarm Manager
* Diagnostics Manager

to provide continuous monitoring of the irrigation system's hydraulic performance.

Unlike conventional irrigation controllers that only report whether a valve is on or off, Zmartify continuously publishes engineering-grade hydraulic telemetry and diagnostics, enabling predictive maintenance, leak detection and advanced analytics.

---

# 6.2 Design Objectives

The Hydraulic namespace shall:

* Publish real-time flow
* Publish real-time pressure
* Publish hydraulic status
* Publish learned hydraulic baselines
* Detect hydraulic faults
* Support predictive maintenance
* Support engineering diagnostics
* Support dashboard visualization
* Remain independent of specific sensor manufacturers

---

# 6.3 Namespace

Root namespaces:

```text
zmartify/flow/

zmartify/pressure/
```

Associated namespaces:

```text
zmartify/zones/<zone>/flow

zmartify/zones/<zone>/pressure

zmartify/zones/<zone>/hydraulics

zmartify/hydraulics/

zmartify/alarms/
```

---

# 6.4 Hydraulic Architecture

```text
             Flow Sensor

                  │

             Flow Manager

                  │

                  ▼

            Hydraulic Model

                  ▲

                  │

         Pressure Manager

                  │

          Pressure Sensor

                  │

                  ▼

         MQTT Manager

                  │

                  ▼

             MQTT Broker

                  │

      HOMEIO • Home Assistant • Homey
```

The MQTT layer publishes processed engineering information rather than raw sensor data wherever practical.

---

# 6.5 Current Flow

Topic

```text
zmartify/flow/current
```

QoS

1

Retained

Yes

Published

Every second while irrigation is active.

Every 30 seconds while idle.

---

Example

```json
{
  "timestamp":"2026-07-14T18:12:24Z",
  "device":"ZIC-S3-202600001",
  "type":"flow_current",
  "payload":
  {
      "flow":23.8,
      "unit":"L/min",
      "status":"Normal"
  }
}
```

---

# 6.6 Flow Statistics

Topic

```text
zmartify/flow/statistics
```

Example

```json
{
  "payload":
  {
      "today":3845,
      "week":22860,
      "month":96844,
      "year":862300,
      "lifetime":1843245
  }
}
```

Units

Litres

---

# 6.7 Flow Learning

Topic

```text
zmartify/flow/learning
```

Published whenever Flow Learning completes.

Example

```json
{
  "payload":
  {
      "zone":4,
      "average":24.3,
      "minimum":23.8,
      "maximum":25.1,
      "confidence":98,
      "date":"2026-07-14"
  }
}
```

---

# 6.8 Flow Calibration

Topic

```text
zmartify/flow/calibration
```

Example

```json
{
  "payload":
  {
      "sensor":"DN50 G2",
      "pulses_per_litre":450,
      "calibration_version":"2.0",
      "last_calibrated":"2026-06-20"
  }
}
```

Calibration data shall only be modifiable by authorized users.

---

# 6.9 Current Pressure

Topic

```text
zmartify/pressure/current
```

QoS

1

Retained

Yes

Published every second during irrigation.

---

Example

```json
{
  "payload":
  {
      "pressure":3.46,
      "unit":"bar",
      "status":"Normal"
  }
}
```

---

# 6.10 Pressure Learning

Topic

```text
zmartify/pressure/learning
```

Example

```json
{
  "payload":
  {
      "zone":4,
      "average":3.48,
      "minimum":3.32,
      "maximum":3.60,
      "confidence":96
  }
}
```

---

# 6.11 Pressure Calibration

Topic

```text
zmartify/pressure/calibration
```

Example

```json
{
  "payload":
  {
      "sensor":"0-10 bar",
      "offset":0.02,
      "span":10.00,
      "adc":"ADS1115"
  }
}
```

---

# 6.12 Zone Flow

Example topic

```text
zmartify/zones/04/flow
```

Example

```json
{
  "payload":
  {
      "expected":24.5,
      "measured":24.2,
      "difference":-1.2,
      "status":"Normal"
  }
}
```

Difference

Percent

---

# 6.13 Zone Pressure

Topic

```text
zmartify/zones/04/pressure
```

Example

```json
{
  "payload":
  {
      "expected":3.50,
      "measured":3.42,
      "difference":-2.3,
      "status":"Normal"
  }
}
```

---

# 6.14 Hydraulic Health

Topic

```text
zmartify/hydraulics/health
```

Example

```json
{
  "payload":
  {
      "health_score":97,
      "status":"Excellent",
      "leak_probability":0,
      "restriction_probability":2,
      "sensor_confidence":99
  }
}
```

---

# 6.15 Hydraulic Status

Topic

```text
zmartify/hydraulics/status
```

Example

```json
{
  "payload":
  {
      "state":"Healthy",
      "master_valve":"Open",
      "active_zone":4,
      "flow":"Normal",
      "pressure":"Normal"
  }
}
```

---

# 6.16 Hydraulic Events

Topic

```text
zmartify/hydraulics/event
```

Examples

```json
{
  "payload":
  {
      "event":"Flow Learning Completed",
      "zone":4
  }
}
```

```json
{
  "payload":
  {
      "event":"Pressure Learning Completed",
      "zone":4
  }
}
```

```json
{
  "payload":
  {
      "event":"Leak Suspected",
      "severity":"Warning"
  }
}
```

Events are not retained.

---

# 6.17 Hydraulic Alarms

The Alarm Manager publishes hydraulic alarms under:

```text
zmartify/alarms/
```

Hydraulic alarm types include:

* Leak Detected
* Dry Flow
* Excess Flow
* Pressure Collapse
* Overpressure
* Hydraulic Instability
* Sensor Failure
* Calibration Error

Each alarm includes severity, timestamp, affected zone and recommended action.

---

# 6.18 Publish Rates

| Topic             |     Interval |
| ----------------- | -----------: |
| flow/current      | 1 s (active) |
| pressure/current  | 1 s (active) |
| hydraulics/status |          5 s |
| hydraulics/health |         30 s |
| flow/statistics   |         60 s |
| pressure/learning |        Event |
| flow/learning     |        Event |
| hydraulic events  |        Event |

---

# 6.19 QoS Policy

| Topic             | QoS | Retained |
| ----------------- | :-: | :------: |
| flow/current      |  1  |    Yes   |
| pressure/current  |  1  |    Yes   |
| hydraulics/status |  1  |    Yes   |
| hydraulics/health |  1  |    Yes   |
| learning          |  1  |    Yes   |
| events            |  0  |    No    |
| alarms            |  2  |    No    |

Critical hydraulic alarms shall always use **QoS 2**.

---

# 6.20 Error Handling

If a sensor becomes unavailable, the payload shall explicitly indicate the fault.

Example

```json
{
  "payload":
  {
      "status":"Sensor Fault",
      "sensor":"Flow",
      "last_valid":"2026-07-14T18:10:42Z"
  }
}
```

Consumers shall treat such messages as diagnostic information rather than valid measurements.

---

# 6.21 Engineering Notes

The hydraulic MQTT interface is one of the defining characteristics of the Zmartify platform.

Rather than publishing only raw sensor values, the controller exposes **interpreted hydraulic intelligence**, including learned baselines, health scores and anomaly probabilities. This significantly reduces the complexity of client applications while enabling sophisticated monitoring and predictive maintenance.

The combination of Flow Learning and Pressure Learning allows external systems to visualize hydraulic performance over time without needing knowledge of the controller's internal algorithms.

---

# 6.22 Chapter Summary

This chapter defines the MQTT interface for hydraulic monitoring, including flow, pressure, learned baselines and hydraulic health.

By publishing both measured values and processed engineering information, the interface supports advanced diagnostics, smart-home integration and long-term analytics while maintaining strict separation between measurement, interpretation and control.

---

# End of Chapter 6

**Next Chapter**

**Chapter 7 – MQTT Topic Specification: Alarm, Diagnostics & System Health**

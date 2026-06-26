# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 7

# MQTT Topic Specification – Alarm, Diagnostics & System Health

---

# 7.1 Purpose

This chapter defines the MQTT interface used by the Alarm Manager and Diagnostics Manager to report controller health, operational status and abnormal conditions.

The Alarm and Diagnostics interfaces are intended for:

* Real-time monitoring
* Service technicians
* Smart-home platforms
* Predictive maintenance
* Historical analysis
* Fleet management
* Engineering diagnostics

Unlike irrigation telemetry, alarm messages represent exceptional conditions requiring attention, while diagnostics provide continuous insight into controller performance and health.

---

# 7.2 Design Objectives

The Alarm and Diagnostics namespaces shall:

* Report abnormal conditions immediately
* Publish system health continuously
* Distinguish between alarms and diagnostics
* Support alarm acknowledgement
* Support historical analysis
* Minimize network traffic
* Provide deterministic alarm delivery
* Remain extensible for future hardware

---

# 7.3 Namespace

Alarm namespace:

```text
zmartify/alarms/
```

Diagnostics namespace:

```text
zmartify/diagnostics/
```

System namespace:

```text
zmartify/system/
```

Primary topics:

```text
current
active
history
statistics
health
cpu
memory
storage
network
wifi
mqtt
tasks
display
i2c
selftest
event
```

---

# 7.4 Alarm Philosophy

The Alarm Manager shall only publish alarms representing genuine abnormal operating conditions.

Informational events shall not be published as alarms.

Alarm categories include:

* Hydraulic
* Electrical
* Communication
* Weather
* Configuration
* Hardware
* Firmware
* Security

---

# 7.5 Alarm Severity Levels

All alarms shall include a severity field.

| Severity    | Description                      |
| ----------- | -------------------------------- |
| Information | Informational only               |
| Warning     | User attention recommended       |
| Critical    | Immediate attention required     |
| Emergency   | Irrigation stopped automatically |

Severity shall remain consistent across firmware revisions.

---

# 7.6 Active Alarm

Topic

```text
zmartify/alarms/current
```

QoS

```text
2
```

Retained

```text
Yes
```

Published immediately whenever the active alarm state changes.

---

Example

```json
{
  "timestamp":"2026-07-14T18:42:00Z",
  "device":"ZIC-S3-202600001",
  "type":"alarm",
  "payload":
  {
      "id":102,
      "severity":"Critical",
      "category":"Hydraulic",
      "code":"FLOW_HIGH",
      "title":"Excessive Flow",
      "zone":4,
      "message":"Measured flow exceeds learned baseline.",
      "recommended_action":"Inspect zone for broken pipe or sprinkler.",
      "active":true
  }
}
```

---

# 7.7 Active Alarm List

Topic

```text
zmartify/alarms/active
```

Purpose

Publishes every currently active alarm.

Example

```json
{
  "payload":
  [
      {
          "id":102,
          "severity":"Critical",
          "title":"Excessive Flow"
      },
      {
          "id":205,
          "severity":"Warning",
          "title":"Low Wi-Fi Signal"
      }
  ]
}
```

---

# 7.8 Alarm History

Topic

```text
zmartify/alarms/history
```

Purpose

Provides historical alarm records.

Example

```json
{
  "payload":
  [
      {
          "timestamp":"2026-07-14T18:32:12Z",
          "severity":"Warning",
          "title":"Low Pressure",
          "duration":84
      }
  ]
}
```

History size is configurable.

Default:

**1,000 entries**

---

# 7.9 Alarm Statistics

Topic

```text
zmartify/alarms/statistics
```

Example

```json
{
  "payload":
  {
      "today":3,
      "week":8,
      "month":14,
      "critical":1,
      "warning":11,
      "information":2
  }
}
```

---

# 7.10 Alarm Categories

The Alarm Manager shall classify alarms according to the following categories.

| Category   | Examples                        |
| ---------- | ------------------------------- |
| Hydraulic  | Leak, Excess Flow, Low Pressure |
| Electrical | Relay Failure, Power Loss       |
| Sensor     | Flow Sensor Failure             |
| Network    | Wi-Fi Lost, MQTT Disconnected   |
| Storage    | Filesystem Corruption           |
| Firmware   | Watchdog Reset                  |
| Weather    | Freeze Warning                  |
| Security   | Authentication Failure          |

---

# 7.11 Diagnostics Overview

Diagnostics are published under:

```text
zmartify/diagnostics/
```

Unlike alarms, diagnostics describe the operational condition of the controller and are expected to change continuously.

---

# 7.12 System Health

Topic

```text
zmartify/diagnostics/health
```

Published

Every 60 seconds.

Example

```json
{
  "payload":
  {
      "health_score":98,
      "status":"Excellent",
      "uptime_hours":312,
      "alarms_active":0,
      "warnings":1
  }
}
```

Health Score

| Score  | Meaning   |
| ------ | --------- |
| 95–100 | Excellent |
| 85–94  | Good      |
| 70–84  | Attention |
| 50–69  | Poor      |
| <50    | Critical  |

---

# 7.13 CPU Diagnostics

Topic

```text
zmartify/diagnostics/cpu
```

Example

```json
{
  "payload":
  {
      "usage_percent":24,
      "idle_percent":76,
      "temperature":46.8,
      "tasks":18
  }
}
```

Published every 60 seconds.

---

# 7.14 Memory Diagnostics

Topic

```text
zmartify/diagnostics/memory
```

Example

```json
{
  "payload":
  {
      "heap_free":482364,
      "minimum_heap":446212,
      "largest_block":318944,
      "fragmentation_percent":5
  }
}
```

---

# 7.15 Storage Diagnostics

Topic

```text
zmartify/diagnostics/storage
```

Example

```json
{
  "payload":
  {
      "filesystem":"LittleFS",
      "used_percent":31,
      "free_bytes":2818048,
      "wear_level":"Normal",
      "errors":0
  }
}
```

---

# 7.16 Network Diagnostics

Topic

```text
zmartify/diagnostics/network
```

Example

```json
{
  "payload":
  {
      "wifi":"Connected",
      "mqtt":"Connected",
      "ntp":"Synchronized",
      "latency_ms":18
  }
}
```

---

# 7.17 Wi-Fi Diagnostics

Topic

```text
zmartify/diagnostics/wifi
```

Example

```json
{
  "payload":
  {
      "ssid":"HomeNetwork",
      "rssi":-52,
      "quality":"Excellent",
      "reconnects":2
  }
}
```

---

# 7.18 MQTT Diagnostics

Topic

```text
zmartify/diagnostics/mqtt
```

Example

```json
{
  "payload":
  {
      "broker":"Mosquitto",
      "connected":true,
      "latency_ms":12,
      "messages_sent":28462,
      "messages_received":892
  }
}
```

---

# 7.19 Task Diagnostics

Topic

```text
zmartify/diagnostics/tasks
```

Purpose

Summarizes FreeRTOS task execution.

Example

```json
{
  "payload":
  [
      {
          "task":"FlowManager",
          "cpu":4,
          "stack_free":2848
      },
      {
          "task":"MQTTManager",
          "cpu":2,
          "stack_free":3100
      }
  ]
}
```

Detailed per-task diagnostics are intended primarily for engineering tools.

---

# 7.20 I²C Diagnostics

Topic

```text
zmartify/diagnostics/i2c
```

Example

```json
{
  "payload":
  {
      "devices":
      [
          "MCP23017",
          "ADS1115",
          "GT911"
      ],
      "bus_errors":0
  }
}
```

---

# 7.21 Display Diagnostics

Topic

```text
zmartify/diagnostics/display
```

Example

```json
{
  "payload":
  {
      "display":"Online",
      "touch":"Online",
      "brightness":70,
      "sleep":false
  }
}
```

---

# 7.22 Self-Test Results

Topic

```text
zmartify/diagnostics/selftest
```

Published after boot or manual self-test.

Example

```json
{
  "payload":
  {
      "result":"PASS",
      "tests":18,
      "failed":0,
      "duration_ms":1284
  }
}
```

---

# 7.23 Diagnostic Events

Topic

```text
zmartify/diagnostics/event
```

Example

```json
{
  "payload":
  {
      "event":"Flow Learning Completed",
      "zone":7,
      "confidence":97
  }
}
```

Diagnostic events are not retained.

---

# 7.24 Publish Rates

| Topic              |            Interval |
| ------------------ | ------------------: |
| alarms/current     |               Event |
| alarms/active      |               Event |
| alarms/history     | On request / change |
| diagnostics/health |                60 s |
| cpu                |                60 s |
| memory             |                60 s |
| storage            |               5 min |
| network            |                60 s |
| wifi               |                60 s |
| mqtt               |                60 s |
| tasks              |               5 min |
| selftest           |       Boot / Manual |

---

# 7.25 QoS Policy

| Topic              | QoS | Retained |
| ------------------ | :-: | :------: |
| alarms/current     |  2  |    Yes   |
| alarms/active      |  2  |    Yes   |
| alarms/history     |  1  |    No    |
| diagnostics/health |  1  |    Yes   |
| cpu                |  0  |    No    |
| memory             |  0  |    No    |
| storage            |  0  |    No    |
| selftest           |  1  |    Yes   |

Emergency alarms shall always use **QoS 2** to ensure reliable delivery.

---

# 7.26 Error Handling

Diagnostic payloads shall explicitly report unavailable subsystems.

Example

```json
{
  "payload":
  {
      "status":"Unavailable",
      "reason":"Sensor Timeout",
      "last_valid":"2026-07-14T18:40:12Z"
  }
}
```

Consumers shall distinguish between missing data and reported subsystem failures.

---

# 7.27 Engineering Notes

The Alarm and Diagnostics interfaces are deliberately separated to distinguish **fault conditions** from **operational insight**.

Alarms are intended to trigger immediate action by users or automation systems, whereas diagnostics provide continuous visibility into controller performance and long-term health. This separation simplifies automation logic and aligns with established practices in industrial control systems.

The use of structured alarm codes, severity levels and recommended actions also enables supervisory software to present meaningful information without requiring knowledge of internal firmware implementation.

---

# 7.28 Chapter Summary

This chapter defines the MQTT interface for alarms, diagnostics and system health.

Together, these namespaces provide comprehensive visibility into the operational state of the Zmartify Irrigation Controller, enabling real-time monitoring, preventive maintenance and reliable integration with smart-home platforms, dashboards and future fleet management systems.

---

# End of Chapter 7

**Next Chapter**

**Chapter 8 – MQTT Topic Specification: Configuration, OTA & Controller Management**

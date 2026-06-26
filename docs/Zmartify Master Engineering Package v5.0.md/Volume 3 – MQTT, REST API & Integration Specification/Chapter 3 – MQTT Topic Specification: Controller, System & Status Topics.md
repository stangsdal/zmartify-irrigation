# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 3

# MQTT Topic Specification – Controller, System & Status Topics

---

# 3.1 Purpose

This chapter specifies the MQTT topics that expose the operational state of the Zmartify Irrigation Controller.

These topics represent the highest level of the MQTT hierarchy and allow external systems to determine the controller's identity, health, firmware version, operating mode and current availability.

Unlike telemetry topics, these topics describe the controller itself rather than irrigation activities.

---

# 3.2 Design Objectives

The Controller namespace shall provide:

* Device identification
* Firmware identification
* Hardware identification
* Current controller state
* Operational status
* Health information
* Boot information
* Runtime statistics
* Heartbeat monitoring
* Network identity

The information shall allow third-party systems to integrate without requiring additional configuration.

---

# 3.3 Namespace

Root namespace:

```text
zmartify/controller/
```

Primary topics:

```text
status
identity
hardware
firmware
version
uptime
heartbeat
network
capabilities
reboot
```

---

# 3.4 Controller Status

Topic

```text
zmartify/controller/status
```

QoS

```
1
```

Retained

```
Yes
```

Published

* At boot
* Every 30 seconds
* Whenever state changes

---

Example

```json
{
  "timestamp":"2026-07-14T12:30:45Z",
  "device":"ZIC-S3-202600001",
  "firmware":"5.0.0",
  "type":"controller_status",
  "payload":
  {
      "state":"idle",
      "online":true,
      "healthy":true,
      "active_program":null,
      "active_zone":null,
      "alarm":false
  }
}
```

---

Allowed controller states

| Value       | Description         |
| ----------- | ------------------- |
| booting     | Firmware starting   |
| idle        | Ready               |
| irrigating  | Irrigation active   |
| paused      | Program paused      |
| rain_delay  | Irrigation delayed  |
| manual      | Manual watering     |
| alarm       | Alarm active        |
| maintenance | Service mode        |
| ota         | Firmware update     |
| shutdown    | Controlled shutdown |

---

# 3.5 Controller Identity

Topic

```text
zmartify/controller/identity
```

Published:

* Boot
* Configuration change

Retained

```
Yes
```

---

Example

```json
{
    "payload":
    {
        "device":"ZIC-S3-202600001",
        "name":"Garden Controller",
        "product":"Zmartify Irrigation Controller",
        "manufacturer":"Zmartify",
        "serial":"202600001"
    }
}
```

---

# 3.6 Hardware Information

Topic

```text
zmartify/controller/hardware
```

Example

```json
{
  "payload":
  {
      "board":"ESP32-S3",
      "display":"Waveshare 7 IPS",
      "relay_board":"HL-58S V1.2",
      "relay_outputs":16,
      "io_expander":"MCP23017",
      "adc":"ADS1115",
      "firmware_partition":"OTA_0",
      "pcb_revision":"RevB"
  }
}
```

Future hardware revisions may extend this payload.

---

# 3.7 Firmware Information

Topic

```text
zmartify/controller/firmware
```

Retained

```
Yes
```

---

Example

```json
{
  "payload":
  {
      "version":"5.0.0",
      "build":"2026.07.14",
      "branch":"main",
      "commit":"9f4b3c2",
      "compiler":"ESP-IDF 5.3",
      "build_type":"Release"
  }
}
```

---

# 3.8 Capabilities

Topic

```text
zmartify/controller/capabilities
```

Purpose

Allows clients to dynamically determine supported controller features.

Example

```json
{
  "payload":
  {
      "zones":15,
      "master_valves":1,
      "flow_sensor":true,
      "pressure_sensor":true,
      "weather":true,
      "et":true,
      "mqtt":true,
      "ota":true,
      "lvgl":true,
      "homeassistant_discovery":true,
      "homeio":true
  }
}
```

Future firmware shall only extend this payload.

---

# 3.9 Uptime

Topic

```text
zmartify/controller/uptime
```

Publication interval

60 seconds

Example

```json
{
  "payload":
  {
      "uptime_seconds":182345,
      "boot_count":18,
      "last_boot":"2026-07-13T08:42:00Z"
  }
}
```

---

# 3.10 Heartbeat

Topic

```text
zmartify/controller/heartbeat
```

Purpose

Provides a lightweight indication that the controller is operational.

Published every:

```
30 seconds
```

Example

```json
{
    "payload":
    {
        "alive":true
    }
}
```

The heartbeat should remain intentionally small to minimize network traffic.

---

# 3.11 Network Information

Topic

```text
zmartify/controller/network
```

Example

```json
{
  "payload":
  {
      "hostname":"zmartify-controller",
      "ip":"192.168.1.40",
      "mac":"84:F3:EB:12:45:91",
      "wifi_rssi":-54,
      "ssid":"HomeNetwork",
      "mqtt_connected":true,
      "ntp_synced":true
  }
}
```

Sensitive information such as Wi-Fi passwords shall never be published.

---

# 3.12 Restart Notification

Topic

```text
zmartify/controller/reboot
```

Published

Only when a reboot occurs.

Example

```json
{
  "payload":
  {
      "reason":"OTA Update",
      "requested_by":"Administrator"
  }
}
```

Possible reasons include:

* Power On
* OTA Update
* User Request
* Watchdog Recovery
* Brownout Recovery
* Firmware Panic
* Factory Reset

---

# 3.13 Controller Health

Topic

```text
zmartify/system/health
```

Published every:

60 seconds

Example

```json
{
  "payload":
  {
      "health_score":98,
      "status":"excellent",
      "cpu":28,
      "memory":42,
      "storage":21,
      "wifi":"good"
  }
}
```

Health Score definitions:

| Score  | Interpretation |
| ------ | -------------- |
| 95–100 | Excellent      |
| 85–94  | Good           |
| 70–84  | Attention      |
| 50–69  | Poor           |
| <50    | Critical       |

---

# 3.14 System State

Topic

```text
zmartify/system/state
```

Example

```json
{
  "payload":
  {
      "controller":"idle",
      "irrigation":"inactive",
      "weather":"normal",
      "alarms":0,
      "manual_override":false
  }
}
```

---

# 3.15 CPU Status

Topic

```text
zmartify/system/cpu
```

Example

```json
{
  "payload":
  {
      "usage":24,
      "temperature":46.2,
      "idle":76,
      "tasks":18
  }
}
```

CPU temperature is optional and hardware-dependent.

---

# 3.16 Memory Status

Topic

```text
zmartify/system/memory
```

Example

```json
{
  "payload":
  {
      "heap_free":483212,
      "heap_minimum":442108,
      "largest_block":318944,
      "fragmentation":6
  }
}
```

Values are expressed in bytes unless otherwise specified.

---

# 3.17 Time Information

Topic

```text
zmartify/system/time
```

Example

```json
{
  "payload":
  {
      "utc":"2026-07-14T12:42:00Z",
      "timezone":"Europe/Copenhagen",
      "ntp":true,
      "drift_ms":12
  }
}
```

The controller always publishes UTC timestamps. The configured timezone is provided as informational metadata.

---

# 3.18 Last Will & Testament

Topic

```text
zmartify/controller/status
```

Offline payload

```json
{
  "payload":
  {
      "online":false
  }
}
```

Online payload

```json
{
  "payload":
  {
      "online":true
  }
}
```

Clients should subscribe to this topic to detect controller availability.

---

# 3.19 Publish Rates

| Topic        |     Interval |
| ------------ | -----------: |
| status       | Event + 30 s |
| heartbeat    |         30 s |
| uptime       |         60 s |
| network      |         60 s |
| health       |         60 s |
| cpu          |         60 s |
| memory       |         60 s |
| firmware     |         Boot |
| hardware     |         Boot |
| capabilities |         Boot |

These intervals are configurable through the Configuration Manager.

---

# 3.20 MQTT QoS Policy

| Topic        | QoS | Retained |
| ------------ | :-: | :------: |
| status       |  1  |    Yes   |
| firmware     |  1  |    Yes   |
| hardware     |  1  |    Yes   |
| capabilities |  1  |    Yes   |
| heartbeat    |  0  |    No    |
| uptime       |  0  |    No    |
| cpu          |  0  |    No    |
| memory       |  0  |    No    |
| reboot       |  1  |    No    |

---

# 3.21 Error Handling

If information cannot be obtained, the payload shall contain an explicit status rather than omitting the field.

Example

```json
{
  "payload":
  {
      "mqtt_connected":false,
      "status":"disconnected"
  }
}
```

Null values should be avoided unless the value is genuinely unknown.

---

# 3.22 Engineering Notes

The Controller and System namespaces provide the "digital identity" of the Zmartify Irrigation Controller.

By separating controller metadata from irrigation telemetry, client applications can quickly determine controller health and capabilities before subscribing to more detailed operational topics. This separation also simplifies multi-controller installations where supervisory software must first discover and classify available devices.

The published capabilities payload is especially valuable for future hardware revisions, allowing external applications to adapt automatically to different controller configurations without requiring firmware-specific logic.

---

# 3.23 Chapter Summary

This chapter defines the MQTT topics that describe the identity, status and operational health of the Zmartify Irrigation Controller.

These topics form the foundation for all external integrations by providing a consistent, self-describing interface for controller discovery, health monitoring and system management. Subsequent chapters build upon this foundation by defining telemetry, command and event topics for irrigation, weather, hydraulics and diagnostics.

---

# End of Chapter 3

**Next Chapter**

**Chapter 4 – MQTT Topic Specification: Weather, ET & Environmental Data**

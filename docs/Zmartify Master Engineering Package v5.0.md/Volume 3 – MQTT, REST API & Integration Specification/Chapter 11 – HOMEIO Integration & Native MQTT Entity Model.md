# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 11

# HOMEIO Integration & Native MQTT Entity Model

---

# 11.1 Purpose

This chapter defines the native integration between the Zmartify Irrigation Controller and **HOMEIO**, which is the primary smart-home platform targeted by the Zmartify project.

Unlike many irrigation controllers that simply expose a collection of MQTT topics, the Zmartify platform is designed to behave as a first-class HOMEIO device with automatic discovery, engineering-grade telemetry and bidirectional command support.

The HOMEIO integration shall expose every significant controller capability while preserving the controller's autonomous operation and internal safety mechanisms.

---

# 11.2 Design Objectives

The HOMEIO integration shall:

* Support automatic discovery
* Require minimal user configuration
* Expose engineering telemetry
* Support bidirectional communication
* Remain MQTT-native
* Support multiple controllers
* Preserve firmware independence
* Support future Zmartify products

---

# 11.3 Integration Philosophy

HOMEIO is considered the reference smart-home platform for the Zmartify ecosystem.

The controller therefore exposes its functionality using engineering entities rather than generic MQTT topics wherever possible.

The integration shall provide:

* Automatic entity creation
* Automatic updates
* Automatic reconnection
* Real-time event handling
* Historical telemetry
* Diagnostic visibility

No custom firmware modifications shall be required.

---

# 11.4 Integration Architecture

```text
                 Zmartify Controller

                        │

                 Internal Event Bus

                        │

                 MQTT Manager

                        │

                 MQTT Broker

                        │

                    HOMEIO

        ┌────────────┼────────────┐

        ▼            ▼            ▼

    Dashboard    Automation     Logging

```

HOMEIO shall consume only public MQTT interfaces.

No internal firmware interfaces are exposed.

---

# 11.5 Controller Discovery

HOMEIO shall automatically discover the controller through:

```text
zmartify/discovery/controller
```

Example

```json
{
  "payload":
  {
      "device":"ZIC-S3-202600001",
      "name":"Garden Controller",
      "manufacturer":"Zmartify",
      "model":"Irrigation Controller",
      "firmware":"5.0.0",
      "hardware":"RevB"
  }
}
```

Discovery information shall be retained.

---

# 11.6 Entity Philosophy

Each engineering function shall become an individual HOMEIO entity.

Examples include:

* Controller Status
* Active Program
* Zone Status
* Flow
* Pressure
* Rain Delay
* Water Consumption
* Weather
* Diagnostics
* Alarm Status

Entities shall remain stable across firmware versions.

---

# 11.7 Controller Entity

Entity

```
Garden Controller
```

Primary state:

* Offline
* Booting
* Idle
* Irrigating
* Manual
* Alarm
* Maintenance

Associated attributes:

* Firmware
* Hardware
* IP Address
* Uptime
* Health Score
* Active Zone
* Active Program

---

# 11.8 Zone Entities

Each irrigation zone shall be represented as an independent HOMEIO entity.

Example:

```
Front Lawn
```

Attributes:

* Zone Number
* Enabled
* Running
* Runtime
* Remaining Time
* Flow
* Pressure
* Water Used
* Water Budget

Commands:

* Start
* Stop
* Enable
* Disable

---

# 11.9 Program Entities

Programs shall appear as individual entities.

Example:

```
Morning Irrigation
```

Attributes:

* Enabled
* Next Start
* Runtime
* Last Execution
* Water Used

Commands:

* Start
* Pause
* Stop

---

# 11.10 Weather Entity

Weather information shall be aggregated into a single engineering entity.

Attributes:

* Temperature
* Humidity
* Rain
* Wind
* Solar Radiation
* UV Index
* ET
* Forecast
* Weather Recommendation

The weather entity is read-only.

---

# 11.11 Hydraulic Entity

One of the defining Zmartify features.

Entity:

```
Hydraulic System
```

Attributes:

* Current Flow
* Current Pressure
* Hydraulic Health
* Leak Probability
* Restriction Probability
* Learned Flow
* Learned Pressure

Commands:

None

The Hydraulic System entity represents diagnostics rather than control.

---

# 11.12 Water Usage Entity

Entity

```
Water Consumption
```

Attributes:

* Today
* Week
* Month
* Season
* Lifetime

Charts may be generated directly from these values.

---

# 11.13 Alarm Entity

Entity

```
Controller Alarms
```

Attributes:

* Active Alarm Count
* Highest Severity
* Last Alarm
* Alarm History

Commands:

* Acknowledge Alarm

The Alarm Manager remains responsible for alarm state.

---

# 11.14 Diagnostics Entity

Entity

```
Diagnostics
```

Attributes:

* CPU
* Memory
* Storage
* Wi-Fi
* MQTT
* Health Score
* Last Self-Test

Commands:

* Run Self-Test

---

# 11.15 Maintenance Entity

Entity

```
Maintenance
```

Commands:

* Backup Configuration
* Restore Configuration
* OTA Update
* Reboot Controller
* Enter Service Mode

Administrator authorization shall be required.

---

# 11.16 Event Mapping

The following Event Bus events shall be forwarded to HOMEIO.

| Firmware Event | HOMEIO Event      |
| -------------- | ----------------- |
| Zone Started   | Zone Running      |
| Zone Completed | Zone Finished     |
| Leak Detected  | Hydraulic Alarm   |
| Rain Delay     | Rain Delay Active |
| OTA Completed  | Firmware Updated  |
| Alarm Raised   | Alarm Active      |
| Alarm Cleared  | Alarm Cleared     |

---

# 11.17 Suggested Dashboard Layout

Recommended HOMEIO dashboard sections:

### Controller

* Status
* Active Program
* Active Zone
* Health Score

---

### Weather

* Temperature
* Humidity
* Rain
* Wind
* ET

---

### Hydraulics

* Flow
* Pressure
* Hydraulic Health

---

### Irrigation

* Programs
* Zones
* Water Consumption

---

### Diagnostics

* CPU
* Memory
* Wi-Fi
* MQTT

---

### Alarms

* Active Alarms
* Alarm History

---

# 11.18 Automation Examples

Typical HOMEIO automations include:

### Leak Protection

```
Leak Alarm

↓

Stop Irrigation

↓

Notify User
```

---

### Rain Delay

```
Rain Forecast > 10 mm

↓

Enable Rain Delay

↓

Resume Automatically
```

---

### High Wind

```
Wind > 10 m/s

↓

Suspend Irrigation
```

---

### Freeze Protection

```
Temperature < 2°C

↓

Suspend Irrigation
```

---

### Maintenance Reminder

```
Flow Deviation > 15%

↓

Notify User

↓

Schedule Inspection
```

---

# 11.19 Historical Data

Recommended historical logging:

| Parameter      | Interval |
| -------------- | -------: |
| Flow           |    1 min |
| Pressure       |    1 min |
| Water Usage    |    5 min |
| Weather        |    5 min |
| CPU            |   15 min |
| Health Score   |   15 min |
| Active Program |    Event |
| Alarms         |    Event |

---

# 11.20 Performance Targets

Target update latency:

| Item        |  Target |
| ----------- | ------: |
| Zone Status | <250 ms |
| Flow        |    <1 s |
| Pressure    |    <1 s |
| Alarm       | <250 ms |
| Weather     |  <5 min |
| Diagnostics |   <60 s |

---

# 11.21 Security Considerations

HOMEIO shall only receive information available through authenticated MQTT sessions.

Critical commands such as:

* Factory Reset
* OTA Update
* Configuration Restore
* Security Configuration

shall require administrator privileges.

Credentials shall never be transmitted or exposed through entity attributes.

---

# 11.22 Multi-Controller Installations

Future versions shall support multiple controllers.

Recommended naming convention:

```
Garden Controller

Front Garden

Back Garden

Greenhouse

Sports Field
```

Every entity shall include the originating Device ID to ensure uniqueness.

---

# 11.23 Future HOMEIO Enhancements

The architecture has been designed to support future capabilities including:

* Interactive irrigation maps
* Live hydraulic diagrams
* AI irrigation advisor
* Water cost analytics
* Predictive maintenance
* Fleet management
* Multi-property dashboards

These enhancements shall build upon the existing MQTT entity model without requiring changes to established topic structures.

---

# 11.24 Engineering Notes

HOMEIO is the primary integration target for the Zmartify ecosystem because it aligns closely with the project's philosophy of local-first, event-driven automation.

Rather than exposing low-level controller internals, the integration presents high-level engineering entities that reflect how users understand an irrigation system: zones, programs, weather, hydraulics and diagnostics. This approach simplifies dashboard design, automation logic and long-term maintenance while remaining fully compatible with the underlying MQTT API.

The entity model also establishes a consistent pattern that can be reused across future Zmartify products such as pump controllers, weather stations and greenhouse controllers.

---

# 11.25 Chapter Summary

This chapter defines the native HOMEIO integration model for the Zmartify Irrigation Controller.

By mapping MQTT topics into meaningful engineering entities and supporting automatic discovery, real-time events and secure bidirectional control, the integration provides a seamless user experience while preserving the modular architecture and safety mechanisms defined throughout the Zmartify firmware.

The HOMEIO entity model serves as the reference implementation for future smart-home integrations and forms the basis for the broader Zmartify ecosystem.

---

# End of Chapter 11

**Next Chapter**

**Chapter 12 – Home Assistant Integration, MQTT Discovery & Entity Mapping**

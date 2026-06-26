# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 9

# MQTT Topic Specification – Events, Discovery & Smart-Home Integration

---

# 9.1 Purpose

This chapter defines the event-driven communication model used by the Zmartify platform, together with automatic controller discovery and smart-home integration.

Unlike telemetry topics, which publish the current state of the controller at regular intervals, events represent instantaneous occurrences that may require immediate action by external systems.

This chapter also defines the discovery mechanism used by HOMEIO, Home Assistant, Homey and future Zmartify ecosystem devices.

---

# 9.2 Design Objectives

The Event and Discovery architecture shall:

* Minimize network traffic
* Eliminate unnecessary polling
* Support automatic controller discovery
* Support automatic entity discovery
* Enable plug-and-play integration
* Support multiple controllers
* Support future Zmartify products
* Remain platform independent

---

# 9.3 Namespace

Event namespace:

```text
zmartify/events/
```

Discovery namespace:

```text
zmartify/discovery/
```

Integration namespace:

```text
zmartify/integration/
```

Primary topics:

```text
events/system
events/zone
events/program
events/weather
events/alarm
events/hydraulics
events/diagnostics
events/configuration
events/ota

discovery/controller
discovery/entities
discovery/capabilities

integration/homeio
integration/homeassistant
integration/homey
integration/nodered
```

---

# 9.4 Event Philosophy

The controller follows an event-driven architecture.

Every significant state transition shall immediately generate an event.

Events shall:

* Represent completed actions
* Be timestamped
* Be self-describing
* Not require polling
* Be published exactly once per occurrence

Events are notifications—not state storage.

Clients requiring the current state shall subscribe to the appropriate state topics.

---

# 9.5 Event Lifecycle

```text
Firmware Module

↓

Event Bus

↓

MQTT Manager

↓

MQTT Publish

↓

Broker

↓

Subscribers
```

The Event Bus remains the sole source of MQTT event publication.

---

# 9.6 System Events

Topic

```text
zmartify/events/system
```

Examples

* Controller Booted
* Controller Ready
* Restart Requested
* Restart Completed
* Factory Reset
* Configuration Loaded
* Time Synchronized

Example payload

```json
{
  "timestamp":"2026-07-14T19:00:00Z",
  "device":"ZIC-S3-202600001",
  "type":"system_event",
  "payload":
  {
      "event":"Controller Ready",
      "uptime":12
  }
}
```

---

# 9.7 Zone Events

Topic

```text
zmartify/events/zone
```

Published whenever a zone changes state.

Examples

* Zone Started
* Zone Completed
* Zone Paused
* Zone Resumed
* Zone Skipped
* Manual Zone Started
* Manual Zone Stopped

Example

```json
{
  "payload":
  {
      "event":"Zone Started",
      "zone":4,
      "program":"Morning"
  }
}
```

---

# 9.8 Program Events

Topic

```text
zmartify/events/program
```

Examples

* Program Started
* Program Completed
* Program Cancelled
* Program Delayed
* Program Suspended

Example

```json
{
  "payload":
  {
      "event":"Program Completed",
      "duration":2462,
      "water":1186
  }
}
```

---

# 9.9 Hydraulic Events

Topic

```text
zmartify/events/hydraulics
```

Examples

* Flow Learning Completed
* Pressure Learning Completed
* Leak Detected
* Leak Cleared
* Hydraulic Stable
* Hydraulic Warning

Example

```json
{
  "payload":
  {
      "event":"Pressure Learning Completed",
      "zone":7,
      "confidence":98
  }
}
```

---

# 9.10 Weather Events

Topic

```text
zmartify/events/weather
```

Examples

* Rain Delay Activated
* Rain Delay Cancelled
* Freeze Protection Enabled
* High Wind Warning
* Weather Updated
* ET Updated

Example

```json
{
  "payload":
  {
      "event":"Rain Delay Activated",
      "duration_hours":24
  }
}
```

---

# 9.11 Alarm Events

Topic

```text
zmartify/events/alarm
```

Examples

* Alarm Raised
* Alarm Cleared
* Alarm Acknowledged

Example

```json
{
  "payload":
  {
      "event":"Alarm Raised",
      "alarm":"Low Pressure",
      "severity":"Critical"
  }
}
```

Critical alarm events shall always use **QoS 2**.

---

# 9.12 Configuration Events

Topic

```text
zmartify/events/configuration
```

Examples

* Zone Updated
* Schedule Modified
* Weather Settings Changed
* Display Updated
* Backup Created
* Restore Completed

Example

```json
{
  "payload":
  {
      "event":"Zone Configuration Updated",
      "zone":3
  }
}
```

---

# 9.13 OTA Events

Topic

```text
zmartify/events/ota
```

Examples

* Update Available
* Download Started
* Download Completed
* Verification Passed
* Installation Started
* Rollback
* Update Completed

Example

```json
{
  "payload":
  {
      "event":"OTA Completed",
      "version":"5.1.0"
  }
}
```

---

# 9.14 Discovery Philosophy

Discovery enables client software to identify controllers automatically without prior configuration.

Every controller periodically announces:

* Identity
* Capabilities
* Supported entities
* Firmware version
* Hardware revision

---

# 9.15 Controller Discovery

Topic

```text
zmartify/discovery/controller
```

Published:

* Boot
* Every 15 minutes
* Manual discovery request

Example

```json
{
  "payload":
  {
      "device":"ZIC-S3-202600001",
      "name":"Garden Controller",
      "ip":"192.168.1.40",
      "firmware":"5.0.0",
      "hardware":"RevB"
  }
}
```

---

# 9.16 Entity Discovery

Topic

```text
zmartify/discovery/entities
```

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
      "display":true
  }
}
```

Clients may dynamically build user interfaces from this information.

---

# 9.17 Capability Discovery

Topic

```text
zmartify/discovery/capabilities
```

Example

```json
{
  "payload":
  {
      "ota":true,
      "et":true,
      "weather":true,
      "hydraulics":true,
      "homeassistant_discovery":true,
      "homeio":true,
      "diagnostics":true
  }
}
```

---

# 9.18 HOMEIO Integration

Primary integration platform:

**HOMEIO**

Integration features:

* Native MQTT
* Automatic controller discovery
* Automatic entity creation
* Event-driven updates
* Complete telemetry
* Alarm forwarding
* Diagnostics
* Historical logging

No custom firmware is required.

---

# 9.19 Home Assistant Integration

The controller supports Home Assistant through MQTT.

Features:

* MQTT Discovery
* Automatic entity registration
* Device Registry
* Area assignment
* Availability reporting
* Entity categories

Entity types include:

* Sensors
* Binary Sensors
* Switches
* Buttons
* Numbers
* Selects
* Diagnostics

Home Assistant-specific discovery payloads shall remain isolated from the generic Zmartify namespace.

---

# 9.20 Homey Integration

The controller supports Homey through standard MQTT.

Features:

* Automatic controller detection
* Flow cards
* Device capabilities
* Real-time events
* Historical values

Future native Homey applications may extend functionality while preserving the MQTT interface.

---

# 9.21 Node-RED Integration

The MQTT namespace has been designed for Node-RED.

Recommended subscriptions:

```text
zmartify/events/#

zmartify/alarms/#

zmartify/flow/#

zmartify/pressure/#

zmartify/weather/#

zmartify/irrigation/#
```

This enables low-code automation workflows with minimal configuration.

---

# 9.22 Multi-Controller Support

Future deployments may include multiple controllers.

Recommended topic structure:

```text
zmartify/<device-id>/controller/status

zmartify/<device-id>/zones/01/state
```

Version 5.0 maintains a single-controller namespace but reserves this extension for future scalability.

---

# 9.23 Publish Rates

| Topic                  |      Interval |
| ---------------------- | ------------: |
| events/*               |    Event only |
| discovery/controller   | Boot + 15 min |
| discovery/entities     |          Boot |
| discovery/capabilities |          Boot |
| integration/*          |     On demand |

Events shall never be published periodically.

---

# 9.24 QoS Policy

| Topic                  | QoS | Retained |
| ---------------------- | :-: | :------: |
| events/system          |  1  |    No    |
| events/zone            |  1  |    No    |
| events/alarm           |  2  |    No    |
| events/ota             |  1  |    No    |
| discovery/controller   |  1  |    Yes   |
| discovery/entities     |  1  |    Yes   |
| discovery/capabilities |  1  |    Yes   |

---

# 9.25 Error Handling

Discovery payloads shall include a compatibility version.

Example

```json
{
  "payload":
  {
      "api_version":"1.0",
      "compatible":true
  }
}
```

Clients shall ignore unsupported optional fields while continuing to process mandatory fields.

---

# 9.26 Engineering Notes

The Event and Discovery architecture completes the MQTT communication model by allowing client applications to react immediately to significant controller events while minimizing network traffic.

The discovery mechanism has been designed with future ecosystem expansion in mind. As additional Zmartify products—such as pump controllers, weather stations and lighting controllers—are introduced, they will use the same discovery conventions, allowing supervisory software to build a complete system topology automatically.

The separation of generic discovery topics from platform-specific integration topics also ensures that the core API remains stable and vendor-neutral while still supporting advanced features offered by HOMEIO, Home Assistant and Homey.

---

# 9.27 Chapter Summary

This chapter defines the MQTT interfaces for event publication, automatic discovery and smart-home integration.

Together with the telemetry and command interfaces defined in previous chapters, these mechanisms provide a complete event-driven communication model that is scalable, interoperable and ready for integration into both current and future Zmartify ecosystem products.

---

# End of Chapter 9

**Next Chapter**

**Chapter 10 – MQTT Command Interface, JSON Schemas & Transaction Protocol**

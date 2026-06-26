# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 12

# Home Assistant Integration, MQTT Discovery & Entity Mapping

---

# 12.1 Purpose

This chapter specifies the native Home Assistant integration for the Zmartify Irrigation Controller using the MQTT Discovery protocol.

The objective is to provide a zero-configuration integration where Home Assistant automatically discovers, registers and maintains all controller entities without requiring manual MQTT configuration.

The implementation shall follow the official Home Assistant MQTT Discovery specification while remaining fully compatible with the generic Zmartify MQTT API defined in previous chapters.

---

# 12.2 Design Objectives

The Home Assistant integration shall:

* Support MQTT Discovery
* Require no manual YAML configuration
* Automatically register all entities
* Publish device metadata
* Support entity availability
* Expose engineering diagnostics
* Preserve API compatibility
* Support future firmware revisions without breaking entity identifiers

---

# 12.3 Integration Architecture

```text
                  Zmartify Controller

                          │

                  MQTT Manager

                          │

                Discovery Publisher

                          │

                    MQTT Broker

                          │

                 Home Assistant

                          │

        ┌─────────────────┼─────────────────┐

        ▼                 ▼                 ▼

    Device Registry   Entity Registry   Automation Engine

```

Home Assistant shall interact exclusively through the public MQTT interface.

---

# 12.4 MQTT Discovery Philosophy

Upon startup, the controller shall publish discovery payloads for every supported entity.

Discovery shall occur:

* At boot
* After reconnecting to the broker
* After firmware upgrades
* Upon explicit discovery request
* After configuration changes affecting available entities

Discovery messages shall be retained.

---

# 12.5 Discovery Prefix

Default MQTT Discovery prefix:

```text
homeassistant/
```

This value shall be configurable.

Example:

```text
homeassistant/sensor/zmartify_controller/flow/config
```

---

# 12.6 Device Registration

The controller shall register as a single Home Assistant Device.

Example device information:

```json
{
  "identifiers":["ZIC-S3-202600001"],
  "manufacturer":"Zmartify",
  "model":"Irrigation Controller",
  "name":"Garden Controller",
  "sw_version":"5.0.0",
  "hw_version":"RevB"
}
```

The Device Registry shall remain stable across firmware updates.

---

# 12.7 Entity Naming Convention

Entity IDs shall follow predictable naming rules.

Examples:

```text
sensor.garden_controller_flow

sensor.garden_controller_pressure

sensor.garden_controller_et

binary_sensor.garden_controller_alarm

switch.zone_01

switch.zone_02

button.controller_reboot
```

Friendly names shall be user configurable.

---

# 12.8 Sensor Entities

The following MQTT topics shall be represented as Sensor entities.

| MQTT Topic          | Home Assistant Entity |
| ------------------- | --------------------- |
| flow/current        | sensor                |
| pressure/current    | sensor                |
| weather/temperature | sensor                |
| weather/humidity    | sensor                |
| weather/rain        | sensor                |
| weather/solar       | sensor                |
| weather/uv          | sensor                |
| weather/et          | sensor                |
| irrigation/runtime  | sensor                |
| water/today         | sensor                |
| cpu                 | sensor                |
| memory              | sensor                |

---

# 12.9 Binary Sensor Entities

Binary sensors represent boolean states.

Examples:

| State             | Entity        |
| ----------------- | ------------- |
| Controller Online | binary_sensor |
| Alarm Active      | binary_sensor |
| Rain Delay        | binary_sensor |
| Freeze Protection | binary_sensor |
| Leak Detected     | binary_sensor |
| MQTT Connected    | binary_sensor |
| Wi-Fi Connected   | binary_sensor |

---

# 12.10 Switch Entities

Switches represent controllable outputs.

Examples:

```text
switch.zone_01

switch.zone_02

...

switch.zone_15
```

Supported actions:

* ON → Manual irrigation
* OFF → Stop irrigation

The MQTT Manager shall translate switch actions into validated command transactions.

---

# 12.11 Button Entities

Buttons initiate one-time actions.

Examples:

```text
button.self_test

button.configuration_backup

button.reboot

button.ota_update
```

Button presses shall generate MQTT command messages rather than directly invoking firmware functions.

---

# 12.12 Number Entities

Number entities expose adjustable numeric values.

Examples:

* Display brightness
* Rain delay duration
* Runtime adjustment
* Flow calibration
* Pressure calibration

Example:

```text
number.rain_delay_hours
```

---

# 12.13 Select Entities

Select entities expose predefined options.

Examples:

* Weather provider
* Language
* Display theme
* Irrigation mode
* ET calculation method

Example:

```text
select.weather_provider
```

---

# 12.14 Text Entities

Text entities allow free-form user input.

Examples:

* Controller name
* Zone names
* Program names
* MQTT client ID

Maximum field lengths shall be validated by the Configuration Manager.

---

# 12.15 Diagnostic Entities

Entities intended primarily for engineering use shall be categorized as Diagnostics.

Examples:

* Heap memory
* CPU utilization
* Wi-Fi RSSI
* MQTT latency
* Firmware build
* Hardware revision
* Boot count

These entities may be hidden by default in Home Assistant.

---

# 12.16 Availability Topic

All entities shall share a common availability topic.

```text
zmartify/controller/status
```

Online payload:

```json
{
  "state":"online"
}
```

Offline payload:

```json
{
  "state":"offline"
}
```

This corresponds to the MQTT Last Will and Testament defined in Chapter 3.

---

# 12.17 Discovery Payload Example

Example discovery payload for the Flow Sensor:

```json
{
  "name":"Current Flow",
  "unique_id":"zmartify_flow_current",
  "state_topic":"zmartify/flow/current",
  "availability_topic":"zmartify/controller/status",
  "unit_of_measurement":"L/min",
  "device_class":null,
  "state_class":"measurement",
  "device":{
      "identifiers":["ZIC-S3-202600001"],
      "manufacturer":"Zmartify",
      "model":"Irrigation Controller"
  }
}
```

Actual discovery payloads shall conform to the current Home Assistant MQTT Discovery specification.

---

# 12.18 Device Classes

Recommended Home Assistant device classes:

| Measurement     | Device Class    |
| --------------- | --------------- |
| Temperature     | temperature     |
| Humidity        | humidity        |
| Pressure        | pressure        |
| Signal Strength | signal_strength |
| Duration        | duration        |
| Power           | power           |
| Voltage         | voltage         |
| Current         | current         |

Where no suitable device class exists, the entity shall omit the field.

---

# 12.19 State Classes

Recommended state classes:

| Parameter       | State Class      |
| --------------- | ---------------- |
| Temperature     | measurement      |
| Flow            | measurement      |
| Pressure        | measurement      |
| Water Usage     | total_increasing |
| Runtime         | total_increasing |
| Rain            | measurement      |
| Energy (future) | total            |

Correct state classes enable Home Assistant statistics and long-term energy dashboards.

---

# 12.20 Entity Categories

Entities shall be categorized according to their intended purpose.

| Category      | Examples                          |
| ------------- | --------------------------------- |
| Configuration | Display settings, language        |
| Diagnostic    | CPU, memory, Wi-Fi                |
| None          | Irrigation status, flow, pressure |

This improves dashboard organization.

---

# 12.21 Suggested Dashboard

Recommended Home Assistant dashboard sections:

### Controller

* Status
* Active Program
* Active Zone
* Uptime

### Weather

* Temperature
* Rain
* Wind
* ET

### Irrigation

* Zones
* Water Usage
* Runtime

### Hydraulics

* Flow
* Pressure
* Hydraulic Health

### Diagnostics

* CPU
* Memory
* Wi-Fi
* MQTT

### Alarms

* Current Alarm
* Alarm History

---

# 12.22 Automation Examples

### Leak Protection

```
Leak Detected

↓

Stop Irrigation

↓

Send Mobile Notification

↓

Flash Dashboard Alert
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

### Freeze Protection

```
Temperature < 2°C

↓

Suspend Irrigation
```

---

### Low Wi-Fi Signal

```
RSSI < -80 dBm

↓

Create Maintenance Notification
```

---

# 12.23 Multi-Controller Installations

The integration supports multiple controllers.

Each controller shall expose:

* Unique Device ID
* Unique Entity IDs
* Unique Discovery Topics

Entity names may be customized without affecting the underlying unique identifiers.

---

# 12.24 Future Home Assistant Features

The architecture supports future enhancements including:

* Native Irrigation Dashboard
* Water Cost Analytics
* AI Irrigation Advisor
* Lovelace Cards
* Blueprint Automations
* Voice Assistant Integration
* Matter Bridge
* Energy Dashboard compatibility for future pump power monitoring

These features shall build upon the existing MQTT entity model.

---

# 12.25 Engineering Notes

The Home Assistant integration intentionally separates the generic Zmartify MQTT namespace from Home Assistant–specific discovery payloads.

This design preserves the MQTT API as the canonical interface while allowing Home Assistant to benefit from automatic entity creation, device registry integration and advanced dashboard capabilities. It also ensures that firmware changes affecting Home Assistant discovery do not impact other MQTT clients such as HOMEIO, Homey or Node-RED.

The use of stable unique identifiers guarantees that user dashboards, automations and entity customizations survive firmware upgrades without requiring reconfiguration.

---

# 12.26 Chapter Summary

This chapter defines the Home Assistant integration model for the Zmartify Irrigation Controller.

By implementing MQTT Discovery, standardized entity mapping and stable device metadata, the controller integrates seamlessly into Home Assistant while maintaining full compatibility with the broader Zmartify MQTT architecture. The result is a robust, zero-configuration integration that supports both residential users and advanced automation scenarios.

---

# End of Chapter 12

**Next Chapter**

**Chapter 13 – Homey Integration, MQTT Client Design & Flow Card Mapping**

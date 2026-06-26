# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 13

# Homey Integration, MQTT Client Design & Flow Card Mapping

---

# 13.1 Purpose

This chapter defines the native integration between the Zmartify Irrigation Controller and the **Athom Homey** smart home platform.

Unlike generic MQTT integrations, the Homey implementation is designed to expose Zmartify functionality through native Homey capabilities, Flow Cards, Insights and device capabilities while remaining fully compatible with the public MQTT API.

The Homey integration shall operate without requiring any firmware modifications to the controller.

---

# 13.2 Design Objectives

The Homey integration shall:

* Support automatic device discovery
* Expose native Homey capabilities
* Support bidirectional MQTT communication
* Support Flow automation
* Publish Insights data
* Support multiple controllers
* Preserve firmware independence
* Support future Homey applications

---

# 13.3 Integration Architecture

```text
             Zmartify Controller

                     │

               MQTT Manager

                     │

               MQTT Broker

                     │

             Homey MQTT Client

                     │

        ┌────────────┼────────────┐

        ▼            ▼            ▼

      Devices      Flow Cards    Insights

```

The MQTT Broker remains the only communication interface between the controller and Homey.

---

# 13.4 Homey Device Philosophy

The Zmartify controller shall appear in Homey as one primary device with multiple logical capabilities.

The Homey application shall avoid creating unnecessary child devices unless required for scalability.

Primary device:

```text
Garden Controller
```

Future multi-controller installations may expose additional devices.

---

# 13.5 Device Metadata

Example device information:

| Property     | Value                 |
| ------------ | --------------------- |
| Manufacturer | Zmartify              |
| Product      | Irrigation Controller |
| Hardware     | Rev B                 |
| Firmware     | 5.x                   |
| Device ID    | ZIC-S3-202600001      |

Device metadata shall remain stable throughout the controller lifetime.

---

# 13.6 Homey Capabilities

Recommended capabilities include:

| Capability        | Source MQTT Topic     |
| ----------------- | --------------------- |
| Controller Status | controller/status     |
| Active Zone       | irrigation/state      |
| Active Program    | irrigation/program    |
| Flow              | flow/current          |
| Pressure          | pressure/current      |
| ET                | weather/et            |
| Water Today       | irrigation/statistics |
| Hydraulic Health  | hydraulics/health     |
| Alarm Status      | alarms/current        |

Capabilities shall update automatically when MQTT messages are received.

---

# 13.7 Zone Representation

Each irrigation zone may optionally appear as an individual Homey device.

Example:

```text
Front Lawn
```

Capabilities:

* Running
* Remaining Time
* Water Used
* Flow
* Pressure

Actions:

* Start
* Stop
* Enable
* Disable

This behavior shall be configurable within the Homey application.

---

# 13.8 MQTT Client Design

The Homey application subscribes to:

```text
zmartify/controller/#

zmartify/system/#

zmartify/weather/#

zmartify/irrigation/#

zmartify/zones/#

zmartify/flow/#

zmartify/pressure/#

zmartify/alarms/#

zmartify/events/#

zmartify/diagnostics/#
```

Wildcard subscriptions minimize maintenance when new topics are added.

---

# 13.9 Flow Cards

The Homey application shall expose automation triggers using native Flow Cards.

Trigger examples:

* Zone Started
* Zone Finished
* Program Started
* Program Finished
* Rain Delay Enabled
* Rain Delay Disabled
* Alarm Raised
* Alarm Cleared
* Leak Detected
* OTA Completed

---

# 13.10 Condition Cards

Condition cards evaluate controller state.

Examples:

* Controller Online?
* Irrigation Running?
* Rain Delay Active?
* Alarm Active?
* Hydraulic Healthy?
* Weather Available?
* MQTT Connected?

Each condition shall evaluate directly from MQTT state topics.

---

# 13.11 Action Cards

Action cards publish validated MQTT commands.

Examples:

* Start Zone
* Stop Irrigation
* Pause Program
* Resume Program
* Skip Zone
* Enable Rain Delay
* Backup Configuration
* Reboot Controller
* Run Self-Test

The Homey application shall never manipulate MQTT state topics directly.

---

# 13.12 Flow Example – Leak Protection

```text
Leak Alarm Raised

↓

Stop Irrigation

↓

Send Push Notification

↓

Log Event

↓

Create Maintenance Reminder
```

---

# 13.13 Flow Example – Rain Delay

```text
Weather Forecast Updated

↓

Expected Rain > 10 mm

↓

Enable Rain Delay

↓

Notify User
```

---

# 13.14 Flow Example – Water Budget

```text
ET Increased

↓

Runtime Factor > 110%

↓

Adjust Water Budget

↓

Update Dashboard
```

---

# 13.15 Homey Insights

Recommended Insight Logs:

| Insight          | Interval |
| ---------------- | -------: |
| Flow             |    1 min |
| Pressure         |    1 min |
| Water Usage      |    5 min |
| Temperature      |    5 min |
| ET               |   30 min |
| Hydraulic Health |   15 min |
| CPU              |   15 min |
| Wi-Fi RSSI       |   15 min |

These logs enable long-term trend analysis.

---

# 13.16 Notifications

The Homey application should generate notifications for:

* Leak detected
* Pressure failure
* Flow anomaly
* Rain delay activated
* Controller offline
* OTA completed
* Backup completed
* Self-test failed

Notification severity shall mirror the Alarm Manager severity levels.

---

# 13.17 Multi-Controller Support

Future Homey versions shall support multiple Zmartify controllers.

Each device shall be uniquely identified by:

* Device ID
* MQTT namespace
* Controller serial number

Suggested device names:

```text
Front Garden

Back Garden

Greenhouse

Sports Field
```

---

# 13.18 Offline Behaviour

If MQTT communication is interrupted:

* Device availability shall change to Offline.
* Previously received values shall remain visible.
* No commands shall be issued while offline.
* Normal operation shall resume automatically after reconnection.

The irrigation controller continues autonomous operation regardless of Homey availability.

---

# 13.19 Performance Targets

| Function                       |  Target |
| ------------------------------ | ------: |
| State update latency           | <250 ms |
| Flow update                    |    <1 s |
| Pressure update                |    <1 s |
| Alarm notification             | <250 ms |
| Command acknowledgement        | <500 ms |
| Reconnection after broker loss |    <5 s |

---

# 13.20 Security

The Homey integration shall:

* Use authenticated MQTT sessions
* Support TLS-secured brokers
* Never store plaintext credentials in logs
* Validate all outgoing command payloads
* Reject malformed MQTT messages

Critical operations such as Factory Reset and OTA Update shall require administrator confirmation.

---

# 13.21 Future Homey Features

The architecture supports future enhancements including:

* Native irrigation dashboard
* Zone grouping
* Interactive garden maps
* Water consumption analytics
* Predictive maintenance
* AI irrigation advisor
* Voice assistant integration
* Energy monitoring for future pump controllers

These enhancements shall be implemented without modifying the public MQTT API.

---

# 13.22 Engineering Notes

The Homey integration complements the HOMEIO and Home Assistant integrations by providing a native experience for Homey users while relying entirely on the standardized Zmartify MQTT interface.

By exposing controller functions through Homey Capabilities, Flow Cards and Insights, users can build sophisticated automations without needing detailed knowledge of MQTT topics or payload formats. The Homey application acts as an adapter layer, translating Homey interactions into validated MQTT transactions while preserving the controller's internal safety architecture.

This approach ensures consistent behavior across all supported smart-home platforms and minimizes maintenance as the Zmartify ecosystem expands.

---

# 13.23 Chapter Summary

This chapter defines the Homey integration architecture for the Zmartify Irrigation Controller.

The design leverages MQTT as the transport layer while providing native Homey devices, capabilities, Flow Cards and Insights. By maintaining a strict separation between platform-specific integration logic and the generic Zmartify MQTT API, the solution achieves excellent interoperability, long-term maintainability and compatibility with future firmware and ecosystem products.

---

# End of Chapter 13

**Next Chapter**

**Chapter 14 – Node-RED, Grafana & Third-Party Integration Guide**

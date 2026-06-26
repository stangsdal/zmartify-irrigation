# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 16

# HMI Integration with Firmware, Event Bus & Real-Time Data Binding

---

# 16.1 Purpose

This chapter defines the integration architecture between the Human–Machine Interface (HMI) and the underlying firmware described in **Volume 2 – Firmware Architecture**.

A fundamental design principle of the Zmartify platform is the complete separation of presentation, application logic and hardware abstraction. The HMI shall never communicate directly with hardware drivers or implement irrigation logic.

Instead, all interaction occurs through a structured Event Bus and a centralized Data Binding Framework.

This architecture provides:

* Deterministic firmware behavior
* Loose coupling
* High maintainability
* Simplified testing
* Reusable UI components
* Future cloud integration
* Multi-display support

---

# 16.2 Design Objectives

The integration architecture shall:

* Completely isolate LVGL from firmware logic
* Eliminate direct hardware access from the GUI
* Support asynchronous communication
* Prevent race conditions
* Support future distributed architectures
* Enable deterministic updates
* Simplify debugging
* Support automated testing

---

# 16.3 Architectural Overview

```text
                    Human Interface

                          │

                   Display Manager

                          │

                   Event Bus Manager

          ┌───────────────┼───────────────┐

          ▼               ▼               ▼

     Application      Notification     Data Cache

       Events           Events          Updates

          ▼               ▼               ▼

             Application Managers

                          │

                  Hardware Managers

                          │

                      Hardware
```

No LVGL object shall communicate directly with any firmware module.

---

# 16.4 Layer Responsibilities

| Layer                | Responsibility       |
| -------------------- | -------------------- |
| LVGL Widgets         | Presentation only    |
| Display Manager      | UI coordination      |
| Event Bus            | Communication        |
| Application Managers | Business logic       |
| Hardware Managers    | Hardware abstraction |
| Drivers              | Physical hardware    |

Each layer communicates only with adjacent layers.

---

# 16.5 Event Bus Philosophy

The Event Bus provides asynchronous communication between the firmware and the HMI.

Advantages include:

* Loose coupling
* Thread safety
* Scalability
* Event logging
* Deferred processing
* Reduced blocking
* Easier testing

All firmware events shall pass through the Event Bus.

---

# 16.6 Event Categories

Supported event categories:

| Category      | Description           |
| ------------- | --------------------- |
| UI            | User interactions     |
| Irrigation    | Zone/program events   |
| Weather       | Environmental updates |
| Hydraulics    | Flow and pressure     |
| Diagnostics   | Health updates        |
| Network       | Wi-Fi and MQTT        |
| Alarm         | Alarm state changes   |
| Configuration | Settings updates      |
| OTA           | Firmware updates      |
| System        | Controller state      |

Each category shall have a unique event identifier.

---

# 16.7 Standard Event Structure

Every event shall contain:

```text
Event ID

Timestamp

Source

Destination

Priority

Payload
```

Reference structure:

```json
{
  "event":"ZONE_STARTED",
  "source":"IrrigationManager",
  "timestamp":"2026-07-14T20:15:00Z",
  "priority":"Normal",
  "payload":{
      "zone":4
  }
}
```

---

# 16.8 Event Priorities

Priority levels:

| Priority | Description          |
| -------- | -------------------- |
| Critical | Immediate processing |
| High     | Next scheduler cycle |
| Normal   | Standard queue       |
| Low      | Background update    |

Critical alarms shall always preempt normal updates.

---

# 16.9 Event Lifecycle

```text
Application Event

↓

Event Creation

↓

Event Queue

↓

Priority Scheduler

↓

Subscriber Dispatch

↓

Processing

↓

Completion
```

Completed events shall be released immediately.

---

# 16.10 Event Queue

Recommended architecture:

```text
Critical Queue

↓

High Queue

↓

Normal Queue

↓

Background Queue
```

Queues shall be independent.

Queue overflow shall never affect irrigation control.

---

# 16.11 Event Subscribers

Typical subscribers include:

* Display Manager
* Notification Manager
* Widget Manager
* MQTT Manager
* Logger
* Statistics Manager

One event may have multiple subscribers.

---

# 16.12 Data Binding Philosophy

Widgets shall never poll firmware modules directly.

Instead, widgets bind to observable data objects.

Workflow:

```text
Application Manager

↓

Data Model

↓

Display Manager

↓

Widget Binding

↓

Display
```

This ensures consistent and efficient updates.

---

# 16.13 Data Models

Primary data models include:

* ControllerModel
* WeatherModel
* ZoneModel
* ProgramModel
* HydraulicModel
* AlarmModel
* DiagnosticModel
* ConfigurationModel

Each model represents a logical view of controller state.

---

# 16.14 Observable Properties

Each model exposes observable properties.

Example:

```text
ZoneModel

├── running

├── flow

├── pressure

├── runtime

├── remaining

└── health
```

Property changes automatically notify bound widgets.

---

# 16.15 Binding Types

Supported binding modes:

| Type     | Description           |
| -------- | --------------------- |
| One-Way  | Firmware → UI         |
| Two-Way  | UI ↔ Configuration    |
| Event    | One-time notification |
| Computed | Derived value         |

Most telemetry uses one-way binding.

---

# 16.16 UI Update Pipeline

```text
Sensor Update

↓

Application Manager

↓

Model Updated

↓

Event Published

↓

Display Manager

↓

Widget Updated

↓

Dirty Region

↓

Render
```

Only affected widgets shall be refreshed.

---

# 16.17 Change Detection

The Display Manager shall compare new values against previous values.

```text
Value Changed?

↓

No

↓

Ignore

↓

Yes

↓

Update Widget
```

Redundant redraws shall be avoided.

---

# 16.18 Widget Binding Examples

Example:

```text
FlowCard

↓

HydraulicModel.flow
```

Pressure Gauge:

```text
PressureGauge

↓

HydraulicModel.pressure
```

Weather Summary:

```text
WeatherCard

↓

WeatherModel.summary
```

---

# 16.19 Controller State Synchronization

The Display Manager maintains an internal snapshot of controller state.

Synchronization occurs:

* After startup
* After reconnect
* After configuration changes
* Following OTA updates

State synchronization shall be atomic.

---

# 16.20 Bidirectional Binding

Configuration widgets may use two-way binding.

Example:

```text
Slider

↓

Water Budget

↓

ConfigurationModel

↓

Configuration Manager

↓

Persistent Storage
```

Validation occurs before persistence.

---

# 16.21 Event Coalescing

High-frequency updates may be combined.

Examples:

* Flow updates
* Pressure updates
* CPU statistics
* Memory usage

Coalescing reduces unnecessary GUI workload.

---

# 16.22 Thread Safety

Only the Display Manager may access LVGL objects.

Other tasks communicate via:

* Event queues
* Message queues
* FreeRTOS notifications

Direct widget access from background tasks is prohibited.

---

# 16.23 Error Handling

Integration failures include:

* Queue overflow
* Invalid payload
* Missing subscriber
* Model inconsistency
* Binding failure

Recovery sequence:

```text
Detect

↓

Log

↓

Retry

↓

Fallback

↓

Recover
```

Errors shall never crash the GUI.

---

# 16.24 Logging

Integration diagnostics shall record:

* Event publication
* Queue delays
* Subscriber failures
* Binding errors
* Model updates
* Synchronization events

Logging verbosity shall be configurable.

---

# 16.25 Performance Requirements

| Metric              | Target |
| ------------------- | -----: |
| Event Dispatch      |  <1 ms |
| Queue Latency       |  <5 ms |
| Model Update        |  <2 ms |
| Widget Binding      |  <5 ms |
| Complete UI Refresh | <50 ms |

These values assume normal operating conditions.

---

# 16.26 Future Extensions

The integration framework has been designed to support:

* Distributed HMIs
* Remote dashboards
* WebSocket subscribers
* Matter UI integration
* Mobile applications
* Cloud synchronization
* Digital twins
* Multi-controller systems

These extensions shall reuse the existing Event Bus and Data Model architecture.

---

# 16.27 Relationship to Other Volumes

This chapter bridges multiple engineering volumes:

| Volume    | Relationship                         |
| --------- | ------------------------------------ |
| Volume 2  | Application Managers & Event Sources |
| Volume 3  | MQTT and External Events             |
| Volume 5  | Hardware Drivers                     |
| Volume 7  | Integration Testing                  |
| Volume 10 | SDK and Third-Party Development      |

The Event Bus defined here is the primary communication mechanism between the HMI and the firmware.

---

# 16.28 Engineering Notes

The Event Bus and Data Binding Framework are central architectural elements of the Zmartify platform. By eliminating direct dependencies between graphical widgets and firmware modules, the design achieves exceptional modularity, simplifies testing and enables future expansion without architectural changes.

This event-driven model also aligns with modern embedded software practices and supports scalable integration with cloud services, mobile applications and future distributed HMI solutions.

---

# 16.29 Chapter Summary

This chapter defines the integration architecture connecting the Human–Machine Interface with the underlying firmware.

Through a centralized Event Bus, observable data models and structured data binding, the platform ensures efficient, thread-safe and maintainable communication between application logic and presentation. Together with the Display Manager architecture defined earlier in this volume, it completes the software integration model for the Zmartify HMI.

---

# End of Chapter 16

**Next Chapter**

**Chapter 17 – HMI Testing, UI Verification & Automated GUI Validation**

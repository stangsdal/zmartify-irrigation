# Chapter 7 – System Software Baseline & Firmware Platform Specification

---

# 7 System Software Baseline

## 7.1 Purpose

This chapter defines the approved software platform for the Zmartify Irrigation Controller (ZIC-S3 Rev.B).

It establishes the software architecture, development framework, coding standards, module structure and execution model that shall be used throughout the project.

The objective is to ensure a scalable, maintainable and testable firmware platform capable of supporting future hardware revisions without major architectural changes.

---

# 7.2 Software Design Philosophy

The firmware architecture shall be based upon the following principles.

### SW-001 — Modular Design

Every functional subsystem shall exist as an independent software component.

Each component shall:

* Have a clearly defined responsibility
* Expose a documented API
* Minimize dependencies
* Be independently testable

---

### SW-002 — Separation of Concerns

The firmware shall separate:

* User Interface
* Business Logic
* Hardware Drivers
* Communications
* Data Storage

No component shall perform multiple unrelated responsibilities.

---

### SW-003 — Event-Driven Architecture

Subsystems shall communicate through events.

Modules shall not directly invoke unrelated modules.

This architecture provides:

* Loose coupling
* Scalability
* Improved debugging
* Simplified testing

---

### SW-004 — Hardware Abstraction

All hardware shall be accessed exclusively through the Hardware Abstraction Layer (HAL).

The application layer shall never manipulate:

* GPIO
* ADC
* I²C
* SPI
* UART
* PWM

directly.

---

### SW-005 — Safety First

Safety functions shall always have higher execution priority than convenience features.

Emergency actions shall never depend on:

* Display updates
* MQTT availability
* Internet connectivity
* Smart-home systems

---

# 7.3 Approved Software Platform

The approved firmware platform is:

| Component   | Approved Platform   |
| ----------- | ------------------- |
| SDK         | ESP-IDF             |
| RTOS        | FreeRTOS            |
| Graphics    | LVGL                |
| Networking  | ESP-IDF Wi-Fi Stack |
| MQTT        | ESP-IDF MQTT Client |
| OTA         | ESP-IDF OTA         |
| File System | LittleFS            |
| NVS         | ESP-IDF NVS         |

Arduino framework shall not be used.

---

# 7.4 Programming Language

Firmware shall be developed in:

* C (primary)
* C++ (limited use where appropriate)

Coding shall comply with modern embedded development practices.

---

# 7.5 Directory Structure

The project shall follow the standard ESP-IDF component model.

```text
zmartify-controller/
│
├── components/
│   ├── alarm_manager/
│   ├── config_manager/
│   ├── diagnostics/
│   ├── display/
│   ├── et_engine/
│   ├── event_bus/
│   ├── flow_manager/
│   ├── hal/
│   ├── irrigation_engine/
│   ├── logging/
│   ├── mqtt_manager/
│   ├── ota_manager/
│   ├── pressure_manager/
│   ├── relay_manager/
│   ├── storage/
│   ├── weather_manager/
│   └── zone_manager/
│
├── main/
│
├── managed_components/
│
├── docs/
│
├── test/
│
└── tools/
```

Each component shall contain:

```text
include/
src/
CMakeLists.txt
README.md
unit_tests/
```

---

# 7.6 Firmware Layers

The firmware architecture shall follow a six-layer model.

```text
+------------------------------------------------+
|             Graphical User Interface           |
+------------------------------------------------+
|            Application Services                |
+------------------------------------------------+
|          Business Logic / Engines              |
+------------------------------------------------+
|               Event Bus                        |
+------------------------------------------------+
|         Hardware Abstraction Layer             |
+------------------------------------------------+
|          ESP-IDF / FreeRTOS Drivers            |
+------------------------------------------------+
```

Application logic shall never bypass lower layers.

---

# 7.7 Core Software Modules

The following modules are mandatory.

| Module            | Responsibility             |
| ----------------- | -------------------------- |
| relay_manager     | Relay control              |
| zone_manager      | Zone configuration         |
| irrigation_engine | Irrigation sequencing      |
| weather_manager   | Weather processing         |
| et_engine         | ET calculations            |
| flow_manager      | Flow measurement           |
| pressure_manager  | Pressure supervision       |
| mqtt_manager      | MQTT communication         |
| alarm_manager     | Alarm processing           |
| config_manager    | Configuration storage      |
| logging           | Event logging              |
| diagnostics       | System diagnostics         |
| storage           | Persistent storage         |
| ota_manager       | Firmware updates           |
| display           | LVGL interface             |
| event_bus         | Inter-module communication |

---

# 7.8 FreeRTOS Task Model

The firmware shall execute as multiple independent FreeRTOS tasks.

| Task             | Priority | Responsibility       |
| ---------------- | -------: | -------------------- |
| task_safety      |       10 | Emergency monitoring |
| task_flow        |        9 | Flow processing      |
| task_irrigation  |        8 | Irrigation engine    |
| task_pressure    |        7 | Pressure supervision |
| task_mqtt        |        6 | MQTT communication   |
| task_ui          |        5 | LVGL                 |
| task_weather     |        5 | Weather processing   |
| task_logging     |        4 | Logging              |
| task_diagnostics |        3 | Diagnostics          |
| task_idle        |        0 | FreeRTOS Idle        |

Safety tasks shall always have highest priority.

---

# 7.9 State Machines

The controller shall use explicit state machines.

## Controller States

```text
BOOT

↓

INITIALIZATION

↓

SELF TEST

↓

IDLE

↓

IRRIGATING

↓

PAUSED

↓

RAIN DELAY

↓

SERVICE MODE

↓

OTA UPDATE

↓

FAULT

↓

EMERGENCY STOP
```

Every state transition shall be logged.

---

## Zone States

Each zone shall maintain its own state.

```text
Disabled

↓

Idle

↓

Waiting

↓

Starting

↓

Running

↓

Stopping

↓

Completed

↓

Fault
```

---

# 7.10 Event Bus

The Event Bus shall provide asynchronous communication between firmware modules.

Example events:

* Zone Started
* Zone Stopped
* Alarm Raised
* Alarm Cleared
* Flow Updated
* Pressure Updated
* Weather Updated
* MQTT Connected
* MQTT Disconnected
* OTA Started
* OTA Completed
* User Input
* Emergency Stop

The Event Bus shall support queued, non-blocking message delivery.

---

# 7.11 Hardware Abstraction Layer (HAL)

The HAL shall provide a consistent interface to all hardware.

Submodules include:

* GPIO
* ADC
* I²C
* UART
* SPI
* PCNT
* PWM
* Relay Driver
* Touch Interface
* Display Driver

Business logic shall never call ESP-IDF drivers directly.

---

# 7.12 Configuration Management

Configuration shall be stored in ESP-IDF NVS.

Configuration categories include:

* Network
* MQTT
* Zones
* Programs
* Sensors
* Weather
* Display
* Users
* Diagnostics

Each configuration structure shall include a version number to support migration between firmware releases.

---

# 7.13 Persistent Storage

The firmware shall use:

* NVS for configuration
* LittleFS for logs and exported data

Persistent data shall survive:

* Power failures
* OTA updates
* Watchdog resets

---

# 7.14 Logging Framework

The logging subsystem shall support:

* INFO
* WARNING
* ERROR
* CRITICAL
* DEBUG (development only)

Log destinations:

* Local storage
* MQTT
* Serial console
* Future remote diagnostics

---

# 7.15 Software Watchdog

The controller shall implement:

* Hardware watchdog
* Task watchdog
* Communication watchdog

If any critical task becomes unresponsive:

1. Attempt graceful recovery.
2. Restart failed task if possible.
3. Enter safe shutdown if recovery fails.
4. Log incident.
5. Publish alarm after restart.

---

# 7.16 OTA Framework

Firmware updates shall use the ESP-IDF OTA framework.

The update process shall include:

1. Download
2. Checksum verification
3. Digital signature verification (future)
4. Install inactive partition
5. Reboot
6. Self-test
7. Commit or rollback

Rollback shall occur automatically if the new firmware fails validation.

---

# 7.17 Software Quality Requirements

Firmware shall meet the following objectives.

| Requirement          |                       Target |
| -------------------- | ---------------------------: |
| Boot Time            |                        <30 s |
| UI Response          |                      <100 ms |
| MQTT Publish Latency |                         <1 s |
| Alarm Response       |                      <500 ms |
| Memory Leaks         |               None permitted |
| Watchdog Timeouts    | None during normal operation |

---

# 7.18 Coding Standards

The project shall adopt the following standards:

* Consistent naming conventions
* Self-documenting code
* Doxygen-compatible comments
* Static analysis before release
* Compiler warnings treated as errors
* Version control using Git

Each public API shall include:

* Purpose
* Parameters
* Return values
* Error codes
* Usage example

---

# 7.19 Software Versioning

Firmware shall follow Semantic Versioning.

Example:

```text
Major.Minor.Patch

5.0.0
5.1.0
5.1.3
6.0.0
```

Major versions may introduce architectural changes.

Minor versions introduce new functionality while maintaining compatibility.

Patch versions correct defects without changing functionality.

---

# 7.20 Software Baseline Approval

The software architecture defined in this chapter constitutes the approved firmware baseline for:

**ZIC-S3 Rev.B**

Future firmware changes shall maintain compatibility with this architecture unless explicitly approved through an Engineering Change Request (ECR).

---

# 7.21 Chapter Summary

This chapter establishes the approved firmware platform and software architecture for the Zmartify Irrigation Controller.

By adopting ESP-IDF, FreeRTOS, a modular component structure and an event-driven architecture, the platform is designed to support reliable long-term operation while remaining scalable and maintainable as new functionality and hardware revisions are introduced.

This software baseline provides the foundation for the detailed firmware architecture specified in **Volume 2 – Firmware Architecture**, while ensuring that all implementation remains consistent with the engineering principles established in Volume 1.

---

# End of Chapter 7

**Next Chapter**

**Chapter 8 – System Operational Concept (ConOps) & Operating Modes**

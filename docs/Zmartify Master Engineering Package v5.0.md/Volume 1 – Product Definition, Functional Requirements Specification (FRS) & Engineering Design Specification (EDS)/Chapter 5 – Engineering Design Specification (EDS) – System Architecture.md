# Chapter 5 – Engineering Design Specification (EDS) – System Architecture

---

# 5 Engineering Design Specification (EDS)

## 5.1 Purpose

The Engineering Design Specification (EDS) translates the Functional Requirements Specification into a complete technical architecture that forms the foundation for the Zmartify Irrigation Controller.

Where the FRS describes **what** the controller shall accomplish, the EDS describes **how** the system is architected to achieve those objectives.

This chapter defines the high-level architecture. Detailed implementation is covered in later volumes.

---

# 5.2 Engineering Design Principles

The ZIC platform is based on the following engineering principles:

### EDS-001 — Modularity

Hardware and software shall be divided into independent functional modules.

Each module shall expose well-defined interfaces while minimizing dependencies.

---

### EDS-002 — Layered Architecture

The controller shall follow a strict layered architecture.

Applications shall never communicate directly with hardware.

All hardware access shall occur through dedicated abstraction layers.

---

### EDS-003 — Event-Driven Operation

Subsystems shall communicate through asynchronous events.

Modules shall not call unrelated modules directly.

Benefits include:

* Loose coupling
* High reliability
* Easier testing
* Easier expansion

---

### EDS-004 — Fail-Safe Design

Any detected critical fault shall place the irrigation system into a hydraulically safe condition.

Safe condition is defined as:

* Master valve closed
* All irrigation zones closed
* Alarm generated
* Event logged
* MQTT notification published

---

### EDS-005 — Data Persistence

Configuration shall survive:

* Power failures
* Firmware updates
* Unexpected resets

Operational history shall survive normal power interruptions.

---

# 5.3 Overall System Architecture

The ZIC platform consists of six logical layers.

```text
+-------------------------------------------------------+
|                 User Interface Layer                  |
|        LVGL • Touch • Local Buttons • Display         |
+-------------------------------------------------------+
|             Application Services Layer                |
| Programs • Weather • Alarms • Diagnostics             |
+-------------------------------------------------------+
|             Irrigation Decision Layer                 |
| ET Engine • Zone Manager • Flow Manager               |
| Pressure Manager • Irrigation Engine                  |
+-------------------------------------------------------+
|                  Event Bus Layer                      |
| Event Queue • State Machine • Notifications           |
+-------------------------------------------------------+
|          Hardware Abstraction Layer (HAL)             |
| GPIO • I²C • ADC • PCNT • PWM • MQTT Drivers          |
+-------------------------------------------------------+
|                 ESP-IDF / FreeRTOS                    |
+-------------------------------------------------------+
```

---

# 5.4 Functional Subsystems

The controller consists of the following primary subsystems.

| ID    | Subsystem             |
| ----- | --------------------- |
| SS-01 | Irrigation Engine     |
| SS-02 | Relay Manager         |
| SS-03 | Zone Manager          |
| SS-04 | Weather Engine        |
| SS-05 | Flow Manager          |
| SS-06 | Pressure Manager      |
| SS-07 | Alarm Manager         |
| SS-08 | MQTT Manager          |
| SS-09 | User Interface        |
| SS-10 | Configuration Manager |
| SS-11 | OTA Manager           |
| SS-12 | Diagnostics Manager   |

Each subsystem shall operate independently while exchanging information through the Event Bus.

---

# 5.5 Irrigation Engine

The Irrigation Engine is the central decision-making component.

Responsibilities include:

* Program execution
* Manual irrigation
* Rain delay
* Seasonal adjustment
* ET adjustment
* Runtime calculations
* Zone sequencing
* Hydraulic verification
* Safety supervision

The Irrigation Engine shall never control hardware directly.

---

# 5.6 Relay Manager

The Relay Manager provides the only interface to irrigation outputs.

Responsibilities:

* Master valve control
* Zone relay control
* Active-low translation
* Safe startup
* Safe shutdown
* Relay diagnostics

Hardware abstraction:

```
ESP-IDF

↓

Relay Manager

↓

MCP23017

↓

ULN2803A

↓

HL-58S Relay Boards

↓

24 VAC Valves
```

No other firmware module shall directly access relay hardware.

---

# 5.7 Zone Manager

The Zone Manager maintains configuration and runtime state for each irrigation zone.

Each zone contains:

* Name
* Relay assignment
* Runtime
* Plant type
* Soil type
* Area
* Sprinkler type
* Expected flow
* Expected pressure
* ET coefficient
* Priority
* Enable state

The Zone Manager shall provide a consistent interface to the Irrigation Engine and User Interface.

---

# 5.8 Flow Manager

The Flow Manager continuously evaluates hydraulic performance.

Responsibilities include:

* Pulse counting
* Flow calculations
* Daily consumption
* Monthly consumption
* Lifetime consumption
* Flow learning
* Leak detection
* Statistical analysis

The Flow Manager shall establish and maintain hydraulic baselines for each zone.

---

# 5.9 Pressure Manager

The Pressure Manager supervises the hydraulic pressure throughout irrigation.

Responsibilities:

* Pressure measurement
* Filtering
* Calibration
* Pressure trend analysis
* Pressure fault detection

Pressure data shall be acquired through the ADS1115 precision ADC.

---

# 5.10 Weather Engine

The Weather Engine combines local and remote weather information.

Inputs include:

Local:

* Temperature
* Humidity
* Rainfall
* Wind
* UV
* Solar radiation

Remote:

* Forecast rainfall
* Forecast temperature
* Forecast wind
* Forecast humidity

Outputs:

* Irrigation recommendation
* ET calculations
* Rain delay recommendation
* Seasonal adjustment factor

---

# 5.11 Alarm Manager

The Alarm Manager is responsible for:

* Alarm creation
* Alarm prioritization
* Alarm acknowledgement
* MQTT publication
* Event logging
* User notification

Alarm categories:

* Information
* Warning
* Critical

Critical alarms shall invoke the system safety sequence.

---

# 5.12 MQTT Manager

The MQTT Manager provides all external communication.

Responsibilities:

* Broker connection
* Topic management
* Telemetry publication
* Command processing
* Retained messages
* Last Will & Testament (LWT)

The MQTT Manager shall remain independent of application logic.

---

# 5.13 Configuration Manager

Configuration Manager maintains all persistent configuration.

Configuration categories include:

* Network
* MQTT
* Zones
* Programs
* Weather
* Sensors
* Display
* Security

Configuration shall be stored in non-volatile memory with version control.

---

# 5.14 OTA Manager

The OTA Manager handles firmware updates.

Update sequence:

1. Download firmware
2. Verify signature/checksum
3. Store inactive image
4. Reboot
5. Self-test
6. Confirm update
7. Roll back if necessary

The controller shall always be capable of recovering from an interrupted update.

---

# 5.15 Diagnostics Manager

The Diagnostics Manager continuously evaluates controller health.

Monitored parameters include:

* CPU load
* Heap usage
* Stack usage
* Wi-Fi signal strength
* MQTT connection
* Relay status
* Flow sensor
* Pressure sensor
* 24 VAC monitor
* Cabinet temperature

Diagnostics shall be accessible through:

* Local UI
* MQTT
* Service Mode

---

# 5.16 Hardware Architecture

The approved hardware architecture is shown below.

```text
                  230 VAC
                      │
        ┌─────────────┴──────────────┐
        │                            │
   Hager ST315                  HDR-30-5
   24 VAC / 63 VA              5 VDC / 3 A
        │                            │
        │                     ESP32-S3 Display
        │                            │
        │                      I²C Bus
        │          ┌────────────┴─────────────┐
        │          │                          │
        │      MCP23017                 ADS1115
        │          │                          │
        │      ULN2803A              Pressure Sensor
        │          │
        │     HL-58S Relays
        │          │
        │      Irrigation Valves
        │
   Master Valve
```

---

# 5.17 Software Architecture

The software architecture follows strict separation of concerns.

```text
LVGL UI

↓

Application Services

↓

Business Logic

↓

Event Bus

↓

Hardware Abstraction Layer

↓

ESP-IDF Drivers

↓

Hardware
```

No application component shall bypass the Event Bus or HAL.

---

# 5.18 Safety Architecture

The controller implements multiple independent safety layers.

### Electrical Safety

* Surge protection
* Fuses
* Transformer isolation
* SELV separation

---

### Hydraulic Safety

* Flow supervision
* Pressure supervision
* Master valve
* Leak detection

---

### Software Safety

* Watchdog
* Task monitoring
* Exception handling
* OTA rollback

---

### Operational Safety

* Emergency Stop
* Manual override
* Service mode
* Alarm acknowledgement

---

# 5.19 Expansion Philosophy

Future hardware shall integrate without redesigning the platform.

Reserved interfaces include:

* RS485
* CAN Bus
* Additional I²C devices
* SPI peripherals
* Remote valve controllers
* Soil moisture sensors
* Weather stations
* Pump controllers

All future modules shall follow the same architectural principles.

---

# 5.20 Design Verification

The architecture defined in this chapter shall be verified against the following criteria.

| Requirement          | Verification Method |
| -------------------- | ------------------- |
| Layer separation     | Architecture review |
| Modularity           | Code review         |
| Safety strategy      | Failure testing     |
| Expandability        | Integration testing |
| Hardware abstraction | Firmware testing    |
| MQTT independence    | Functional testing  |

Verification shall occur before production release.

---

# 5.21 Chapter Summary

This Engineering Design Specification establishes the architectural foundation of the Zmartify Irrigation Controller.

The layered architecture, modular subsystem design and event-driven communication model provide a scalable, maintainable and testable platform capable of supporting future hardware revisions and firmware enhancements while maintaining backward compatibility.

The subsequent chapters build upon this foundation by defining the detailed engineering requirements for each subsystem and their interactions.

---

# End of Chapter 5

**Next Chapter**

**Chapter 6 – System Hardware Baseline & Platform Specification**

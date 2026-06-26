# Zmartify Master Engineering Package v5.0

## Volume 1 – Product Definition, Functional Requirements Specification (FRS) & Engineering Design Specification (EDS)

---

# Chapter 1 – Document Control

---

## 1.1 Document Information

| Item                   | Value                                                                                                    |
| ---------------------- | -------------------------------------------------------------------------------------------------------- |
| Document Title         | Zmartify Master Engineering Package                                                                      |
| Volume                 | Volume 1                                                                                                 |
| Subtitle               | Product Definition, Functional Requirements Specification (FRS) & Engineering Design Specification (EDS) |
| Document ID            | ZIC-MEP-V5-VOL1                                                                                          |
| Product                | Zmartify Irrigation Controller                                                                           |
| Product Abbreviation   | ZIC                                                                                                      |
| Hardware Platform      | ZIC-S3 Rev.B                                                                                             |
| Firmware Platform      | ESP-IDF                                                                                                  |
| UI Framework           | LVGL                                                                                                     |
| Communication Standard | MQTT                                                                                                     |
| Document Revision      | 5.0                                                                                                      |
| Status                 | Draft – Engineering Baseline                                                                             |
| Classification         | Engineering Documentation                                                                                |
| Language               | English                                                                                                  |
| File Format            | Markdown (.md)                                                                                           |

---

# 1.2 Purpose

This document defines the overall product architecture and engineering requirements for the Zmartify Irrigation Controller (ZIC).

It establishes the technical baseline for all subsequent hardware, firmware, manufacturing and integration activities.

This document shall serve as the authoritative engineering reference for:

* Hardware development
* Firmware development
* User interface development
* Smart-home integration
* PCB development
* Manufacturing
* Testing
* Commissioning
* Future product revisions

No implementation shall intentionally deviate from this document without an approved engineering change.

---

# 1.3 Scope

This volume defines:

* Product definition
* Product vision
* Functional Requirements Specification (FRS)
* Engineering Design Specification (EDS)
* Overall system architecture
* Product objectives
* System constraints
* High-level design philosophy

Detailed implementation is defined in later volumes.

---

# 1.4 Related Documents

| Document ID     | Document                            |
| --------------- | ----------------------------------- |
| ZIC-MEP-V5-VOL2 | Firmware Architecture               |
| ZIC-MEP-V5-VOL3 | Communication & API Specification   |
| ZIC-MEP-V5-VOL4 | Hardware Design                     |
| ZIC-MEP-V5-VOL5 | Manufacturing, Testing & Operations |

Supporting Appendices

| Appendix   | Description       |
| ---------- | ----------------- |
| Appendix A | GPIO Allocation   |
| Appendix B | MQTT Topics       |
| Appendix C | JSON Schemas      |
| Appendix D | Wiring Standards  |
| Appendix E | Bill of Materials |

---

# 1.5 Intended Audience

This document is intended for:

### Product Owner

Responsible for product direction and feature approval.

---

### System Architect

Responsible for overall system architecture.

---

### Hardware Engineer

Responsible for electrical and mechanical implementation.

---

### Firmware Engineer

Responsible for ESP-IDF software development.

---

### UI Developer

Responsible for the graphical user interface.

---

### Integration Engineer

Responsible for MQTT, HOMEIO and Home Assistant integration.

---

### Manufacturing Engineer

Responsible for production documentation.

---

### Test Engineer

Responsible for Factory Acceptance Testing (FAT) and Site Acceptance Testing (SAT).

---

### Service Technician

Responsible for commissioning and maintenance.

---

# 1.6 Product Overview

The Zmartify Irrigation Controller (ZIC) is a professional irrigation controller designed for medium and large landscape installations.

The controller combines:

* Local touchscreen operation
* Intelligent irrigation scheduling
* Weather-based irrigation
* Flow monitoring
* Pressure monitoring
* Leak detection
* MQTT integration
* Smart-home integration
* OTA firmware updates
* Industrial reliability

The controller is designed to operate independently of cloud services.

Internet connectivity shall enhance functionality but shall never be required for safe irrigation operation.

---

# 1.7 Product Goals

The primary goals of the project are:

1. Minimise water consumption.
2. Maximise irrigation quality.
3. Reduce maintenance.
4. Detect hydraulic faults automatically.
5. Operate autonomously.
6. Support local control at all times.
7. Integrate with modern smart-home systems.
8. Provide long-term reliability.
9. Maintain complete operational history.
10. Support future expansion.

---

# 1.8 Approved Hardware Baseline

The following hardware constitutes the approved baseline for ZIC-S3 Rev.B.

## Controller

* Waveshare ESP32-S3 7" Touch Display
* 1024 × 600 LCD
* Capacitive Touch
* ESP32-S3 Dual-Core LX7
* Wi-Fi 2.4 GHz
* Bluetooth

## Digital I/O

* MCP23017
* I²C Interface

## Output Driver

* ULN2803A

## Relay Modules

* HL-58S V1.2
* Active-Low Logic
* 2 × 8-Channel Modules
* 16 Total Outputs

## Valve Power Supply

* Hager ST315
* 230 VAC Input
* 24 VAC Output
* 63 VA
* DIN Rail Mounted

## Logic Power Supply

* Mean Well HDR-30-5
* 5 VDC
* 3 A
* DIN Rail Mounted

## Sensors

* DN50 G2 Turbine Flow Meter
* 0–10 bar Pressure Transmitter
* ADS1115 Precision ADC
* MCP9808 Temperature Sensor
* Cabinet Reed Switch
* 24 VAC Presence Monitor

## Reserved Interfaces

* RS485
* CAN Bus (Future)
* Soil Moisture Sensors
* Weather Station

---

# 1.9 Approved Software Baseline

Operating Environment

* ESP-IDF
* FreeRTOS

Graphics

* LVGL

Protocols

* MQTT
* OTA Update

Supported Integrations

* HOMEIO
* Home Assistant
* Homey
* Node-RED

---

# 1.10 Document Conventions

The following terminology shall be used throughout the engineering package.

| Prefix | Meaning                    |
| ------ | -------------------------- |
| FR     | Functional Requirement     |
| HW     | Hardware Requirement       |
| FW     | Firmware Requirement       |
| UI     | User Interface Requirement |
| MQTT   | MQTT Requirement           |
| SAF    | Safety Requirement         |
| ELE    | Electrical Requirement     |
| MECH   | Mechanical Requirement     |
| TEST   | Verification Requirement   |

Requirement Example

```
FR-IRR-001

Title:
Maximum Irrigation Zones

Requirement:

The controller shall support
15 irrigation zones
and
1 dedicated master valve.

Priority:
Mandatory

Verification:
Functional Test

Status:
Approved
```

---

# 1.11 Revision Control

Engineering documentation shall be maintained using semantic versioning.

Major Revision

Changes affecting:

* System architecture
* Hardware platform
* Firmware architecture
* Product capabilities

Example:

Version 5.x

Minor Revision

Changes affecting:

* New features
* Documentation improvements
* Clarifications

Example:

Version 5.1

Patch Revision

Changes affecting:

* Typographical corrections
* Minor diagrams
* Formatting

Example:

Version 5.0.1

---

# 1.12 Engineering Change Control

All engineering changes shall be documented.

Each change shall include:

* Change ID
* Description
* Reason
* Author
* Approval
* Date
* Affected documents
* Verification status

No hardware or firmware changes shall be released without updating the Master Engineering Package.

---

# 1.13 Approval Status

Current Status

Draft

Hardware Baseline

Approved

Firmware Baseline

Approved

Production Release

Pending

PCB Revision

Pending

Manufacturing Approval

Pending

---

# End of Chapter 1

**Next Chapter**

Chapter 2 – Executive Summary & Product Vision

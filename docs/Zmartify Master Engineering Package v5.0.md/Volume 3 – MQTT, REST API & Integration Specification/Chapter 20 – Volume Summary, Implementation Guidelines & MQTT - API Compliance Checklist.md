# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 20

# Volume Summary, Implementation Guidelines & MQTT/API Compliance Checklist

---

# 20.1 Purpose

This final chapter consolidates the engineering principles presented throughout **Volume 3** and establishes the implementation requirements for all firmware, software and third-party integrations using the Zmartify communication architecture.

It serves as the definitive implementation baseline for:

* Firmware developers
* Software developers
* Integration partners
* Test engineers
* QA engineers
* System integrators
* Future Zmartify product teams

Compliance with this chapter ensures interoperability across the entire Zmartify ecosystem.

---

# 20.2 Scope of Volume 3

Volume 3 defines the complete external communication architecture of the Zmartify platform.

Covered interfaces include:

* MQTT Telemetry
* MQTT Commands
* MQTT Discovery
* MQTT Events
* JSON Schemas
* Home Assistant Integration
* HOMEIO Integration
* Homey Integration
* Node-RED Integration
* Third-party Interfaces
* REST API Architecture
* WebSocket Architecture
* Security Architecture
* API Versioning

---

# 20.3 Communication Philosophy

The Zmartify platform follows three fundamental communication principles:

### Principle 1

**MQTT is the canonical communication protocol.**

All external integrations shall be based on the documented MQTT interface.

---

### Principle 2

**Business logic is transport independent.**

MQTT, REST and WebSocket interfaces all invoke the same internal application services.

---

### Principle 3

**The public API is contractual.**

Internal firmware architecture may evolve without affecting public interfaces.

---

# 20.4 Architectural Layers

```text
Applications

↓

REST / MQTT / WebSocket

↓

API Translation Layer

↓

Application Managers

↓

Internal Event Bus

↓

Drivers

↓

Hardware
```

Only the Application Managers communicate with hardware managers.

---

# 20.5 Engineering Rules

The following rules are mandatory.

### API-001

Never rename published MQTT topics.

---

### API-002

Never remove required JSON fields.

---

### API-003

Always preserve backwards compatibility.

---

### API-004

Every command requires a response.

---

### API-005

Every command requires validation.

---

### API-006

Every controller shall publish discovery information.

---

### API-007

Every controller shall publish Last Will information.

---

### API-008

Critical alarms shall use MQTT QoS 2.

---

### API-009

Controller operation shall never depend on external software.

---

### API-010

Firmware shall continue autonomous irrigation if MQTT becomes unavailable.

---

# 20.6 MQTT Compliance Checklist

## Controller

| Requirement  | Status |
| ------------ | :----: |
| Status Topic |    □   |
| Discovery    |    □   |
| Last Will    |    □   |
| Heartbeat    |    □   |
| Identity     |    □   |

---

## Weather

| Requirement     | Status |
| --------------- | :----: |
| Current Weather |    □   |
| Forecast        |    □   |
| ET              |    □   |
| Recommendation  |    □   |

---

## Irrigation

| Requirement    | Status |
| -------------- | :----: |
| Zone State     |    □   |
| Runtime        |    □   |
| Remaining Time |    □   |
| Program Status |    □   |
| Statistics     |    □   |

---

## Hydraulics

| Requirement      | Status |
| ---------------- | :----: |
| Flow             |    □   |
| Pressure         |    □   |
| Learning         |    □   |
| Calibration      |    □   |
| Hydraulic Health |    □   |

---

## Diagnostics

| Requirement | Status |
| ----------- | :----: |
| CPU         |    □   |
| Memory      |    □   |
| Wi-Fi       |    □   |
| MQTT        |    □   |
| Storage     |    □   |
| Self-Test   |    □   |

---

## Commands

| Requirement       | Status |
| ----------------- | :----: |
| Manual Irrigation |    □   |
| Pause             |    □   |
| Resume            |    □   |
| Stop              |    □   |
| OTA               |    □   |
| Backup            |    □   |
| Restore           |    □   |
| Reboot            |    □   |

---

## Events

| Requirement    | Status |
| -------------- | :----: |
| Zone Events    |    □   |
| Program Events |    □   |
| Alarm Events   |    □   |
| Weather Events |    □   |
| OTA Events     |    □   |

---

# 20.7 JSON Compliance Checklist

Each payload shall satisfy:

| Requirement        | Mandatory |
| ------------------ | :-------: |
| Valid JSON         |     ✔     |
| UTF-8 Encoding     |     ✔     |
| ISO-8601 Timestamp |     ✔     |
| Device Identifier  |     ✔     |
| Message Type       |     ✔     |
| Payload Object     |     ✔     |

Recommended:

* Stable field names
* Human-readable enumerations
* SI units
* Minimal payload size

---

# 20.8 Security Compliance Checklist

| Requirement                |  Mandatory  |
| -------------------------- | :---------: |
| Authentication             |      ✔      |
| Authorization              |      ✔      |
| JSON Validation            |      ✔      |
| Transaction IDs            |      ✔      |
| Replay Protection          |      ✔      |
| TLS Support                | Recommended |
| Audit Logging              |      ✔      |
| OTA Signature Verification |      ✔      |

---

# 20.9 Home Assistant Checklist

The implementation shall verify:

* MQTT Discovery
* Device Registry
* Entity Registry
* Availability
* Diagnostics
* Automatic reconnect
* Stable Unique IDs

No manual YAML configuration should be required.

---

# 20.10 HOMEIO Checklist

Verify:

* Automatic discovery
* Engineering entities
* Flow
* Pressure
* Weather
* Diagnostics
* Alarms
* Bidirectional commands

---

# 20.11 Homey Checklist

Verify:

* Device discovery
* Capabilities
* Flow Cards
* Condition Cards
* Action Cards
* Insights
* Notifications

---

# 20.12 Third-Party Integration Checklist

Applications shall verify:

* MQTT connectivity
* Discovery compatibility
* JSON schema validation
* Command acknowledgement
* Error handling
* Automatic reconnection

Applications shall never depend upon undocumented firmware behavior.

---

# 20.13 Testing Requirements

Every firmware release shall include:

* MQTT regression tests
* JSON schema validation
* Discovery verification
* Command interface verification
* Alarm verification
* OTA verification
* Integration testing
* Security testing

Testing procedures are specified in **Volume 7 – Verification & Validation**.

---

# 20.14 Future Evolution

Future firmware versions may introduce:

* Additional MQTT topics
* Additional discovery metadata
* New JSON fields
* REST resources
* WebSocket channels
* Matter bridges
* OPC UA gateway
* Cloud synchronization

These additions shall preserve compatibility with API Version 1.x.

---

# 20.15 Relationship to Other Volumes

Volume 3 interfaces directly with:

| Volume    | Relationship                         |
| --------- | ------------------------------------ |
| Volume 1  | Overall System Architecture          |
| Volume 2  | Firmware Architecture                |
| Volume 4  | Display & User Interface             |
| Volume 5  | Hardware Design                      |
| Volume 6  | Manufacturing & Production           |
| Volume 7  | Verification & Validation            |
| Volume 8  | Installation & Service               |
| Volume 9  | Operations & Maintenance             |
| Volume 10 | Developer SDK & Integration Examples |

Volume 3 is the definitive interface specification referenced by all other engineering documentation.

---

# 20.16 Engineering Notes

Volume 3 establishes the communication contract for the entire Zmartify ecosystem. By defining stable MQTT namespaces, standardized JSON schemas, secure command processing and platform-neutral integration principles, it decouples external software from internal firmware implementation.

This architectural separation enables continuous firmware evolution while protecting long-term compatibility for dashboards, automation systems and third-party applications. It also provides the foundation for future protocol adapters—including REST, WebSocket, Matter and OPC UA—without requiring changes to the core application model.

---

# 20.17 Volume Summary

Volume 3 specifies the complete external communication architecture of the Zmartify Irrigation Controller.

The document defines:

* A comprehensive MQTT namespace
* Standardized JSON message formats
* Transactional command processing
* Automatic discovery mechanisms
* Native integrations for HOMEIO, Home Assistant and Homey
* Third-party integration guidelines
* Security architecture
* API versioning strategy
* Future REST and WebSocket interfaces
* Compliance and implementation checklists

Together, these specifications form a stable, extensible and transport-independent communication framework that supports the current Zmartify Irrigation Controller and the broader Zmartify ecosystem for many years to come.

---

# Volume 3 Revision History

| Version | Date            | Description                                    |
| ------- | --------------- | ---------------------------------------------- |
| 5.0     | Initial Release | Complete MQTT, API & Integration Specification |
| 5.x     | Future          | Backward-compatible enhancements only          |
| 6.0     | Future          | Major API revision (if required)               |

---

# End of Volume 3

**Status**

**Volume 3 – Complete**

**Approximate Size:** 180–220 engineering pages

**Next Volume**

**Volume 4 – Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification**

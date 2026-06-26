# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

**Document ID:** ZMEP-V3-5.0

**Revision:** A

**Status:** Engineering Baseline

**Classification:** Public Interface Specification

**Author:** OpenAI / Zmartify Engineering

**Date:** July 2026

---

# Revision History

| Revision | Date      | Description                  |
| -------- | --------- | ---------------------------- |
| A        | July 2026 | Initial Engineering Baseline |

---

# Related Documents

This document shall be read together with:

| Document | Description             |
| -------- | ----------------------- |
| Volume 1 | System Architecture     |
| Volume 2 | Firmware Architecture   |
| Volume 4 | Hardware Design Package |
| Volume 5 | PCB Design Package      |

---

# Chapter 1

# Communication Architecture & Design Philosophy

---

# 1.1 Purpose

This volume specifies every external interface implemented by the Zmartify Irrigation Controller.

Where Volume 2 defines the internal firmware architecture, this document defines how external systems communicate with the controller.

It therefore serves as the Interface Control Document (ICD) for the complete Zmartify ecosystem.

The interfaces described herein are considered stable engineering interfaces intended to remain backwards compatible throughout the lifetime of the Version 5.x firmware series.

---

# 1.2 Objectives

The communication architecture has been designed to satisfy the following objectives:

* Simple integration
* High reliability
* Deterministic behaviour
* Hardware independence
* Platform independence
* Offline operation
* Long-term API stability
* Minimal network bandwidth
* Easy third-party integration

---

# 1.3 Intended Audience

This document is intended for:

* Firmware developers
* HOMEIO developers
* Home Assistant users
* Homey developers
* MQTT broker administrators
* Node-RED users
* Integrators
* Third-party application developers
* Future Zmartify product developers

---

# 1.4 Scope

This volume specifies:

* MQTT communication
* MQTT namespace
* Topic hierarchy
* Topic ownership
* JSON payloads
* Command interface
* Event interface
* Discovery
* Authentication
* Security
* Future REST API
* Future WebSocket API
* Integration recommendations
* Version compatibility

---

# 1.5 Communication Philosophy

Unlike many IoT devices, the Zmartify controller has **not** been designed around cloud connectivity.

Instead, the controller follows an **Offline First** philosophy.

The irrigation controller shall always remain fully operational even if:

* Internet access is unavailable
* MQTT broker is unavailable
* Smart Home platform is offline
* DNS is unavailable
* NTP synchronization fails

The communication layer enhances the controller but never replaces its autonomous operation.

---

# 1.6 Architectural Overview

The communication subsystem is isolated from the remainder of the firmware through the MQTT Manager.

```text
                 User Interface

                       │

                 Event Bus

                       │

                       ▼

                MQTT Manager

                       │

          ESP-IDF MQTT Client

                       │

                    Wi-Fi

                       │

                 MQTT Broker

          ┌─────────┼─────────┐

          ▼         ▼         ▼

      HOMEIO   Home Assistant   Homey

```

No firmware module communicates directly with the MQTT client.

---

# 1.7 Communication Layers

The communication architecture consists of six independent layers.

| Layer        | Responsibility         |
| ------------ | ---------------------- |
| Application  | Irrigation logic       |
| Event Bus    | Internal communication |
| MQTT Manager | Translation layer      |
| ESP-IDF      | MQTT implementation    |
| TCP/IP       | Network transport      |
| Wi-Fi        | Physical communication |

This separation allows future migration to Ethernet, Thread or CAN without modifying application software.

---

# 1.8 Interface Principles

The following engineering principles apply throughout this document.

## COM-001

Publish State

The controller publishes actual controller state.

Clients shall never infer controller state.

---

## COM-002

Loose Coupling

Communication shall occur through abstract interfaces.

No application module depends upon MQTT.

---

## COM-003

Deterministic Operation

Communication delays shall never influence irrigation timing.

---

## COM-004

Platform Independence

Every interface shall operate identically regardless of:

* HOMEIO
* Home Assistant
* Homey
* Node-RED
* Custom software

---

## COM-005

Backward Compatibility

Future firmware revisions shall preserve existing topic names whenever practical.

Breaking API changes shall only occur with a major API version.

---

# 1.9 Supported Communication Interfaces

Current interfaces:

| Interface     | Status     |
| ------------- | ---------- |
| MQTT          | Production |
| OTA HTTPS     | Production |
| Local Display | Production |

Future interfaces:

* REST API
* WebSocket API
* Matter
* Thread
* Modbus TCP
* CAN Bus
* RS485

---

# 1.10 Why MQTT?

MQTT has been selected because it provides:

* Low bandwidth
* Low latency
* Publish/Subscribe architecture
* Excellent Home Assistant support
* Native HOMEIO compatibility
* Excellent Homey support
* Proven industrial reliability

Alternative protocols may be supported in future, but MQTT shall remain the primary integration interface.

---

# 1.11 Controller Identity

Every controller has a globally unique identity.

Example:

```
ZIC-S3-202600001
```

Naming convention:

```
<Product>-<Hardware>-<Serial Number>
```

Examples:

```
ZIC-S3-202600001

ZIC-S3-202600002

ZIC-S3-202600003
```

The Device ID shall never change during the controller lifetime.

---

# 1.12 MQTT Namespace

All products use a common namespace.

```
zmartify/
```

Current product:

```
zmartify/controller/
```

Future ecosystem:

```
zmartify/

controller/

pump/

lighting/

weather/

tank/

greenhouse/

sensor/

gateway/
```

The namespace is intentionally generic to support future products.

---

# 1.13 Message Categories

Every MQTT message belongs to one of four categories.

### State

Continuous controller state.

Example:

```
controller/status
```

---

### Telemetry

Measured values.

Example:

```
flow/current
```

---

### Events

Instantaneous notifications.

Example:

```
event/alarm
```

---

### Commands

Incoming requests.

Example:

```
command/start
```

---

# 1.14 Message Lifecycle

A typical communication sequence is shown below.

```
Zone Starts

↓

Event Bus

↓

MQTT Manager

↓

Publish

↓

Broker

↓

HOMEIO

↓

Dashboard Update
```

Communication shall always originate from actual controller state.

---

# 1.15 JSON Design Philosophy

Every MQTT payload follows the same engineering structure.

```json
{
    "timestamp":"2026-07-14T10:42:13Z",
    "device":"ZIC-S3-202600001",
    "firmware":"5.0.0",
    "message":"telemetry",
    "payload":
    {

    }
}
```

This allows generic software libraries to parse all messages consistently.

---

# 1.16 Time Synchronization

All timestamps shall use:

**ISO-8601 UTC**

Example:

```
2026-07-14T10:42:13Z
```

Reasons:

* Global consistency
* No daylight-saving ambiguity
* Easy database storage
* Platform independence

Time zone conversion shall be performed by the client.

---

# 1.17 Engineering Units

Unless otherwise specified:

| Parameter   | Unit    |
| ----------- | ------- |
| Pressure    | bar     |
| Flow        | L/min   |
| Water       | L       |
| Runtime     | seconds |
| Rain        | mm      |
| Wind        | m/s     |
| Temperature | °C      |
| Humidity    | %       |
| Voltage     | V       |
| Current     | A       |

---

# 1.18 Reliability Strategy

Communication failures shall never affect irrigation.

Examples:

Broker Offline

↓

Controller continues normally

Wi-Fi Lost

↓

Programs continue

Internet Lost

↓

Weather uses cached data

MQTT Lost

↓

Local controller unaffected

---

# 1.19 Broker Compatibility

The controller has been verified against:

* Eclipse Mosquitto
* EMQX
* HiveMQ
* Home Assistant Mosquitto Add-on

Future compatibility:

* AWS IoT
* Azure IoT Hub
* Google IoT Core equivalents

No broker-specific extensions are required.

---

# 1.20 Integration Philosophy

The communication interface has deliberately been designed around open standards.

No vendor-specific protocol shall be required.

Consequently, any software capable of standard MQTT communication can integrate with the controller.

This makes the platform suitable for:

* Residential automation
* Commercial irrigation
* Agricultural monitoring
* Municipal installations
* Research applications

---

# 1.21 Engineering Notes

Unlike conventional smart irrigation controllers that expose only a limited set of MQTT topics, the Zmartify platform is designed around a comprehensive and stable communication model.

The MQTT namespace forms a long-term Application Programming Interface (API) that is independent of firmware implementation details. This allows dashboards, automation rules and third-party integrations to remain compatible across future firmware updates.

The communication architecture also reflects the broader vision of Zmartify as an ecosystem rather than a single controller, providing a consistent foundation for future pump controllers, lighting controllers, weather stations and other intelligent outdoor automation products.

---

# 1.22 Chapter Summary

This chapter establishes the architectural principles governing all external communications with the Zmartify Irrigation Controller.

The following chapters build upon this foundation by defining the complete MQTT namespace, topic hierarchy, payload schemas, command interfaces and discovery mechanisms required for reliable integration with HOMEIO, Home Assistant, Homey and future Zmartify ecosystem devices.

The architecture emphasizes stability, interoperability and long-term maintainability, ensuring that the communication layer can evolve alongside the firmware without disrupting existing integrations.

---

# End of Chapter 1

**Next Chapter**

**Chapter 2 – Complete MQTT Namespace & Topic Hierarchy**

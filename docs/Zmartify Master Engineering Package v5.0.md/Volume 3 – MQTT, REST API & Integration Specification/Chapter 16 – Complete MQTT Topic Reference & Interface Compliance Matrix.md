# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 16

# Complete MQTT Topic Reference & Interface Compliance Matrix

---

# 16.1 Purpose

This chapter serves as the normative reference for every MQTT topic defined within the Zmartify platform.

Unlike previous chapters, which explain architectural concepts and subsystem behavior, this chapter provides a consolidated engineering reference suitable for:

* Firmware developers
* Integration developers
* Test engineers
* Commissioning engineers
* QA engineers
* Future product development

This chapter shall be considered the authoritative reference for the MQTT namespace implemented by firmware version **5.x** and API version **1.x**.

---

# 16.2 Scope

The chapter documents:

* Complete topic hierarchy
* Topic ownership
* QoS level
* Retained status
* Publication strategy
* Publisher module
* Subscriber expectations
* Payload type
* Engineering requirements
* Compliance verification

---

# 16.3 Topic Classification

Every MQTT topic belongs to one of the following classes.

| Class         | Description                 |
| ------------- | --------------------------- |
| State         | Current controller state    |
| Telemetry     | Measured engineering values |
| Command       | Incoming requests           |
| Event         | One-shot notifications      |
| Configuration | Persistent settings         |
| Discovery     | Automatic registration      |
| Diagnostics   | Engineering health data     |
| Statistics    | Historical information      |
| Management    | Administrative operations   |

Each class has unique publication and retention requirements.

---

# 16.4 Root Namespace

The root namespace is reserved for the Zmartify ecosystem.

```text
zmartify/
```

The root namespace shall remain unchanged throughout the lifetime of API Version 1.x.

---

# 16.5 Complete Namespace Tree

```text
zmartify/

├── controller/
├── system/
├── irrigation/
├── zones/
├── flow/
├── pressure/
├── hydraulics/
├── weather/
├── alarms/
├── diagnostics/
├── configuration/
├── statistics/
├── discovery/
├── ota/
├── commands/
├── response/
├── events/
└── integration/
```

---

# 16.6 Controller Topics

| Topic                   | Publisher       | QoS | Retained | Type      |
| ----------------------- | --------------- | :-: | :------: | --------- |
| controller/status       | System Manager  |  1  |    Yes   | State     |
| controller/identity     | System Manager  |  1  |    Yes   | State     |
| controller/hardware     | System Manager  |  1  |    Yes   | State     |
| controller/firmware     | System Manager  |  1  |    Yes   | State     |
| controller/network      | Network Manager |  1  |    Yes   | State     |
| controller/heartbeat    | System Manager  |  0  |    No    | Telemetry |
| controller/uptime       | System Manager  |  0  |    No    | Telemetry |
| controller/capabilities | System Manager  |  1  |    Yes   | Discovery |

---

# 16.7 System Topics

| Topic         | Publisher           | QoS | Retained |
| ------------- | ------------------- | :-: | :------: |
| system/state  | System Manager      |  1  |    Yes   |
| system/health | Diagnostics Manager |  1  |    Yes   |
| system/time   | System Manager      |  0  |    No    |
| system/cpu    | Diagnostics Manager |  0  |    No    |
| system/memory | Diagnostics Manager |  0  |    No    |

---

# 16.8 Irrigation Topics

| Topic                 | Publisher          | QoS | Retained |
| --------------------- | ------------------ | :-: | :------: |
| irrigation/state      | Irrigation Engine  |  1  |    Yes   |
| irrigation/program    | Program Scheduler  |  1  |    Yes   |
| irrigation/runtime    | Irrigation Engine  |  0  |    No    |
| irrigation/remaining  | Irrigation Engine  |  0  |    No    |
| irrigation/statistics | Statistics Manager |  0  |    No    |
| irrigation/history    | Statistics Manager |  1  |    No    |

---

# 16.9 Zone Topics

Each irrigation zone shall implement the following namespace.

Example:

```text
zmartify/zones/04/
```

Mandatory topics:

| Topic         | QoS | Retained |
| ------------- | :-: | :------: |
| state         |  1  |    Yes   |
| runtime       |  0  |    No    |
| remaining     |  0  |    No    |
| flow          |  0  |    No    |
| pressure      |  0  |    No    |
| water         |  0  |    No    |
| budget        |  1  |    Yes   |
| configuration |  1  |    Yes   |
| alarm         |  1  |    No    |

---

# 16.10 Weather Topics

| Topic                  | QoS | Retained |
| ---------------------- | :-: | :------: |
| weather/current        |  0  |    Yes   |
| weather/forecast       |  0  |    Yes   |
| weather/hourly         |  0  |    Yes   |
| weather/daily          |  0  |    Yes   |
| weather/temperature    |  0  |    Yes   |
| weather/humidity       |  0  |    Yes   |
| weather/rain           |  0  |    Yes   |
| weather/wind           |  0  |    Yes   |
| weather/solar          |  0  |    Yes   |
| weather/uv             |  0  |    Yes   |
| weather/et             |  1  |    Yes   |
| weather/recommendation |  1  |    Yes   |

---

# 16.11 Hydraulic Topics

| Topic                | QoS | Retained |
| -------------------- | :-: | :------: |
| flow/current         |  1  |    Yes   |
| flow/statistics      |  0  |    No    |
| flow/learning        |  1  |    Yes   |
| flow/calibration     |  1  |    Yes   |
| pressure/current     |  1  |    Yes   |
| pressure/learning    |  1  |    Yes   |
| pressure/calibration |  1  |    Yes   |
| hydraulics/status    |  1  |    Yes   |
| hydraulics/health    |  1  |    Yes   |

---

# 16.12 Alarm Topics

| Topic             | QoS | Retained |
| ----------------- | :-: | :------: |
| alarms/current    |  2  |    Yes   |
| alarms/active     |  2  |    Yes   |
| alarms/history    |  1  |    No    |
| alarms/statistics |  1  |    Yes   |

Critical alarm topics shall always use QoS 2.

---

# 16.13 Diagnostics Topics

| Topic                | QoS | Retained |
| -------------------- | :-: | :------: |
| diagnostics/health   |  1  |    Yes   |
| diagnostics/cpu      |  0  |    No    |
| diagnostics/memory   |  0  |    No    |
| diagnostics/storage  |  0  |    No    |
| diagnostics/network  |  0  |    No    |
| diagnostics/wifi     |  0  |    No    |
| diagnostics/mqtt     |  0  |    No    |
| diagnostics/tasks    |  0  |    No    |
| diagnostics/selftest |  1  |    Yes   |

---

# 16.14 Configuration Topics

| Topic                  | QoS | Retained |
| ---------------------- | :-: | :------: |
| configuration/system   |  1  |    Yes   |
| configuration/network  |  1  |    Yes   |
| configuration/zones    |  1  |    Yes   |
| configuration/programs |  1  |    Yes   |
| configuration/weather  |  1  |    Yes   |
| configuration/display  |  1  |    Yes   |
| configuration/security |  1  |    Yes   |

---

# 16.15 OTA Topics

| Topic        | QoS | Retained |
| ------------ | :-: | :------: |
| ota/status   |  1  |    Yes   |
| ota/progress |  1  |    No    |
| ota/version  |  1  |    Yes   |
| ota/history  |  1  |    Yes   |

---

# 16.16 Discovery Topics

| Topic                  | QoS | Retained |
| ---------------------- | :-: | :------: |
| discovery/controller   |  1  |    Yes   |
| discovery/entities     |  1  |    Yes   |
| discovery/capabilities |  1  |    Yes   |

These topics are published automatically at startup and after MQTT reconnection.

---

# 16.17 Event Topics

| Topic                | QoS | Retained |
| -------------------- | :-: | :------: |
| events/system        |  1  |    No    |
| events/zone          |  1  |    No    |
| events/program       |  1  |    No    |
| events/weather       |  1  |    No    |
| events/hydraulics    |  1  |    No    |
| events/alarm         |  2  |    No    |
| events/configuration |  1  |    No    |
| events/ota           |  1  |    No    |

Events shall never be retained.

---

# 16.18 Command Topics

| Topic               | QoS | Retained |
| ------------------- | :-: | :------: |
| commands/manual     |  2  |    No    |
| commands/start      |  2  |    No    |
| commands/stop       |  2  |    No    |
| commands/pause      |  2  |    No    |
| commands/resume     |  2  |    No    |
| commands/skip       |  2  |    No    |
| commands/reboot     |  2  |    No    |
| commands/selftest   |  2  |    No    |
| commands/backup     |  2  |    No    |
| commands/restore    |  2  |    No    |
| commands/ota/update |  2  |    No    |

Every command shall produce a response message.

---

# 16.19 Response Topic

Generic response topic:

```text
zmartify/response
```

Every response shall include:

* Transaction ID
* Timestamp
* Status
* Result code
* Human-readable message

---

# 16.20 MQTT Manager Responsibilities

The MQTT Manager shall be solely responsible for:

* Topic ownership
* Publication scheduling
* QoS assignment
* Retained messages
* Serialization
* JSON validation
* Command routing
* Response generation
* Discovery publication

Subsystem managers shall never publish directly to MQTT.

---

# 16.21 Compliance Matrix

| Requirement           | Verification Method     |
| --------------------- | ----------------------- |
| Topic exists          | MQTT namespace test     |
| QoS correct           | Broker inspection       |
| Retained flag correct | Broker restart test     |
| JSON valid            | JSON Schema validation  |
| Timestamp UTC         | Payload inspection      |
| Transaction ID unique | Command regression test |
| Discovery published   | Startup verification    |
| Last Will published   | Network disconnect test |

This matrix forms the basis of the MQTT compliance test suite described in **Volume 7 – Verification & Validation**.

---

# 16.22 Conformance Levels

| Level   | Description                  |
| ------- | ---------------------------- |
| Level 1 | Core MQTT communication      |
| Level 2 | Telemetry and events         |
| Level 3 | Command interface            |
| Level 4 | Discovery support            |
| Level 5 | Full ecosystem compatibility |

A production Zmartify controller shall achieve **Level 5** compliance.

---

# 16.23 Future Namespace Reservation

The following namespaces are reserved for future ecosystem products:

```text
zmartify/pump/
zmartify/tank/
zmartify/weatherstation/
zmartify/lighting/
zmartify/power/
zmartify/security/
zmartify/gateway/
zmartify/cloud/
```

These reservations prevent future namespace conflicts while allowing the ecosystem to grow.

---

# 16.24 Engineering Notes

This chapter functions as the normative MQTT reference for the Zmartify platform. Firmware developers should regard the topic definitions, QoS levels and retained-message policies as contractual obligations rather than implementation details.

By consolidating the complete namespace into a single reference chapter, implementation, integration, testing and future product development can all reference the same authoritative specification. This approach mirrors the Interface Control Documents (ICDs) commonly used in industrial automation, aerospace and embedded systems engineering.

---

# 16.25 Chapter Summary

This chapter consolidates the complete MQTT namespace into a single engineering reference and defines the compliance requirements for every published interface.

Together with the architectural guidance presented in the preceding chapters, this reference establishes a stable and verifiable API baseline for the Zmartify ecosystem. It also provides the foundation for automated conformance testing, third-party certification and long-term interface governance.

---

# End of Chapter 16

**Next Chapter**

**Chapter 17 – JSON Schema Library, Message Validation & Reference Payload Catalogue**

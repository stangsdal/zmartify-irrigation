# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 17

# JSON Schema Library, Message Validation & Reference Payload Catalogue

---

# 17.1 Purpose

This chapter defines the canonical JSON schema conventions used throughout the Zmartify MQTT API.

Every MQTT payload published or consumed by the controller shall conform to a documented JSON schema to ensure interoperability, predictable parsing and long-term compatibility.

This chapter serves as the normative reference for:

* JSON object structure
* Data types
* Required properties
* Optional properties
* Enumerations
* Validation rules
* Payload examples
* Message quality requirements

---

# 17.2 Design Objectives

The JSON specification shall:

* Remain human readable
* Be machine verifiable
* Minimize payload size
* Support future expansion
* Preserve backwards compatibility
* Avoid ambiguity
* Be transport independent
* Support automated code generation

---

# 17.3 General JSON Philosophy

Every payload shall be self-describing.

Messages shall include sufficient metadata to allow independent interpretation without prior context.

Generic payload structure:

```json
{
  "timestamp":"2026-07-14T20:00:00Z",
  "device":"ZIC-S3-202600001",
  "type":"controller_status",
  "payload":{

  }
}
```

This structure is recommended for all telemetry and event messages.

---

# 17.4 Required Top-Level Fields

Unless otherwise specified, the following fields are mandatory.

| Field     | Type   | Required |
| --------- | ------ | :------: |
| timestamp | string |    Yes   |
| device    | string |    Yes   |
| type      | string |    Yes   |
| payload   | object |    Yes   |

Additional fields may be introduced in future API revisions.

---

# 17.5 Timestamp Format

All timestamps shall use:

* ISO-8601
* UTC
* RFC 3339 compatible

Example:

```text
2026-07-14T20:14:52Z
```

Millisecond precision is optional.

---

# 17.6 Device Identifier

Every payload shall identify the originating controller.

Example:

```json
{
    "device":"ZIC-S3-202600001"
}
```

The identifier shall remain constant throughout the controller lifetime.

---

# 17.7 Message Type

The `type` field identifies the payload category.

Examples:

```text
controller_status

weather_current

flow_current

pressure_current

alarm

event

diagnostic

configuration
```

Message types shall use lowercase snake_case.

---

# 17.8 Payload Object

The `payload` object contains application-specific information.

Example:

```json
{
  "payload":
  {
      "flow":24.6,
      "unit":"L/min"
  }
}
```

Only the payload object changes between message types.

---

# 17.9 Standard Data Types

| Type           | JSON                       |
| -------------- | -------------------------- |
| Boolean        | true / false               |
| Integer        | 123                        |
| Floating Point | 23.45                      |
| String         | "text"                     |
| Array          | []                         |
| Object         | {}                         |
| Null           | null (avoid when possible) |

Null values should only be used when the value is genuinely unknown.

---

# 17.10 Numeric Precision

Recommended precision:

| Parameter   | Precision |
| ----------- | --------: |
| Flow        | 0.1 L/min |
| Pressure    |  0.01 bar |
| Temperature |    0.1 °C |
| Humidity    |       1 % |
| Rain        |    0.1 mm |
| Wind        |   0.1 m/s |
| Voltage     |    0.01 V |
| Current     |    0.01 A |

---

# 17.11 Enumerations

Enumerated values shall be represented as readable strings.

Example:

```json
{
    "status":"Running"
}
```

Avoid numeric enumeration values.

---

# 17.12 Boolean Conventions

Boolean values shall never be encoded as strings.

Correct:

```json
{
    "enabled":true
}
```

Incorrect:

```json
{
    "enabled":"true"
}
```

---

# 17.13 Controller Status Schema

Reference payload:

```json
{
  "timestamp":"2026-07-14T20:15:00Z",
  "device":"ZIC-S3-202600001",
  "type":"controller_status",
  "payload":
  {
      "state":"idle",
      "online":true,
      "healthy":true,
      "active_program":null,
      "active_zone":null
  }
}
```

Required payload fields:

* state
* online
* healthy

---

# 17.14 Weather Schema

Reference payload:

```json
{
  "payload":
  {
      "temperature":22.8,
      "humidity":61,
      "pressure":1015.4,
      "wind":3.2,
      "rain":0.0
  }
}
```

Units are defined in Chapter 1.

---

# 17.15 Flow Schema

Reference payload:

```json
{
  "payload":
  {
      "flow":24.6,
      "expected":24.2,
      "confidence":98,
      "status":"Normal"
  }
}
```

---

# 17.16 Pressure Schema

Reference payload:

```json
{
  "payload":
  {
      "pressure":3.48,
      "expected":3.50,
      "confidence":97,
      "status":"Normal"
  }
}
```

---

# 17.17 Alarm Schema

Reference payload:

```json
{
  "payload":
  {
      "severity":"Critical",
      "code":"FLOW_HIGH",
      "title":"High Flow",
      "message":"Measured flow exceeds learned baseline.",
      "active":true
  }
}
```

Alarm codes shall remain stable across firmware releases.

---

# 17.18 Event Schema

Reference payload:

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

Events shall contain only information describing the occurrence.

---

# 17.19 Command Schema

Reference payload:

```json
{
  "transaction_id":"TX-20260714-000412",
  "timestamp":"2026-07-14T20:20:00Z",
  "origin":"HOMEIO",
  "command":
  {
      "zone":5,
      "runtime":900
  }
}
```

Mandatory fields:

* transaction_id
* timestamp
* origin
* command

---

# 17.20 Response Schema

Reference payload:

```json
{
  "transaction_id":"TX-20260714-000412",
  "timestamp":"2026-07-14T20:20:01Z",
  "status":"Completed",
  "message":"Manual irrigation completed successfully."
}
```

Every command shall generate exactly one terminal response.

---

# 17.21 Discovery Schema

Reference payload:

```json
{
  "payload":
  {
      "device":"ZIC-S3-202600001",
      "firmware":"5.0.0",
      "hardware":"RevB",
      "api_version":"1.0"
  }
}
```

This payload enables automatic client compatibility checks.

---

# 17.22 JSON Schema Validation

Incoming payloads shall be validated in the following order:

```text
Receive JSON

↓

UTF-8 Validation

↓

JSON Syntax Validation

↓

Schema Validation

↓

Business Logic Validation

↓

Command Execution
```

Malformed payloads shall never reach the application layer.

---

# 17.23 Error Payload

Validation failures shall return a structured error.

Example:

```json
{
  "transaction_id":"TX-20260714-000412",
  "status":"Rejected",
  "error":
  {
      "code":"INVALID_SCHEMA",
      "field":"runtime",
      "message":"Runtime exceeds permitted limit."
  }
}
```

Error codes shall be documented and stable.

---

# 17.24 Payload Size Guidelines

Recommended maximum payload sizes:

| Message Type    | Maximum Size |
| --------------- | -----------: |
| Status          |        512 B |
| Telemetry       |        512 B |
| Event           |        256 B |
| Alarm           |        512 B |
| Discovery       |         2 KB |
| Configuration   |         8 KB |
| Backup Metadata |        16 KB |

Large datasets should be segmented into multiple messages where practical.

---

# 17.25 Character Encoding

All MQTT payloads shall use:

* UTF-8 encoding
* Unix line endings (where applicable)
* Unicode support for zone and program names

Control characters shall not be included in payload values.

---

# 17.26 Engineering Notes

A consistent JSON structure greatly simplifies integration across different programming languages and platforms. By standardizing top-level metadata and limiting variation to the `payload` object, client libraries can implement generic parsing, logging and validation routines that work across all Zmartify message types.

This approach also facilitates automated code generation from JSON Schemas, enabling strongly typed client SDKs for C++, C#, Python, JavaScript and future REST or WebSocket interfaces.

---

# 17.27 Chapter Summary

This chapter defines the canonical JSON structures used throughout the Zmartify MQTT API.

The standardized schema conventions ensure that all telemetry, events, commands and configuration messages are consistent, machine-verifiable and extensible. Together with the MQTT topic definitions in previous chapters, these schemas form the complete public interface contract for the Zmartify ecosystem.

---

# End of Chapter 17

**Next Chapter**

**Chapter 18 – Security Architecture, Authentication & MQTT Hardening**

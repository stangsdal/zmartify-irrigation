# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 10

# MQTT Command Interface, JSON Schemas & Transaction Protocol

---

# 10.1 Purpose

This chapter defines the command interface used to remotely control the Zmartify Irrigation Controller via MQTT.

Unlike telemetry topics, command topics request the controller to perform an action.

Every command shall be:

* Authenticated
* Authorized
* Validated
* Transactional
* Auditable
* Deterministic

The controller shall never execute a command directly without passing it through the Application Layer.

---

# 10.2 Design Objectives

The command interface shall:

* Support reliable remote control
* Prevent accidental execution
* Support acknowledgements
* Return deterministic responses
* Provide meaningful error messages
* Maintain backwards compatibility
* Prevent replay attacks
* Allow future expansion

---

# 10.3 Command Architecture

Every MQTT command follows the same processing pipeline.

```text
MQTT Client

      │

      ▼

MQTT Manager

      │

Authentication

      │

Authorization

      │

JSON Validation

      │

Schema Validation

      │

Application Manager

      │

Command Execution

      │

Publish Response

      │

MQTT Broker
```

Only the Application Manager may invoke subsystem managers.

---

# 10.4 Command Namespace

All commands are published under:

```text
zmartify/commands/
```

Current command categories:

```text
manual

start

stop

pause

resume

skip

configuration

ota

backup

restore

reboot

factory_reset

maintenance

diagnostics

selftest
```

Future command categories shall extend this hierarchy rather than replace existing topics.

---

# 10.5 Command Philosophy

Every command shall be:

* Stateless
* Idempotent where practical
* Self-contained
* Timestamped
* Traceable

A command shall contain all information required for execution.

Commands shall never depend on previous commands unless explicitly documented.

---

# 10.6 Generic Command Envelope

All MQTT commands shall follow the same JSON envelope.

```json
{
    "transaction_id":"TX-20260714-000124",
    "timestamp":"2026-07-14T19:42:13Z",
    "origin":"HOMEIO",
    "user":"Administrator",
    "command":
    {

    }
}
```

This envelope allows common validation for all command types.

---

# 10.7 Transaction ID

Every command shall include a unique Transaction ID.

Format:

```text
TX-YYYYMMDD-NNNNNN
```

Example

```text
TX-20260714-000124
```

Transaction IDs provide:

* Duplicate detection
* Audit traceability
* Response correlation
* Retry protection

---

# 10.8 Timestamp

Commands shall include an ISO-8601 UTC timestamp.

Example

```text
2026-07-14T19:42:13Z
```

Commands older than the configured timeout (default 60 seconds) shall be rejected unless explicitly permitted.

---

# 10.9 Origin

The `origin` field identifies the requesting client.

Examples:

```text
HOMEIO

HomeAssistant

Homey

NodeRED

MobileApp

WebPortal

LocalDisplay
```

The origin is recorded in the audit log.

---

# 10.10 Manual Irrigation Command

Topic

```text
zmartify/commands/manual
```

Example

```json
{
    "transaction_id":"TX-20260714-000125",
    "timestamp":"2026-07-14T19:43:00Z",
    "origin":"HOMEIO",
    "command":
    {
        "zone":5,
        "runtime":900
    }
}
```

Validation shall verify:

* Zone exists
* Zone enabled
* Runtime within limits
* No emergency alarms
* Water available
* Hydraulic system healthy

---

# 10.11 Start Program Command

Topic

```text
zmartify/commands/start
```

Example

```json
{
    "command":
    {
        "program":"Morning"
    }
}
```

The Program Scheduler shall determine execution order.

---

# 10.12 Stop Command

Topic

```text
zmartify/commands/stop
```

Example

```json
{
    "command":
    {
        "reason":"User Request"
    }
}
```

Execution sequence:

```text
Validate

↓

Close Zone Valve

↓

Close Master Valve

↓

Store Runtime

↓

Publish Event

↓

Return Success
```

---

# 10.13 Pause Command

Topic

```text
zmartify/commands/pause
```

Example

```json
{
    "command":
    {
        "reason":"Rain Detected"
    }
}
```

The irrigation context shall be preserved for later resumption.

---

# 10.14 Resume Command

Topic

```text
zmartify/commands/resume
```

Example

```json
{
    "command":
    {
        "resume":true
    }
}
```

The controller shall resume the interrupted irrigation sequence.

---

# 10.15 Skip Zone Command

Topic

```text
zmartify/commands/skip
```

Example

```json
{
    "command":
    {
        "zone":4
    }
}
```

The skipped zone shall be recorded in irrigation history.

---

# 10.16 Reboot Command

Topic

```text
zmartify/commands/reboot
```

Administrator authorization required.

Example

```json
{
    "command":
    {
        "delay":10,
        "reason":"Maintenance"
    }
}
```

---

# 10.17 OTA Command

Topic

```text
zmartify/commands/ota/update
```

Example

```json
{
    "command":
    {
        "version":"5.1.0"
    }
}
```

Before installation, the OTA Manager shall verify:

* Digital signature
* SHA-256 checksum
* Hardware compatibility
* Firmware compatibility
* Available flash space

---

# 10.18 Backup Command

Topic

```text
zmartify/commands/backup
```

Example

```json
{
    "command":
    {
        "type":"Configuration"
    }
}
```

Supported backup types:

* Configuration
* Statistics
* Diagnostics
* Complete System

---

# 10.19 Restore Command

Topic

```text
zmartify/commands/restore
```

Example

```json
{
    "command":
    {
        "backup_id":"CFG-20260714-001"
    }
}
```

Restore shall not begin until compatibility has been verified.

---

# 10.20 Self-Test Command

Topic

```text
zmartify/commands/selftest
```

Example

```json
{
    "command":
    {
        "scope":"Full"
    }
}
```

Supported scopes:

* Quick
* Full
* Hydraulic
* Network
* Display
* Storage

---

# 10.21 Generic Response Topic

All command responses shall be published to:

```text
zmartify/response
```

Example

```json
{
    "transaction_id":"TX-20260714-000125",
    "timestamp":"2026-07-14T19:43:02Z",
    "status":"Accepted",
    "message":"Manual irrigation started."
}
```

Clients correlate responses using the Transaction ID.

---

# 10.22 Response Status Codes

| Status       | Meaning                    |
| ------------ | -------------------------- |
| Accepted     | Command accepted           |
| Completed    | Successfully executed      |
| Rejected     | Validation failed          |
| Unauthorized | Authentication failed      |
| Forbidden    | Insufficient permissions   |
| Busy         | Controller unavailable     |
| Invalid      | Invalid payload            |
| Timeout      | Command expired            |
| Error        | Internal execution failure |

---

# 10.23 Error Response Example

```json
{
    "transaction_id":"TX-20260714-000125",
    "status":"Rejected",
    "error":
    {
        "code":"ZONE_DISABLED",
        "message":"Zone 5 is disabled."
    }
}
```

Error codes shall remain stable across firmware versions.

---

# 10.24 JSON Schema Philosophy

Every payload shall comply with a formal JSON Schema.

Schemas shall define:

* Required fields
* Optional fields
* Data types
* Enumerations
* Numeric limits
* String lengths
* Regular expressions

Schema validation shall occur before application validation.

---

# 10.25 Idempotency

Certain commands are idempotent.

Examples:

* Pause
* Resume
* Reboot (scheduled)
* Configuration update (same values)

Repeated identical commands shall not produce unintended side effects.

---

# 10.26 Security Requirements

All management commands shall require:

* Valid MQTT session
* Authorized user role
* Timestamp validation
* Transaction ID uniqueness
* Payload schema validation
* Audit logging

Critical commands additionally require:

* Administrator role
* Confirmation where applicable

---

# 10.27 Audit Logging

Every accepted command shall generate an audit entry containing:

* Timestamp
* Transaction ID
* User
* Origin
* Command
* Result
* Execution time

Audit records shall be retained independently of operational logs.

---

# 10.28 Engineering Notes

The command interface has been designed around transactional integrity rather than simple message exchange.

By combining unique transaction identifiers, schema validation, authorization and structured acknowledgements, external systems can reliably determine the outcome of every command without relying on implementation-specific behaviour.

This approach closely follows practices used in industrial SCADA systems and provides a solid foundation for future REST and WebSocket APIs.

---

# 10.29 Chapter Summary

This chapter defines the complete MQTT command interface for the Zmartify Irrigation Controller.

The standardized command envelope, transaction protocol and response model ensure reliable, secure and deterministic remote control while maintaining full compatibility with the controller's internal safety mechanisms and event-driven architecture.

These principles form the basis for all future remote management interfaces across the Zmartify ecosystem.

---

# End of Chapter 10

**Next Chapter**

**Chapter 11 – HOMEIO Integration & Native MQTT Entity Model**

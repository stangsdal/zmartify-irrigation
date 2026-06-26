# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 19

# REST API & WebSocket Architecture (Future Interface Specification)

---

# 19.1 Purpose

Although MQTT is the primary communication protocol for the Zmartify platform, future deployments will also expose a REST API and WebSocket interface to support:

* Mobile applications
* Web dashboards
* Cloud gateways
* Enterprise software
* Third-party integrations
* Remote diagnostics
* API gateways

The REST and WebSocket interfaces are designed as alternative transports built upon the same logical API defined throughout this volume.

No business logic shall exist exclusively within the REST layer.

---

# 19.2 Design Objectives

The REST architecture shall:

* Mirror the MQTT API
* Be stateless
* Support JSON payloads
* Follow RESTful conventions
* Support secure authentication
* Be versioned
* Preserve backward compatibility
* Remain transport independent

---

# 19.3 Layered Architecture

```text
               Client Application

                       │

             REST / WebSocket API

                       │

              API Translation Layer

                       │

               Application Manager

                       │

         Internal Event Bus / Managers

                       │

                 Hardware Drivers
```

The REST layer shall never communicate directly with hardware drivers.

---

# 19.4 URI Structure

Base URI

```text
/api/v1/
```

Example resources

```text
/api/v1/controller

/api/v1/system

/api/v1/weather

/api/v1/programs

/api/v1/zones

/api/v1/flow

/api/v1/pressure

/api/v1/hydraulics

/api/v1/alarms

/api/v1/diagnostics

/api/v1/configuration
```

Versioning in the URI ensures compatibility across future API revisions.

---

# 19.5 REST Design Principles

The REST interface shall adhere to the following principles:

* Resources are nouns
* HTTP methods define actions
* Responses are JSON
* Stateless requests
* Predictable error handling
* Idempotent operations where appropriate
* Consistent resource naming

---

# 19.6 HTTP Methods

| Method | Purpose                      |
| ------ | ---------------------------- |
| GET    | Retrieve data                |
| POST   | Execute commands             |
| PUT    | Replace configuration        |
| PATCH  | Partial configuration update |
| DELETE | Reserved for future use      |

Unsafe operations shall never be performed using GET.

---

# 19.7 Controller Resource

Retrieve controller status.

```http
GET /api/v1/controller
```

Example response

```json
{
  "state":"Idle",
  "firmware":"5.0.0",
  "hardware":"RevB",
  "uptime":248733,
  "health_score":98
}
```

---

# 19.8 Zone Resources

Retrieve all zones

```http
GET /api/v1/zones
```

Retrieve one zone

```http
GET /api/v1/zones/4
```

Example response

```json
{
  "zone":4,
  "name":"Front Lawn",
  "enabled":true,
  "running":false,
  "flow":24.4,
  "pressure":3.48
}
```

---

# 19.9 Program Resources

Retrieve programs

```http
GET /api/v1/programs
```

Start program

```http
POST /api/v1/programs/start
```

Example request

```json
{
    "program":"Morning"
}
```

---

# 19.10 Weather Resources

Retrieve current weather

```http
GET /api/v1/weather
```

Forecast

```http
GET /api/v1/weather/forecast
```

Evapotranspiration

```http
GET /api/v1/weather/et
```

Responses shall mirror the MQTT payload structures.

---

# 19.11 Hydraulic Resources

Retrieve hydraulic status

```http
GET /api/v1/hydraulics
```

Retrieve flow

```http
GET /api/v1/flow
```

Retrieve pressure

```http
GET /api/v1/pressure
```

Example response

```json
{
    "flow":24.3,
    "pressure":3.49,
    "health":"Excellent"
}
```

---

# 19.12 Alarm Resources

Current alarms

```http
GET /api/v1/alarms
```

Alarm history

```http
GET /api/v1/alarms/history
```

Statistics

```http
GET /api/v1/alarms/statistics
```

---

# 19.13 Diagnostics Resources

Available endpoints

```text
GET /api/v1/diagnostics

GET /api/v1/diagnostics/cpu

GET /api/v1/diagnostics/memory

GET /api/v1/diagnostics/storage

GET /api/v1/diagnostics/network

GET /api/v1/diagnostics/selftest
```

---

# 19.14 Configuration Resources

Retrieve configuration

```http
GET /api/v1/configuration
```

Update configuration

```http
PUT /api/v1/configuration
```

Partial update

```http
PATCH /api/v1/configuration
```

Configuration updates shall use the same validation rules as MQTT commands.

---

# 19.15 Command Resources

Manual irrigation

```http
POST /api/v1/commands/manual
```

Pause irrigation

```http
POST /api/v1/commands/pause
```

Resume irrigation

```http
POST /api/v1/commands/resume
```

Reboot controller

```http
POST /api/v1/commands/reboot
```

Run self-test

```http
POST /api/v1/commands/selftest
```

These endpoints internally invoke the same command handlers used by MQTT.

---

# 19.16 HTTP Status Codes

| Status | Meaning             |
| ------ | ------------------- |
| 200    | Success             |
| 201    | Created             |
| 202    | Accepted            |
| 204    | No Content          |
| 400    | Invalid Request     |
| 401    | Unauthorized        |
| 403    | Forbidden           |
| 404    | Not Found           |
| 409    | Conflict            |
| 422    | Validation Failed   |
| 500    | Internal Error      |
| 503    | Service Unavailable |

The API shall use standard HTTP semantics wherever possible.

---

# 19.17 Error Response Format

Example

```json
{
  "status":"error",
  "code":"ZONE_DISABLED",
  "message":"Zone 4 is disabled.",
  "transaction_id":"TX-20260714-000812"
}
```

Error codes shall align with those used by the MQTT response interface.

---

# 19.18 Authentication

Recommended authentication mechanisms:

* Bearer Tokens
* OAuth2 (future)
* Mutual TLS (enterprise)
* API Keys (optional)

Authentication shall be independent of transport.

---

# 19.19 WebSocket Interface

The WebSocket interface provides low-latency streaming updates.

Example endpoint

```text
ws://controller.local/api/v1/ws
```

or

```text
wss://controller.local/api/v1/ws
```

Recommended production deployments shall use secure WebSockets (`wss://`).

---

# 19.20 WebSocket Message Types

Supported message categories:

* Controller State
* Weather Updates
* Zone Updates
* Hydraulic Telemetry
* Alarm Events
* Diagnostics
* OTA Progress
* Discovery Messages

Each message shall reuse the JSON schemas defined in Chapter 17.

---

# 19.21 Event Subscription Model

Clients may subscribe to logical channels.

Example

```json
{
    "subscribe":[
        "controller",
        "weather",
        "flow",
        "alarms"
    ]
}
```

This minimizes unnecessary traffic.

---

# 19.22 REST-to-MQTT Mapping

The REST API is a façade over the MQTT/Application Layer.

| REST Resource      | MQTT Topic          |
| ------------------ | ------------------- |
| `/controller`      | `controller/status` |
| `/weather`         | `weather/current`   |
| `/flow`            | `flow/current`      |
| `/pressure`        | `pressure/current`  |
| `/alarms`          | `alarms/current`    |
| `/commands/manual` | `commands/manual`   |
| `/commands/start`  | `commands/start`    |

Business logic shall remain identical regardless of transport.

---

# 19.23 Performance Targets

| Operation       |  Target |
| --------------- | ------: |
| GET Controller  | <100 ms |
| GET Weather     | <100 ms |
| Manual Command  | <250 ms |
| Alarm Push      | <100 ms |
| WebSocket Event |  <50 ms |

These targets assume operation on a local network.

---

# 19.24 Future Extensions

The architecture has been designed to support:

* GraphQL
* gRPC
* Server-Sent Events (SSE)
* Matter bridge services
* OPC UA gateway
* Cloud synchronization
* Digital Twin APIs

These additions shall preserve the canonical application model and JSON schemas.

---

# 19.25 Engineering Notes

The REST and WebSocket interfaces are intentionally designed as transport adapters rather than independent APIs. By reusing the same application managers, validation logic and JSON schemas defined for MQTT, Zmartify avoids protocol divergence and significantly reduces long-term maintenance effort.

This architecture enables developers to choose the communication method best suited to their application while ensuring consistent behavior across MQTT, REST and WebSocket clients.

---

# 19.26 Chapter Summary

This chapter defines the future REST and WebSocket interfaces for the Zmartify platform.

Although MQTT remains the primary communication protocol, these additional transports provide broader integration opportunities for mobile applications, web services and enterprise software while preserving a single, stable application model. The result is a flexible, transport-independent architecture capable of supporting future ecosystem growth without compromising API consistency.

---

# End of Chapter 19

**Next Chapter**

**Chapter 20 – Volume Summary, Implementation Guidelines & MQTT/API Compliance Checklist**

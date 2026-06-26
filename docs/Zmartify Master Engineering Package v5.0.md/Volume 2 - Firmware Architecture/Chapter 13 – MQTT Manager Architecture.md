# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 13 – MQTT Manager Architecture

---

# 13 MQTT Manager

---

# 13.1 Purpose

The MQTT Manager provides all communication between the Zmartify Irrigation Controller and external systems.

It implements a **MQTT-first architecture**, making MQTT the primary interface for integration with:

* HOMEIO
* Home Assistant
* Homey
* Node-RED
* OpenHAB
* Grafana
* InfluxDB
* Cloud services
* Future Zmartify ecosystem devices

The MQTT Manager shall isolate the remainder of the firmware from all MQTT implementation details.

No firmware component shall communicate directly with the ESP-IDF MQTT library.

---

# 13.2 Design Objectives

The MQTT Manager shall:

* Maintain broker connection
* Publish controller state
* Publish telemetry
* Publish alarms
* Receive commands
* Validate commands
* Buffer messages during outages
* Support retained messages
* Support Last Will & Testament
* Support automatic reconnection
* Maintain deterministic behaviour

Loss of MQTT connectivity shall **never** stop irrigation.

---

# 13.3 Architectural Position

```text
                 Home Assistant

                    HOMEIO

                     Homey

                   Node-RED

                 Other Clients

                        │

                        ▼

                 MQTT Broker

                        │

                        ▼

                MQTT Manager

                        │

        ┌───────────────┼───────────────┐

        ▼               ▼               ▼

 Event Bus        Configuration     Logging

                        │

                        ▼

                 Firmware Modules
```

---

# 13.4 MQTT Philosophy

The MQTT Manager follows four principles.

### MQTT-001

Publish State

Never publish assumptions.

---

### MQTT-002

Command Validation

Every incoming command shall be validated before execution.

---

### MQTT-003

Loose Coupling

MQTT shall communicate through the Event Bus.

---

### MQTT-004

Offline Operation

The controller shall continue operating autonomously if MQTT becomes unavailable.

---

# 13.5 MQTT Client

Approved implementation:

ESP-IDF MQTT Client

Transport:

* TCP
* TLS (recommended)

Protocol:

MQTT 3.1.1

Future support:

MQTT 5.0

---

# 13.6 Connection State Machine

```text
Disconnected

↓

DNS Resolve

↓

TCP Connect

↓

TLS Handshake

↓

MQTT CONNECT

↓

Connected

↓

Operational

↓

Connection Lost

↓

Reconnect
```

Reconnect shall occur automatically.

---

# 13.7 Broker Configuration

Configuration parameters include:

* Broker Address
* Port
* Username
* Password
* Client ID
* TLS Enable
* CA Certificate
* Keep Alive
* Retry Interval

All parameters are stored by the Configuration Manager.

---

# 13.8 Topic Hierarchy

The controller shall use a hierarchical topic structure.

```text
zmartify/

controller/

system/

weather/

flow/

pressure/

zone/

relay/

alarm/

diagnostics/

config/

command/

event/
```

Future devices shall follow the same namespace.

---

# 13.9 Controller Topics

Examples:

```text
zmartify/controller/state

zmartify/controller/version

zmartify/controller/uptime

zmartify/controller/ip

zmartify/controller/status
```

Retained:

Yes

---

# 13.10 System Topics

```text
zmartify/system/state

zmartify/system/time

zmartify/system/boot

zmartify/system/restart

zmartify/system/runtime
```

---

# 13.11 Weather Topics

```text
zmartify/weather/current

zmartify/weather/forecast

zmartify/weather/recommendation

zmartify/weather/rain_delay

zmartify/weather/freeze
```

---

# 13.12 Flow Topics

```text
zmartify/flow/current

zmartify/flow/today

zmartify/flow/month

zmartify/flow/lifetime
```

Per zone

```text
zmartify/zone/01/flow
```

---

# 13.13 Pressure Topics

```text
zmartify/pressure/current

zmartify/pressure/status

zmartify/pressure/history
```

---

# 13.14 Zone Topics

Each zone has its own hierarchy.

Example:

```text
zmartify/zone/01/state

zmartify/zone/01/runtime

zmartify/zone/01/flow

zmartify/zone/01/pressure

zmartify/zone/01/statistics

zmartify/zone/01/config
```

---

# 13.15 Alarm Topics

```text
zmartify/alarm/current

zmartify/alarm/history

zmartify/alarm/critical

zmartify/alarm/warning
```

Critical alarms shall always be published immediately.

---

# 13.16 Diagnostic Topics

```text
zmartify/diagnostics/cpu

zmartify/diagnostics/memory

zmartify/diagnostics/tasks

zmartify/diagnostics/network

zmartify/diagnostics/storage
```

---

# 13.17 Command Topics

Incoming commands:

```text
zmartify/command/start

zmartify/command/stop

zmartify/command/pause

zmartify/command/resume

zmartify/command/config

zmartify/command/reboot

zmartify/command/update
```

Commands shall never execute directly.

Sequence:

```text
MQTT

↓

MQTT Manager

↓

Validation

↓

Event Bus

↓

Firmware Module
```

---

# 13.18 Event Topics

Published events:

```text
zmartify/event/zone

zmartify/event/alarm

zmartify/event/weather

zmartify/event/system

zmartify/event/config
```

---

# 13.19 JSON Payload Standard

Every payload shall contain:

```json
{
  "timestamp": "...",
  "device": "ZIC-S3",
  "version": "5.0",
  "type": "...",
  "payload": { }
}
```

Payload schemas are defined in **Volume 3**.

---

# 13.20 Retained Messages

Retained topics include:

* Controller status
* Zone state
* Configuration
* Weather recommendation
* Firmware version
* Current alarms

Historical telemetry shall **not** be retained.

---

# 13.21 Last Will & Testament

Mandatory.

Example:

Topic

```text
zmartify/controller/status
```

Payload

```json
{
    "state":"offline"
}
```

Upon successful connection the controller publishes:

```json
{
    "state":"online"
}
```

---

# 13.22 Publish Rates

Default publication intervals:

| Topic             |                 Default |
| ----------------- | ----------------------: |
| Controller Status |                    30 s |
| Weather           |                   5 min |
| Flow              | 5 s (during irrigation) |
| Pressure          | 5 s (during irrigation) |
| Diagnostics       |                    60 s |
| Statistics        |                   5 min |

Intervals shall be configurable.

---

# 13.23 Quality of Service

Recommended QoS:

| Data          | QoS |
| ------------- | --: |
| Commands      |   2 |
| Alarms        |   2 |
| Configuration |   1 |
| Telemetry     |   0 |
| Statistics    |   0 |

---

# 13.24 Offline Buffer

During broker outages:

Outgoing messages shall be buffered.

Priority:

1. Alarms
2. Events
3. Configuration
4. Statistics

Maximum buffer size shall be configurable.

Old telemetry may be discarded if storage limits are reached.

---

# 13.25 Security

Supported authentication:

* Username
* Password
* TLS
* CA Certificates

Future:

* Client Certificates
* Mutual TLS
* Token Authentication

Passwords shall never be published.

---

# 13.26 MQTT Discovery

Version 5.0 shall support:

* HOMEIO integration
* Home Assistant MQTT Discovery (optional, configurable)
* Zmartify Native Discovery (future)

The discovery subsystem shall automatically publish device and entity definitions after initial connection if enabled.

---

# 13.27 Public API

Example interface.

```c
mqtt_manager_init();

mqtt_manager_start();

mqtt_manager_stop();

mqtt_manager_publish();

mqtt_manager_subscribe();

mqtt_manager_is_connected();

mqtt_manager_publish_alarm();

mqtt_manager_publish_event();

mqtt_manager_publish_state();
```

Other modules shall communicate exclusively through these APIs.

---

# 13.28 Event Subscription

MQTT Manager subscribes to:

* EVT_ALARM_*
* EVT_FLOW_*
* EVT_PRESS_*
* EVT_ZONE_*
* EVT_SYSTEM_*
* EVT_CONFIG_*
* EVT_WEATHER_*

Publishes:

* EVT_MQTT_CONNECTED
* EVT_MQTT_DISCONNECTED
* EVT_MQTT_COMMAND
* EVT_MQTT_ERROR

---

# 13.29 Diagnostics

Diagnostic information includes:

* Connection status
* Broker latency
* Publish queue size
* Message count
* Reconnect count
* TLS status
* Wi-Fi RSSI
* Packet errors
* Buffer utilization

---

# 13.30 Unit Testing

Automated tests shall verify:

* Broker connection
* TLS
* Reconnection
* Last Will
* Retained messages
* JSON formatting
* Command validation
* Buffering
* Queue overflow
* Invalid payload handling

Minimum code coverage:

**95%**

---

# 13.31 Engineering Notes

The MQTT Manager is a key architectural component because it enables the Zmartify controller to function as a first-class smart home device while remaining completely autonomous.

Special consideration has been given to compatibility with **HOMEIO**, which is intended to be the primary integration platform. At the same time, the topic hierarchy is deliberately generic so that Home Assistant, Homey, Node-RED and future Zmartify ecosystem devices can integrate without modification.

The use of retained state topics, structured JSON payloads and a stable namespace ensures long-term compatibility with dashboards, automation rules and historical data storage.

---

# 13.32 Chapter Summary

The MQTT Manager provides a secure, modular and resilient communication layer between the Zmartify Irrigation Controller and external automation systems.

By abstracting all MQTT functionality behind a dedicated manager and using an event-driven architecture, the controller remains independent of any specific smart home platform while offering rich telemetry, reliable command handling and future-proof integration capabilities.

This communication layer is a cornerstone of the Zmartify ecosystem and enables seamless interaction with HOMEIO, Home Assistant, Homey and future distributed Zmartify devices.

---

# End of Chapter 13

**Next Chapter**

**Chapter 14 – Configuration Manager Architecture**

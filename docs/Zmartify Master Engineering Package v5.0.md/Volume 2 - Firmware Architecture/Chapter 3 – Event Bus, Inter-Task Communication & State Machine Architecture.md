# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 3 – Event Bus, Inter-Task Communication & State Machine Architecture

---

# 3 Event Bus Architecture

## 3.1 Purpose

The Event Bus is the central communication mechanism of the Zmartify firmware.

It provides loose coupling between firmware components by allowing modules to exchange information without direct dependencies.

Every subsystem shall communicate through the Event Bus unless direct hardware synchronization is explicitly required.

This architecture provides:

* High modularity
* Simple testing
* Reduced coupling
* Better maintainability
* Easier future expansion

---

# 3.2 Design Philosophy

The Event Bus follows five fundamental principles.

### FW-EVT-001

Events represent **facts**, not commands.

Examples:

✔ Zone Started

✔ Flow Updated

✔ Pressure Low

✘ Start Relay 3

---

### FW-EVT-002

Events are immutable.

Once published they shall never be modified.

---

### FW-EVT-003

Publishers shall never know who receives an event.

---

### FW-EVT-004

Subscribers shall never know who generated an event.

---

### FW-EVT-005

The Event Bus shall never contain business logic.

It shall only distribute information.

---

# 3.3 Overall Architecture

```text
                   Event Producers

 Flow Manager
 Weather Manager
 UI Manager
 MQTT Manager
 Pressure Manager
 Relay Manager
 OTA Manager

          │
          ▼

    +------------------+
    |    Event Bus     |
    |------------------|
    | Event Queue      |
    | Dispatcher       |
    | Subscription DB  |
    +------------------+

          │
          ▼

     Event Subscribers

 Irrigation Engine
 Alarm Manager
 Logging Manager
 Display Manager
 MQTT Manager
 Diagnostics
```

---

# 3.4 Event Lifecycle

Every event follows the same lifecycle.

```text
Generate Event

↓

Validate

↓

Timestamp

↓

Assign Priority

↓

Insert Queue

↓

Dispatcher

↓

Subscribers

↓

Complete
```

The dispatcher shall never modify event contents.

---

# 3.5 Event Structure

All events shall use a common data structure.

```c
typedef struct
{
    event_id_t      id;
    event_source_t  source;
    uint64_t        timestamp;
    event_priority_t priority;
    uint16_t        data_length;
    void           *data;
} event_t;
```

The structure shall be version-controlled to maintain compatibility across firmware releases.

---

# 3.6 Event Categories

Events are grouped into functional categories.

| Category       | Prefix    |
| -------------- | --------- |
| System         | EVT_SYS   |
| Irrigation     | EVT_IRR   |
| Relay          | EVT_RLY   |
| Flow           | EVT_FLOW  |
| Pressure       | EVT_PRESS |
| Weather        | EVT_WEA   |
| Alarm          | EVT_ALM   |
| MQTT           | EVT_MQTT  |
| Configuration  | EVT_CFG   |
| OTA            | EVT_OTA   |
| Diagnostics    | EVT_DIAG  |
| User Interface | EVT_UI    |

---

# 3.7 Event Priority

Four priority levels are defined.

| Priority | Description                 |
| -------- | --------------------------- |
| Critical | Immediate processing        |
| High     | Process as soon as possible |
| Normal   | Standard queue              |
| Low      | Background processing       |

Critical events shall bypass non-critical queue traffic where practical.

---

# 3.8 Core Event Definitions

## System Events

```text
EVT_SYS_BOOT

EVT_SYS_READY

EVT_SYS_SHUTDOWN

EVT_SYS_RESTART

EVT_SYS_WATCHDOG

EVT_SYS_FACTORY_RESET
```

---

## Irrigation Events

```text
EVT_IRR_START

EVT_IRR_STOP

EVT_IRR_PAUSE

EVT_IRR_RESUME

EVT_IRR_COMPLETE

EVT_IRR_SKIP
```

---

## Zone Events

```text
EVT_ZONE_START

EVT_ZONE_RUNNING

EVT_ZONE_STOP

EVT_ZONE_TIMEOUT

EVT_ZONE_FAULT
```

---

## Relay Events

```text
EVT_RELAY_ON

EVT_RELAY_OFF

EVT_RELAY_FAULT
```

---

## Flow Events

```text
EVT_FLOW_UPDATED

EVT_FLOW_HIGH

EVT_FLOW_LOW

EVT_FLOW_NONE

EVT_FLOW_LEAK
```

---

## Pressure Events

```text
EVT_PRESS_UPDATED

EVT_PRESS_HIGH

EVT_PRESS_LOW

EVT_PRESS_COLLAPSE

EVT_PRESS_OSCILLATION
```

---

## Weather Events

```text
EVT_WEA_UPDATED

EVT_WEA_RAIN

EVT_WEA_WIND

EVT_WEA_ET_UPDATED
```

---

## MQTT Events

```text
EVT_MQTT_CONNECTED

EVT_MQTT_DISCONNECTED

EVT_MQTT_MESSAGE

EVT_MQTT_PUBLISHED
```

---

## Alarm Events

```text
EVT_ALARM_NEW

EVT_ALARM_ACK

EVT_ALARM_CLEAR
```

---

# 3.9 Event Queue

The Event Queue shall be implemented using a FreeRTOS Queue.

Recommended capacity:

256 events

Overflow strategy:

* Log overflow
* Raise diagnostic warning
* Never discard Critical events

---

# 3.10 Event Dispatcher

Responsibilities:

* Receive events
* Determine subscribers
* Forward events
* Maintain order
* Measure queue latency

Dispatcher execution time shall remain below:

5 ms

under normal operation.

---

# 3.11 Event Subscription

Each module shall register the events it wishes to receive.

Example

```text
Flow Manager

Publishes

↓

EVT_FLOW_UPDATED

↓

Subscribers

Irrigation Engine

Alarm Manager

Logging Manager

MQTT Manager

Display Manager
```

Modules shall not receive unnecessary events.

---

# 3.12 State Machine Philosophy

Every major firmware component shall implement a deterministic finite state machine (FSM).

Advantages include:

* Predictable behaviour
* Easier debugging
* Clear transitions
* Improved reliability
* Better testing

---

# 3.13 Controller State Machine

```text
BOOT

↓

INIT

↓

SELF TEST

↓

READY

↓

IDLE

↓

IRRIGATING

↓

PAUSED

↓

RAIN DELAY

↓

SERVICE

↓

FAULT

↓

EMERGENCY STOP
```

All state transitions shall generate an event.

---

# 3.14 Irrigation Engine State Machine

```text
Idle

↓

Waiting Program

↓

Evaluate Weather

↓

Evaluate Hydraulic

↓

Open Master Valve

↓

Verify Pressure

↓

Open Zone

↓

Running

↓

Stopping

↓

Completed

↓

Idle
```

Any detected fault shall transition immediately to:

Fault

---

# 3.15 Zone State Machine

Each zone maintains an independent state.

```text
Disabled

↓

Idle

↓

Queued

↓

Opening

↓

Running

↓

Closing

↓

Completed
```

Fault states:

* Valve Failure
* Flow Fault
* Pressure Fault

---

# 3.16 Alarm State Machine

```text
Normal

↓

Warning

↓

Critical

↓

Acknowledged

↓

Resolved
```

Critical alarms require explicit acknowledgement before returning to Normal.

---

# 3.17 Weather Engine State Machine

```text
Waiting

↓

Acquire Data

↓

Validate

↓

Calculate ET

↓

Publish Update

↓

Waiting
```

Failure to obtain online weather data shall not prevent operation using local weather station measurements.

---

# 3.18 Event Timing Requirements

| Event             | Maximum Delay |
| ----------------- | ------------: |
| Emergency Stop    |        100 ms |
| Leak Detection    |        250 ms |
| Pressure Collapse |        250 ms |
| Relay Update      |        100 ms |
| UI Event          |        100 ms |
| MQTT Publish      |      1 second |

---

# 3.19 Event Logging

Every event shall be timestamped.

Selected events shall also be written to persistent storage.

Mandatory logged events include:

* Boot
* Restart
* Shutdown
* Irrigation Start
* Irrigation Stop
* Alarm
* Configuration Change
* OTA Update
* Manual Override

---

# 3.20 Error Handling

If an event cannot be processed:

1. Record diagnostic information.
2. Log the error.
3. Notify Diagnostics Manager.
4. Continue processing remaining events.

The Event Bus shall never stop because of a malformed or unsupported event.

---

# 3.21 Future Expansion

The Event Bus architecture has been designed to support future distributed Zmartify devices.

Future event sources may include:

* Remote Valve Controllers
* Soil Moisture Modules
* Pump Controllers
* Water Reservoir Controllers
* Fertigation Controllers
* CAN Bus Devices
* RS485 Devices

No architectural changes shall be required to integrate additional event publishers or subscribers.

---

# 3.22 Chapter Summary

The Event Bus is the central nervous system of the Zmartify firmware architecture.

By enforcing asynchronous communication, deterministic state machines and standardized event structures, the Event Bus decouples firmware components while providing a scalable foundation for future expansion. This architecture allows the irrigation controller to evolve into a distributed automation platform without introducing tight coupling between software modules.

---

# End of Chapter 3

**Next Chapter**

**Chapter 4 – Hardware Abstraction Layer (HAL) Architecture**

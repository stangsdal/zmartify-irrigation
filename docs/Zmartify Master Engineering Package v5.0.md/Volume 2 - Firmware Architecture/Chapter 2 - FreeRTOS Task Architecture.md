# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 2 – FreeRTOS Task Architecture

---

# 2 FreeRTOS Task Architecture

## 2.1 Purpose

The Zmartify Irrigation Controller is designed as a real-time embedded control system based on **ESP-IDF** and **FreeRTOS**.

Unlike a traditional sequential application, ZIC executes multiple independent tasks concurrently while maintaining deterministic behaviour for safety-critical functions.

This chapter defines:

* Task architecture
* Task priorities
* Scheduling strategy
* CPU utilization targets
* Inter-task communication
* Synchronization
* Watchdog supervision

The task architecture shall remain unchanged throughout the lifetime of Hardware Revision B unless approved through an Engineering Change Request (ECR).

---

# 2.2 Design Philosophy

The task model is based on five principles.

### FW-TASK-001

Safety-Critical First

Tasks responsible for hydraulic protection shall always execute with higher priority than user interface or communication tasks.

---

### FW-TASK-002

Short Execution Time

Tasks shall never block longer than necessary.

Long operations shall be divided into smaller state-machine driven operations.

---

### FW-TASK-003

No Busy Waiting

Busy waiting is prohibited.

Tasks shall sleep using:

* Event Groups
* Queues
* Semaphores
* Task Notifications
* Timers

---

### FW-TASK-004

Deterministic Execution

Execution time shall be predictable.

Dynamic memory allocation inside cyclic tasks shall be avoided.

---

### FW-TASK-005

Graceful Degradation

Loss of lower-priority tasks shall never affect hydraulic safety.

---

# 2.3 Overall Task Model

```text
                    FreeRTOS Scheduler
                           │
 ┌─────────────────────────┼──────────────────────────┐
 │                         │                          │
 │                         │                          │
 ▼                         ▼                          ▼
Safety Tasks         Control Tasks            Service Tasks
 │                         │                          │
 ▼                         ▼                          ▼
UI Tasks             Communication        Background Tasks
```

The scheduler shall use fixed priorities.

Round-robin scheduling shall only occur between tasks of equal priority.

---

# 2.4 Task Priority Assignment

| Priority | Task                  | Criticality |
| -------- | --------------------- | ----------- |
| 10       | Safety Supervisor     | Critical    |
| 9        | Flow Manager          | Critical    |
| 8        | Pressure Manager      | Critical    |
| 8        | Irrigation Engine     | Critical    |
| 7        | Relay Manager         | High        |
| 6        | Zone Manager          | High        |
| 6        | Alarm Manager         | High        |
| 5        | MQTT Manager          | Medium      |
| 5        | Weather Manager       | Medium      |
| 5        | Configuration Manager | Medium      |
| 4        | Display Manager       | Medium      |
| 4        | LVGL Task             | Medium      |
| 3        | Logging Manager       | Low         |
| 3        | Diagnostics Manager   | Low         |
| 2        | OTA Manager           | Low         |
| 1        | Housekeeping          | Background  |
| 0        | Idle Task             | System      |

---

# 2.5 Safety Supervisor Task

## Responsibilities

The Safety Supervisor is the highest-priority application task.

Responsibilities include:

* Emergency Stop
* Critical Alarm Supervision
* Watchdog Monitoring
* Master Valve Shutdown
* Safe State Enforcement

Execution Period

100 ms

Maximum execution time

5 ms

---

# 2.6 Irrigation Engine Task

Execution Interval

500 ms

Responsibilities

* Program scheduler
* Manual irrigation
* Runtime management
* Cycle & Soak
* Zone sequencing
* Weather approval
* Hydraulic approval

The Irrigation Engine shall never manipulate GPIO directly.

---

# 2.7 Flow Manager Task

Execution Interval

250 ms

Responsibilities

* Read PCNT counters
* Calculate L/min
* Update averages
* Leak detection
* Unexpected flow detection
* Publish events

---

# 2.8 Pressure Manager Task

Execution Interval

250 ms

Responsibilities

* Read ADS1115
* Filter measurements
* Detect pressure collapse
* Detect overpressure
* Detect oscillation

---

# 2.9 Relay Manager Task

Execution Model

Event Driven

Responsibilities

* Receive relay commands
* Verify permissions
* Operate MCP23017
* Maintain relay state
* Verify output status

---

# 2.10 MQTT Task

Execution Model

ESP-IDF MQTT Client

Responsibilities

* Broker connection
* Publish telemetry
* Receive commands
* Publish alarms
* Publish state
* Handle retained topics

Communication failures shall never block higher-priority tasks.

---

# 2.11 Weather Manager Task

Execution Interval

15 minutes (default)

Responsibilities

* Download forecast
* Process local weather
* Update ET inputs
* Publish weather events

Weather processing shall never delay irrigation decisions.

---

# 2.12 Display Manager

Execution Interval

50 ms

Responsibilities

* Screen updates
* Navigation
* Dashboard refresh
* Alarm display
* Touch processing

The display task shall automatically suspend when the display enters sleep mode.

---

# 2.13 Logging Task

Execution Model

Background

Responsibilities

* Store events
* Store alarms
* Store statistics
* Rotate log files

Logging shall never block critical tasks.

---

# 2.14 Diagnostics Task

Execution Interval

5 seconds

Responsibilities

* CPU load
* Heap usage
* Stack usage
* Wi-Fi RSSI
* MQTT health
* Sensor health
* Internal temperatures

---

# 2.15 OTA Task

Execution Model

On Demand

Responsibilities

* Download firmware
* Verify firmware
* Flash inactive partition
* Trigger reboot

OTA execution shall pause irrigation scheduling but shall not interrupt an active irrigation cycle unless explicitly requested by the user.

---

# 2.16 Housekeeping Task

Execution Interval

60 seconds

Responsibilities

* File cleanup
* Memory statistics
* Runtime statistics
* Storage maintenance

---

# 2.17 FreeRTOS Synchronization

The firmware shall use the following synchronization primitives.

| Mechanism         | Purpose             |
| ----------------- | ------------------- |
| Queue             | Event transfer      |
| Event Group       | System state        |
| Binary Semaphore  | Resource protection |
| Mutex             | Shared data         |
| Task Notification | Fast signalling     |
| Software Timer    | Timed events        |

Critical sections shall be kept as short as possible.

---

# 2.18 CPU Load Targets

Target CPU utilization

| Task Group        | Target |
| ----------------- | -----: |
| Safety            |    <5% |
| Irrigation        |   <10% |
| Sensors           |   <10% |
| MQTT              |   <10% |
| UI                |   <20% |
| Logging           |    <5% |
| Background        |   <10% |
| Total Normal Load |   <60% |

This provides sufficient headroom for future firmware expansion.

---

# 2.19 Memory Allocation Policy

Static allocation shall be preferred.

Dynamic allocation is permitted only during:

* System initialization
* OTA
* Configuration loading
* Network initialization

Dynamic allocation inside cyclic tasks is discouraged.

---

# 2.20 Watchdog Integration

Every critical task shall register with the Task Watchdog.

Failure to respond shall result in:

1. Diagnostic event
2. Attempted recovery
3. Safe shutdown if recovery fails
4. System restart if required

---

# 2.21 Task Communication

Tasks shall communicate exclusively through the Event Bus.

Example:

```text
Flow Manager

↓

EVENT_FLOW_UPDATED

↓

Event Queue

↓

Irrigation Engine

↓

Decision

↓

Relay Manager

↓

Valve Control
```

Direct calls between unrelated managers are prohibited.

---

# 2.22 Timing Requirements

| Function           | Maximum Latency |
| ------------------ | --------------: |
| Emergency Stop     |          100 ms |
| Critical Alarm     |          250 ms |
| Relay Activation   |          100 ms |
| Flow Update        |          250 ms |
| Pressure Update    |          250 ms |
| UI Touch Response  |          100 ms |
| MQTT Alarm Publish |             1 s |

---

# 2.23 Future Scalability

The task architecture shall support future additions including:

* Multiple irrigation controllers
* Distributed valve stations
* AI irrigation advisor
* Pump controller
* Soil moisture engine
* Water reservoir controller

Additional tasks shall be integrated without modifying existing safety-critical scheduling.

---

# 2.24 Chapter Summary

The FreeRTOS architecture provides a deterministic, safety-oriented execution model in which hydraulic protection and irrigation control always take precedence over user interface, communication and background services.

By combining fixed task priorities, event-driven communication and minimal inter-task dependencies, the architecture is designed for long-term reliability, predictable timing and straightforward future expansion.

---

# End of Chapter 2

**Next Chapter**

**Chapter 3 – Event Bus, Inter-Task Communication & State Machine Architecture**

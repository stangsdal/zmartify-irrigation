# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 5

# MQTT Topic Specification – Irrigation Programs, Zones & Manual Control

---

# 5.1 Purpose

This chapter specifies the MQTT interface used to monitor and control irrigation programs, irrigation zones and manual watering operations.

The Irrigation namespace represents the operational heart of the controller and provides external systems with complete visibility of irrigation activities while maintaining the controller's autonomous decision-making capability.

The interfaces defined herein enable:

* Monitoring irrigation progress
* Starting manual irrigation
* Stopping irrigation
* Pausing programs
* Skipping zones
* Viewing schedules
* Viewing runtime statistics
* Supervising irrigation execution

All irrigation commands shall be validated by the Irrigation Engine before execution.

---

# 5.2 Design Objectives

The Irrigation namespace shall:

* Expose current irrigation status
* Support manual irrigation
* Publish program execution
* Publish zone execution
* Prevent unsafe commands
* Remain deterministic
* Support smart-home integration
* Support future multi-controller systems

---

# 5.3 Namespace

Root namespace:

```text
zmartify/irrigation/
```

Primary topics:

```text
state
program
queue
manual
runtime
remaining
pause
resume
history
statistics
schedule
```

Zone namespace:

```text
zmartify/zones/
```

Each zone is independently addressable.

---

# 5.4 Irrigation State

Topic

```text
zmartify/irrigation/state
```

QoS

```text
1
```

Retained

```text
Yes
```

Published:

* Program start
* Program completion
* State change
* Every 30 seconds while irrigating

---

Example

```json
{
  "timestamp":"2026-07-14T18:00:00Z",
  "device":"ZIC-S3-202600001",
  "type":"irrigation_state",
  "payload":
  {
      "state":"running",
      "mode":"automatic",
      "program":"Morning",
      "zone":4,
      "remaining_seconds":1180
  }
}
```

---

Allowed states

| State      | Description          |
| ---------- | -------------------- |
| idle       | No irrigation        |
| running    | Automatic irrigation |
| manual     | Manual watering      |
| paused     | Program paused       |
| rain_delay | Waiting              |
| suspended  | Safety stop          |
| completed  | Program completed    |

---

# 5.5 Active Program

Topic

```text
zmartify/irrigation/program
```

Example

```json
{
  "payload":
  {
      "id":2,
      "name":"Morning",
      "priority":1,
      "zone_count":6,
      "current_zone":3,
      "start_time":"06:00",
      "estimated_finish":"06:42"
  }
}
```

---

# 5.6 Irrigation Queue

Topic

```text
zmartify/irrigation/queue
```

Purpose

Displays remaining zones in execution order.

Example

```json
{
  "payload":
  [
    {
      "zone":4,
      "runtime":600
    },
    {
      "zone":5,
      "runtime":720
    }
  ]
}
```

---

# 5.7 Remaining Runtime

Topic

```text
zmartify/irrigation/remaining
```

Example

```json
{
  "payload":
  {
      "zone_remaining":320,
      "program_remaining":1560
  }
}
```

Unit

Seconds

---

# 5.8 Manual Irrigation

Command Topic

```text
zmartify/commands/manual
```

Example command

```json
{
  "zone":5,
  "runtime":900
}
```

The Irrigation Engine shall verify:

* Zone enabled
* No emergency alarms
* Water available
* Master valve operational
* Hydraulic safety

Only then shall irrigation begin.

---

# 5.9 Pause Irrigation

Command

```text
zmartify/commands/pause
```

Payload

```json
{
    "reason":"User"
}
```

The Irrigation Engine shall:

* Close all active zone valves
* Maintain program state
* Preserve remaining runtime

---

# 5.10 Resume Irrigation

Command

```text
zmartify/commands/resume
```

Example

```json
{
    "resume":true
}
```

Execution shall resume at the interrupted zone.

---

# 5.11 Stop Irrigation

Command

```text
zmartify/commands/stop
```

Payload

```json
{
    "stop":true
}
```

Sequence:

```text
Receive Command

↓

Validate

↓

Close Zone Valve

↓

Close Master Valve

↓

Publish Event

↓

Idle
```

---

# 5.12 Skip Zone

Command

```text
zmartify/commands/skip
```

Example

```json
{
    "zone":3
}
```

The controller immediately advances to the next scheduled zone.

---

# 5.13 Zone Namespace

Each irrigation zone maintains an independent namespace.

Example

```text
zmartify/zones/04/
```

Available topics

```text
state
runtime
remaining
flow
pressure
water
statistics
configuration
budget
alarm
enabled
```

---

# 5.14 Zone State

Topic

```text
zmartify/zones/04/state
```

Example

```json
{
  "payload":
  {
      "zone":4,
      "name":"Front Lawn",
      "state":"running",
      "manual":false
  }
}
```

---

# 5.15 Zone Runtime

Topic

```text
zmartify/zones/04/runtime
```

Example

```json
{
  "payload":
  {
      "scheduled":900,
      "adjusted":810,
      "remaining":522
  }
}
```

All values

Seconds

---

# 5.16 Zone Water Usage

Topic

```text
zmartify/zones/04/water
```

Example

```json
{
  "payload":
  {
      "current":46.2,
      "today":218.4,
      "month":3146.8,
      "lifetime":284663
  }
}
```

Units

Litres

---

# 5.17 Zone Flow

Topic

```text
zmartify/zones/04/flow
```

Example

```json
{
  "payload":
  {
      "expected":24.8,
      "measured":24.3,
      "status":"Normal"
  }
}
```

Units

L/min

---

# 5.18 Zone Pressure

Topic

```text
zmartify/zones/04/pressure
```

Example

```json
{
  "payload":
  {
      "expected":3.5,
      "measured":3.4,
      "status":"Normal"
  }
}
```

Units

bar

---

# 5.19 Zone Water Budget

Topic

```text
zmartify/zones/04/budget
```

Example

```json
{
  "payload":
  {
      "deficit":2.3,
      "runtime_factor":0.81,
      "et":3.8
  }
}
```

Units

Millimetres

---

# 5.20 Zone Configuration

Topic

```text
zmartify/zones/04/configuration
```

Example

```json
{
  "payload":
  {
      "enabled":true,
      "soil":"Loam",
      "plant":"Turf",
      "sprinkler":"MP Rotator",
      "area":165
  }
}
```

Configuration updates require administrator authorization.

---

# 5.21 Irrigation History

Topic

```text
zmartify/irrigation/history
```

Example

```json
{
  "payload":
  [
    {
      "program":"Morning",
      "date":"2026-07-14",
      "duration":2430,
      "water":982
    }
  ]
}
```

---

# 5.22 Irrigation Statistics

Topic

```text
zmartify/irrigation/statistics
```

Example

```json
{
  "payload":
  {
      "today_runtime":3820,
      "today_water":1860,
      "week_runtime":18244,
      "month_runtime":69312
  }
}
```

---

# 5.23 Schedule

Topic

```text
zmartify/irrigation/schedule
```

Example

```json
{
  "payload":
  {
      "next_program":"Morning",
      "next_start":"2026-07-15T06:00:00Z",
      "enabled":true
  }
}
```

---

# 5.24 Publish Rates

| Topic              | Interval |
| ------------------ | -------: |
| irrigation/state   |    Event |
| irrigation/program |    Event |
| queue              |    Event |
| runtime            |      5 s |
| remaining          |      5 s |
| zone/state         |    Event |
| flow               |      5 s |
| pressure           |      5 s |
| water              |     10 s |
| statistics         |     60 s |

---

# 5.25 QoS Policy

| Topic      | QoS | Retained |
| ---------- | :-: | :------: |
| state      |  1  |    Yes   |
| program    |  1  |    Yes   |
| runtime    |  0  |    No    |
| flow       |  0  |    No    |
| pressure   |  0  |    No    |
| commands   |  2  |    No    |
| statistics |  0  |    No    |

Commands shall always use **QoS 2**.

---

# 5.26 Safety Requirements

Incoming irrigation commands shall never bypass firmware safety mechanisms.

Before execution, every command shall verify:

* Controller state
* Active alarms
* Flow Manager status
* Pressure Manager status
* Weather restrictions
* Rain delay
* Freeze protection
* Master valve availability

Unsafe commands shall be rejected with an explanatory response.

---

# 5.27 Engineering Notes

The irrigation MQTT interface intentionally exposes high-level irrigation concepts such as *programs*, *zones* and *water budgets* rather than low-level relay operations. This abstraction allows future firmware to modify the internal implementation—such as introducing distributed relay modules or adaptive scheduling—without breaking external integrations.

By requiring all commands to pass through the Irrigation Engine and associated safety checks, MQTT control remains fully compatible with the controller's autonomous protection mechanisms and hydraulic safety system.

---

# 5.28 Chapter Summary

This chapter defines the MQTT interface for irrigation programs, zone control and manual watering.

The namespace provides complete visibility into irrigation execution while ensuring that all external commands are validated by the controller before any physical action is taken. The resulting interface is deterministic, safe and well suited for integration with HOMEIO, Home Assistant, Homey and other automation platforms.

---

# End of Chapter 5

**Next Chapter**

**Chapter 6 – MQTT Topic Specification: Flow, Pressure & Hydraulic Monitoring**

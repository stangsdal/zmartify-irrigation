# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 5 – Irrigation Engine Architecture

---

# 5 Irrigation Engine

## 5.1 Purpose

The Irrigation Engine is the heart of the Zmartify Irrigation Controller firmware.

It is the only software component authorized to make irrigation decisions.

Every automatic or manual irrigation request shall pass through the Irrigation Engine before any valve is activated.

This architecture guarantees that every irrigation action is evaluated against:

* Safety conditions
* Hydraulic conditions
* Weather conditions
* User configuration
* System status

No other firmware component may bypass this decision process.

---

# 5.2 Responsibilities

The Irrigation Engine is responsible for:

* Automatic irrigation scheduling
* Manual irrigation
* Program execution
* Zone sequencing
* Runtime calculation
* Seasonal adjustment
* ET adjustment
* Rain delay
* Hydraulic approval
* Master valve control authorization
* Irrigation interruption
* Irrigation recovery
* Irrigation completion

The engine shall **not**:

* Operate relays directly
* Read sensors directly
* Draw user interface elements
* Publish MQTT messages directly

These responsibilities belong to dedicated managers.

---

# 5.3 Architectural Position

```text
                 User Interface
                      │
                      ▼
              Irrigation Request
                      │
                      ▼
             Irrigation Engine
                      │
      ┌───────────────┼────────────────┐
      ▼               ▼                ▼
Weather Engine   Flow Manager   Pressure Manager
      │               │                │
      └───────────────┼────────────────┘
                      ▼
             Safety Evaluation
                      │
          Approved? ──┼── No → Alarm/Event
                      │
                     Yes
                      ▼
               Relay Manager
                      ▼
              Master Valve + Zone
```

The Irrigation Engine shall never communicate directly with the MCP23017 or GPIO.

---

# 5.4 Irrigation Philosophy

Unlike conventional irrigation controllers, ZIC does **not** simply execute a timer.

Instead, every irrigation event follows a decision process.

```text
Scheduled Event

↓

Program Enabled?

↓

Controller Healthy?

↓

Rain Delay Active?

↓

Weather Acceptable?

↓

Hydraulic System Healthy?

↓

Master Valve Available?

↓

Start Irrigation
```

Failure at any stage shall terminate the request safely.

---

# 5.5 Irrigation Request Sources

Requests may originate from:

### Automatic Scheduler

Normal daily irrigation.

---

### Manual User Operation

Touchscreen

Hardware buttons

MQTT command

---

### Smart Home

HOMEIO

Home Assistant

Homey

Node-RED

---

### Future Sources

* AI Water Optimizer
* Soil Moisture Engine
* External API

All requests shall be processed identically.

---

# 5.6 Irrigation States

The Irrigation Engine maintains a global state machine.

```text
Disabled

↓

Idle

↓

Waiting

↓

Preparing

↓

Master Valve Opening

↓

Pressure Verification

↓

Zone Opening

↓

Flow Verification

↓

Running

↓

Stopping

↓

Completed

↓

Idle
```

Fault transitions:

* Leak
* Pressure Collapse
* Emergency Stop
* Hardware Failure

---

# 5.7 Program Scheduler

Responsibilities

* Evaluate irrigation calendar
* Determine next event
* Apply seasonal adjustment
* Apply ET adjustment
* Skip disabled programs
* Handle rain delay

Programs shall never directly activate zones.

Instead:

```text
Program

↓

Irrigation Request

↓

Irrigation Engine

↓

Approval Process

↓

Zone Execution
```

---

# 5.8 Pre-Irrigation Validation

Before irrigation begins, the following checks shall be performed.

## System Checks

* Firmware healthy
* No watchdog events
* Configuration valid

---

## Weather Checks

* Rain delay
* Rain forecast
* Wind speed
* Temperature
* ET recommendation

---

## Hydraulic Checks

* Flow sensor available
* Pressure sensor available
* Master valve healthy
* Water supply available

---

## User Checks

* Zone enabled
* Program enabled
* Manual override
* Service Mode inactive

Only if all checks succeed shall irrigation proceed.

---

# 5.9 Master Valve Sequence

The Master Valve shall always operate before any irrigation zone.

Sequence:

```text
Master Valve ON

↓

Delay (2 s default)

↓

Pressure Stable?

↓

Zone Valve ON
```

Pressure shall stabilize before the zone valve is energized.

---

# 5.10 Zone Start Sequence

Each zone shall follow the same startup sequence.

```text
Zone Selected

↓

Relay Command

↓

Valve Open

↓

Flow Verification

↓

Pressure Verification

↓

Runtime Start

↓

Continuous Monitoring
```

If verification fails:

Immediate shutdown.

---

# 5.11 Continuous Supervision

During irrigation the engine continuously evaluates:

Hydraulic

* Flow
* Pressure

Environmental

* Rain
* Wind
* Freeze

System

* Emergency Stop
* Relay faults
* Sensor faults
* MQTT commands

Sampling intervals are defined in Volume 1.

---

# 5.12 Runtime Management

Runtime calculation considers:

Base Runtime

×

Seasonal Adjustment

×

ET Adjustment

×

Zone Factor

=

Final Runtime

Manual runtime ignores seasonal adjustment unless configured otherwise.

---

# 5.13 Cycle & Soak

For heavy soils the engine supports Cycle & Soak.

Example

```text
10 min

↓

Pause

15 min

↓

10 min

↓

Pause

15 min

↓

10 min
```

Advantages:

* Reduced runoff
* Improved infiltration
* Lower water loss

---

# 5.14 Zone Transition

Default sequence

```text
Zone A OFF

↓

Delay

↓

Zone B ON
```

Optional overlap may be supported in future revisions for systems with sufficient hydraulic capacity.

---

# 5.15 Parallel Irrigation

Version 5.0 supports configurable parallel operation.

Default:

One active irrigation zone.

Maximum:

Determined by hydraulic configuration.

Before allowing simultaneous zones the engine shall verify:

* Transformer capacity
* Water supply
* Pressure stability
* Learned flow profiles

---

# 5.16 Weather Integration

Weather Manager provides recommendations.

The Irrigation Engine remains responsible for final decisions.

Possible actions:

* Run normally
* Reduce runtime
* Increase runtime
* Delay irrigation
* Skip irrigation

---

# 5.17 ET Integration

ET Engine provides:

Daily water demand.

The Irrigation Engine applies ET adjustment to:

* Runtime
* Seasonal factor
* Water budget

ET calculations shall never directly operate valves.

---

# 5.18 Flow Learning Integration

Flow Learning mode operates through the Irrigation Engine.

Sequence:

1. Select zone

2. Open master valve

3. Stabilize pressure

4. Open zone

5. Measure flow

6. Measure pressure

7. Store baseline

Flow Learning shall not overwrite existing values without user confirmation.

---

# 5.19 Emergency Handling

Emergency Stop immediately interrupts all irrigation.

Sequence

```text
Emergency Event

↓

Cancel Runtime

↓

Close Zone

↓

Close Master Valve

↓

Disable Relays

↓

Raise Alarm

↓

Log Event

↓

Publish MQTT

↓

Idle (Fault)
```

---

# 5.20 Recovery Strategy

Automatic recovery is permitted only after:

* Power restoration
* Wi-Fi restoration
* MQTT restoration

Automatic recovery is **not** permitted after:

* Leak detection
* Pipe burst
* Emergency Stop
* Pressure collapse

User acknowledgement is required.

---

# 5.21 Irrigation Statistics

The engine maintains:

Per Zone

* Runtime
* Water consumption
* Average flow
* Average pressure
* Number of starts

System

* Daily water
* Weekly water
* Monthly water
* Seasonal water
* Lifetime water

Statistics shall survive power loss.

---

# 5.22 Public API

Example public interface.

```c
irrigation_engine_init();

irrigation_engine_start();

irrigation_engine_stop();

irrigation_engine_pause();

irrigation_engine_resume();

irrigation_engine_manual_zone();

irrigation_engine_cancel();

irrigation_engine_status();
```

Application code shall interact with the engine only through documented APIs.

---

# 5.23 Internal Events

The Irrigation Engine subscribes to:

* EVT_FLOW_UPDATED
* EVT_PRESS_UPDATED
* EVT_WEA_UPDATED
* EVT_PROGRAM_START
* EVT_PROGRAM_STOP
* EVT_ALARM_NEW
* EVT_BUTTON_START
* EVT_BUTTON_STOP
* EVT_MQTT_COMMAND

The Irrigation Engine publishes:

* EVT_IRR_START
* EVT_IRR_STOP
* EVT_ZONE_START
* EVT_ZONE_STOP
* EVT_IRR_PAUSE
* EVT_IRR_COMPLETE
* EVT_IRR_ABORT

---

# 5.24 Diagnostics

Diagnostic information includes:

Current State

Current Program

Current Zone

Remaining Runtime

Flow

Pressure

Weather Decision

Last Fault

Memory Usage

Task Runtime

This information shall be available through:

* LVGL
* MQTT
* Diagnostics Manager

---

# 5.25 Unit Testing

The Irrigation Engine shall be verified through automated tests covering:

* Program scheduling
* Manual operation
* Rain delay
* ET adjustment
* Cycle & Soak
* Parallel irrigation
* Flow failure
* Pressure failure
* Emergency Stop
* Recovery logic

Minimum code coverage target:

**90%**

---

# 5.26 Future Enhancements

The architecture supports future capabilities including:

* AI-assisted irrigation optimization
* Predictive weather adaptation
* Soil moisture closed-loop control
* Dynamic water pricing optimization
* Multi-controller coordination
* Reservoir-aware irrigation
* Pump speed optimization

These features shall integrate through the existing Event Bus without altering the core architecture.

---

# 5.27 Chapter Summary

The Irrigation Engine is the central decision-making component of the Zmartify firmware.

By separating decision logic from hardware control and enforcing a structured validation process before every irrigation event, the engine provides a deterministic, safe and extensible platform capable of supporting advanced irrigation strategies while maintaining strict protection of people, property and water resources.

---

# End of Chapter 5

**Next Chapter**

**Chapter 6 – Zone Manager Architecture**

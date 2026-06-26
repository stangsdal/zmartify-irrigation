# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 7 – Relay Manager Architecture

---

# 7 Relay Manager

---

# 7.1 Purpose

The Relay Manager is responsible for the safe and deterministic control of all physical relay outputs within the Zmartify Irrigation Controller.

It provides the only software interface capable of energizing or de-energizing irrigation valves.

No other firmware component shall directly access:

* MCP23017
* ULN2803A
* HL-58S relay boards
* GPIO
* I²C relay registers

The Relay Manager therefore forms the final safety barrier between firmware decisions and physical valve operation.

---

# 7.2 Design Objectives

The Relay Manager shall satisfy the following objectives.

### REL-001

Provide deterministic relay control.

---

### REL-002

Prevent illegal relay combinations.

---

### REL-003

Guarantee fail-safe startup.

---

### REL-004

Guarantee fail-safe shutdown.

---

### REL-005

Provide relay diagnostics.

---

### REL-006

Remain independent of irrigation logic.

---

# 7.3 Hardware Architecture

```text
                Irrigation Engine
                       │
                       ▼
                 Relay Manager
                       │
                HAL Relay Driver
                       │
                  MCP23017
                       │
                  ULN2803A
                       │
              HL-58S Relay Boards
                       │
                24 VAC Solenoids
```

The Relay Manager shall never access hardware directly.

All hardware access shall occur through HAL.

---

# 7.4 Supported Outputs

The current hardware platform supports:

| Output   | Purpose      |
| -------- | ------------ |
| Relay 0  | Master Valve |
| Relay 1  | Zone 1       |
| Relay 2  | Zone 2       |
| Relay 3  | Zone 3       |
| Relay 4  | Zone 4       |
| Relay 5  | Zone 5       |
| Relay 6  | Zone 6       |
| Relay 7  | Zone 7       |
| Relay 8  | Zone 8       |
| Relay 9  | Zone 9       |
| Relay 10 | Zone 10      |
| Relay 11 | Zone 11      |
| Relay 12 | Zone 12      |
| Relay 13 | Zone 13      |
| Relay 14 | Zone 14      |
| Relay 15 | Zone 15      |

Relay assignments are configurable through the Zone Manager, except Relay 0, which is permanently reserved for the Master Valve.

---

# 7.5 Relay Characteristics

The approved relay hardware is:

**HL-58S V1.2**

Characteristics:

* 5 V logic
* Opto-isolated inputs
* Active-Low logic
* SPDT relay contacts
* LED status indication

Firmware shall compensate for Active-Low logic internally.

Application code shall always use logical ON/OFF semantics.

---

# 7.6 Startup Sequence

After power-up:

1. Initialize MCP23017.
2. Configure all outputs.
3. Set all outputs OFF.
4. Verify communication.
5. Publish relay status.
6. Enable Relay Manager.

No relay shall ever momentarily energize during initialization.

---

# 7.7 Shutdown Sequence

Whenever the controller shuts down normally:

1. Cancel pending relay commands.
2. Close irrigation zones.
3. Close Master Valve.
4. Disable all relay outputs.
5. Store relay status.
6. Publish shutdown event.

---

# 7.8 Emergency Shutdown

During Emergency Stop:

```text
Emergency Event

↓

Cancel Queue

↓

Close Zone Relays

↓

Close Master Valve

↓

Disable Outputs

↓

Generate Alarm

↓

Publish MQTT

↓

Fault State
```

Maximum shutdown time:

**<100 ms**

---

# 7.9 Relay State Machine

Each relay maintains an independent state.

```text
Disabled

↓

Idle

↓

Pending ON

↓

ON

↓

Pending OFF

↓

OFF
```

Fault transitions:

* Communication Failure
* Timeout
* Illegal State

---

# 7.10 Master Valve Protection

The Master Valve has special protection rules.

It:

* Opens before every irrigation zone.
* Closes after the final zone.
* Shall never remain open without an active irrigation zone (except during configured hydraulic stabilization delays or service mode).
* Shall immediately close following a critical alarm.

Default opening delay:

**2 seconds**

Default closing delay:

**1 second**

These values are configurable.

---

# 7.11 Relay Command Validation

Every relay request shall be validated.

Checks include:

* Relay exists.
* Relay enabled.
* Controller not in Fault state.
* Emergency Stop inactive.
* Configuration valid.
* Requested state differs from current state.

Invalid requests shall be rejected and logged.

---

# 7.12 Relay Arbitration

Multiple firmware modules may request relay actions.

The Relay Manager shall arbitrate according to priority.

Priority order:

1. Emergency Stop
2. Safety Supervisor
3. Irrigation Engine
4. Service Mode
5. Manual Commands
6. Diagnostics

Lower-priority requests shall never override higher-priority safety actions.

---

# 7.13 Command Queue

Relay requests are processed sequentially.

Example:

```text
Open Master Valve

↓

Delay

↓

Open Zone

↓

Runtime

↓

Close Zone

↓

Delay

↓

Close Master Valve
```

The queue shall guarantee execution order.

---

# 7.14 Timing Requirements

| Function                 |      Maximum |
| ------------------------ | -----------: |
| Relay ON                 |       100 ms |
| Relay OFF                |       100 ms |
| Queue Processing         |        10 ms |
| MCP23017 Write           |         5 ms |
| Complete Zone Transition | Configurable |

---

# 7.15 Relay Diagnostics

The Relay Manager continuously monitors:

* MCP23017 communication
* Relay command success
* Relay timeout
* Illegal states
* Active relay count

Future custom hardware may additionally support:

* Relay feedback contacts
* Current sensing
* Coil diagnostics

---

# 7.16 Active Relay Limits

To protect the transformer and hydraulic system, firmware shall enforce limits.

Version 5.0 defaults:

* One Master Valve
* Up to three irrigation valves simultaneously (configurable according to hydraulic design and transformer capacity)

The Irrigation Engine shall determine whether simultaneous operation is permitted.

---

# 7.17 Illegal Relay Combinations

The Relay Manager shall reject:

* Non-existent relay numbers
* Duplicate relay assignments
* Commands during Emergency Stop
* Commands during critical hardware failure
* Conflicting Master Valve commands

Rejected commands shall generate diagnostic events.

---

# 7.18 Relay Status Model

Each relay maintains:

* Relay Number
* Logical State
* Physical State
* Command Timestamp
* Last Transition
* Error Counter
* Total Operations
* Assigned Zone

Statistics shall be available through MQTT and Diagnostics.

---

# 7.19 Public API

Example interface.

```c
relay_manager_init();

relay_manager_start();

relay_manager_stop();

relay_manager_enable();

relay_manager_disable();

relay_manager_on();

relay_manager_off();

relay_manager_toggle();

relay_manager_all_off();

relay_manager_get_state();

relay_manager_get_statistics();
```

Application software shall never call HAL directly.

---

# 7.20 Event Subscription

Relay Manager subscribes to:

* EVT_IRR_START
* EVT_IRR_STOP
* EVT_ZONE_START
* EVT_ZONE_STOP
* EVT_EMERGENCY_STOP
* EVT_SERVICE_MODE
* EVT_CONFIGURATION_CHANGED

Relay Manager publishes:

* EVT_RELAY_ON
* EVT_RELAY_OFF
* EVT_RELAY_FAULT
* EVT_MASTER_OPEN
* EVT_MASTER_CLOSED

---

# 7.21 MQTT Integration

Relay status shall be published using retained topics.

Example:

```text
zmartify/relay/master/state

zmartify/relay/01/state

zmartify/relay/02/state

...

zmartify/relay/15/state
```

Additional telemetry:

* Transition Counter
* Error Counter
* Last Change Timestamp

---

# 7.22 Diagnostics

Diagnostic information includes:

* Active relays
* Queue length
* MCP23017 status
* I²C communication errors
* Relay operation count
* Last fault
* Last command
* Average switching latency

These diagnostics shall be visible through:

* Local UI
* MQTT
* Diagnostics Manager

---

# 7.23 Unit Testing

The Relay Manager shall include automated tests covering:

* Initialization
* Active-Low translation
* Queue operation
* Illegal commands
* Master Valve sequencing
* Simultaneous relay activation
* Emergency shutdown
* MCP23017 communication failure
* Recovery after fault

Minimum code coverage:

**95%**

---

# 7.24 Future Enhancements

The architecture supports future capabilities including:

* Relay feedback contacts
* Electronic output stages
* Solid-state relays
* Distributed remote relay modules
* CAN Bus relay expansion
* Redundant Master Valve control
* Automatic relay health monitoring

These enhancements shall not require changes to the public Relay Manager API.

---

# 7.25 Chapter Summary

The Relay Manager provides the only authorized path between firmware and the physical irrigation valves.

By isolating relay control behind a dedicated manager and enforcing strict validation, sequencing and safety rules, the architecture guarantees deterministic valve operation while protecting both the irrigation system and the controller hardware.

Its hardware-independent API ensures compatibility with future custom PCB revisions and distributed I/O architectures, making it a key component of the long-term Zmartify platform strategy.

---

# End of Chapter 7

**Next Chapter**

**Chapter 8 – Weather Manager Architecture**

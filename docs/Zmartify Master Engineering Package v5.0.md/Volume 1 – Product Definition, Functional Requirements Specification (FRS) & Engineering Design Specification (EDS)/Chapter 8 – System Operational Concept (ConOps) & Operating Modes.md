# Chapter 8 – System Operational Concept (ConOps) & Operating Modes

---

# 8 System Operational Concept (ConOps)

## 8.1 Purpose

The Operational Concept (ConOps) defines how the Zmartify Irrigation Controller (ZIC) is expected to operate throughout its complete lifecycle.

This chapter describes:

* Normal operation
* Automatic operation
* User interaction
* Safety behaviour
* Failure handling
* Service procedures

The ConOps provides the operational foundation for firmware implementation and future user documentation.

---

# 8.2 Operational Philosophy

The ZIC controller shall behave as an autonomous irrigation supervisor rather than a traditional irrigation timer.

Instead of simply executing programmed schedules, the controller shall continuously evaluate whether irrigation is appropriate based on:

* Program schedules
* Weather conditions
* Forecast weather
* Hydraulic conditions
* Water availability
* System health
* User preferences
* Alarm conditions

Only when all required conditions are satisfied shall irrigation begin.

---

# 8.3 Operational States

The controller operates in one of the following global states.

```text
                     +-------------+
                     |    BOOT     |
                     +------+------+ 
                            |
                            v
                   +--------+--------+
                   | INITIALIZATION  |
                   +--------+--------+
                            |
                            v
                     +------+------+
                     |  SELF TEST  |
                     +------+------+
                            |
                +-----------+-----------+
                |                       |
                v                       v
        +-------+-------+        +------+------+
        |     IDLE      |<------>| SERVICE MODE|
        +-------+-------+        +-------------+
                |
      Automatic | Manual
                |
                v
        +-------+-------+
        | IRRIGATING    |
        +-------+-------+
                |
                +------------------+
                |                  |
                v                  v
          +-----+-----+      +-----+------+
          |  PAUSED   |      | RAIN DELAY |
          +-----+-----+      +------------+
                |
                v
         +------+------+
         | COMPLETED   |
         +------+------+
                |
                v
              IDLE

Any Critical Fault
        |
        v
+-----------------------+
| EMERGENCY SHUTDOWN    |
+-----------+-----------+
            |
            v
         FAULT
```

---

# 8.4 Boot Sequence

After power is applied, the controller shall perform the following sequence.

## Stage 1 – Hardware Initialization

Initialize:

* CPU
* Memory
* Display
* Touch controller
* GPIO
* I²C bus
* ADC
* PCNT
* UART
* PWM

---

## Stage 2 – Configuration Loading

Load configuration from NVS.

Verify:

* Configuration version
* Integrity
* CRC

If configuration is invalid:

Restore factory defaults.

---

## Stage 3 – Hardware Detection

Verify communication with:

* MCP23017
* ADS1115
* MCP9808
* Display
* Touch controller

Record missing hardware.

---

## Stage 4 – Self Test

Execute:

* Relay test (optional)
* Sensor validation
* Memory test
* File system validation
* RTC synchronization
* Network initialization

---

## Stage 5 – Network Initialization

Initialize:

* Wi-Fi
* MQTT
* Time synchronization (NTP)

Internet availability is optional.

---

## Stage 6 – Ready State

Enter:

IDLE

---

# 8.5 Idle Operation

The Idle state represents the normal standby condition.

During Idle the controller continuously monitors:

* Weather
* Flow sensor
* Pressure sensor
* Cabinet temperature
* Door switch
* Network
* MQTT
* Scheduled programs

The display shall automatically dim after inactivity.

---

# 8.6 Automatic Irrigation

Automatic irrigation follows the sequence below.

```text
Scheduled Start
        |
        v
Program Enabled?
        |
       Yes
        |
        v
Rain Delay?
        |
       No
        |
        v
Weather Acceptable?
        |
       Yes
        |
        v
Hydraulic System OK?
        |
       Yes
        |
        v
Open Master Valve
        |
        v
Verify Pressure
        |
        v
Open Zone Valve
        |
        v
Verify Flow
        |
        v
Start Runtime Timer
        |
        v
Monitor Continuously
        |
        v
Runtime Complete
        |
        v
Close Zone
        |
        v
Next Zone?
```

---

# 8.7 Manual Irrigation

Manual irrigation shall always take precedence over scheduled irrigation.

The user may manually:

* Start a zone
* Stop a zone
* Pause irrigation
* Resume irrigation

Manual operation shall still enforce all safety checks.

---

# 8.8 Hydraulic Supervision

During irrigation the controller shall continuously verify:

* Expected flow
* Expected pressure
* Valve operation
* Master valve state

Any significant deviation shall initiate fault handling.

Sampling intervals:

Flow:

1 second

Pressure:

1 second

---

# 8.9 Weather Evaluation

Before irrigation begins the Weather Engine shall evaluate:

Measured:

* Rainfall
* Temperature
* Humidity
* Wind speed
* UV
* Solar radiation

Forecast:

* Rain probability
* Forecast rainfall
* Wind
* Temperature

Decision outcomes:

* Irrigate normally
* Reduce runtime
* Delay irrigation
* Skip irrigation

---

# 8.10 Flow Learning Mode

Flow Learning is a commissioning function.

Sequence:

1. Open Master Valve

2. Open selected zone

3. Wait stabilization

4. Record flow

5. Record pressure

6. Calculate averages

7. Store baseline

The process may be repeated at any time.

---

# 8.11 Alarm Operation

Alarm severity levels:

Information

↓

Warning

↓

Critical

Critical alarms immediately interrupt irrigation.

Typical sequence:

```text
Fault Detected

↓

Generate Alarm

↓

Log Event

↓

Publish MQTT

↓

Wake Display

↓

Stop Irrigation

↓

Close Master Valve

↓

Enter Fault State
```

---

# 8.12 Emergency Stop

Emergency Stop may be activated by:

* Hardware button
* Touchscreen
* MQTT
* Internal safety system

Emergency Stop shall immediately:

* Disable all relays
* Close master valve
* Cancel running programs
* Store event
* Publish alarm

Recovery requires user acknowledgement.

---

# 8.13 Rain Delay Mode

Rain Delay temporarily suspends automatic irrigation.

Activation methods:

* Manual
* Weather forecast
* Rain sensor (future)
* MQTT command

Manual irrigation remains available.

---

# 8.14 Service Mode

Service Mode supports installation and maintenance.

Functions include:

* Relay test
* Valve test
* Flow calibration
* Pressure calibration
* Touch calibration
* Wi-Fi diagnostics
* MQTT diagnostics
* Display test

Automatic irrigation is suspended.

---

# 8.15 OTA Update Mode

Firmware update sequence:

```text
Download

↓

Verify

↓

Install

↓

Reboot

↓

Self Test

↓

Commit

↓

Resume Operation
```

If validation fails:

Rollback to previous firmware.

---

# 8.16 Power Failure Recovery

Unexpected power loss shall result in:

* Safe valve closure
* Configuration preservation
* Log preservation

After power restoration:

* Complete boot sequence
* Verify system health
* Do not resume interrupted irrigation automatically
* Wait for the next scheduled event or user action

---

# 8.17 User Interaction Model

The controller supports three levels of interaction.

### Local

* Touchscreen
* Hardware buttons

---

### Remote

* MQTT
* HOMEIO
* Home Assistant
* Homey
* Node-RED

---

### Automatic

* Scheduled programs
* Weather Engine
* ET Engine
* Alarm Manager

The same safety rules apply regardless of the source of the command.

---

# 8.18 Maintenance Operation

Routine maintenance includes:

* Cleaning cabinet
* Verifying wiring
* Testing valves
* Calibrating pressure sensor
* Verifying flow meter
* Installing firmware updates
* Backing up configuration

Maintenance activities shall be logged.

---

# 8.19 Seasonal Operation

Typical annual cycle:

```text
Spring
↓

Commissioning

↓

Flow Learning

↓

Normal Irrigation

↓

Summer Optimization

↓

Autumn Shutdown

↓

Winter Standby
```

The controller shall maintain configuration and historical data throughout the year.

---

# 8.20 Operational Priorities

The controller shall prioritize actions according to the following order:

| Priority | Function             |
| -------- | -------------------- |
| 1        | Human safety         |
| 2        | Hydraulic safety     |
| 3        | Equipment protection |
| 4        | Alarm processing     |
| 5        | Irrigation control   |
| 6        | Data logging         |
| 7        | User interface       |
| 8        | Remote communication |
| 9        | Analytics            |

Lower-priority functions shall never delay higher-priority safety actions.

---

# 8.21 Operational Assumptions

The controller assumes:

* Stable 230 VAC supply
* Correctly installed valves
* Functional pressure sensor
* Functional flow meter
* Reliable Wi-Fi coverage (recommended RSSI > –65 dBm)
* Proper cabinet ventilation

Failure of any assumption shall be detected where technically feasible and reported to the user.

---

# 8.22 Chapter Summary

This chapter defines how the Zmartify Irrigation Controller operates under normal, maintenance and fault conditions.

The Operational Concept establishes a deterministic operating model in which every irrigation decision is validated against environmental conditions, hydraulic status and safety rules before execution. This operational philosophy forms the behavioural foundation for the firmware architecture defined in Volume 2.

---

# End of Chapter 8

**Next Chapter**

**Chapter 9 – System Requirements & Performance Specification**

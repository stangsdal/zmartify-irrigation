# Chapter 10 – System Safety, Reliability & Risk Management

---

# 10 System Safety, Reliability & Risk Management

## 10.1 Purpose

The purpose of this chapter is to define the safety philosophy, reliability objectives and risk management strategy for the Zmartify Irrigation Controller (ZIC).

Safety is considered a primary design objective and shall always take precedence over irrigation performance, convenience or smart-home integration.

All hardware and firmware decisions shall be evaluated against the principles defined in this chapter.

---

# 10.2 Safety Philosophy

The ZIC controller shall implement a **Fail-Safe by Default** design philosophy.

Whenever uncertainty exists regarding the safe operation of the irrigation system, the controller shall always choose the safest possible state.

The safe hydraulic state is defined as:

* Master Valve Closed
* All Zone Valves Closed
* All Relay Outputs OFF
* Irrigation Programs Suspended
* Alarm Generated
* Event Logged
* MQTT Alarm Published
* Local Display Activated

Under no circumstances shall the controller continue irrigation after detection of a critical safety condition.

---

# 10.3 Safety Objectives

The controller shall be designed to achieve the following objectives:

### SAF-001

Prevent uncontrolled water flow.

---

### SAF-002

Prevent flooding caused by valve or pipe failure.

---

### SAF-003

Protect irrigation equipment from abnormal operating conditions.

---

### SAF-004

Protect users from electrical hazards by maintaining complete separation between mains voltage and SELV circuits.

---

### SAF-005

Maintain complete operational history following safety-related incidents.

---

### SAF-006

Allow rapid diagnosis and recovery after faults.

---

# 10.4 Hazard Categories

Potential hazards are divided into four categories.

| Category | Description         |
| -------- | ------------------- |
| H1       | Electrical Hazards  |
| H2       | Hydraulic Hazards   |
| H3       | Software Hazards    |
| H4       | Operational Hazards |

---

# 10.5 Electrical Hazards

Potential hazards include:

* Loss of mains power
* Transformer failure
* 24 VAC loss
* Short circuits
* Relay failure
* Power supply failure
* Overvoltage
* Lightning-induced transients

Mitigation measures:

* DIN rail circuit protection
* Surge protection device (SPD)
* Transformer isolation
* Separate SELV wiring
* Continuous 24 VAC monitoring
* Hardware watchdog

---

# 10.6 Hydraulic Hazards

Potential hazards include:

* Pipe rupture
* Broken sprinkler
* Solenoid valve failure
* Master valve failure
* Water hammer
* Blocked filter
* Empty water supply

Mitigation measures:

* Flow monitoring
* Pressure monitoring
* Flow Learning
* Pressure baseline learning
* Master valve supervision
* Automatic shutdown

---

# 10.7 Software Hazards

Potential hazards include:

* Deadlock
* Memory corruption
* Stack overflow
* Heap exhaustion
* Infinite loops
* Task starvation
* MQTT communication failure
* OTA interruption

Mitigation measures:

* FreeRTOS task watchdog
* Hardware watchdog
* Heap monitoring
* Stack monitoring
* Exception logging
* OTA rollback
* Configuration CRC verification

---

# 10.8 Operational Hazards

Potential hazards include:

* Incorrect configuration
* Incorrect irrigation programs
* Accidental manual activation
* Maintenance during operation
* Unauthorized access

Mitigation measures:

* User confirmation dialogs
* Service Mode
* Door switch monitoring
* User access levels
* Configuration backup
* Audit logging

---

# 10.9 Safety Architecture

The controller implements multiple independent safety layers.

```text
Layer 1
Electrical Protection

↓

Layer 2
Hardware Monitoring

↓

Layer 3
Firmware Diagnostics

↓

Layer 4
Hydraulic Supervision

↓

Layer 5
Alarm Manager

↓

Layer 6
Safe Shutdown
```

Failure of one layer shall not disable the remaining layers.

---

# 10.10 Alarm Classification

Three alarm severities are defined.

## Information

No immediate action required.

Examples:

* OTA completed
* Configuration changed
* Wi-Fi connected

---

## Warning

Requires user attention but irrigation may continue.

Examples:

* Weak Wi-Fi signal
* Cabinet door open
* High cabinet temperature
* Weather data unavailable

---

## Critical

Immediate shutdown required.

Examples:

* Leak detected
* Pipe burst
* Pressure collapse
* Master valve failure
* Flow sensor failure during irrigation
* Emergency Stop
* Watchdog reset during irrigation

---

# 10.11 Safe Shutdown Procedure

The following sequence shall be executed whenever a Critical Alarm occurs.

```text
Critical Fault

↓

Cancel Irrigation Program

↓

Close Zone Valve(s)

↓

Close Master Valve

↓

Disable All Relays

↓

Store Alarm

↓

Store Event Log

↓

Publish MQTT Alarm

↓

Wake Display

↓

Enter Fault State
```

This sequence shall complete in the shortest practical time while maintaining orderly system shutdown.

---

# 10.12 Watchdog Strategy

The controller shall implement multiple watchdog mechanisms.

## Hardware Watchdog

Provided by the ESP32.

Purpose:

Recover from catastrophic firmware failure.

---

## Task Watchdog

Monitor:

* Irrigation Engine
* Flow Manager
* Pressure Manager
* MQTT Manager
* UI Task

Failure shall be logged.

---

## Communication Watchdog

Monitor:

* Wi-Fi
* MQTT
* I²C
* ADS1115
* MCP23017

Communication failures shall generate diagnostic events.

---

# 10.13 Redundancy Strategy

Where practical, critical decisions shall use multiple sources of information.

Examples:

Flow Verification

* Expected Flow
* Measured Flow

Pressure Verification

* Expected Pressure
* Measured Pressure

Weather Verification

* Local Station
* Internet Forecast

Configuration

* CRC Verification
* Version Verification

---

# 10.14 Risk Assessment Matrix

| Severity | Description          | Required Action |
| -------- | -------------------- | --------------- |
| Low      | Informational        | Log Only        |
| Medium   | Warning              | Notify User     |
| High     | Restricted Operation | Safe Recovery   |
| Critical | Immediate Hazard     | Safe Shutdown   |

---

# 10.15 Reliability Objectives

The controller shall meet the following design objectives.

| Parameter               |        Target |
| ----------------------- | ------------: |
| Seasonal Availability   |        >99.9% |
| MTBF                    | >50,000 hours |
| Data Retention          |     >10 years |
| Configuration Integrity |          100% |
| OTA Success Rate        |          >99% |

---

# 10.16 Failure Recovery

The controller shall automatically recover from:

* Wi-Fi loss
* MQTT loss
* Power restoration
* Temporary sensor failures
* OTA interruption

Recovery shall not require user intervention unless system safety cannot be guaranteed.

---

# 10.17 Manual Override

Manual control shall remain available whenever safe operation is possible.

Manual override shall **not** bypass:

* Emergency Stop
* Leak detection
* Pressure collapse protection
* Critical hardware failures

The controller shall reject unsafe manual commands.

---

# 10.18 User Safety

The controller shall minimize risk to personnel by:

* Maintaining SELV separation
* Clearly identifying hazardous voltages
* Preventing unintended valve activation during service mode
* Supporting Emergency Stop
* Logging all service access through the cabinet door sensor

---

# 10.19 Data Integrity

The controller shall protect:

* Configuration
* Historical logs
* Water consumption data
* Learned hydraulic baselines

Protection methods:

* CRC validation
* Version control
* Atomic writes
* Configuration backup
* OTA rollback

---

# 10.20 Engineering Change Control

Safety-related modifications shall require:

* Engineering review
* Risk assessment
* Regression testing
* Documentation update

Changes affecting safety functions shall not be implemented without updating the Master Engineering Package.

---

# 10.21 Reliability Growth

The firmware shall support continuous improvement through:

* Diagnostic logging
* Anonymous field statistics (optional)
* Failure analysis
* Engineering change requests
* Preventive maintenance recommendations

---

# 10.22 Future Safety Enhancements

Future hardware revisions may include:

* Dual flow sensors
* Dual pressure sensors
* Battery-backed RTC
* Redundant power monitoring
* Water reservoir monitoring
* Pump dry-run protection
* Isolation monitoring
* Functional safety self-tests

The architecture defined in Version 5.0 shall support these enhancements without fundamental redesign.

---

# 10.23 Risk Register (Initial)

| Risk ID | Description              | Likelihood |   Impact | Mitigation                                             |
| ------- | ------------------------ | ---------: | -------: | ------------------------------------------------------ |
| R-001   | Pipe rupture             |     Medium | Critical | Flow + pressure supervision                            |
| R-002   | Master valve failure     |        Low | Critical | Shutdown + alarm                                       |
| R-003   | Wi-Fi loss               |     Medium |      Low | Local autonomous operation                             |
| R-004   | MQTT broker unavailable  |     Medium |      Low | Buffered events, local logging                         |
| R-005   | Pressure sensor failure  |        Low |     High | Diagnostics + fail-safe operation                      |
| R-006   | Flow sensor failure      |        Low |     High | Alarm, disable automatic irrigation until acknowledged |
| R-007   | Power failure            |     Medium |   Medium | Safe shutdown, automatic recovery                      |
| R-008   | Configuration corruption |   Very Low |     High | CRC validation, backup, factory recovery               |

The Risk Register shall be maintained throughout the project lifecycle and updated as new hazards are identified.

---

# 10.24 Safety Validation

All safety functions shall be validated through:

* Functional testing
* Failure injection
* Long-duration endurance testing
* Factory Acceptance Test (FAT)
* Site Acceptance Test (SAT)

Every safety requirement shall be traceable to one or more verification procedures defined in **Volume 5 – Manufacturing, Testing & Operations**.

---

# 10.25 Chapter Summary

This chapter defines the safety philosophy, reliability objectives and initial risk management framework for the Zmartify Irrigation Controller.

The platform is designed around a fail-safe architecture in which protection of people, property and water resources always takes precedence over irrigation continuity. By combining layered electrical protection, hydraulic supervision, watchdog monitoring and comprehensive diagnostics, ZIC aims to deliver reliable long-term operation while remaining maintainable and extensible throughout its lifecycle.

---

# End of Chapter 10

**Next Chapter**

**Chapter 11 – Requirements Traceability Matrix (RTM) & Volume 1 Summary**

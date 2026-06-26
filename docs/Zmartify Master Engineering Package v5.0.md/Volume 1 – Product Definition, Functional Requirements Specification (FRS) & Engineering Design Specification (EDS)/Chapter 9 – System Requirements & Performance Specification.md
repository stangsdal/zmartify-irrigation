# Chapter 9 – System Requirements & Performance Specification

---

# 9 System Requirements & Performance Specification

## 9.1 Purpose

This chapter defines the measurable system-level performance requirements for the Zmartify Irrigation Controller (ZIC).

Where previous chapters describe functionality and operational concepts, this chapter establishes the engineering performance targets against which the completed system shall be verified.

All requirements defined herein shall be measurable through Factory Acceptance Testing (FAT), Site Acceptance Testing (SAT) or long-term operational testing.

---

# 9.2 System Design Objectives

The controller shall be designed to achieve the following high-level objectives:

| Objective        | Target                       |
| ---------------- | ---------------------------- |
| Reliability      | >99.9% seasonal availability |
| Safety           | Fail-safe operation          |
| Expandability    | Modular hardware & firmware  |
| Maintainability  | OTA firmware updates         |
| Water Efficiency | Weather and ET optimized     |
| Interoperability | MQTT-first architecture      |
| Serviceability   | Field replaceable modules    |

---

# 9.3 Availability Requirements

## SYS-PER-001

The controller shall be capable of operating continuously throughout the irrigation season.

Target uptime:

**>99.9 %**

excluding planned maintenance.

---

## SYS-PER-002

Unexpected software crashes shall automatically recover through the watchdog system.

Maximum recovery time:

**60 seconds**

---

## SYS-PER-003

Power interruptions shall not result in permanent loss of:

* Configuration
* Zone definitions
* Programs
* Historical statistics
* Alarm history

---

# 9.4 Boot Performance

## SYS-PER-010

Maximum boot time

From power application until Idle state:

**≤30 seconds**

---

## SYS-PER-011

Following boot completion, all scheduled irrigation programs shall be fully operational without requiring user intervention.

---

# 9.5 User Interface Performance

## SYS-UI-001

Touch response time

Maximum:

**100 ms**

---

## SYS-UI-002

Screen transition

Maximum:

**300 ms**

---

## SYS-UI-003

Display wake-up

Maximum:

**500 ms**

---

## SYS-UI-004

Backlight timeout

Default:

**600 seconds**

User configurable:

60–3600 seconds

---

# 9.6 Irrigation Performance

## SYS-IRR-001

Maximum supported zones

15 irrigation zones

1 master valve

---

## SYS-IRR-002

Zone runtime resolution

1 second

---

## SYS-IRR-003

Program scheduling accuracy

±1 second

---

## SYS-IRR-004

Maximum concurrent zones

Configurable

Default:

One zone

Future hardware revisions may permit multiple simultaneous zones.

---

## SYS-IRR-005

Master valve opening delay

Configurable

Default:

2 seconds

Purpose:

Allow pressure stabilization before opening irrigation valves.

---

## SYS-IRR-006

Zone changeover delay

Configurable

Default:

1 second

Purpose:

Reduce hydraulic shock.

---

# 9.7 Hydraulic Performance

## SYS-HYD-001

Flow update interval

≤1 second

---

## SYS-HYD-002

Pressure update interval

≤1 second

---

## SYS-HYD-003

Leak detection response

Maximum:

5 seconds

---

## SYS-HYD-004

Pressure fault response

Maximum:

2 seconds

---

## SYS-HYD-005

Emergency valve shutdown

Maximum:

1 second

---

# 9.8 Weather Processing

## SYS-WEA-001

Weather update interval

Configurable

Default:

15 minutes

---

## SYS-WEA-002

Forecast refresh interval

Configurable

Default:

60 minutes

---

## SYS-WEA-003

ET calculation

Minimum frequency:

Once daily

Preferred:

Hourly recalculation during irrigation season.

---

# 9.9 MQTT Performance

## SYS-MQTT-001

MQTT connection recovery

Maximum:

30 seconds

---

## SYS-MQTT-002

Alarm publication

Maximum:

1 second

---

## SYS-MQTT-003

Telemetry publication

Default:

Every 30 seconds

Configurable.

---

## SYS-MQTT-004

Retained configuration messages

Supported.

---

## SYS-MQTT-005

Last Will and Testament (LWT)

Mandatory.

---

# 9.10 Storage Requirements

## SYS-STO-001

Configuration storage

Non-volatile.

---

## SYS-STO-002

Minimum retained history

| Category            | Minimum   |
| ------------------- | --------- |
| Irrigation Log      | 365 days  |
| Alarm Log           | 365 days  |
| Daily Water Usage   | 5 years   |
| Monthly Water Usage | Lifetime  |
| Lifetime Counter    | Permanent |

If storage limits are reached, the oldest records shall be archived or overwritten according to a configurable retention policy.

---

# 9.11 Sensor Performance

## Flow Meter

| Parameter           | Requirement           |
| ------------------- | --------------------- |
| Update Interval     | ≤1 second             |
| Learning Resolution | Better than 2%        |
| Detection Accuracy  | ±5% after calibration |

---

## Pressure Sensor

| Parameter            | Requirement |
| -------------------- | ----------- |
| ADC Resolution       | 16 bit      |
| Sampling Rate        | ≥1 Hz       |
| Measurement Accuracy | ±0.1 bar    |
| Calibration          | Two-point   |

---

## Cabinet Temperature

Resolution:

0.5°C or better

---

# 9.12 Electrical Performance

## Logic Supply

Voltage:

5 VDC

Maximum variation:

±5%

---

## Valve Supply

Voltage:

24 VAC

Continuous monitoring required.

---

## Current Consumption

Normal controller electronics:

<10 W

Transformer sizing shall include adequate margin for simultaneous valve operation.

---

# 9.13 Network Performance

Supported Wi-Fi:

802.11 b/g/n (2.4 GHz)

Minimum recommended signal:

RSSI > –65 dBm

Controller shall continue operating if Wi-Fi is temporarily unavailable.

---

# 9.14 Reliability Requirements

Target Mean Time Between Failures (MTBF):

> 50,000 hours

Expected service life:

> 10 years

Firmware updates shall not reduce stored configuration reliability.

---

# 9.15 Environmental Requirements

Operating temperature:

-10°C to +50°C

Storage temperature:

-20°C to +70°C

Relative humidity:

0–95%

Non-condensing.

Cabinet protection:

Minimum IP65.

---

# 9.16 Maintainability

Routine maintenance shall not require:

* Firmware recompilation
* Special programming tools
* Hardware disassembly beyond normal service access

Firmware updates shall be performed through OTA whenever possible.

Configuration backup and restore shall be available through the user interface and MQTT.

---

# 9.17 Scalability

The software architecture shall support future expansion without requiring redesign.

Future supported devices include:

* Additional valve controllers
* Remote I/O modules
* Weather stations
* Soil moisture sensors
* Pump controllers
* Fertigation controllers
* Water reservoir monitoring

---

# 9.18 Safety Performance

Critical alarms shall initiate a safe shutdown within the following maximum response times.

| Event                     |     Maximum Response |
| ------------------------- | -------------------: |
| Emergency Stop            |             1 second |
| Leak Detection            |            5 seconds |
| Pressure Collapse         |            2 seconds |
| Watchdog Failure          | Immediate on restart |
| 24 VAC Failure            |            2 seconds |
| Controller Internal Fault |            Immediate |

Safe shutdown shall always include:

* Master valve closed
* Zone valves closed
* Alarm logged
* MQTT notification
* Display wake-up

---

# 9.19 Verification Matrix

All performance requirements shall be verified prior to production release.

| Category              | Verification     |
| --------------------- | ---------------- |
| Boot Time             | FAT              |
| UI Performance        | FAT              |
| Hydraulic Performance | FAT + SAT        |
| MQTT Performance      | Integration Test |
| OTA                   | Integration Test |
| Weather               | Field Test       |
| Long-Term Stability   | Endurance Test   |
| Safety Functions      | Functional Test  |

Each requirement shall be traceable to one or more acceptance tests defined in **Volume 5 – Manufacturing, Testing & Operations**.

---

# 9.20 Engineering Margin

The ZIC platform shall be designed with engineering margins appropriate for long-term field operation.

Recommended margins include:

* CPU utilization <60% under normal load
* Heap usage <70%
* Power supply utilization <70%
* Transformer utilization <80%
* Wi-Fi RSSI > –65 dBm
* Relay switching frequency within manufacturer specifications

These margins provide capacity for future firmware enhancements while maintaining reliable operation.

---

# 9.21 Future Performance Objectives

The architecture shall support future enhancements including:

* Multi-controller synchronization
* Predictive irrigation using AI models
* Cloud analytics (optional)
* Dynamic hydraulic optimization
* Distributed remote valve stations
* High-availability MQTT architectures

These features shall be implemented without compromising compatibility with the Version 5.0 engineering baseline.

---

# 9.22 Chapter Summary

This chapter defines the measurable performance characteristics required of the Zmartify Irrigation Controller.

Together with the Functional Requirements Specification and Engineering Design Specification, these requirements establish the engineering acceptance criteria against which the controller shall be designed, implemented, tested and maintained.

The performance targets defined herein provide the foundation for verification activities in later volumes and ensure that the finished product meets the expectations of a professional irrigation controller suitable for continuous operation in demanding residential and commercial environments.

---

# End of Chapter 9

**Next Chapter**

**Chapter 10 – System Safety, Reliability & Risk Management**

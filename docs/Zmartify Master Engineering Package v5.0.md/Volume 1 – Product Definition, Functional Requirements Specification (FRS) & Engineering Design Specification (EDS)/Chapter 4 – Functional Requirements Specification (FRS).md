# Chapter 4 – Functional Requirements Specification (FRS)

---

# 4 Functional Requirements Specification (FRS)

## 4.1 Purpose

The Functional Requirements Specification (FRS) defines **what** the Zmartify Irrigation Controller (ZIC) shall do.

The FRS intentionally does **not** define implementation details. These are specified in later volumes covering firmware architecture, hardware design and communication interfaces.

Each requirement shall be:

* Unique
* Traceable
* Verifiable
* Testable
* Version controlled

Every requirement shall receive a unique Requirement ID.

---

# 4.2 Requirement Classification

Requirements are grouped into the following categories.

| Prefix  | Category                    |
| ------- | --------------------------- |
| FR-SYS  | System Requirements         |
| FR-IRR  | Irrigation Requirements     |
| FR-HYD  | Hydraulic Requirements      |
| FR-WEA  | Weather Requirements        |
| FR-UI   | User Interface Requirements |
| FR-MQTT | MQTT Requirements           |
| FR-LOG  | Logging Requirements        |
| FR-ALM  | Alarm Requirements          |
| FR-DIA  | Diagnostic Requirements     |
| FR-MNT  | Maintenance Requirements    |
| FR-PER  | Performance Requirements    |
| FR-SEC  | Security Requirements       |

---

# 4.3 Requirement Format

Each requirement shall follow the format below.

**Requirement ID**

FR-XXX-001

**Title**

Short descriptive title.

**Description**

Formal requirement statement.

**Priority**

* Mandatory
* Recommended
* Optional

**Verification**

* Inspection
* Functional Test
* Integration Test
* Long-Term Test
* Field Test

**Status**

* Draft
* Approved
* Implemented
* Verified

---

# 4.4 System Requirements

---

## FR-SYS-001

### Controller Operation

**Requirement**

The controller shall operate continuously throughout the irrigation season.

Priority

Mandatory

Verification

Long-Term Test

---

## FR-SYS-002

### Autonomous Operation

The controller shall continue operating during loss of Internet connectivity.

Weather forecast services may become unavailable without affecting safe irrigation operation.

Priority

Mandatory

---

## FR-SYS-003

### Safe Startup

Following power restoration the controller shall:

* Initialize hardware
* Verify configuration
* Perform self-test
* Restore MQTT connection
* Enter Idle state

The controller shall never automatically resume irrigation after unexpected power restoration.

Priority

Mandatory

---

## FR-SYS-004

### Safe Shutdown

Whenever a critical fault occurs the controller shall:

* Stop all irrigation
* Close master valve
* Record event
* Publish MQTT alarm

Priority

Mandatory

---

# 4.5 Irrigation Requirements

---

## FR-IRR-001

### Number of Zones

The controller shall support:

* 15 irrigation zones
* 1 dedicated master valve

Priority

Mandatory

---

## FR-IRR-002

### Zone Naming

Each irrigation zone shall support:

* Custom name
* Icon
* Colour
* Zone type

Maximum name length:

32 characters

---

## FR-IRR-003

### Zone Enable

Each irrigation zone shall be independently:

* Enabled
* Disabled

Disabled zones shall never be activated by automatic programs.

---

## FR-IRR-004

### Manual Irrigation

The user shall be able to manually start any irrigation zone.

Manual runtime shall be configurable.

---

## FR-IRR-005

### Programmed Irrigation

The controller shall support unlimited irrigation programs, subject only to available non-volatile storage.

Each program shall contain:

* Name
* Enabled state
* One or more start times
* Assigned zones
* Zone runtimes
* Watering days
* Seasonal adjustment

---

## FR-IRR-006

### Cycle & Soak

Each irrigation zone shall optionally support Cycle & Soak operation.

Parameters:

* Runtime
* Soak time
* Number of cycles

Purpose:

Reduce runoff on heavy soils and slopes.

---

## FR-IRR-007

### Seasonal Adjustment

The controller shall support a seasonal adjustment factor between:

0 %

and

300 %

in 1 % increments.

---

## FR-IRR-008

### Rain Delay

The controller shall support user-configurable rain delay.

Range:

1–30 days

Remaining delay shall survive power failure.

---

# 4.6 Hydraulic Requirements

---

## FR-HYD-001

### Flow Measurement

The controller shall continuously measure water flow during irrigation.

Resolution shall be better than:

0.1 L/min

---

## FR-HYD-002

### Pressure Measurement

Pressure shall be measured continuously whenever irrigation is active.

Measurement resolution shall be sufficient to detect pressure deviations greater than ±0.1 bar.

---

## FR-HYD-003

### Flow Learning

The controller shall learn expected flow for each irrigation zone.

Stored values shall include:

* Average flow
* Minimum flow
* Maximum flow
* Standard deviation

---

## FR-HYD-004

### Leak Detection

Unexpected flow exceeding configured limits shall generate a critical alarm.

The controller shall immediately:

* Stop irrigation
* Close master valve
* Publish MQTT alarm
* Record event

---

## FR-HYD-005

### Pressure Failure

Loss of pressure during irrigation shall generate a hydraulic fault.

The controller shall safely terminate irrigation.

---

# 4.7 Weather Requirements

---

## FR-WEA-001

### Local Weather Station

The controller shall accept weather information from a local weather station.

Supported parameters:

* Temperature
* Humidity
* Wind speed
* Rainfall
* Solar radiation
* UV index

---

## FR-WEA-002

### Internet Forecast

The controller shall optionally retrieve weather forecast information.

Supported providers include:

* Weather Underground
* OpenWeather
* Yr.no

Additional providers may be added.

---

## FR-WEA-003

### Irrigation Decision

Automatic irrigation shall consider:

* Measured rainfall
* Forecast rainfall
* Wind speed
* Temperature
* Humidity
* Evapotranspiration

before irrigation begins.

---

## FR-WEA-004

### Freeze Protection

Automatic irrigation shall not begin when configured freeze conditions exist.

---

# 4.8 User Interface Requirements

---

## FR-UI-001

### Dashboard

The dashboard shall display:

* Current state
* Active zone
* Weather
* Flow
* Pressure
* Water consumption
* Active alarms

---

## FR-UI-002

### Screen Timeout

Display backlight shall automatically switch off after:

10 minutes

of inactivity.

Touch or button press shall immediately wake the display.

---

## FR-UI-003

### Local Buttons

Four hardware buttons shall be supported:

* Home / Wake
* Manual Start
* Stop
* Emergency Stop

---

## FR-UI-004

### Language Support

The firmware shall support multiple user interface languages.

Initial languages:

* English
* Danish

Future translations shall be possible without firmware redesign.

---

# 4.9 MQTT Requirements

---

## FR-MQTT-001

The controller shall publish operational state.

---

## FR-MQTT-002

The controller shall publish alarms immediately.

---

## FR-MQTT-003

The controller shall publish telemetry including:

* Flow
* Pressure
* Water usage
* Weather
* Zone status

---

## FR-MQTT-004

The controller shall subscribe to:

* Commands
* Configuration updates
* OTA requests

---

# 4.10 Alarm Requirements

---

## FR-ALM-001

Alarm categories:

* Information
* Warning
* Critical

---

## FR-ALM-002

Critical alarms shall include:

* Leak detected
* Pipe burst
* No flow
* Pressure collapse
* Master valve fault
* Controller watchdog fault
* Emergency stop

---

## FR-ALM-003

Critical alarms shall remain active until acknowledged.

---

# 4.11 Logging Requirements

---

## FR-LOG-001

The controller shall maintain persistent logs.

Minimum log categories:

* Irrigation
* Alarms
* Weather
* Flow
* Pressure
* User actions
* Configuration changes

---

## FR-LOG-002

Each log entry shall contain:

* Timestamp
* Source
* Event
* Severity
* Additional data

---

# 4.12 Performance Requirements

---

## FR-PER-001

Boot time shall not exceed:

30 seconds

---

## FR-PER-002

Touch response shall be less than:

100 milliseconds

---

## FR-PER-003

MQTT alarm publication shall occur within:

1 second

after alarm detection.

---

## FR-PER-004

Flow measurement update interval:

1 second or better.

---

# 4.13 Maintenance Requirements

---

## FR-MNT-001

Firmware shall support OTA updates.

---

## FR-MNT-002

Complete controller configuration shall be exportable.

---

## FR-MNT-003

Configuration restoration shall be supported.

---

## FR-MNT-004

Hardware self-test shall be available from the service menu.

---

# 4.14 Verification Matrix

Each requirement shall be verified before production release.

Verification methods include:

| Method | Description                |
| ------ | -------------------------- |
| INS    | Inspection                 |
| FT     | Functional Test            |
| IT     | Integration Test           |
| LT     | Long-Term Reliability Test |
| SAT    | Site Acceptance Test       |

Every requirement in this chapter shall be referenced in the Factory Acceptance Test (Volume 5).

---

# 4.15 Chapter Summary

This Functional Requirements Specification defines the operational capabilities required of the Zmartify Irrigation Controller. These requirements form the contractual baseline for all hardware and firmware development.

Subsequent chapters will translate these functional requirements into an Engineering Design Specification (EDS), firmware architecture, hardware architecture and verification procedures.

---

# End of Chapter 4

**Next Chapter**

**Chapter 5 – Engineering Design Specification (EDS) – System Architecture**

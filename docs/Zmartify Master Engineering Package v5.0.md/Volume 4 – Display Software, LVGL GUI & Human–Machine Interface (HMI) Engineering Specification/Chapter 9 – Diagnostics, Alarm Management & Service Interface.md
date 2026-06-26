# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 9

# Diagnostics, Alarm Management & Service Interface

---

# 9.1 Purpose

This chapter defines the engineering diagnostics, alarm management and service interface of the Zmartify Human–Machine Interface (HMI).

Unlike consumer irrigation controllers, the Zmartify platform continuously evaluates controller health, hydraulic performance, communications, sensors and firmware integrity.

The Service Interface provides structured access to this information while maintaining strict separation between operational users and service personnel.

The interface supports:

* Real-time diagnostics
* Alarm visualization
* Event logging
* Predictive maintenance
* Engineering troubleshooting
* Factory testing
* Commissioning
* Service operations

---

# 9.2 Design Objectives

The Diagnostics Interface shall:

* Present engineering information clearly
* Support rapid fault isolation
* Minimize unnecessary service visits
* Support predictive maintenance
* Provide actionable recommendations
* Protect service functions through authorization
* Operate independently of cloud services

---

# 9.3 Diagnostics Screen Hierarchy

```text id="diag-screen-tree"
Diagnostics

│

├── System Health

├── Controller Status

├── Network

├── MQTT

├── Sensors

├── Valves

├── Hydraulics

├── Weather

├── Storage

├── Tasks

├── Event Log

├── Alarm History

├── Self-Test

└── Service Tools
```

---

# 9.4 Diagnostic Philosophy

The Diagnostics Interface follows four engineering principles.

### DIA-001

Explain, Don't Just Report

Every detected issue shall include a human-readable explanation.

---

### DIA-002

Recommend an Action

Whenever possible, the interface shall recommend corrective actions.

---

### DIA-003

Separate Symptoms from Causes

Detected symptoms and probable root causes shall be distinguished.

---

### DIA-004

Support Predictive Maintenance

The interface shall identify developing problems before they become failures.

---

# 9.5 System Health Dashboard

The System Health Dashboard provides an overview of controller condition.

Displayed information:

* Overall Health Score
* Active Alarms
* System Status
* Hydraulic Status
* Communication Status
* Storage Status
* Last Self-Test

Reference layout:

```text id="system-health-layout"
+------------------------------------------------+

System Health

98 %

Excellent

+------------------------------------------------+

Hydraulics

Excellent

Network

Connected

Storage

Healthy

+------------------------------------------------+
```

---

# 9.6 Health Score

The controller continuously calculates an engineering health score.

Range:

```text id="health-score"
0 – 100
```

Classification:

| Score  | Status    |
| ------ | --------- |
| 95–100 | Excellent |
| 85–94  | Good      |
| 70–84  | Attention |
| 50–69  | Warning   |
| <50    | Critical  |

The Health Score combines:

* CPU utilization
* Memory availability
* Storage integrity
* Hydraulic performance
* Sensor availability
* Communication quality
* Alarm state

---

# 9.7 Controller Status Screen

Displays:

* Controller Mode
* Firmware Version
* Hardware Revision
* Uptime
* Boot Count
* Restart Reason
* Active Tasks

Example:

```text id="controller-status"
Firmware

5.0.0

Hardware

Rev B

Uptime

18 Days

Restart

Power-On
```

---

# 9.8 Network Diagnostics

Displays:

* Wi-Fi Status
* SSID
* IP Address
* RSSI
* DHCP Status
* Gateway
* DNS

Connection quality:

| RSSI       | Quality   |
| ---------- | --------- |
| > -60 dBm  | Excellent |
| -60 to -70 | Good      |
| -70 to -80 | Fair      |
| < -80      | Poor      |

---

# 9.9 MQTT Diagnostics

Displays:

* Broker Status
* Connection Time
* Client ID
* Publish Rate
* Subscribe Rate
* Last Reconnect
* Last Will Status

Example:

```text id="mqtt-diagnostics"
Broker

Connected

QoS

1

Messages

2,481

Reconnects

0
```

---

# 9.10 Sensor Diagnostics

Each sensor displays:

* Status
* Last Reading
* Update Age
* Calibration Status
* Signal Quality

Supported sensors:

* Flow
* Pressure
* Temperature
* Rain
* Wind
* GT911 Touch
* Ambient Light (future)

---

# 9.11 Valve Diagnostics

Valve diagnostics include:

* Valve State
* Response Time
* Current Runtime
* Activation Count
* Last Test
* Estimated Lifetime

Valve test functions are available only in Service Mode.

---

# 9.12 Hydraulic Diagnostics

Displays:

* Flow Stability
* Pressure Stability
* Hydraulic Health
* Leak Detection
* Restriction Detection
* Calibration Confidence

Hydraulic diagnostics shall link directly to the Hydraulic Dashboard.

---

# 9.13 Weather Diagnostics

Displays:

* Weather Provider
* Last Update
* Forecast Age
* API Response Time
* ET Calculation Status

Failures shall include:

* Cause
* Last Successful Update
* Retry Countdown

---

# 9.14 Storage Diagnostics

Displays:

* Flash Usage
* File System Health
* Configuration Size
* Log Size
* OTA Partition Status

Example:

```text id="storage"
Flash

38 %

Used

OTA

Healthy
```

---

# 9.15 FreeRTOS Task Monitor

Displays:

* Task Name
* State
* Priority
* CPU Usage
* Stack Usage

Example:

| Task       | CPU | Stack |
| ---------- | --: | ----: |
| Display    |  6% |   42% |
| MQTT       |  3% |   38% |
| Irrigation |  2% |   35% |
| Weather    | <1% |   30% |

Visible only in Service Mode.

---

# 9.16 Event Log

The Event Log records significant controller activity.

Examples:

* Boot
* Shutdown
* Program Started
* Program Completed
* Alarm Raised
* Alarm Cleared
* Firmware Updated
* Configuration Changed

Events are ordered chronologically.

---

# 9.17 Alarm History

Alarm history displays:

* Timestamp
* Severity
* Category
* Description
* Resolution

Example:

```text id="alarm-history"
14 Jul

08:15

Warning

Low Pressure

Resolved

08:18
```

Filtering options:

* Severity
* Date
* Category
* Status

---

# 9.18 Alarm Categories

| Category      | Examples         |
| ------------- | ---------------- |
| Hydraulic     | Leak, Pressure   |
| Electrical    | Valve Fault      |
| Weather       | Freeze           |
| Network       | Wi-Fi            |
| MQTT          | Broker           |
| Firmware      | OTA              |
| Configuration | Invalid Settings |
| Hardware      | Sensor Failure   |

---

# 9.19 Alarm Detail Screen

Displays:

* Severity
* Description
* Root Cause
* Recommended Action
* Event Timeline
* Related Measurements

Example:

```text id="alarm-detail"
Low Pressure

Possible Causes

Dirty Filter

Restricted Pipe

Pump Failure

Recommendation

Inspect Filter
```

---

# 9.20 Self-Test Interface

Available tests:

* Quick Test
* Complete Test
* Valve Test
* Flow Test
* Pressure Test
* Display Test
* Touch Test
* Network Test
* MQTT Test

Progress display:

```text id="self-test"
System Test

████████░░

82 %

Testing

Flow Sensor
```

---

# 9.21 Service Tools

Available only in Service Mode.

Functions include:

* Valve Calibration
* Flow Calibration
* Pressure Calibration
* Touch Calibration
* Display Test
* Firmware Information
* Factory Diagnostics
* Export Logs

Access requires Service or Administrator authorization.

---

# 9.22 Diagnostic Charts

Historical engineering charts include:

* CPU Utilization
* Memory Usage
* Wi-Fi RSSI
* MQTT Latency
* Flow Stability
* Pressure Stability
* Health Score

Supported time periods:

* Hour
* Day
* Week
* Month

---

# 9.23 Predictive Maintenance

The controller continuously evaluates long-term trends.

Maintenance indicators include:

* Valve Aging
* Flow Degradation
* Pressure Drift
* Sensor Stability
* Storage Wear
* Wi-Fi Quality

Example:

```text id="predictive"
Maintenance

Recommended

Flow Reduced

11 %

Since Last Month
```

---

# 9.24 Service Mode

Service Mode enables:

* Advanced diagnostics
* Calibration
* Factory testing
* Engineering charts
* Raw measurements
* Debug information

A persistent visual indicator shall remain visible while Service Mode is active.

---

# 9.25 Export Functions

The Service Interface supports exporting:

* Configuration
* Diagnostics
* Alarm History
* Event Log
* Self-Test Results
* Calibration Report

Supported formats:

* JSON
* CSV
* PDF (future)

---

# 9.26 Security

The Diagnostics Interface shall enforce role-based access.

| Function         | User | Installer | Service | Admin |
| ---------------- | :--: | :-------: | :-----: | :---: |
| View Diagnostics |   ✔  |     ✔     |    ✔    |   ✔   |
| Run Self-Test    |   ✖  |     ✔     |    ✔    |   ✔   |
| Calibration      |   ✖  |     ✖     |    ✔    |   ✔   |
| Export Logs      |   ✖  |     ✖     |    ✔    |   ✔   |
| Factory Tools    |   ✖  |     ✖     |    ✖    |   ✔   |

---

# 9.27 Performance Requirements

| Operation        |  Target |
| ---------------- | ------: |
| Open Diagnostics | <200 ms |
| Alarm Popup      | <250 ms |
| Self-Test Start  | <500 ms |
| Event Log Search | <300 ms |
| Chart Refresh    |    1 Hz |

---

# 9.28 Future Extensions

The Diagnostics Framework is designed to support:

* AI fault diagnosis
* Remote service sessions
* Digital twin diagnostics
* Cloud-assisted analytics
* QR-code service reports
* Video-assisted troubleshooting
* Predictive component replacement
* Fleet-wide health monitoring

These additions shall integrate without altering the existing diagnostics architecture.

---

# 9.29 Engineering Notes

The Diagnostics and Service Interface transforms the controller from a simple irrigation appliance into a professional engineering system. By combining real-time monitoring, historical analysis, predictive maintenance and guided troubleshooting, the interface significantly reduces diagnostic time while improving long-term system reliability.

The separation between user, installer and service functions also ensures that advanced engineering capabilities remain available without overwhelming everyday users.

---

# 9.30 Chapter Summary

This chapter defines the Diagnostics, Alarm Management and Service Interface for the Zmartify Human–Machine Interface.

The architecture provides comprehensive visibility into controller operation through engineering dashboards, alarm management, historical logs and predictive diagnostics. Together with the Dashboard, Irrigation, Weather and Hydraulic interfaces, it completes the operational HMI required for professional irrigation control while establishing the foundation for future AI-assisted maintenance and remote service capabilities.

---

# End of Chapter 9

**Next Chapter**

**Chapter 10 – Widget Library, Reusable UI Components & Design System**

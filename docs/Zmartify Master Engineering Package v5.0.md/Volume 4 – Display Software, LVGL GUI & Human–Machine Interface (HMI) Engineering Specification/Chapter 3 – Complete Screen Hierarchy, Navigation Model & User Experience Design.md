# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 3

# Complete Screen Hierarchy, Navigation Model & User Experience Design

---

# 3.1 Purpose

This chapter defines the complete navigation architecture of the Zmartify Human–Machine Interface.

The navigation model has been designed to minimize user interaction while providing rapid access to all controller functions.

The design emphasizes:

* Simple navigation
* Consistent layout
* Predictable behavior
* Fast operation
* Minimal learning curve
* Professional appearance

---

# 3.2 Navigation Philosophy

The Zmartify interface follows four fundamental principles.

### UX-001

Dashboard First

The dashboard shall always be the primary screen.

---

### UX-002

Maximum Three Touches

Any common operation shall require no more than three touches.

---

### UX-003

No Hidden Functions

Critical controller functions shall never be hidden behind complex menus.

---

### UX-004

Consistent Navigation

Navigation controls shall remain in identical positions throughout the interface.

---

# 3.3 Screen Hierarchy

```text id="8m7ahv"
Dashboard

│

├── Irrigation

│     ├── Zones

│     ├── Programs

│     ├── Water Budget

│     └── Statistics

│

├── Weather

│     ├── Current

│     ├── Forecast

│     ├── ET

│     └── Rain Delay

│

├── Hydraulics

│     ├── Flow

│     ├── Pressure

│     ├── Learning

│     └── Health

│

├── Diagnostics

│     ├── Controller

│     ├── CPU

│     ├── Memory

│     ├── Network

│     └── Self-Test

│

├── Settings

│     ├── Controller

│     ├── Display

│     ├── Network

│     ├── MQTT

│     ├── Security

│     └── Firmware

│

└── Service

      ├── Calibration

      ├── Logs

      ├── Backup

      └── Factory Tools
```

---

# 3.4 Main Navigation

The primary navigation bar shall remain permanently visible.

Recommended layout:

```text id="g5hijw"
+------------------------------------------------+

 Dashboard  Irrigation Weather Hydraulics Settings

+------------------------------------------------+
```

The active screen shall be visually highlighted.

---

# 3.5 Header Bar

The header is visible on every screen.

Contents:

* Controller Name
* Current Time
* Wi-Fi Status
* MQTT Status
* Alarm Indicator
* Active User

Example:

```text id="wxmz7b"
Garden Controller

20:15

Wi-Fi ●

MQTT ●

Alarm ○
```

---

# 3.6 Status Bar

The status bar provides continuously updated system information.

Displayed information:

* Active Zone
* Remaining Runtime
* Water Consumption
* Hydraulic Status
* Weather Alert
* Controller Mode

---

# 3.7 Dashboard Screen

The Dashboard provides an operational overview.

Widgets:

* Current Program
* Active Zone
* Weather Summary
* Flow
* Pressure
* Water Today
* Controller Status
* Active Alarm

The Dashboard shall be displayed after boot.

---

# 3.8 Irrigation Section

Subscreens:

* Zone Overview
* Program Schedule
* Manual Irrigation
* Runtime History
* Water Statistics

Primary actions:

* Start Zone
* Stop Zone
* Pause Program
* Resume Program

---

# 3.9 Weather Section

Displays:

* Current Conditions
* Forecast
* Rain Probability
* Wind Speed
* Temperature
* Humidity
* Solar Radiation
* ET
* Irrigation Recommendation

Weather updates occur automatically.

---

# 3.10 Hydraulic Section

Displays:

* Current Flow
* Current Pressure
* Hydraulic Health
* Learned Values
* Leak Probability
* Restriction Probability

The Hydraulic screen is intended for both users and service technicians.

---

# 3.11 Diagnostics Section

Displays:

* CPU Load
* Heap Memory
* Storage
* Wi-Fi Signal
* MQTT Status
* Uptime
* Task Statistics
* Controller Health

Advanced diagnostic data may be hidden by default.

---

# 3.12 Settings Section

Configuration categories:

* General
* Display
* Language
* Date & Time
* Network
* MQTT
* Security
* Firmware
* Backup

Each settings page follows a common layout.

---

# 3.13 Service Section

Restricted to authorized users.

Contains:

* Flow Calibration
* Pressure Calibration
* Valve Test
* Self-Test
* Factory Diagnostics
* Firmware Information
* Event Logs

Service Mode shall display a persistent indicator.

---

# 3.14 Modal Dialogs

Dialogs interrupt the current workflow.

Examples:

* Confirmation
* Warning
* Alarm
* Configuration
* Input
* Progress

Dialogs shall be centered.

Background interaction shall be disabled.

---

# 3.15 Notification System

Notification priorities:

| Priority    | Behavior       |
| ----------- | -------------- |
| Information | Auto dismiss   |
| Success     | Auto dismiss   |
| Warning     | Timeout + icon |
| Alarm       | Persistent     |
| Emergency   | Full-screen    |

Emergency notifications override every screen.

---

# 3.16 Navigation History

The Screen Manager maintains navigation history.

Example:

```text id="6wjdko"
Dashboard

↓

Hydraulics

↓

Pressure

↓

Calibration

↓

Back

↓

Pressure

↓

Hydraulics
```

The Back action shall restore the previous screen state.

---

# 3.17 Gesture Support

Supported gestures:

| Gesture     | Action        |
| ----------- | ------------- |
| Tap         | Select        |
| Double Tap  | Reserved      |
| Swipe Left  | Next Page     |
| Swipe Right | Previous Page |
| Swipe Down  | Notifications |
| Long Press  | Context Menu  |

Gestures shall never replace critical controls.

---

# 3.18 Context Menus

Context menus provide secondary actions.

Examples:

Zone:

* Start
* Stop
* Rename
* Statistics

Program:

* Edit
* Duplicate
* Disable

Weather:

* Refresh
* Details

---

# 3.19 Alarm Navigation

When a critical alarm occurs:

```text id="fg29u8"
Alarm Raised

↓

Alarm Dialog

↓

Alarm Details

↓

Recommended Action

↓

Acknowledge
```

The user may postpone acknowledgement only for non-critical alarms.

---

# 3.20 User Experience Targets

| Operation        | Maximum Time |
| ---------------- | -----------: |
| Boot → Dashboard |          5 s |
| Screen Change    |       150 ms |
| Dialog Open      |       100 ms |
| Alarm Popup      |       250 ms |
| Touch Feedback   |        50 ms |

---

# 3.21 Screen Naming Convention

Internal screen identifiers:

```text id="1h7knb"
SCR_DASHBOARD

SCR_IRRIGATION

SCR_WEATHER

SCR_HYDRAULICS

SCR_SETTINGS

SCR_DIAGNOSTICS

SCR_SERVICE

SCR_OTA
```

Naming shall remain consistent across firmware releases.

---

# 3.22 Future Navigation Extensions

The navigation framework supports future additions including:

* Landscape/Portrait switching
* Multi-display operation
* External HDMI displays
* Remote HMI panels
* Tablet interface
* Voice navigation
* AI assistant integration

These additions shall preserve the established navigation hierarchy.

---

# 3.23 Engineering Notes

The navigation architecture has been designed around the operational workflow of irrigation rather than the underlying firmware structure. Users interact with concepts such as zones, weather and water usage instead of internal software modules, reducing cognitive load and simplifying operation.

The hierarchical yet shallow navigation model ensures rapid access to all major functions while remaining scalable as future features are introduced. Consistent placement of navigation controls, status indicators and dialogs also minimizes user training requirements and supports predictable operation in both residential and professional environments.

---

# 3.24 Chapter Summary

This chapter defines the complete screen hierarchy and navigation model for the Zmartify Human–Machine Interface.

The architecture provides a clear, scalable and user-centered navigation framework that supports daily operation, advanced configuration and professional service activities while maintaining consistent interaction patterns throughout the interface.

---

# End of Chapter 3

**Next Chapter**

**Chapter 4 – Dashboard Design, Status Panels & Real-Time Information Layout**

# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 18 – User Interface (LVGL) Architecture

---

# 18 User Interface (LVGL) Architecture

---

# 18.1 Purpose

The User Interface (UI) is the primary human-machine interface (HMI) of the Zmartify Irrigation Controller.

It shall provide an intuitive, modern and responsive interface that allows both inexperienced homeowners and professional installers to operate, configure and diagnose the irrigation system.

Unlike traditional irrigation controllers that require navigating complex text menus, the Zmartify UI is designed around graphical workflows, touch interaction and contextual information.

The UI shall expose the full functionality of the controller while keeping routine operation simple.

---

# 18.2 Design Philosophy

The UI is designed around five principles.

### UI-001 — Simplicity

Everyday irrigation tasks shall require no more than three touches.

---

### UI-002 — Information Hierarchy

Frequently used information shall always be visible.

Less frequently used configuration shall remain accessible without cluttering the interface.

---

### UI-003 — Touch First

The interface shall be optimized for capacitive touch.

Future support for keyboard, mouse or remote display shall not affect the UI architecture.

---

### UI-004 — Event Driven

The display shall never poll firmware modules.

All screen updates shall be triggered by Event Bus notifications.

---

### UI-005 — Safety

The UI shall never directly operate hardware.

All user actions shall be routed through:

```text
Touch

↓

UI Manager

↓

Event Bus

↓

Application Manager

↓

Relay Manager
```

---

# 18.3 Display Hardware

Reference Hardware

| Component        | Value            |
| ---------------- | ---------------- |
| Display          | Waveshare 7" IPS |
| Resolution       | 1024 × 600       |
| Touch            | Capacitive       |
| Controller       | ESP32-S3         |
| Graphics Library | LVGL 9.x         |
| Color Depth      | 16-bit RGB565    |

The UI shall automatically adapt to future display sizes.

---

# 18.4 Display Lifecycle

```text
Boot

↓

Splash Screen

↓

Hardware Initialization

↓

Dashboard

↓

User Interaction

↓

Idle

↓

Screen Sleep

↓

Touch

↓

Wake Up
```

---

# 18.5 Automatic Display Sleep

To reduce power consumption and extend display lifetime:

Default timeout

**10 minutes**

User configurable:

* Disabled
* 1 min
* 2 min
* 5 min
* 10 min
* 15 min
* 30 min

During sleep:

* LCD backlight OFF
* Touch interrupt active
* Controller continues operating normally

Wake-up sources:

* Touch
* Hardware button
* Alarm
* MQTT notification (optional)
* Active irrigation (optional user setting)

---

# 18.6 Screen Hierarchy

```text
Dashboard

├── Irrigation

├── Zones

├── Programs

├── Weather

├── Water Usage

├── Diagnostics

├── Alarms

├── Configuration

├── Maintenance

└── About
```

All navigation shall require a maximum of three levels.

---

# 18.7 Dashboard

The Dashboard is the controller's home screen.

Displayed information:

Current Status

* Idle
* Irrigating
* Rain Delay
* Alarm
* Manual Mode

Weather

* Temperature
* Humidity
* Wind
* Rain
* ET

Water

* Current Flow
* Current Pressure
* Today's Water Consumption

Quick Actions

* Start Irrigation
* Stop Irrigation
* Rain Delay
* Manual Zone
* Alarms

---

# 18.8 Irrigation Screen

Displays:

Current Program

Current Zone

Remaining Runtime

Flow

Pressure

Water Used

Progress Indicator

Buttons:

* Pause
* Stop
* Skip Zone
* Next Zone
* Extend Runtime

---

# 18.9 Zone Screen

Each irrigation zone displays:

* Name
* Icon
* Relay
* Status
* Runtime
* Learned Flow
* Learned Pressure
* Water Budget
* ET Adjustment
* Soil Type
* Plant Type

Actions:

* Manual Start
* Manual Stop
* Edit
* Flow Learn
* Pressure Learn
* Disable

---

# 18.10 Program Screen

Displays:

* Program Name
* Schedule
* Zones
* Runtime
* Seasonal Adjustment
* ET Adjustment
* Cycle & Soak
* Last Execution
* Next Execution

Editing shall be performed using guided wizard pages.

---

# 18.11 Weather Screen

Displays:

Current

* Temperature
* Humidity
* Wind
* Rain
* UV
* Solar Radiation

Forecast

* 24 Hour
* 3 Day
* 7 Day

Controller Decision

* Water Normally
* Reduce
* Delay
* Skip

Historical charts:

* Rainfall
* Temperature
* ET

---

# 18.12 Hydraulic Screen

One of the unique Zmartify features.

Displays:

Flow

Pressure

Expected Flow

Measured Flow

Baseline Pressure

Hydraulic Health

Leak Status

Graphs:

* Flow
* Pressure

Correlation view:

```text
Pressure

──────────────

Flow

──────────────
```

This allows rapid diagnosis of irrigation problems.

---

# 18.13 Water Usage Screen

Displays:

Today

This Week

This Month

This Season

Lifetime

Per Zone

Graphs:

* Daily Water
* Monthly Water
* Historical Consumption

The user may select:

* Litres
* Cubic metres
* Gallons

---

# 18.14 Alarm Screen

Shows:

Active Alarms

Severity

Time

Duration

Acknowledgement

Recommended Action

History

Search

Filter

Alarm colours:

Blue

Information

Yellow

Warning

Orange

Critical

Red

Emergency

---

# 18.15 Diagnostics Screen

Displays:

CPU

Memory

Storage

Wi-Fi

MQTT

Task List

Health Score

Hardware Status

Sensor Status

I²C Devices

Engineering mode exposes additional information.

---

# 18.16 Configuration Screen

Organized into sections:

System

Network

Weather

Zones

Programs

Hydraulics

Display

Security

Maintenance

Configuration changes shall be validated before being committed.

---

# 18.17 Service Mode

Installer mode provides:

Relay Test

Flow Test

Pressure Test

Display Test

Touch Calibration

I²C Scanner

Firmware Update

Factory Reset

Configuration Backup

Access requires administrator authentication.

---

# 18.18 UI Themes

Supported themes:

Light

Dark

Automatic

Future:

High Contrast

Installer

Outdoor Daylight

Theme changes shall not require restart.

---

# 18.19 Icons

Icons shall follow a consistent design language.

Examples:

* Irrigation
* Lawn
* Trees
* Flowers
* Drip
* Rain
* Wind
* Alarm
* Wi-Fi
* MQTT
* Flow
* Pressure
* Settings

Icons shall remain recognizable at all supported resolutions.

---

# 18.20 Notifications

Notification types:

Toast

Information

Banner

Warning

Popup

Critical

Full Screen

Emergency

Notifications shall never obstruct critical safety actions.

---

# 18.21 User Roles

Supported roles:

Guest

View Only

Operator

Manual Irrigation

Administrator

Full Configuration

Installer

Engineering Tools

Future firmware may support user authentication via MQTT or cloud identity providers.

---

# 18.22 Accessibility

Supported features:

Large Fonts

High Contrast

Colour-blind Friendly Indicators

Long Press Confirmation

Touch Calibration

Audible feedback (future)

Screen brightness shall automatically dim during idle operation.

---

# 18.23 LVGL Architecture

The UI shall be divided into reusable modules.

```text
UI Manager

├── Dashboard

├── Weather

├── Zones

├── Programs

├── Irrigation

├── Diagnostics

├── Alarms

├── Configuration

├── Service

└── Widgets
```

Every screen shall be implemented as an independent LVGL component.

---

# 18.24 Event Integration

The UI subscribes to:

```text
EVT_ZONE_*

EVT_FLOW_*

EVT_PRESS_*

EVT_ALARM_*

EVT_SYSTEM_*

EVT_WEATHER_*

EVT_DIAG_*

EVT_MQTT_*
```

The UI publishes:

```text
EVT_UI_BUTTON

EVT_UI_TOUCH

EVT_UI_CONFIG

EVT_UI_SLEEP

EVT_UI_WAKE
```

---

# 18.25 Performance Targets

| Function          |  Target |
| ----------------- | ------: |
| Touch Response    |  <50 ms |
| Screen Change     | <100 ms |
| Dashboard Refresh | <100 ms |
| Graph Update      | <250 ms |
| Alarm Popup       | <100 ms |

Animation frame rate target:

**60 FPS**

---

# 18.26 Unit Testing

Automated tests shall verify:

* Navigation
* Widget rendering
* Theme switching
* Display sleep
* Wake-up
* Alarm presentation
* Graph updates
* Configuration dialogs
* Touch calibration
* Event handling

Minimum code coverage:

**85%**

(UI testing is typically more difficult to automate than logic modules.)

---

# 18.27 Future Enhancements

The UI architecture supports:

* Garden map visualization
* Interactive irrigation layout
* Animated hydraulic diagrams
* AI irrigation advisor
* Voice control
* Multi-language support
* Remote HMI
* Tablet and web interface
* Mobile companion application

These additions can be implemented without changing the underlying application architecture.

---

# 18.28 Engineering Notes

The User Interface is intended to differentiate the Zmartify Irrigation Controller from conventional irrigation controllers.

Rather than presenting configuration as lists of parameters, the UI emphasizes visualization of hydraulic performance, weather conditions and irrigation efficiency. By integrating Flow, Pressure, ET and Weather information into intuitive dashboards, users gain a much deeper understanding of system operation.

The chosen 7-inch capacitive display provides sufficient resolution to present engineering-grade diagnostics while remaining simple enough for everyday operation. The modular LVGL architecture also allows future migration to web and mobile interfaces with minimal changes to business logic.

---

# 18.29 Chapter Summary

The User Interface provides a modern, touch-optimized and event-driven human-machine interface for the Zmartify Irrigation Controller.

Its architecture combines intuitive navigation, comprehensive diagnostics, rich graphical visualization and robust separation from application logic, ensuring that users can confidently operate and maintain complex irrigation systems with minimal effort.

The UI is a key element of the overall Zmartify experience and has been designed to support future expansion into web, mobile and cloud-based interfaces while maintaining a consistent user experience.

---

# End of Chapter 18

**Next Chapter**

**Chapter 19 – Security Architecture**

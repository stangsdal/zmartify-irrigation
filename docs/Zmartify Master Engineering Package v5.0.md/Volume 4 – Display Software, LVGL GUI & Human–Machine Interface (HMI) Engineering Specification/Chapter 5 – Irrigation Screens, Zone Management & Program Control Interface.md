# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 5

# Irrigation Screens, Zone Management & Program Control Interface

---

# 5.1 Purpose

This chapter defines the graphical user interface used for irrigation management.

The Irrigation Interface is the operational core of the Human–Machine Interface (HMI), allowing users to monitor, configure and manually control irrigation activities while preserving the controller's autonomous scheduling capabilities.

The interface shall support:

* Manual irrigation
* Program monitoring
* Zone management
* Runtime visualization
* Water budget adjustment
* Operational history

---

# 5.2 Design Objectives

The Irrigation Interface shall:

* Provide rapid access to irrigation controls
* Display real-time operational data
* Prevent accidental activation
* Minimize user interaction
* Support service diagnostics
* Scale to future controller models
* Remain consistent with the overall HMI architecture

---

# 5.3 Irrigation Screen Hierarchy

```text
Irrigation

│

├── Zone Overview

├── Zone Details

├── Manual Irrigation

├── Program Overview

├── Program Details

├── Schedule Calendar

├── Water Budget

├── Runtime History

└── Irrigation Statistics
```

Each screen follows the lifecycle defined in Chapter 2.

---

# 5.4 Zone Overview Screen

The Zone Overview screen presents all configured irrigation zones.

Each zone is represented by a standardized card displaying:

* Zone Number
* Zone Name
* Current Status
* Runtime
* Flow
* Pressure
* Water Consumption

Example:

```text
──────────────────────────────────────

Zone 1

Front Lawn

Idle

──────────────────────────────────────

Zone 2

Garden Beds

Running

08:12 Remaining

──────────────────────────────────────
```

Running zones shall always appear first.

---

# 5.5 Zone Status Indicators

Each zone shall display one of the following operational states.

| State     | Description                   |
| --------- | ----------------------------- |
| Disabled  | Zone unavailable              |
| Idle      | Ready                         |
| Waiting   | Queued                        |
| Running   | Active irrigation             |
| Paused    | Program paused                |
| Completed | Finished                      |
| Alarm     | Hydraulic or electrical fault |

State transitions shall be animated.

---

# 5.6 Zone Detail Screen

Selecting a zone opens the detailed engineering view.

Displayed information includes:

* Zone Number
* Zone Name
* Valve Status
* Runtime
* Remaining Time
* Flow
* Pressure
* Water Used
* Water Budget
* Last Irrigation
* Historical Average

Quick actions:

* Start
* Stop
* Enable
* Disable
* Rename

---

# 5.7 Manual Irrigation Screen

Manual irrigation shall require explicit confirmation.

Workflow:

```text
Select Zone

↓

Select Runtime

↓

Review

↓

Confirm

↓

Start Irrigation
```

Confirmation reduces accidental activation.

---

# 5.8 Runtime Selection

Runtime shall be configurable using:

* Slider
* Increment buttons
* Numeric keypad

Range:

```text
1 minute

↓

12 hours
```

Runtime limits are enforced by the Application Manager.

---

# 5.9 Manual Irrigation Display

While manual irrigation is active, the screen displays:

* Zone
* Runtime
* Remaining Time
* Flow
* Pressure
* Water Used
* Estimated Water Remaining

A large **STOP** button shall always remain visible.

---

# 5.10 Program Overview

Programs are displayed as engineering cards.

Each card contains:

* Program Name
* Enabled
* Next Start
* Runtime
* Last Execution
* Water Used

Example:

```text
Morning

Enabled

Next

06:00 Tomorrow

Runtime

42 min
```

---

# 5.11 Program Detail Screen

Displays:

* Program Schedule
* Assigned Zones
* Runtime per Zone
* Water Budget
* ET Adjustment
* Rain Delay Status
* Seasonal Adjustment

Available actions:

* Start
* Pause
* Disable
* Duplicate
* Edit

---

# 5.12 Program Execution Screen

When a program is running, the interface displays:

```text
Morning Irrigation

Zone 4

██████████░░░░

18 Minutes Remaining

Flow

24.3 L/min

Pressure

3.47 bar
```

Progress shall update once per second.

---

# 5.13 Water Budget Screen

Displays irrigation adjustment factors.

Information includes:

* Base Runtime
* Weather Adjustment
* ET Adjustment
* Seasonal Factor
* Final Runtime

Example:

```text
Base Runtime

20 min

ET Adjustment

110 %

Season

95 %

Final Runtime

21 min
```

---

# 5.14 Schedule Calendar

Calendar view:

```text
Monday

06:00

Morning

18:00

Garden

Wednesday

06:00

Morning
```

Users may switch between:

* Daily
* Weekly
* Monthly views

---

# 5.15 Runtime History

Historical irrigation records include:

* Date
* Program
* Zone
* Runtime
* Water Used
* Flow Average
* Pressure Average

Sorting options:

* Date
* Zone
* Program
* Water Consumption

---

# 5.16 Irrigation Statistics

Statistics include:

* Daily Runtime
* Weekly Runtime
* Monthly Runtime
* Water Usage
* Program Frequency
* Zone Utilization

Charts may be presented using LVGL Chart widgets.

---

# 5.17 Zone Card Layout

Standard zone card:

```text
+--------------------------------+

Zone 04

Front Lawn

Running

Flow

24.3 L/min

Pressure

3.48 bar

Remaining

08:15

+--------------------------------+
```

Card dimensions shall remain consistent across the application.

---

# 5.18 Zone Actions

Available actions depend on current state.

| State    | Actions      |
| -------- | ------------ |
| Idle     | Start        |
| Running  | Stop, Pause  |
| Paused   | Resume, Stop |
| Disabled | Enable       |
| Alarm    | View Alarm   |

Invalid actions shall be hidden or disabled.

---

# 5.19 Confirmation Dialogs

The following operations require confirmation:

* Stop Program
* Manual Irrigation
* Disable Zone
* Delete Program
* Reset Statistics

Example:

```text
Stop Irrigation?

Current Zone:

Front Lawn

Remaining:

08:15

[Cancel]

[Stop]
```

---

# 5.20 Live Engineering Data

The Irrigation Interface continuously displays:

| Parameter      | Refresh |
| -------------- | ------: |
| Runtime        |     1 s |
| Remaining      |     1 s |
| Flow           |     1 s |
| Pressure       |     1 s |
| Water Used     |     5 s |
| Program Status |   Event |

Only modified widgets shall be redrawn.

---

# 5.21 Error Presentation

Typical irrigation errors:

* No Water Supply
* Low Pressure
* High Flow
* Valve Failure
* Rain Delay Active
* Zone Disabled

Errors shall be presented with:

* Icon
* Color
* Description
* Recommended action

---

# 5.22 Accessibility

Minimum button size:

```text
48 × 48 px
```

Primary actions:

* Green

Dangerous actions:

* Red

Disabled actions:

* Gray

Touch targets shall remain usable with gloves.

---

# 5.23 Performance Targets

| Operation        |  Target |
| ---------------- | ------: |
| Open Zone        | <150 ms |
| Start Irrigation | <250 ms |
| Stop Irrigation  | <250 ms |
| Screen Refresh   |  60 FPS |
| Zone Update      |    1 Hz |

---

# 5.24 Future Extensions

The Irrigation Interface has been designed to support future capabilities including:

* Interactive irrigation maps
* Zone grouping
* Multi-controller irrigation management
* Pump station visualization
* AI runtime recommendations
* Satellite imagery overlays
* Water cost analysis
* Predictive irrigation optimization

These enhancements shall preserve the established navigation model and widget framework.

---

# 5.25 Engineering Notes

The Irrigation Interface represents the operational heart of the Zmartify HMI. Unlike many irrigation controllers that focus primarily on scheduling, the interface combines operational control with real-time hydraulic telemetry, allowing users and service technicians to understand not only **what** the controller is doing, but also **how well** the irrigation system is performing.

The modular card-based layout also supports future expansion without redesigning the navigation architecture, ensuring compatibility with larger controllers and additional ecosystem products.

---

# 5.26 Chapter Summary

This chapter defines the complete Irrigation Interface, including zone management, program control, manual irrigation, runtime visualization and historical statistics.

By combining intuitive controls with engineering-grade telemetry, the interface provides a professional yet approachable operational experience suitable for homeowners, installers and service technicians alike.

---

# End of Chapter 5

**Next Chapter**

**Chapter 6 – Weather Dashboard, ET Visualization & Environmental Information Screens**

# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 2

# LVGL Software Architecture, Display Manager & Screen Framework

---

# 2.1 Purpose

This chapter defines the software architecture of the Zmartify graphical user interface.

The display software is implemented using **LVGL v9.x** and follows a modular architecture in which presentation, navigation and user interaction are separated from irrigation control logic.

This chapter specifies:

* Display Manager
* Screen Manager
* Widget Manager
* Event routing
* Screen lifecycle
* Memory management
* Rendering pipeline
* Thread safety

---

# 2.2 Design Objectives

The GUI architecture shall:

* Remain modular
* Support future display hardware
* Support dynamic themes
* Minimize RAM usage
* Prevent memory fragmentation
* Support asynchronous updates
* Provide deterministic behavior
* Remain independent of irrigation logic

---

# 2.3 Software Architecture

The GUI subsystem consists of the following major components.

```text
                     Application Managers

                             │

                             ▼

                    Display Manager

      ┌─────────────┼─────────────┐

      ▼             ▼             ▼

 Screen Manager  Widget Manager  Theme Manager

      ▼             ▼             ▼

         Notification Manager

                 ▼

          Animation Manager

                 ▼

             LVGL Core

                 ▼

          ESP-IDF Display Driver

                 ▼

          RGB TFT Display
```

---

# 2.4 Display Manager Responsibilities

The Display Manager is the central coordinator.

Responsibilities include:

* Initialize LVGL
* Initialize display driver
* Initialize touch driver
* Create screen hierarchy
* Process UI events
* Schedule screen updates
* Coordinate themes
* Coordinate animations
* Maintain display state

The Display Manager shall not perform irrigation control.

---

# 2.5 Screen Manager Responsibilities

The Screen Manager controls navigation.

Responsibilities:

* Create screens
* Destroy screens
* Switch screens
* Maintain navigation history
* Manage dialogs
* Handle modal windows
* Restore previous screens
* Support future multi-display operation

---

# 2.6 Widget Manager

The Widget Manager owns reusable UI components.

Examples:

* Buttons
* Labels
* Cards
* Status indicators
* Charts
* Tables
* Lists
* Dialogs
* Progress indicators
* Gauges

Widgets shall never access firmware directly.

---

# 2.7 Theme Manager

The Theme Manager controls appearance.

Supported themes:

* Light
* Dark
* High Contrast
* Service Mode

Future themes:

* Outdoor Sunlight
* Accessibility
* Corporate Branding

Changing themes shall not recreate screens.

---

# 2.8 Notification Manager

The Notification Manager handles transient user information.

Notification types:

| Type           |                      Duration |
| -------------- | ----------------------------: |
| Information    |                           3 s |
| Success        |                           2 s |
| Warning        | Until acknowledged or timeout |
| Critical Alarm |                    Persistent |

Notifications shall appear independently of the active screen.

---

# 2.9 Animation Manager

Animations improve usability without delaying operation.

Supported animation types:

* Fade
* Slide
* Scale
* Progress
* Expand
* Collapse
* Loading spinner
* Alarm pulse

Animation duration:

| Animation |     Target |
| --------- | ---------: |
| Fade      |     150 ms |
| Slide     |     200 ms |
| Dialog    |     180 ms |
| Alarm     | Continuous |

Animations shall never delay critical alarm presentation.

---

# 2.10 Screen Object Model

Each screen derives from a common base object.

```text
BaseScreen

│

├── DashboardScreen

├── IrrigationScreen

├── WeatherScreen

├── HydraulicScreen

├── ProgramScreen

├── SettingsScreen

├── DiagnosticsScreen

├── ServiceScreen

└── OTAUpdateScreen
```

Every screen implements the same lifecycle interface.

---

# 2.11 Screen Lifecycle

Every screen shall implement:

```text
Create()

↓

Initialize()

↓

BindData()

↓

Show()

↓

Update()

↓

Hide()

↓

Destroy()
```

Only one primary screen shall be visible at a time.

---

# 2.12 Screen States

Possible screen states:

| State       | Description        |
| ----------- | ------------------ |
| Created     | Allocated          |
| Initialized | Widgets created    |
| Hidden      | Off-screen         |
| Active      | Visible            |
| Updating    | Receiving data     |
| Destroyed   | Resources released |

State transitions shall be deterministic.

---

# 2.13 Event Architecture

Events originate from three sources:

```text
Touch

↓

Display Event Queue

↓

Screen Manager

↓

Widget Callback

↓

Application Manager

↓

Display Update
```

or

```text
Firmware Event

↓

Display Manager

↓

Widget Update
```

or

```text
MQTT Event

↓

Application Manager

↓

Display Manager
```

All event sources converge at the Display Manager.

---

# 2.14 Touch Processing

Touch events follow this sequence.

```text
GT911

↓

Touch Driver

↓

LVGL Input Device

↓

Widget Hit Test

↓

Widget Callback

↓

Display Manager

↓

Application Manager
```

Touch processing shall be asynchronous.

---

# 2.15 Rendering Pipeline

```text
Application Data

↓

Display Manager

↓

Widget Update

↓

LVGL Draw Engine

↓

Display Buffer

↓

RGB LCD DMA

↓

Display
```

Double buffering is recommended.

---

# 2.16 Memory Strategy

The GUI uses three memory classes.

### Static Memory

* Fonts
* Icons
* Themes

---

### Dynamic Memory

* Dialogs
* Temporary lists
* Notifications

---

### Cached Memory

* Charts
* Weather icons
* Frequently used screens

Heap fragmentation shall be minimized.

---

# 2.17 Screen Cache

Frequently accessed screens may remain allocated.

Recommended cache:

| Screen      |  Cached  |
| ----------- | :------: |
| Dashboard   |     ✔    |
| Irrigation  |     ✔    |
| Weather     |     ✔    |
| Programs    |     ✔    |
| Settings    |     ✔    |
| Diagnostics | Optional |

Rarely used service screens may be destroyed when hidden.

---

# 2.18 Thread Safety

LVGL shall execute in a dedicated GUI task.

Only the Display Manager may modify LVGL objects.

Other firmware tasks shall communicate through:

* Event queues
* Message queues
* Notifications

Direct cross-task widget access is prohibited.

---

# 2.19 Update Strategy

Widgets shall update only when data changes.

Avoid:

* Continuous redraws
* Full screen refreshes
* Unnecessary object recreation

Recommended update frequencies:

| Data     | Interval |
| -------- | -------: |
| Clock    |      1 s |
| Flow     |      1 s |
| Pressure |      1 s |
| Weather  |    5 min |
| CPU      |     60 s |
| Memory   |     60 s |

---

# 2.20 Error Handling

Display failures shall never stop controller operation.

If a screen update fails:

1. Log error
2. Retry update
3. Display fallback information
4. Continue irrigation

Display recovery shall occur automatically where possible.

---

# 2.21 Logging

The Display Manager shall generate diagnostic logs for:

* Screen transitions
* Theme changes
* Memory allocation failures
* Rendering errors
* Touch calibration
* Widget creation failures

Logging verbosity shall be configurable.

---

# 2.22 Engineering Notes

The Display Manager acts as the sole gateway between the firmware and the graphical interface. This architectural boundary ensures that application managers remain completely unaware of LVGL implementation details while allowing the GUI to evolve independently.

By using a centralized event queue, screen caching and strict ownership of LVGL objects, the design minimizes race conditions, reduces memory fragmentation and provides predictable rendering performance on the ESP32-S3 platform.

---

# 2.23 Chapter Summary

This chapter defines the software architecture underlying the Zmartify graphical user interface.

The modular Display Manager, Screen Manager and Widget Manager provide a scalable framework for building complex interfaces while maintaining deterministic behavior and clear separation between presentation and control logic.

Subsequent chapters will build upon this architecture by defining the complete screen hierarchy, reusable widget library and navigation system.

---

# End of Chapter 2

**Next Chapter**

**Chapter 3 – Complete Screen Hierarchy, Navigation Model & User Experience Design**

# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 1

# Display System Architecture & Human–Machine Interface Philosophy

---

# 1.1 Purpose

This volume specifies the complete Human–Machine Interface (HMI) architecture for the Zmartify Irrigation Controller.

The display subsystem is designed to provide a modern, responsive and intuitive user interface while maintaining strict separation between graphical presentation, application logic and hardware drivers.

The HMI shall support:

* Residential users
* Professional installers
* Service technicians
* Manufacturing diagnostics
* Future Zmartify ecosystem products

---

# 1.2 Scope

Volume 4 defines:

* LVGL software architecture
* Display Manager
* Screen framework
* Navigation model
* Widget library
* Themes
* Touch interface
* User interaction
* Animation framework
* Alarm presentation
* Configuration dialogs
* Service menus
* Accessibility
* Internationalization
* Performance requirements

---

# 1.3 Design Philosophy

The Zmartify HMI follows five core engineering principles.

### HMI-001

**Information First**

The interface shall always prioritize operational information over decoration.

---

### HMI-002

**One Function — One Screen**

Each screen shall have a clearly defined purpose.

---

### HMI-003

**Maximum Three Touches**

Any frequently used operation shall require no more than three user interactions.

---

### HMI-004

**Always Recoverable**

The user shall never become trapped inside menus.

Every screen shall provide a predictable navigation path.

---

### HMI-005

**Controller First**

Display failure shall never affect irrigation operation.

The controller remains fully autonomous.

---

# 1.4 System Overview

The Display System consists of five primary layers.

```text
+--------------------------------------------------+

              Human–Machine Interface

+--------------------------------------------------+

             Screen Manager (LVGL)

+--------------------------------------------------+

          Display Manager / UI Services

+--------------------------------------------------+

        Application Managers (Firmware)

+--------------------------------------------------+

     Hardware Drivers (RGB LCD + GT911 Touch)

+--------------------------------------------------+
```

Only the Display Manager communicates with LVGL.

---

# 1.5 Hardware Platform

Reference display hardware:

| Component        | Specification    |
| ---------------- | ---------------- |
| Display          | 7" IPS TFT       |
| Resolution       | 800 × 480        |
| Interface        | RGB888           |
| Touch            | GT911 Capacitive |
| Controller       | ESP32-S3         |
| Graphics Library | LVGL v9.x        |

The display subsystem shall remain portable to future display sizes.

---

# 1.6 Display Responsibilities

The Display Manager is responsible for:

* Screen creation
* Screen destruction
* Theme management
* Language updates
* Animation scheduling
* Event routing
* Widget updates
* Touch processing
* Brightness control
* Sleep management

The Display Manager shall not implement irrigation logic.

---

# 1.7 Separation of Responsibilities

The display software follows strict MVC (Model–View–Controller) principles.

```text
Application Managers

        │

        ▼

Display Manager

        │

        ▼

 LVGL Screen Objects

        │

        ▼

 RGB Display Driver
```

Business logic shall never exist inside LVGL widgets.

---

# 1.8 Display Manager Architecture

```text
Display Manager

│

├── Theme Manager

├── Screen Manager

├── Widget Manager

├── Animation Manager

├── Touch Manager

├── Dialog Manager

├── Notification Manager

├── Language Manager

└── Brightness Manager
```

Each manager shall have a single responsibility.

---

# 1.9 LVGL Object Hierarchy

```text
Display

└── Root Screen

      ├── Header

      ├── Navigation Bar

      ├── Content Area

      ├── Floating Notifications

      ├── Dialog Layer

      └── Status Bar
```

This hierarchy shall remain consistent across all screens.

---

# 1.10 Display Resolution Strategy

The reference layout targets:

```text
800 × 480 pixels
```

The UI shall support future resolutions through scalable layouts.

Design baseline:

| Resolution | Support |
| ---------- | ------- |
| 800×480    | Native  |
| 1024×600   | Planned |
| 1280×720   | Planned |

Absolute pixel positioning shall be minimized.

---

# 1.11 Screen Categories

The HMI is divided into functional groups.

| Category      | Purpose             |
| ------------- | ------------------- |
| Dashboard     | Daily operation     |
| Irrigation    | Zone control        |
| Weather       | Environmental data  |
| Hydraulics    | Flow & pressure     |
| Programs      | Scheduling          |
| Configuration | Settings            |
| Diagnostics   | Engineering         |
| Service       | Installer functions |
| OTA           | Firmware updates    |

---

# 1.12 Navigation Philosophy

Navigation shall be shallow rather than deep.

Maximum menu depth:

```text
Dashboard

↓

Category

↓

Screen

↓

Dialog
```

Nested menu structures beyond four levels shall be avoided.

---

# 1.13 Screen Lifecycle

Every screen follows the same lifecycle.

```text
Construct

↓

Initialize

↓

Bind Data

↓

Become Visible

↓

Receive Events

↓

Update

↓

Hide

↓

Destroy
```

Screen objects should be reused where practical to reduce memory fragmentation.

---

# 1.14 Event Flow

```text
Touch Event

↓

Touch Manager

↓

Screen Manager

↓

Widget Callback

↓

Display Manager

↓

Application Manager

↓

Firmware Response

↓

Display Update
```

Display events shall remain asynchronous.

---

# 1.15 Display Refresh Strategy

Target refresh rate:

```text
60 FPS
```

Minimum acceptable:

```text
30 FPS
```

Animations shall remain fluid even during active irrigation.

---

# 1.16 Color Philosophy

Colors communicate system status.

| Color  | Meaning             |
| ------ | ------------------- |
| Blue   | Information         |
| Green  | Normal operation    |
| Yellow | Warning             |
| Orange | Attention           |
| Red    | Critical alarm      |
| Gray   | Disabled            |
| White  | Neutral information |

Colors shall never be the sole indicator of system state.

Icons and text shall reinforce status.

---

# 1.17 Typography

Primary font:

* LVGL Montserrat

Recommended sizes:

| Usage   |  Size |
| ------- | ----: |
| Header  | 28 px |
| Section | 22 px |
| Body    | 18 px |
| Status  | 16 px |
| Caption | 14 px |

Typography shall prioritize readability over density.

---

# 1.18 Iconography

Icons shall follow a consistent visual language.

Examples include:

* Irrigation
* Valve
* Weather
* Flow
* Pressure
* Alarm
* Wi-Fi
* MQTT
* Settings
* Diagnostics

Icons shall remain recognizable at all supported resolutions.

---

# 1.19 Accessibility

The HMI shall support:

* High-contrast themes
* Large touch targets
* Adjustable brightness
* Day/Night themes
* Clear typography
* Color-independent status indicators

Minimum touch target:

```text
48 × 48 px
```

---

# 1.20 Performance Targets

| Metric            |  Target |
| ----------------- | ------: |
| Boot to UI        |    <5 s |
| Screen transition | <150 ms |
| Widget update     |  <50 ms |
| Alarm display     | <250 ms |
| Touch response    |  <50 ms |
| Animation         |  60 FPS |

---

# 1.21 Engineering Notes

The display subsystem is intentionally isolated from irrigation control logic. All user interactions are translated into high-level application requests processed by the Application Managers described in Volume 2. This architecture ensures that graphical updates, theme changes or future display hardware revisions cannot compromise the deterministic operation of the irrigation controller.

The use of LVGL also provides hardware independence, allowing the same HMI architecture to scale from embedded displays to future touchscreen panels and simulation environments with minimal modification.

---

# 1.22 Chapter Summary

This chapter establishes the architectural foundation for the Zmartify Human–Machine Interface.

By separating presentation from application logic, adopting a modular Display Manager architecture and defining clear navigation, performance and accessibility principles, the HMI provides a robust foundation for all subsequent chapters in this volume.

The following chapters build upon this foundation by specifying the complete screen framework, widget library, navigation system and user interaction model.

---

# End of Chapter 1

**Next Chapter**

**Chapter 2 – LVGL Software Architecture, Display Manager & Screen Framework**

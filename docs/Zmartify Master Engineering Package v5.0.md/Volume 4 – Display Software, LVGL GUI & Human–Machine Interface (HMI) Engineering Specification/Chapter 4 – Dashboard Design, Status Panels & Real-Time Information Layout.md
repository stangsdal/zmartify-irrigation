# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 4

# Dashboard Design, Status Panels & Real-Time Information Layout

---

# 4.1 Purpose

The Dashboard is the primary operational interface of the Zmartify Irrigation Controller.

It provides an immediate overview of controller status, irrigation activity, weather conditions and hydraulic performance without requiring navigation through menus.

The Dashboard shall enable users to determine the overall health and operational state of the irrigation system within a few seconds.

---

# 4.2 Design Objectives

The Dashboard shall:

* Present essential information at a glance
* Update in real time
* Minimize user interaction
* Highlight abnormal conditions
* Prioritize operational awareness
* Scale to future controller capabilities
* Remain readable in indoor and outdoor environments

---

# 4.3 Dashboard Philosophy

The Dashboard follows the **"Operational Cockpit"** design principle.

Rather than functioning as a traditional menu, it behaves as an engineering control panel where the most important operational information is continuously visible.

The Dashboard shall answer the following questions immediately:

* Is the controller operating correctly?
* Is irrigation currently active?
* Which zone is running?
* Is weather affecting irrigation?
* Is the hydraulic system healthy?
* Are there any alarms requiring attention?

---

# 4.4 Dashboard Layout

Reference layout:

```text id="dashboard-layout"
+--------------------------------------------------------------+
| Header                                                       |
+--------------------------------------------------------------+
| Controller Status | Weather Summary | Current Program         |
+--------------------------------------------------------------+
| Active Zone       | Flow            | Pressure                |
+--------------------------------------------------------------+
| Water Today       | ET              | Hydraulic Health        |
+--------------------------------------------------------------+
| Notifications / Active Alarms                               |
+--------------------------------------------------------------+
| Navigation Bar                                              |
+--------------------------------------------------------------+
```

The layout shall remain consistent throughout all firmware versions.

---

# 4.5 Header Area

The header is permanently visible.

Displayed information:

* Controller Name
* Current Time
* Date
* Wi-Fi Status
* MQTT Status
* User Profile
* Notification Indicator

Example:

```text id="dashboard-header"
Garden Controller

14 Jul 2026

20:14

Wi-Fi ●

MQTT ●

Admin
```

---

# 4.6 Controller Status Card

The Controller Status card summarizes the overall operating state.

Possible states:

| State       | Description               |
| ----------- | ------------------------- |
| Idle        | Ready                     |
| Irrigating  | Active program            |
| Manual      | Manual irrigation         |
| Paused      | Waiting                   |
| Maintenance | Service mode              |
| Alarm       | Fault present             |
| Offline     | Communication unavailable |

Color coding follows the standard defined in Chapter 1.

---

# 4.7 Current Program Card

Displays:

* Program Name
* Program State
* Start Time
* Remaining Runtime
* Progress Bar

Example:

```text id="program-card"
Morning Irrigation

Running

Remaining

18 min

███████░░░░
```

If no program is active, the card displays:

```text id="no-program"
No Active Program
```

---

# 4.8 Active Zone Card

Displays:

* Zone Number
* Zone Name
* Runtime
* Remaining Time
* Valve Status

Example:

```text id="zone-card"
Zone 4

Front Lawn

Running

08:24 Remaining
```

The card updates every second while irrigation is active.

---

# 4.9 Weather Summary Card

Displays:

* Temperature
* Humidity
* Rain Probability
* Wind Speed
* ET
* Irrigation Recommendation

Example:

```text id="weather-card"
22.8 °C

61 %

Rain: 15 %

ET: 3.8 mm

Recommendation

Normal Irrigation
```

Weather data shall refresh automatically according to the Weather Manager update interval.

---

# 4.10 Flow Card

Displays current measured flow.

Information:

* Current Flow
* Expected Flow
* Deviation
* Status

Example:

```text id="flow-card"
Flow

24.6 L/min

Expected

24.2 L/min

Status

Normal
```

Large deviations shall trigger visual highlighting.

---

# 4.11 Pressure Card

Displays:

* Current Pressure
* Expected Pressure
* Pressure Trend
* Hydraulic Status

Example:

```text id="pressure-card"
Pressure

3.48 bar

Expected

3.50 bar

Stable
```

Pressure trends may be displayed using a miniature sparkline chart.

---

# 4.12 Hydraulic Health Card

The Hydraulic Health indicator summarizes:

* Flow consistency
* Pressure stability
* Leak probability
* Restriction probability

Possible values:

| Status    | Meaning                   |
| --------- | ------------------------- |
| Excellent | Normal operation          |
| Good      | Minor deviations          |
| Attention | Investigation recommended |
| Warning   | Significant anomaly       |
| Critical  | Immediate action required |

---

# 4.13 Water Usage Card

Displays cumulative consumption.

Values:

* Today
* Week
* Month
* Season

Example:

```text id="water-card"
Today

1,284 L

Week

7,864 L
```

Historical trends are available on the Statistics screen.

---

# 4.14 Notification Panel

The Notification Panel displays transient operational messages.

Examples:

* Program Started
* Zone Completed
* Rain Delay Activated
* Backup Completed
* Firmware Updated

Notifications automatically expire unless user acknowledgement is required.

---

# 4.15 Alarm Banner

When an alarm is active, an alarm banner replaces the standard notification panel.

Displayed information:

* Alarm Severity
* Alarm Title
* Time
* Recommended Action

Example:

```text id="alarm-banner"
CRITICAL

Leak Detected

Zone 7

Irrigation Stopped
```

Critical alarms remain visible until acknowledged.

---

# 4.16 Real-Time Update Strategy

Dashboard widgets update independently.

Recommended update intervals:

| Widget      | Interval |
| ----------- | -------: |
| Clock       |      1 s |
| Active Zone |      1 s |
| Flow        |      1 s |
| Pressure    |      1 s |
| Weather     |    5 min |
| ET          |   30 min |
| Water Usage |      5 s |
| Diagnostics |     60 s |

Only changed values shall trigger redraws.

---

# 4.17 Widget Refresh Policy

Widgets shall implement differential updates.

Avoid:

* Complete screen redraw
* Widget recreation
* Unnecessary animations

Recommended sequence:

```text id="widget-refresh"
New Data

↓

Compare Previous Value

↓

Changed?

↓

Yes

↓

Update Widget

↓

Redraw Widget Only
```

---

# 4.18 Dashboard States

The Dashboard operates in one of five visual states.

| State      | Description        |
| ---------- | ------------------ |
| Normal     | Standard operation |
| Irrigation | Active watering    |
| Warning    | Minor issues       |
| Alarm      | Critical fault     |
| Service    | Maintenance mode   |

Visual transitions between states shall be animated.

---

# 4.19 Day and Night Modes

The Dashboard supports automatic theme switching.

Day Mode:

* Bright background
* High readability
* Outdoor optimized

Night Mode:

* Dark background
* Reduced brightness
* Lower eye strain

Automatic switching may be based on:

* Time
* Ambient light sensor (future)
* User preference

---

# 4.20 Touch Interaction

Dashboard interactions:

| Widget       | Action                 |
| ------------ | ---------------------- |
| Status Card  | Open Controller Screen |
| Program Card | Open Program Screen    |
| Zone Card    | Open Zone Details      |
| Weather Card | Open Weather Screen    |
| Flow Card    | Open Hydraulic Screen  |
| Alarm Banner | Open Alarm Details     |

Every card functions as a navigation shortcut.

---

# 4.21 Performance Requirements

| Metric           |  Target |
| ---------------- | ------: |
| Dashboard Load   | <300 ms |
| Widget Update    |  <50 ms |
| Alarm Display    | <250 ms |
| Flow Refresh     |    1 Hz |
| Pressure Refresh |    1 Hz |
| Animation        |  60 FPS |

The Dashboard shall remain responsive during simultaneous irrigation and MQTT activity.

---

# 4.22 Engineering Notes

The Dashboard is the operational center of the Zmartify HMI and is intentionally designed around engineering telemetry rather than decorative graphics. By organizing information into independent status cards with clearly defined responsibilities, the interface remains scalable, maintainable and responsive even as additional controller capabilities are introduced.

The use of differential widget updates minimizes CPU utilization and display bandwidth, allowing the ESP32-S3 to maintain smooth animations and rapid touch response while continuously monitoring irrigation, weather and hydraulic conditions.

---

# 4.23 Chapter Summary

This chapter defines the Dashboard architecture, real-time information layout and operational presentation strategy for the Zmartify Human–Machine Interface.

The Dashboard provides users with immediate situational awareness through modular status cards, real-time telemetry and prioritized alarm presentation. It establishes the visual foundation upon which all subsequent HMI screens are built and serves as the primary operational interface throughout the lifetime of the controller.

---

# End of Chapter 4

**Next Chapter**

**Chapter 5 – Irrigation Screens, Zone Management & Program Control Interface**

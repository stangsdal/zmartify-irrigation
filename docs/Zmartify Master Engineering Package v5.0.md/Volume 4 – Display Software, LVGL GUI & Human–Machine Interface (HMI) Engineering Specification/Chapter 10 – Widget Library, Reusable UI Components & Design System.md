# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 10

# Widget Library, Reusable UI Components & Design System

---

# 10.1 Purpose

This chapter defines the reusable widget library and visual design system used throughout the Zmartify Human–Machine Interface (HMI).

Rather than designing each screen independently, every user interface element is constructed from standardized widgets with consistent appearance, behavior and interaction patterns.

The Widget Library serves as the foundation for:

* Display consistency
* Reduced firmware complexity
* Easier maintenance
* Theme support
* Accessibility
* Future ecosystem products

---

# 10.2 Design Objectives

The Widget Library shall:

* Be modular
* Be reusable
* Support theming
* Minimize RAM consumption
* Support responsive layouts
* Maintain visual consistency
* Be independent of business logic
* Support future LVGL versions

---

# 10.3 Design Philosophy

The Zmartify Design System follows five principles.

### UI-001

Consistency First

Identical functions shall always appear identical.

---

### UI-002

Composition over Duplication

Screens shall be assembled from reusable widgets rather than custom layouts.

---

### UI-003

Single Responsibility

Each widget performs one clearly defined function.

---

### UI-004

Data Driven

Widgets display application data but never implement business logic.

---

### UI-005

Theme Independent

Widget behavior shall remain unchanged regardless of visual theme.

---

# 10.4 Widget Architecture

```text id="widget-architecture"
Application Data

        │

        ▼

Display Manager

        │

        ▼

Widget Manager

        │

        ▼

LVGL Widget

        │

        ▼

Display Driver
```

Widgets receive data exclusively through the Display Manager.

---

# 10.5 Widget Categories

The Widget Library consists of the following categories.

| Category     | Purpose                   |
| ------------ | ------------------------- |
| Layout       | Structure                 |
| Display      | Read-only information     |
| Input        | User interaction          |
| Navigation   | Screen movement           |
| Charts       | Engineering visualization |
| Status       | System state              |
| Dialog       | User interaction          |
| Notification | Messages                  |
| Service      | Engineering functions     |

---

# 10.6 Base Widget Class

All widgets inherit from a common base object.

```text id="base-widget"
BaseWidget

│

├── initialize()

├── bind()

├── update()

├── redraw()

├── enable()

├── disable()

├── destroy()
```

Derived widgets shall not bypass the Base Widget interface.

---

# 10.7 Layout Widgets

Layout widgets organize content.

Supported layouts:

* Vertical Container
* Horizontal Container
* Grid Layout
* Flex Layout
* Card Layout
* Split View
* Tab View

Layouts shall adapt automatically to future display resolutions.

---

# 10.8 Card Widget

The Card Widget is the primary information container.

Standard elements:

* Title
* Icon
* Value
* Subtitle
* Status Indicator
* Action Area

Example:

```text id="card-widget"
+--------------------------------+

Flow

24.3 L/min

Normal

+--------------------------------+
```

Cards shall maintain identical dimensions within the same layout.

---

# 10.9 Label Widget

Label types:

| Type    | Usage               |
| ------- | ------------------- |
| Header  | Screen titles       |
| Section | Group titles        |
| Body    | General information |
| Caption | Supporting text     |
| Status  | Operational state   |

Labels automatically follow the active theme.

---

# 10.10 Button Widget

Button styles:

* Primary
* Secondary
* Success
* Warning
* Danger
* Icon
* Floating Action

Example:

```text id="button-widget"
+-----------+

 START

+-----------+
```

Touch feedback shall occur within 50 ms.

---

# 10.11 Toggle Widget

Used for binary configuration.

Example:

```text id="toggle-widget"
Rain Delay

ON
```

Animated transitions are recommended.

---

# 10.12 Slider Widget

Applications:

* Brightness
* Runtime
* Water Budget
* Calibration
* Volume (future)

Features:

* Tick marks
* Numeric display
* Snap points
* Minimum/Maximum labels

---

# 10.13 Numeric Input Widget

Supports:

* Integer
* Floating Point
* Engineering Units

Example:

```text id="numeric-widget"
Runtime

25

Minutes
```

Validation occurs before values are committed.

---

# 10.14 List Widget

Applications:

* Zone Lists
* Program Lists
* Alarm History
* Event Log
* Diagnostics

Features:

* Sorting
* Filtering
* Search
* Selection

Lists shall support virtualization for large datasets.

---

# 10.15 Table Widget

Engineering tables display:

* Statistics
* Historical Data
* Diagnostics
* Configuration

Features:

* Column sorting
* Alternate row colors
* Sticky headers
* Horizontal scrolling

---

# 10.16 Gauge Widgets

Supported gauges:

* Circular Gauge
* Arc Gauge
* Linear Gauge
* Progress Gauge

Applications:

* Pressure
* Flow
* Health Score
* Battery (future)

Gauges shall support animated transitions.

---

# 10.17 Chart Widgets

Supported chart types:

* Line
* Area
* Bar
* Scatter (Service Mode)
* Trend
* Sparkline

Applications:

* Flow
* Pressure
* ET
* Temperature
* Water Usage
* CPU

Charts shall support live updates without full redraw.

---

# 10.18 Status Indicators

Status indicators communicate system condition.

Standard indicators:

| Color  | Meaning     |
| ------ | ----------- |
| Green  | Normal      |
| Blue   | Information |
| Yellow | Warning     |
| Orange | Attention   |
| Red    | Alarm       |
| Gray   | Disabled    |

Status indicators shall always include text or icons.

---

# 10.19 Progress Widget

Applications:

* OTA Update
* Self-Test
* Backup
* Restore
* Irrigation Runtime

Example:

```text id="progress-widget"
████████░░

82 %
```

---

# 10.20 Dialog Widgets

Dialog types:

* Information
* Confirmation
* Warning
* Error
* Progress
* Input
* Service

Dialogs shall be modal unless explicitly specified otherwise.

---

# 10.21 Notification Widget

Notification styles:

* Toast
* Banner
* Floating
* Alarm
* Status Bar

Notification priorities:

| Type     | Auto Close |
| -------- | :--------: |
| Info     |      ✔     |
| Success  |      ✔     |
| Warning  |  Optional  |
| Alarm    |      ✖     |
| Critical |      ✖     |

---

# 10.22 Icon Library

Standard icon groups:

* Irrigation
* Weather
* Hydraulics
* Diagnostics
* Network
* MQTT
* Settings
* Service
* Alarm
* Navigation

Icons shall use a consistent visual language throughout the interface.

---

# 10.23 Widget Events

Every widget supports:

* Click
* Press
* Release
* Long Press
* Value Changed
* Focus
* Blur
* Enable
* Disable

Widget callbacks shall never contain irrigation logic.

---

# 10.24 Data Binding

Widgets shall bind to data through the Display Manager.

```text id="widget-binding"
Application Manager

↓

Display Manager

↓

Widget Manager

↓

Widget

↓

Display
```

Direct firmware-to-widget communication is prohibited.

---

# 10.25 Widget Lifecycle

Every widget follows the same lifecycle.

```text id="widget-lifecycle"
Create

↓

Initialize

↓

Bind Data

↓

Update

↓

Redraw

↓

Destroy
```

Unused widgets shall release allocated resources.

---

# 10.26 Theme Integration

All widgets shall automatically adapt to:

* Light Theme
* Dark Theme
* High Contrast
* Service Theme

Theme changes shall not require widget recreation.

---

# 10.27 Accessibility

All widgets shall support:

* Minimum 48×48 px touch targets
* Keyboard navigation (future)
* High contrast
* Large fonts
* Color-independent status indication

Animations shall not reduce usability.

---

# 10.28 Performance Requirements

| Widget          | Target |
| --------------- | -----: |
| Create          |  <5 ms |
| Update          | <10 ms |
| Redraw          | <16 ms |
| Button Response | <50 ms |
| Chart Refresh   | 60 FPS |

Widget rendering shall not interfere with irrigation control.

---

# 10.29 Future Widget Extensions

The Widget Library has been designed to support future additions including:

* Interactive maps
* Timeline widgets
* AI recommendation cards
* Voice interaction controls
* 3D hydraulic visualization
* Digital twin widgets
* Camera widgets
* Remote desktop widgets

All future widgets shall inherit from the Base Widget class.

---

# 10.30 Engineering Notes

The Widget Library forms the visual foundation of the Zmartify HMI. By standardizing every user interface element, the platform achieves a consistent appearance, simplified maintenance and predictable behavior across all screens.

This component-based architecture also enables rapid development of future products within the Zmartify ecosystem, as new interfaces can be assembled from proven widgets rather than developed from scratch. The result is a scalable, maintainable and highly reusable design system suitable for long-term product evolution.

---

# 10.31 Chapter Summary

This chapter defines the reusable Widget Library and Design System used throughout the Zmartify Human–Machine Interface.

By adopting a component-based architecture built upon standardized widgets, layouts and interaction patterns, the HMI achieves consistency, performance and maintainability while providing a robust foundation for future controller models and ecosystem products.

---

# End of Chapter 10

**Next Chapter**

**Chapter 11 – Theme Engine, Visual Styling & Responsive Layout Framework**

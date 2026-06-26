# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 7

# Hydraulic Dashboard, Flow Analytics & Pressure Visualization

---

# 7.1 Purpose

This chapter defines the Hydraulic Dashboard of the Zmartify Human–Machine Interface.

Unlike conventional irrigation controllers that merely report valve status, the Zmartify controller continuously evaluates the hydraulic performance of the irrigation system.

The Hydraulic Dashboard presents this engineering information in a format suitable for both homeowners and professional installers.

The dashboard shall support:

* Real-time flow monitoring
* Pressure monitoring
* Leak detection
* Flow learning
* Pressure learning
* Hydraulic diagnostics
* Historical trend analysis
* Predictive maintenance

---

# 7.2 Design Objectives

The Hydraulic Dashboard shall:

* Visualize hydraulic performance
* Detect abnormal conditions immediately
* Explain controller decisions
* Present engineering information clearly
* Minimize unnecessary complexity
* Support troubleshooting
* Enable preventive maintenance

---

# 7.3 Hydraulic Screen Hierarchy

```text id="hyd-screen-tree"
Hydraulics

│

├── Overview

├── Live Flow

├── Live Pressure

├── Hydraulic Health

├── Flow Learning

├── Pressure Learning

├── Historical Trends

├── Leak Detection

├── Restrictions

└── Diagnostics
```

---

# 7.4 Hydraulic Overview

The Hydraulic Overview screen summarizes the complete hydraulic state.

Displayed information:

* Current Flow
* Current Pressure
* Active Zone
* Hydraulic Health
* Flow Deviation
* Pressure Deviation
* Leak Status
* Restriction Status

Reference layout:

```text id="hyd-overview-layout"
+------------------------------------------------+

Hydraulic Health

EXCELLENT

+------------------------------------------------+

Flow

24.4 L/min

Pressure

3.48 bar

+------------------------------------------------+

Leak Risk

Low

Restriction

None

+------------------------------------------------+
```

---

# 7.5 Live Flow Screen

Displays:

* Current Flow
* Learned Flow
* Difference
* Percentage Deviation
* Trend

Example:

```text id="live-flow"
Current

24.6 L/min

Expected

24.3 L/min

Deviation

+1.2 %
```

Values update every second during irrigation.

---

# 7.6 Flow Trend Graph

The Flow Trend graph visualizes hydraulic stability.

Supported time ranges:

* 5 Minutes
* 30 Minutes
* Current Irrigation Cycle
* Today
* Week
* Month

Chart features:

* Zoom
* Pan
* Cursor
* Peak markers
* Event annotations

---

# 7.7 Live Pressure Screen

Displays:

* Current Pressure
* Learned Pressure
* Pressure Stability
* Pressure Ripple
* Pressure Trend

Example:

```text id="live-pressure"
Pressure

3.48 bar

Reference

3.50 bar

Status

Stable
```

---

# 7.8 Pressure Trend Graph

Displays pressure history.

Indicators include:

* Average
* Minimum
* Maximum
* Ripple
* Stability Index

Pressure graphs help diagnose:

* Blocked filters
* Pipe restrictions
* Pump instability
* Valve problems

---

# 7.9 Hydraulic Health Index

The controller continuously calculates a Hydraulic Health Index.

Range:

```text id="health-range"
0–100
```

Interpretation:

| Score  | Status    |
| ------ | --------- |
| 95–100 | Excellent |
| 85–94  | Good      |
| 70–84  | Attention |
| 50–69  | Warning   |
| <50    | Critical  |

The index combines:

* Flow accuracy
* Pressure stability
* Leak probability
* Restriction probability

---

# 7.10 Flow Learning Screen

Displays learned hydraulic characteristics.

Example:

```text id="flow-learning"
Zone 4

Learned Flow

24.2 L/min

Confidence

98 %

Samples

126
```

Learning confidence shall increase as additional irrigation cycles are completed.

---

# 7.11 Pressure Learning Screen

Displays:

* Learned Pressure
* Confidence
* Number of Samples
* Last Learning Date

Example:

```text id="pressure-learning"
Pressure

3.51 bar

Confidence

97 %

Samples

118
```

---

# 7.12 Leak Detection Screen

Displays:

* Leak Probability
* Estimated Leak Size
* Detection Confidence
* First Detection
* Recommended Action

Example:

```text id="leak-screen"
Leak Risk

Moderate

Confidence

92 %

Estimated

0.8 L/min
```

Visual explanations shall accompany detection results.

---

# 7.13 Restriction Detection

Restriction analysis displays:

* Flow Reduction
* Pressure Increase
* Estimated Restriction
* Severity
* Suggested Inspection

Possible causes:

* Dirty filter
* Partially closed valve
* Damaged pipe
* Pump degradation

---

# 7.14 Hydraulic Diagnostics

Displays engineering metrics:

* Current Zone
* Valve Response Time
* Pump Delay
* Flow Stabilization Time
* Pressure Stabilization Time
* Sampling Rate

These values assist service technicians.

---

# 7.15 Historical Analysis

Historical charts include:

* Flow History
* Pressure History
* Hydraulic Health
* Leak Probability
* Restriction Trend

Selectable periods:

* Day
* Week
* Month
* Season

---

# 7.16 Event Timeline

Hydraulic events appear on a timeline.

Examples:

```text id="hyd-timeline"
08:00

Program Started

08:01

Flow Stable

08:18

Pressure Deviation

08:20

Program Completed
```

Events shall synchronize with irrigation history.

---

# 7.17 Alarm Visualization

Hydraulic alarms include:

* High Flow
* Low Flow
* High Pressure
* Low Pressure
* Leak
* Restriction
* Pump Failure

Alarm display:

```text id="hyd-alarm"
WARNING

Flow Lower Than Expected

Zone 4

Recommendation

Inspect Filter
```

---

# 7.18 Mini Dashboard Widgets

Compact widgets are available for:

* Dashboard
* Zone Screen
* Program Screen

Examples:

```text id="mini-widgets"
Flow

24.4

Pressure

3.48

Health

98 %
```

Widgets maintain visual consistency throughout the HMI.

---

# 7.19 Engineering Charts

Supported chart types:

* Line
* Area
* Gauge
* Trend Indicator
* Histogram (future)
* Scatter Plot (service mode)

Charts shall use LVGL native chart widgets where possible.

---

# 7.20 Refresh Strategy

Recommended update frequencies:

| Parameter               | Interval |
| ----------------------- | -------: |
| Flow                    |      1 s |
| Pressure                |      1 s |
| Health Index            |      5 s |
| Leak Probability        |    Event |
| Restriction Probability |    Event |
| Charts                  |      1 s |

Only modified chart segments shall be redrawn.

---

# 7.21 Touch Interaction

Available interactions:

| Action         | Result            |
| -------------- | ----------------- |
| Tap Chart      | Details           |
| Long Press     | Cursor Mode       |
| Pinch (future) | Zoom              |
| Swipe          | Change Time Range |

The interface shall remain fully usable without gesture support.

---

# 7.22 Accessibility

Hydraulic information shall remain understandable for non-technical users.

Visual indicators include:

* Icons
* Status labels
* Health score
* Simple recommendations

Example:

```text id="simple-message"
Hydraulic System

Operating Normally

No Action Required
```

Service Mode exposes additional engineering detail.

---

# 7.23 Future Extensions

The Hydraulic Dashboard supports future capabilities including:

* Pump efficiency analysis
* Variable-speed pump monitoring
* Multi-pump systems
* Water hammer detection
* Pipe aging estimation
* AI hydraulic advisor
* Predictive leak localization
* Digital twin visualization

These features shall integrate without changing the established screen hierarchy.

---

# 7.24 Engineering Notes

The Hydraulic Dashboard is one of the defining innovations of the Zmartify platform. Rather than treating flow and pressure as isolated measurements, the controller continuously evaluates hydraulic behavior to provide a comprehensive assessment of system performance.

By presenting learned reference values alongside live measurements and calculated health indicators, the interface enables both users and service personnel to detect developing problems before they become operational failures. This predictive approach significantly reduces maintenance effort while improving irrigation reliability and water efficiency.

---

# 7.25 Chapter Summary

This chapter defines the Hydraulic Dashboard, including live flow monitoring, pressure visualization, hydraulic health assessment and predictive diagnostics.

The interface transforms complex hydraulic analysis into clear operational information through real-time telemetry, historical trends and intuitive visual indicators. Together with the Dashboard, Irrigation and Weather screens, it forms one of the primary engineering interfaces of the Zmartify Human–Machine Interface.

---

# End of Chapter 7

**Next Chapter**

**Chapter 8 – Configuration Interface, Settings Architecture & System Administration**

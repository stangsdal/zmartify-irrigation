# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 6

# Weather Dashboard, ET Visualization & Environmental Information Screens

---

# 6.1 Purpose

This chapter defines the complete Weather Interface of the Zmartify Human–Machine Interface (HMI).

The Weather Dashboard transforms meteorological data into actionable irrigation information by combining current observations, forecasts and evapotranspiration (ET) calculations with irrigation recommendations.

Rather than functioning as a conventional weather application, the interface presents environmental conditions from the perspective of irrigation management.

---

# 6.2 Design Objectives

The Weather Interface shall:

* Present relevant weather information
* Visualize irrigation impact
* Explain automatic irrigation decisions
* Display ET calculations
* Support future weather services
* Minimize user interpretation
* Update automatically
* Remain readable outdoors

---

# 6.3 Weather Screen Hierarchy

```text id="weather-screen-hierarchy"
Weather

│

├── Current Conditions

├── Hourly Forecast

├── Daily Forecast

├── ET Analysis

├── Rain Delay

├── Irrigation Recommendation

├── Historical Weather

└── Weather Diagnostics
```

---

# 6.4 Weather Dashboard Layout

Reference layout:

```text id="weather-layout"
+------------------------------------------------+

Current Weather

+------------------------------------------------+

Temperature     Humidity     Wind

+------------------------------------------------+

Rain Forecast   ET           Solar

+------------------------------------------------+

Irrigation Recommendation

+------------------------------------------------+

7-Day Forecast

+------------------------------------------------+

Navigation

+------------------------------------------------+
```

---

# 6.5 Current Conditions Card

The Current Conditions card displays the latest environmental measurements.

Displayed parameters:

* Temperature
* Relative Humidity
* Atmospheric Pressure
* Wind Speed
* Wind Direction
* Solar Radiation
* UV Index
* Cloud Cover

Example:

```text id="current-weather"
Temperature

22.8 °C

Humidity

61 %

Wind

3.4 m/s NW
```

---

# 6.6 Temperature Widget

Displays:

* Current Temperature
* Daily Minimum
* Daily Maximum
* Trend Indicator

Trend icons:

| Icon | Meaning |
| ---- | ------- |
| ▲    | Rising  |
| ▼    | Falling |
| ►    | Stable  |

Historical temperature graphs are available from the History screen.

---

# 6.7 Humidity Widget

Displays:

* Relative Humidity
* Dew Point
* Trend
* Comfort Indicator

Example:

```text id="humidity-widget"
Humidity

61 %

Dew Point

15.2 °C
```

Humidity values contribute to ET calculations.

---

# 6.8 Wind Widget

Displays:

* Speed
* Direction
* Gust Speed
* Irrigation Impact

Example:

```text id="wind-widget"
Wind

3.2 m/s

North-East

Gust

5.8 m/s
```

Strong winds shall trigger advisory messages.

---

# 6.9 Rain Widget

Displays:

* Current Rainfall
* Last 24 Hours
* Forecast Rain
* Rain Probability

Example:

```text id="rain-widget"
Rain

0.0 mm

Forecast

8.2 mm

Probability

70 %
```

Forecast rainfall directly influences irrigation recommendations.

---

# 6.10 Solar Radiation Widget

Displays:

* Current Solar Radiation
* Daily Peak
* Daily Energy

Example:

```text id="solar-widget"
Solar

618 W/m²

Peak

801 W/m²
```

Solar radiation contributes to ET calculations.

---

# 6.11 UV Index Widget

Displays:

* Current UV
* Maximum Today
* Exposure Category

Categories:

| UV   | Category  |
| ---- | --------- |
| 0–2  | Low       |
| 3–5  | Moderate  |
| 6–7  | High      |
| 8–10 | Very High |
| 11+  | Extreme   |

---

# 6.12 ET Dashboard

The ET Dashboard is one of the defining features of the Zmartify interface.

Displayed values:

* Today's ET
* Weekly ET
* Monthly ET
* Seasonal ET
* ET Trend
* Irrigation Adjustment

Example:

```text id="et-dashboard"
Today's ET

3.8 mm

Water Budget

110 %

Recommendation

Increase Runtime
```

---

# 6.13 ET Trend Chart

A line chart visualizes ET over time.

Available periods:

* 24 Hours
* 7 Days
* 30 Days
* Season

Charts shall support:

* Zoom
* Pan
* Data markers

---

# 6.14 Irrigation Recommendation Card

The Recommendation Engine summarizes weather effects.

Possible recommendations:

* Normal Irrigation
* Reduce Runtime
* Increase Runtime
* Delay Irrigation
* Suspend Irrigation
* Resume Irrigation

Example:

```text id="recommendation-card"
Recommendation

Delay Irrigation

Expected Rain

12 mm Tonight
```

The recommendation shall include a brief explanation.

---

# 6.15 Rain Delay Screen

Displays:

* Delay Status
* Remaining Delay
* Trigger Reason
* Automatic Resume Time

Example:

```text id="rain-delay"
Rain Delay

Active

Remaining

18 Hours

Reason

Forecast Rain
```

Users may manually override the delay if authorized.

---

# 6.16 Hourly Forecast Screen

Displays:

* Temperature
* Rain Probability
* Wind
* Cloud Cover
* ET Estimate

Forecast period:

```text id="hourly-period"
Next 24 Hours
```

Each hour is represented by a compact forecast card.

---

# 6.17 Daily Forecast Screen

Displays:

* Seven-day forecast
* Daily High/Low
* Rainfall
* Wind
* ET Estimate

Example:

```text id="daily-forecast"
Tuesday

24 °C

Rain

20 %

ET

3.9 mm
```

---

# 6.18 Weather History

Historical information includes:

* Temperature
* Humidity
* Rainfall
* Solar Radiation
* Wind
* ET

Supported periods:

* Day
* Week
* Month
* Season

Historical data may be displayed using LVGL charts.

---

# 6.19 Weather Diagnostics

Engineering information:

* Weather Provider
* Last Update
* API Status
* Update Interval
* Forecast Confidence
* Data Age

Example:

```text id="weather-diagnostics"
Provider

OpenWeather

Updated

2 Minutes Ago

Status

Connected
```

---

# 6.20 Weather Icons

Standard icon set:

| Condition     | Icon |
| ------------- | ---- |
| Sunny         | ☀    |
| Partly Cloudy | ⛅    |
| Cloudy        | ☁    |
| Rain          | 🌧   |
| Storm         | ⛈    |
| Snow          | ❄    |
| Fog           | 🌫   |
| Wind          | 🌬   |

Icons shall remain consistent across all screens.

---

# 6.21 Automatic Updates

Recommended refresh intervals:

| Parameter       |     Interval |
| --------------- | -----------: |
| Current Weather |        5 min |
| Forecast        |       30 min |
| ET              |       30 min |
| Recommendation  | Event Driven |
| Rain Delay      | Event Driven |

Only changed widgets shall be redrawn.

---

# 6.22 Alert Presentation

Weather alerts include:

* Heavy Rain
* High Wind
* Freeze Warning
* Heat Warning
* Lightning
* Weather Service Offline

Alerts shall be color coded according to severity.

Critical alerts may appear on the Dashboard.

---

# 6.23 Accessibility

The Weather Interface shall support:

* High-contrast icons
* Large numerical values
* Color-independent alerts
* Adjustable chart scaling
* Day/Night themes

All weather data shall remain legible under direct sunlight.

---

# 6.24 Future Extensions

The Weather Interface is designed to support:

* Air Quality Index (AQI)
* Pollen Forecast
* Soil Temperature
* Soil Moisture Sensors
* Lightning Detection
* Satellite Imagery
* Radar Overlay
* Hyperlocal Forecasting
* AI Weather Advisor

These features shall integrate without altering the existing navigation structure.

---

# 6.25 Engineering Notes

The Weather Interface is not intended to duplicate a consumer weather application. Instead, it focuses on presenting the environmental variables that directly influence irrigation decisions.

By combining weather observations with ET calculations and irrigation recommendations, the interface explains *why* the controller adjusts watering schedules, increasing user confidence in automatic operation. This transparency is a key differentiator of the Zmartify platform and supports both residential users and professional irrigation managers.

---

# 6.26 Chapter Summary

This chapter defines the Weather Dashboard and environmental information screens for the Zmartify Human–Machine Interface.

The interface integrates current weather, forecasts, ET calculations and irrigation recommendations into a unified engineering view that supports informed irrigation management. Together with the Dashboard and Irrigation screens, it forms one of the primary operational interfaces of the controller.

---

# End of Chapter 6

**Next Chapter**

**Chapter 7 – Hydraulic Dashboard, Flow Analytics & Pressure Visualization**

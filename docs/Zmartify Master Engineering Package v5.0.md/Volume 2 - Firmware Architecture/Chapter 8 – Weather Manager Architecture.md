# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 8 – Weather Manager Architecture

---

# 8 Weather Manager

---

# 8.1 Purpose

The Weather Manager is responsible for collecting, validating, processing and distributing all weather-related information used by the Zmartify Irrigation Controller.

Unlike conventional irrigation controllers that simply use rainfall as a binary input, the Zmartify Weather Manager combines multiple data sources to provide intelligent irrigation recommendations.

The Weather Manager shall provide reliable weather information even if one or more data sources become unavailable.

---

# 8.2 Design Objectives

The Weather Manager shall:

* Collect local weather data
* Retrieve online weather forecasts
* Validate incoming measurements
* Calculate irrigation recommendations
* Detect abnormal weather conditions
* Provide ET input data
* Support multiple weather providers
* Continue operating during Internet outages

---

# 8.3 Architectural Position

```text
                Local Weather Station
                         │
                         ▼
                Weather Manager
                         ▲
                         │
         Weather Underground API
                         ▲
                         │
                 Future Providers
                         │
                         ▼
                 Data Validation
                         │
                         ▼
               Weather Processing
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
     ET Engine   Irrigation Engine   MQTT Manager
```

---

# 8.4 Supported Weather Sources

Version 5.0 supports two primary weather sources.

## Local Weather Station

Preferred source for measured conditions.

Supported measurements:

* Temperature
* Humidity
* Wind Speed
* Wind Gust
* Wind Direction
* Rainfall
* UV Index
* Solar Radiation
* Barometric Pressure

---

## Internet Weather Provider

Primary provider:

**Weather Underground**

Future supported providers:

* OpenWeatherMap
* Tomorrow.io
* Meteomatics
* DMI (Denmark)
* NOAA
* Custom REST API

The provider interface shall be modular to allow additional integrations without affecting application logic.

---

# 8.5 Data Priority

The Weather Manager shall prioritize data sources as follows:

| Priority | Source                       |
| -------- | ---------------------------- |
| 1        | Local Weather Station        |
| 2        | Weather Underground          |
| 3        | Alternative Weather Provider |
| 4        | Cached Historical Data       |

If online services become unavailable, irrigation shall continue using local measurements and cached forecasts where possible.

---

# 8.6 Weather Parameters

The Weather Manager maintains the following environmental variables.

| Parameter            | Source           |
| -------------------- | ---------------- |
| Air Temperature      | Local / Internet |
| Relative Humidity    | Local / Internet |
| Wind Speed           | Local            |
| Wind Gust            | Local            |
| Wind Direction       | Local            |
| Rainfall (Today)     | Local            |
| Rainfall (24 h)      | Local            |
| Forecast Rain        | Internet         |
| UV Index             | Local            |
| Solar Radiation      | Local            |
| Atmospheric Pressure | Local            |
| Cloud Cover          | Internet         |
| Forecast Temperature | Internet         |
| Forecast Humidity    | Internet         |

---

# 8.7 Weather State Machine

```text
Idle

↓

Acquire Local Data

↓

Validate

↓

Acquire Forecast

↓

Validate

↓

Merge Data

↓

Calculate Recommendations

↓

Publish Events

↓

Idle
```

If online acquisition fails:

```text
Forecast Failure

↓

Use Cached Forecast

↓

Continue Operation
```

---

# 8.8 Data Validation

Each incoming measurement shall be validated before use.

Validation includes:

* Range checks
* Rate-of-change checks
* Sensor timeout detection
* Missing value detection
* Timestamp verification

Example:

Temperature

Valid range:

-30°C to +60°C

Values outside this range shall be rejected.

---

# 8.9 Rainfall Processing

The Weather Manager shall maintain:

* Current rainfall
* Hourly rainfall
* Daily rainfall
* Weekly rainfall
* Monthly rainfall
* Seasonal rainfall

Rainfall accumulation shall be reset according to the configured calendar and regional conventions.

---

# 8.10 Forecast Processing

Forecast information shall include:

* Rain probability
* Rain amount
* Temperature
* Wind
* Humidity
* UV
* Cloud cover

Forecast horizon:

* Next 24 hours
* Next 3 days
* Next 7 days

Forecast resolution depends on the provider.

---

# 8.11 Weather Recommendation Engine

The Weather Manager shall provide recommendations to the Irrigation Engine.

Possible recommendations:

| Recommendation | Meaning            |
| -------------- | ------------------ |
| NORMAL         | Irrigate normally  |
| REDUCE         | Reduce runtime     |
| INCREASE       | Increase runtime   |
| DELAY          | Delay irrigation   |
| SKIP           | Skip irrigation    |
| FREEZE         | Lock irrigation    |
| HIGH_WIND      | Suspend sprinklers |

The final irrigation decision always belongs to the Irrigation Engine.

---

# 8.12 Wind Protection

Wind limits are configurable.

Example defaults:

| Condition              | Threshold |
| ---------------------- | --------: |
| Warning                |     6 m/s |
| Suspend Spray Zones    |     8 m/s |
| Suspend All Sprinklers |    12 m/s |

Drip irrigation may continue during moderate wind if configured.

---

# 8.13 Freeze Protection

Freeze protection shall prevent irrigation when:

Air temperature

≤ 2°C (default)

Configurable range:

-5°C to +5°C

Once freeze protection is active:

* Automatic irrigation shall be suspended.
* Manual irrigation shall require user confirmation.
* MQTT notification shall be published.

---

# 8.14 Rain Delay

Rain Delay may be triggered by:

* Local rainfall
* Forecast rainfall
* User request
* MQTT command

Rain Delay duration:

Configurable

Typical:

24–96 hours

The remaining delay shall survive power loss.

---

# 8.15 Weather Events

The Weather Manager publishes:

```text
EVT_WEA_UPDATED

EVT_WEA_RAIN

EVT_WEA_FORECAST

EVT_WEA_FREEZE

EVT_WEA_HIGH_WIND

EVT_WEA_DELAY

EVT_WEA_CLEAR
```

Subscribers include:

* Irrigation Engine
* ET Engine
* MQTT Manager
* UI Manager
* Logging Manager

---

# 8.16 MQTT Topics

Example topics:

```text
zmartify/weather/current

zmartify/weather/local

zmartify/weather/forecast

zmartify/weather/rain

zmartify/weather/wind

zmartify/weather/recommendation

zmartify/weather/status
```

All weather topics shall use JSON payloads defined in Volume 3.

---

# 8.17 User Interface

The Weather Dashboard shall display:

Current:

* Temperature
* Humidity
* Wind
* Rainfall
* UV
* Solar Radiation

Forecast:

* Rain probability
* Temperature trend
* Wind forecast

Controller status:

* Rain Delay
* Freeze Protection
* Weather recommendation

Historical weather graphs shall be available for the previous 24 hours and configurable longer periods.

---

# 8.18 Historical Data

The Weather Manager shall retain:

| Data            |           Retention |
| --------------- | ------------------: |
| Minute Data     |            24 hours |
| Hourly Data     |             90 days |
| Daily Data      | Lifetime statistics |
| Seasonal Totals |            Lifetime |

Historical weather supports trend analysis and irrigation optimization.

---

# 8.19 Diagnostics

Diagnostic information includes:

* Weather provider status
* Last update time
* API latency
* Forecast age
* Local station status
* Sensor validity
* Cached forecast age
* Synchronization status

All diagnostics shall be available via:

* LVGL
* MQTT
* Diagnostics Manager

---

# 8.20 Public API

Example interface.

```c
weather_manager_init();

weather_manager_start();

weather_manager_stop();

weather_manager_get_current();

weather_manager_get_forecast();

weather_manager_get_recommendation();

weather_manager_force_update();

weather_manager_status();
```

All access to weather information shall occur through these APIs.

---

# 8.21 Future Enhancements

The Weather Manager has been designed to support future features including:

* Multiple simultaneous forecast providers
* AI forecast confidence scoring
* Radar rainfall integration
* Hyper-local precipitation nowcasting
* Lightning detection
* Frost prediction
* Evaporation sensors
* Automatic weather provider failover

These additions shall require no changes to the Irrigation Engine interface.

---

# 8.22 Unit Testing

Automated tests shall verify:

* Local weather acquisition
* Forecast retrieval
* Data validation
* Rain Delay
* Freeze Protection
* Wind Protection
* Recommendation logic
* Provider failover
* Cached forecast handling

Minimum code coverage:

**90%**

---

# 8.23 Chapter Summary

The Weather Manager transforms environmental data into actionable irrigation intelligence.

By combining local measurements with online forecasts, validating all incoming data and generating standardized recommendations, it enables the Irrigation Engine to make informed, water-efficient decisions while remaining resilient to network failures and sensor faults.

Its modular architecture ensures compatibility with future weather providers and advanced forecasting technologies, making it a cornerstone of the Zmartify smart irrigation platform.

---

# End of Chapter 8

**Next Chapter**

**Chapter 9 – Evapotranspiration (ET) Engine Architecture**

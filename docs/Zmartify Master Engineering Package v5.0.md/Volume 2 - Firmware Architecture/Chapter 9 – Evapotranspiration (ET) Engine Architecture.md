# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 9 – Evapotranspiration (ET) Engine Architecture

---

# 9 Evapotranspiration (ET) Engine

---

# 9.1 Purpose

The Evapotranspiration (ET) Engine is responsible for calculating the actual water requirement of the landscape based on environmental conditions.

Unlike traditional irrigation controllers that apply a fixed runtime throughout the season, the Zmartify ET Engine continuously adjusts irrigation according to plant water demand.

The objective is to achieve:

* Maximum water efficiency
* Healthy plant growth
* Reduced water consumption
* Automatic seasonal adaptation
* Minimal user intervention

The ET Engine shall never directly control irrigation valves. It provides recommendations to the Irrigation Engine.

---

# 9.2 Design Objectives

The ET Engine shall:

* Calculate daily evapotranspiration
* Calculate zone water demand
* Support multiple ET calculation methods
* Apply crop coefficients (Kc)
* Apply seasonal adjustments
* Estimate soil water balance
* Recommend runtime adjustments
* Operate using both local and forecast weather

---

# 9.3 Architectural Position

```text
                Weather Manager
                      │
                      ▼
              Environmental Data
                      │
                      ▼
                 ET Engine
                      │
      ┌───────────────┼───────────────┐
      ▼               ▼               ▼
 Water Budget   Zone Manager   Irrigation Engine
```

The ET Engine shall never interact directly with relay hardware.

---

# 9.4 ET Philosophy

The ET Engine estimates how much water has been lost from the soil through:

* Plant transpiration
* Soil evaporation

This value represents the amount of irrigation required to restore the root zone moisture balance.

Instead of irrigating according to a timer, the controller irrigates according to calculated plant demand.

---

# 9.5 Supported Calculation Methods

Version 5.0 supports multiple calculation models.

| Method                 | Status    |
| ---------------------- | --------- |
| Simplified FAO-56      | Standard  |
| Hargreaves             | Optional  |
| Weather Provider ET    | Supported |
| Manual ET              | Supported |
| Future Penman-Monteith | Planned   |

The active calculation method shall be configurable.

---

# 9.6 Environmental Inputs

The ET Engine receives data from the Weather Manager.

Required inputs:

* Air Temperature
* Relative Humidity
* Solar Radiation
* Wind Speed
* Rainfall

Optional inputs:

* Barometric Pressure
* UV Index
* Cloud Cover
* Dew Point

Future sensors may provide:

* Leaf Wetness
* Soil Temperature
* Net Radiation

---

# 9.7 Plant Coefficients

Each irrigation zone maintains a Crop Coefficient (Kc).

Typical values:

| Plant         | Default Kc |
| ------------- | ---------: |
| Turf Grass    |       0.80 |
| Flowers       |       0.70 |
| Shrubs        |       0.60 |
| Trees         |       0.55 |
| Vegetables    |       0.90 |
| Native Plants |       0.40 |

Users may override all default coefficients.

---

# 9.8 Daily ET Calculation

The ET Engine shall calculate:

```text
Reference ET (ETo)

×

Crop Coefficient (Kc)

=

Crop ET (ETc)
```

The resulting ETc represents the daily water demand for each irrigation zone.

---

# 9.9 Water Budget

Each zone maintains an independent water budget.

```text
Previous Balance

+

Rainfall

+

Manual Watering

-

ET Loss

=

Current Water Balance
```

If the water balance becomes negative beyond the configured threshold, irrigation is recommended.

---

# 9.10 Soil Water Storage

The ET Engine models soil water storage using:

* Root depth
* Soil type
* Available Water Capacity (AWC)
* Allowable depletion

Typical values:

| Soil       | Water Holding Capacity |
| ---------- | ---------------------: |
| Sand       |                    Low |
| Sandy Loam |                 Medium |
| Loam       |                   High |
| Clay       |              Very High |

These values are configurable per zone.

---

# 9.11 Runtime Recommendation

The ET Engine does not determine valve runtimes directly.

Instead it provides:

* Required water depth (mm)
* Runtime correction factor
* Water deficit
* Water surplus

The Irrigation Engine converts these recommendations into actual runtimes.

---

# 9.12 Seasonal Adjustment

Automatic seasonal adjustment shall be calculated continuously.

Typical adjustment range:

| Season | Typical Factor |
| ------ | -------------: |
| Winter |         20–40% |
| Spring |         60–90% |
| Summer |        90–120% |
| Autumn |         50–80% |

Users may configure minimum and maximum limits.

---

# 9.13 Rainfall Integration

Rainfall shall reduce irrigation demand.

Effective rainfall is calculated using configurable efficiency factors.

Example:

```text
Measured Rain

↓

Runoff Correction

↓

Soil Absorption

↓

Effective Rainfall

↓

Water Budget
```

Very heavy rainfall shall not necessarily contribute 100% to the available soil moisture.

---

# 9.14 Forecast Integration

Forecast rainfall influences irrigation planning.

Example logic:

```text
Heavy Rain Expected

↓

Delay Irrigation

↓

Recalculate Water Budget

↓

Publish Recommendation
```

Forecast confidence shall be considered in future firmware versions.

---

# 9.15 Water Stress Model

Each zone maintains a calculated stress level.

Levels include:

| Level    | Meaning                          |
| -------- | -------------------------------- |
| Normal   | Sufficient moisture              |
| Moderate | Irrigation recommended           |
| High     | Irrigation required              |
| Critical | Immediate irrigation recommended |

Stress levels are advisory; final decisions remain with the Irrigation Engine.

---

# 9.16 ET Engine State Machine

```text
Idle

↓

Acquire Weather

↓

Validate Inputs

↓

Calculate ETo

↓

Apply Crop Coefficient

↓

Update Water Budget

↓

Generate Recommendation

↓

Publish Event

↓

Idle
```

Calculation failures shall generate diagnostic events without affecting overall controller operation.

---

# 9.17 Published Events

The ET Engine publishes:

```text
EVT_ET_UPDATED

EVT_ET_HIGH

EVT_ET_LOW

EVT_WATER_DEFICIT

EVT_WATER_SURPLUS

EVT_ET_RECOMMENDATION
```

Subscribers include:

* Irrigation Engine
* Zone Manager
* MQTT Manager
* UI Manager
* Logging Manager

---

# 9.18 MQTT Topics

Example MQTT hierarchy:

```text
zmartify/et/current

zmartify/et/daily

zmartify/et/history

zmartify/zone/01/et

zmartify/zone/01/water_budget

zmartify/zone/01/water_deficit
```

JSON payloads are defined in **Volume 3**.

---

# 9.19 User Interface

The ET Dashboard shall display:

Current:

* Daily ET
* Weekly ET
* Monthly ET
* Water Budget
* Rainfall
* Irrigation Recommendation

Per Zone:

* Water Deficit
* Soil Moisture Estimate
* Runtime Adjustment
* Crop Coefficient
* Seasonal Factor

Historical charts shall be available for:

* 24 hours
* 7 days
* 30 days
* Current season

---

# 9.20 Historical Statistics

The ET Engine shall retain:

| Data                       |       Retention |
| -------------------------- | --------------: |
| Daily ET                   |        Lifetime |
| Weekly ET                  |        Lifetime |
| Monthly ET                 |        Lifetime |
| Runtime Adjustment History |        Lifetime |
| Water Budget History       | 5 years minimum |

Historical data shall support trend analysis and optimization.

---

# 9.21 Diagnostics

Diagnostic information includes:

* Active ET method
* Last calculation time
* Weather data age
* Missing parameters
* Calculation duration
* Water budget status
* Seasonal adjustment factor
* Calculation errors

Diagnostics shall be accessible through:

* LVGL
* MQTT
* Diagnostics Manager

---

# 9.22 Public API

Example interface:

```c
et_engine_init();

et_engine_start();

et_engine_stop();

et_engine_calculate();

et_engine_get_daily();

et_engine_get_zone_budget();

et_engine_get_recommendation();

et_engine_force_update();
```

Application modules shall obtain ET information exclusively through these APIs.

---

# 9.23 Future Enhancements

The architecture supports future capabilities including:

* Full FAO Penman-Monteith implementation
* AI-assisted ET prediction
* Satellite-derived ET data
* Machine learning optimization
* Soil moisture feedback correction
* Water cost optimization
* Multi-site irrigation management
* Predictive seasonal planning

These enhancements shall integrate without changing the public ET Engine API.

---

# 9.24 Unit Testing

Automated testing shall verify:

* ET calculations
* Crop coefficient application
* Water budget calculations
* Rainfall correction
* Seasonal adjustment
* Forecast integration
* Runtime recommendations
* Historical data management
* Error handling

Minimum code coverage:

**90%**

---

# 9.25 Chapter Summary

The ET Engine transforms environmental measurements into scientifically based irrigation recommendations by modelling plant water demand and soil moisture balance.

By separating evapotranspiration calculations from irrigation control, the architecture maintains clear responsibility boundaries while enabling sophisticated water management strategies. The modular design supports future adoption of advanced agronomic models and artificial intelligence without affecting the Irrigation Engine or user interface.

---

# End of Chapter 9

**Next Chapter**

**Chapter 10 – Flow Manager Architecture**

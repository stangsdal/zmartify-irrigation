# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 6 – Zone Manager Architecture

---

# 6 Zone Manager

## 6.1 Purpose

The Zone Manager is responsible for maintaining the complete definition, configuration, runtime status and historical information for every irrigation zone in the Zmartify Irrigation Controller.

The Zone Manager acts as the authoritative repository for all zone-related information and provides a standardized interface to the Irrigation Engine, User Interface, MQTT Manager and Diagnostics Manager.

The Zone Manager shall **never** directly control hardware.

All valve operations shall be performed through the Irrigation Engine and Relay Manager.

---

# 6.2 Responsibilities

The Zone Manager shall be responsible for:

* Zone configuration
* Zone properties
* Runtime status
* Hydraulic baseline storage
* Water usage statistics
* Enable/Disable status
* Scheduling associations
* Zone diagnostics
* Configuration persistence
* Historical performance

---

# 6.3 Architectural Position

```text
                    User Interface
                          │
                          ▼
                    Zone Manager
                          │
        ┌─────────────────┼─────────────────┐
        ▼                 ▼                 ▼
 Irrigation Engine   Configuration    MQTT Manager
                          │
                          ▼
                    Persistent Storage
```

The Zone Manager does **not** communicate directly with hardware.

---

# 6.4 Zone Capacity

Version 5.0 supports:

| Parameter              |     Value |
| ---------------------- | --------: |
| Irrigation Zones       |        15 |
| Master Valve           |         1 |
| Zone Groups            | Unlimited |
| Programs per Zone      | Unlimited |
| Manual Runtime Presets | Unlimited |

Future firmware shall support expansion without architectural redesign.

---

# 6.5 Zone Data Model

Each irrigation zone shall contain the following configuration.

## Identification

* Zone ID
* Zone Name
* Description
* Icon
* Display Color

---

## Hardware

* Relay Number
* Valve Type
* Normally Closed/Open
* Master Valve Required

---

## Irrigation

* Default Runtime
* Maximum Runtime
* Minimum Runtime
* Cycle & Soak Enabled
* Cycle Duration
* Soak Duration

---

## Hydraulic

* Learned Flow
* Minimum Flow
* Maximum Flow
* Learned Pressure
* Pressure Limits

---

## Plant Information

* Plant Type
* Root Depth
* Crop Coefficient (Kc)

---

## Soil Information

* Soil Type
* Slope
* Drainage
* Infiltration Rate

---

## Environmental

* Sun Exposure
* Wind Exposure
* Shade Factor

---

## Configuration

* Enabled
* Seasonal Adjustment
* ET Adjustment Enabled
* Rain Delay Override

---

# 6.6 Zone Configuration Structure

Example data structure.

```c
typedef struct
{
    uint8_t zone_id;

    char name[32];

    bool enabled;

    uint8_t relay;

    uint32_t default_runtime;

    float learned_flow;

    float learned_pressure;

    float area_m2;

    soil_type_t soil;

    plant_type_t plant;

    sprinkler_type_t sprinkler;

} zone_config_t;
```

Additional parameters may be appended while preserving backward compatibility.

---

# 6.7 Zone Runtime State

Each zone maintains independent runtime information.

```text
Disabled

↓

Idle

↓

Queued

↓

Preparing

↓

Opening

↓

Running

↓

Pausing

↓

Resuming

↓

Stopping

↓

Completed

↓

Idle
```

Fault states:

* Flow Fault
* Pressure Fault
* Valve Fault
* Timeout

---

# 6.8 Zone Categories

Supported zone categories include:

* Lawn
* Rotor Sprinklers
* Spray Heads
* Drip Irrigation
* Trees
* Shrubs
* Flower Beds
* Vegetable Garden
* Greenhouse
* Pots
* Custom

Each category provides default irrigation parameters.

---

# 6.9 Soil Types

Supported soil models:

* Sand
* Sandy Loam
* Loam
* Clay Loam
* Clay
* Custom

Each soil model defines:

* Water holding capacity
* Infiltration rate
* Recommended Cycle & Soak parameters

---

# 6.10 Plant Database

The firmware shall include a configurable plant database.

Examples:

| Plant Type | Root Depth | Default Kc |
| ---------- | ---------: | ---------: |
| Lawn       |     150 mm |       0.80 |
| Flowers    |     250 mm |       0.70 |
| Shrubs     |     400 mm |       0.60 |
| Trees      |     700 mm |       0.55 |
| Vegetables |     300 mm |       0.90 |

Users may override all default values.

---

# 6.11 Sprinkler Types

Supported sprinkler technologies:

* Spray
* Rotary
* MP Rotator
* Bubblers
* Drip Line
* Drip Emitters
* Micro Sprayers
* Soaker Hose

Each sprinkler type defines:

* Typical precipitation rate
* Distribution uniformity
* Recommended operating pressure

---

# 6.12 Hydraulic Baselines

Each zone shall maintain independently learned hydraulic values.

Stored values include:

* Average Flow
* Maximum Flow
* Minimum Flow
* Average Pressure
* Pressure Variance

Flow Learning updates these values only after user approval.

---

# 6.13 Runtime Statistics

Each zone stores:

Daily

* Runtime
* Water Usage

Weekly

* Runtime
* Water Usage

Monthly

* Runtime
* Water Usage

Lifetime

* Runtime
* Starts
* Water Consumption

Statistics shall survive firmware updates.

---

# 6.14 Water Budget

Each zone maintains an independent water budget.

Inputs:

* ET
* Rainfall
* Manual Watering
* Seasonal Factor

Outputs:

* Required Runtime
* Water Deficit
* Water Surplus

This enables intelligent irrigation optimization.

---

# 6.15 Public API

Example interface.

```c
zone_manager_init();

zone_manager_get();

zone_manager_set();

zone_manager_enable();

zone_manager_disable();

zone_manager_runtime();

zone_manager_statistics();

zone_manager_update_flow();

zone_manager_update_pressure();

zone_manager_save();
```

The Irrigation Engine shall access zone data exclusively through these APIs.

---

# 6.16 Event Subscription

Zone Manager subscribes to:

* EVT_ZONE_START
* EVT_ZONE_STOP
* EVT_FLOW_UPDATED
* EVT_PRESS_UPDATED
* EVT_ET_UPDATED
* EVT_CONFIGURATION_CHANGED

Zone Manager publishes:

* EVT_ZONE_CONFIGURATION_CHANGED
* EVT_ZONE_ENABLED
* EVT_ZONE_DISABLED
* EVT_ZONE_UPDATED
* EVT_ZONE_STATISTICS_UPDATED

---

# 6.17 Persistent Storage

Zone configuration shall be stored using NVS.

Historical statistics shall be stored using LittleFS.

Configuration versioning shall support automatic migration after firmware updates.

---

# 6.18 MQTT Representation

Each zone shall be represented individually.

Example topic hierarchy:

```text
zmartify/zone/01/config

zmartify/zone/01/state

zmartify/zone/01/statistics

zmartify/zone/01/flow

zmartify/zone/01/pressure
```

MQTT payloads are specified in **Volume 3**.

---

# 6.19 User Interface Integration

The Zone Manager provides all information required by the LVGL interface.

Examples:

* Zone list
* Icons
* Runtime
* Flow
* Pressure
* Water Usage
* Health Status

The UI shall never access storage directly.

---

# 6.20 Diagnostics

Zone diagnostics include:

* Relay Assignment
* Last Runtime
* Last Water Usage
* Average Flow
* Average Pressure
* Alarm Count
* Last Fault
* Communication Status

These values shall be available through:

* Local Display
* MQTT
* Diagnostics Manager

---

# 6.21 Configuration Validation

The Zone Manager shall validate:

* Relay uniqueness
* Runtime limits
* Hydraulic limits
* Plant parameters
* Soil parameters

Invalid configurations shall be rejected before being committed.

---

# 6.22 Future Expansion

The architecture supports future enhancements including:

* Soil moisture sensor assignment per zone
* Multiple valves per zone
* Smart fertigation parameters
* Dynamic hydraulic balancing
* AI-generated irrigation recommendations
* GIS/Garden map integration
* Water cost optimization

---

# 6.23 Unit Testing

The Zone Manager shall include automated tests covering:

* Configuration loading
* Configuration migration
* Enable/Disable
* Statistics
* Flow Learning storage
* Runtime calculations
* Validation logic
* Persistent storage

Minimum code coverage:

**90%**

---

# 6.24 Chapter Summary

The Zone Manager serves as the authoritative source of all irrigation zone information within the Zmartify firmware.

By separating configuration, runtime data, hydraulic baselines and historical statistics from the Irrigation Engine, the architecture maintains a clear separation of responsibilities while enabling future expansion, simplified testing and efficient smart-home integration.

The Zone Manager forms the foundation upon which intelligent irrigation decisions, water budgeting and advanced analytics are built.

---

# End of Chapter 6

**Next Chapter**

**Chapter 7 – Relay Manager Architecture**

# Zmartify Irrigation Controller (ZIC)
# Complete Documentation Package v3.0

## Document Set
1. Software Requirements Specification (SRS)
2. Engineering Architecture
3. Electrical Design Specification (EDS)
4. MQTT Specification
5. HomeIO Integration Specification
6. UI/UX Specification
7. ET & Weather Engine Specification
8. Alarm & Logging Specification
9. Testing & Acceptance Specification

---

# Executive Summary

Zmartify Irrigation Controller (ZIC) is a professional irrigation platform based on ESP32-S3 and ESP-IDF for large residential gardens and landscape installations.

Key Features:
- 15 irrigation zones
- 1 master valve
- Flow monitoring
- Pressure monitoring
- Weather-aware scheduling
- FAO-56 ET irrigation adjustment
- MQTT-first architecture
- HomeIO integration
- Home Assistant MQTT Discovery
- Homey integration
- 7-inch touchscreen UI
- Historical analytics
- OTA updates
- Multi-controller ready

---

# Hardware Platform

## Main Controller
Waveshare ESP32-S3 7-inch Display

## Expansion
MCP23017 + ULN2803A

## Outputs
Relay 0 = Master Valve
Relay 1-15 = Zones

## Inputs
- DN50 G2 Flow Meter
- 0-10 bar Pressure Sensor
- 4 Push Buttons
- Future rain sensor
- Future soil moisture sensors

---

# MQTT Architecture

Root:
zmartify/irrigation/controller_01

Topics:
- availability
- state
- events
- alarms
- weather
- flow
- pressure
- statistics
- zone/1..15
- command/start_zone
- command/stop_zone
- command/run_program
- command/stop_all
- command/rain_delay

QoS:
Commands = 1
Events = 1
Telemetry = 0

---

# HomeIO Integration

HomeIO receives:
- Zone status
- Alarms
- Water usage
- Weather
- Flow
- Pressure

HomeIO sends:
- Commands
- Schedule changes
- Configuration updates

---

# User Interface Specification

## Dashboard
- Current weather
- Active zone
- Remaining runtime
- Flow gauge
- Pressure gauge
- Water usage today
- Active alarms

## Zones
- Tile layout
- Status
- Manual start/stop
- Statistics

## Programs
- Calendar editor
- Weekly schedules
- Seasonal adjustment

## Weather
- Current conditions
- 24h forecast
- 7-day forecast
- ET calculations

## Analytics
- Daily/weekly/monthly/yearly water usage
- Runtime history
- Water savings

## Diagnostics
- Sensor health
- Relay health
- MQTT status
- Learn Flow Wizard

---

# ET Engine

Method:
FAO-56 Penman-Monteith

Inputs:
- Temperature
- Humidity
- Wind
- Solar radiation

Outputs:
- Daily ET
- Weekly ET
- Runtime adjustment

Formula:
Runtime = Base Runtime × ET Factor × Crop Factor × Seasonal Factor

---

# Weather Rules

Skip watering:
- Rain threshold exceeded
- Forecast rain probability exceeded

Reduce watering:
- High humidity

Increase watering:
- High ET
- High UV
- High temperature

Suspend watering:
- High wind

Block watering:
- Freezing conditions

---

# Flow Monitoring

Learn Flow:
1. Open master valve
2. Open zone
3. Stabilize 60s
4. Record flow
5. Store baseline

Deviation:
15% Warning
30% Critical

---

# Pressure Monitoring

Detect:
- Low pressure
- High pressure
- Pressure collapse
- Oscillation

Actions:
- Warning
- Pause
- Shutdown
- Emergency stop

---

# Alarm System

Critical:
- LEAK_DETECTED
- PIPE_BREAK
- MASTER_VALVE_FAILURE
- EMERGENCY_STOP

Warning:
- HIGH_FLOW
- LOW_FLOW
- HIGH_PRESSURE
- LOW_PRESSURE
- MQTT_OFFLINE

Info:
- RAIN_DELAY_ACTIVE
- OTA_COMPLETE

---

# Logging

Irrigation Log
Alarm Log
Audit Log
Weather Log

Retention Target:
10,000+ entries

Export:
CSV
JSON
MQTT

---

# Electrical Design

Power:
230VAC -> 24VAC Transformer
230VAC -> 5VDC PSU

Protection:
- Breaker
- Fuse
- MOV
- TVS
- Watchdog

Enclosure:
IP65 DIN-Rail Cabinet

---

# ESP-IDF Components

- zone_manager
- irrigation_engine
- flow_manager
- pressure_manager
- weather_manager
- et_engine
- mqtt_manager
- storage_manager
- alarm_manager
- ota_manager
- ui_manager

Communication:
FreeRTOS queues and events

---

# State Machines

Controller:
BOOT
INIT
IDLE
RUNNING
PAUSED
RAIN_DELAY
FAULT
EMERGENCY_STOP

Zone:
DISABLED
OFF
STARTING
RUNNING
STOPPING
FAULT

---

# Security

Roles:
Viewer
Operator
Administrator

Authentication:
PIN
MQTT credentials

---

# OTA

Partitions:
Factory
OTA_A
OTA_B

Features:
Rollback
Signature verification
Remote update

---

# Future Product Family

ZIC-S3  Irrigation Controller
ZWS-S3  Weather Station
ZFS-S3  Flow Sensor Module
ZSM-S3  Soil Moisture Module
ZGW-S3  MQTT Gateway

---

# Acceptance Criteria

- 7-day stability test
- Power recovery test
- MQTT recovery test
- Flow alarm test
- Pressure alarm test
- Emergency stop test
- OTA rollback test

---

# Roadmap

v3.1
- Soil moisture support
- Garden map

v3.2
- Multi-controller clusters

v4.0
- AI irrigation optimization
- Fertigation support

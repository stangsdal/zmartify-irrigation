# Chapter 3 – Product Scope, Stakeholders & Use Cases

---

# 3 Product Scope

## 3.1 Introduction

This chapter defines the intended scope of the Zmartify Irrigation Controller (ZIC) project.

The scope establishes the functional boundaries of the system, identifies its primary stakeholders and describes the operational scenarios that the controller is expected to support.

The objective is to ensure a common understanding between product owner, developers, installers and future users.

---

# 3.2 Product Scope Statement

The ZIC platform is designed as a professional irrigation controller capable of autonomously managing landscape irrigation while continuously optimizing water usage based on measured environmental and hydraulic conditions.

The controller shall:

* Operate independently
* Integrate with Smart Home systems
* Continuously monitor system health
* Provide local operation through an integrated touchscreen
* Support future hardware expansion
* Operate safely under all conditions

The controller is intended to remain operational throughout the entire irrigation season with minimal user intervention.

---

# 3.3 Included Features

The following functionality is included within Version 5.0.

## Irrigation Control

* 15 irrigation zones
* 1 dedicated master valve
* Sequential watering
* Configurable simultaneous watering
* Manual operation
* Scheduled operation
* Cycle & Soak
* Seasonal adjustment
* ET adjustment
* Rain delay

---

## Hydraulic Monitoring

* Continuous flow monitoring
* Continuous pressure monitoring
* Learned hydraulic baselines
* Leak detection
* Pipe burst detection
* Valve failure detection
* Master valve supervision

---

## Weather Awareness

Supported inputs include:

* Temperature
* Humidity
* Rainfall
* Wind speed
* Wind gust
* UV index
* Solar radiation
* Forecast rainfall
* Forecast temperature

The controller shall combine local weather station data with online forecast services when available.

---

## Smart Home Integration

Supported platforms include:

* HOMEIO
* Home Assistant
* Homey
* Node-RED
* Custom MQTT clients

MQTT shall be the primary integration interface.

---

## Local User Interface

Integrated 7-inch capacitive touchscreen.

Functions include:

* Dashboard
* Zone control
* Program editing
* Weather overview
* Water usage
* Alarm handling
* Diagnostics
* Settings

Four local pushbuttons provide:

* Wake/Home
* Manual Start
* Stop
* Emergency Stop

---

## System Services

* OTA firmware updates
* Configuration backup
* Event logging
* Alarm history
* Water usage statistics
* Diagnostics
* Self-test
* Health monitoring

---

# 3.4 Excluded Features

The following functionality is outside the scope of Version 5.0.

## Fertigation

No fertilizer dosing.

Future Version:
Possible.

---

## Pump Speed Control

No VFD control.

Future Version:
Possible via Modbus.

---

## Agricultural Pivot Systems

Not supported.

---

## Multi-Pump Synchronization

Not supported.

---

## Autonomous Cloud Operation

The controller shall never require cloud services for normal operation.

---

# 3.5 System Boundaries

The ZIC controller is responsible for:

* Irrigation decisions
* Valve control
* Sensor acquisition
* Alarm generation
* Event logging
* MQTT communication

External systems are responsible for:

* Water supply
* Electrical installation
* Smart Home automation logic
* Internet connectivity
* Weather provider availability

---

# 3.6 Stakeholders

## Product Owner

Responsibilities:

* Product roadmap
* Feature approval
* Budget
* Release planning

---

## Homeowner

Primary objectives:

* Reliable irrigation
* Water savings
* Easy operation
* Visibility
* Notifications

---

## Landscape Installer

Primary objectives:

* Fast installation
* Easy commissioning
* Reliable diagnostics
* Simple maintenance

---

## Firmware Developer

Primary objectives:

* Modular software
* Maintainability
* Testability
* Safety

---

## Hardware Engineer

Primary objectives:

* Electrical robustness
* EMC compliance
* Expandability
* Serviceability

---

## System Integrator

Primary objectives:

* MQTT
* HOMEIO
* Home Assistant
* Automation support

---

## Service Technician

Primary objectives:

* Diagnostics
* Calibration
* Maintenance
* Repair

---

# 3.7 Primary Use Cases

## UC-001

### Automatic Irrigation

Actor:

Controller

Description:

The controller automatically starts irrigation according to configured programs while considering weather conditions and hydraulic status.

Success Criteria:

No user interaction required.

---

## UC-002

### Manual Zone Start

Actor:

User

Description:

The user manually starts one irrigation zone using the touchscreen.

Expected Result:

* Master valve opens.
* Selected zone opens.
* Flow verified.
* Pressure verified.
* Runtime countdown displayed.

---

## UC-003

### Weather Delay

Actor:

Weather Engine

Description:

Forecast rainfall exceeds configured threshold.

Expected Result:

Program skipped.

MQTT event published.

---

## UC-004

### Leak Detection

Actor:

Flow Manager

Description:

Measured flow exceeds learned baseline.

Expected Result:

* Alarm generated.
* Irrigation stopped.
* Master valve closed.
* MQTT notification.
* Event logged.

---

## UC-005

### Pressure Failure

Actor:

Pressure Manager

Description:

Pressure collapses during irrigation.

Expected Result:

Immediate safe shutdown.

---

## UC-006

### Emergency Stop

Actor:

User

Description:

Emergency Stop button pressed.

Expected Result:

* Close master valve.
* Stop all zones.
* Publish alarm.
* Store event.
* Display alarm screen.

---

## UC-007

### OTA Firmware Update

Actor:

Administrator

Description:

Firmware update initiated.

Expected Result:

* Download
* Verification
* Installation
* Reboot
* Self-test
* Resume operation

---

## UC-008

### Power Recovery

Actor:

Controller

Description:

Power restored after outage.

Expected Result:

* Self-test
* Restore configuration
* Verify hardware
* Restore MQTT
* Return to Idle state

---

# 3.8 Operational Modes

The controller shall support the following operating modes.

### Boot

Hardware initialization.

---

### Idle

Waiting for events.

---

### Irrigating

One or more zones active.

---

### Rain Delay

Programs suspended.

---

### Manual Operation

User controlled.

---

### Service Mode

Commissioning and diagnostics.

---

### Alarm Mode

Critical fault handling.

---

### OTA Update

Firmware maintenance.

---

# 3.9 Operating Environment

Designed for installation inside:

* IP65 polycarbonate cabinet
* Technical room
* Pump house
* Garage
* Utility building

Environmental conditions:

Temperature:

-10°C to +50°C

Humidity:

0–95% RH (non-condensing)

Indoor electronics shall be protected from direct water ingress.

---

# 3.10 Design Constraints

The controller shall be based upon the approved hardware baseline.

Mandatory hardware includes:

* Waveshare ESP32-S3 7"
* MCP23017
* ULN2803A
* HL-58S relay modules
* Hager ST315
* Mean Well HDR-30-5

Firmware shall be developed exclusively using ESP-IDF.

Arduino framework shall not be used.

---

# 3.11 Product Success Metrics

The project shall be considered successful when the following objectives are achieved:

### Reliability

Continuous seasonal operation without critical failures.

---

### Water Efficiency

Lower water consumption compared with fixed-time irrigation.

---

### Safety

Automatic detection and handling of hydraulic failures.

---

### Maintainability

Easy firmware updates and diagnostics.

---

### Expandability

Future hardware modules can be integrated without redesigning the controller.

---

### User Experience

A homeowner shall be able to perform the most common irrigation tasks with no more than two touches on the display.

---

# 3.12 Chapter Summary

This chapter defines the operational scope of the Zmartify Irrigation Controller and establishes the responsibilities, stakeholders and primary operating scenarios that guide all subsequent engineering decisions.

The defined scope forms the foundation for the Functional Requirements Specification presented in the following chapters and ensures that future hardware and firmware development remains aligned with the overall product vision.

---

# End of Chapter 3

**Next Chapter**

**Chapter 4 – Functional Requirements Specification (FRS)**

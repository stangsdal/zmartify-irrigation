# Chapter 6 – System Hardware Baseline & Platform Specification

---

# 6 System Hardware Baseline

## 6.1 Purpose

This chapter defines the approved hardware baseline for the Zmartify Irrigation Controller (ZIC-S3 Rev.B).

The purpose of the hardware baseline is to establish a stable engineering platform upon which all firmware development, testing and future hardware revisions shall be based.

Only components defined in this chapter shall be considered part of the official hardware platform for Version 5.0.

---

# 6.2 Hardware Design Philosophy

The hardware platform has been designed according to the following principles:

### HW-001 – Industrial Reliability

The controller shall be capable of continuous seasonal operation without requiring daily maintenance.

---

### HW-002 – Modular Design

Where practical, commercially available modules shall be used during Revision B development.

This approach provides:

* Rapid development
* Easy replacement
* Simplified troubleshooting
* Lower development cost
* Straightforward migration to a future custom PCB

---

### HW-003 – Electrical Isolation

High-voltage, low-voltage and communication circuits shall be physically and electrically separated.

The controller shall maintain clear separation between:

* 230 VAC mains
* 24 VAC irrigation power
* 5 VDC logic
* Sensor interfaces

---

### HW-004 – Serviceability

Any field-replaceable module shall be removable without requiring replacement of the entire controller.

Modules include:

* Power supplies
* Relay boards
* ESP32 controller
* I/O expansion
* Sensors

---

# 6.3 Approved Hardware Platform

The following hardware is approved for ZIC Version 5.0.

| Item                | Approved Device                     |
| ------------------- | ----------------------------------- |
| Main Controller     | Waveshare ESP32-S3 7" Touch Display |
| GPIO Expansion      | MCP23017                            |
| Relay Driver        | ULN2803A                            |
| Relay Modules       | 2 × HL-58S V1.2                     |
| Valve Transformer   | Hager ST315                         |
| Logic Power Supply  | Mean Well HDR-30-5                  |
| Flow Sensor         | DN50 G2 Turbine Flow Meter          |
| Pressure Interface  | ADS1115                             |
| Pressure Sensor     | 0–10 bar Industrial Transmitter     |
| Cabinet Temperature | MCP9808                             |
| Door Switch         | Reed Switch                         |
| 24 VAC Monitor      | Optically isolated voltage detector |

---

# 6.4 Main Controller Platform

## HW-CPU-001

### Controller

Approved Controller

**Waveshare ESP32-S3 Display Development Board**

Primary characteristics:

* ESP32-S3 Dual Core LX7
* 240 MHz
* Integrated Wi-Fi
* Integrated Bluetooth
* 7" capacitive touch display
* 1024 × 600 LCD
* USB programming
* ESP-IDF compatible

---

## HW-CPU-002

### Controller Responsibilities

The ESP32 shall perform:

* Irrigation control
* Touchscreen interface
* MQTT communication
* OTA updates
* Sensor acquisition
* Event processing
* Data logging
* Weather processing

No secondary MCU is required.

---

# 6.5 GPIO Expansion

## HW-IO-001

Approved Device

Microchip MCP23017

Purpose:

Provide sixteen additional digital outputs through I²C.

---

## HW-IO-002

Allocated Outputs

| Output   | Function     |
| -------- | ------------ |
| Relay 0  | Master Valve |
| Relay 1  | Zone 1       |
| Relay 2  | Zone 2       |
| Relay 3  | Zone 3       |
| Relay 4  | Zone 4       |
| Relay 5  | Zone 5       |
| Relay 6  | Zone 6       |
| Relay 7  | Zone 7       |
| Relay 8  | Zone 8       |
| Relay 9  | Zone 9       |
| Relay 10 | Zone 10      |
| Relay 11 | Zone 11      |
| Relay 12 | Zone 12      |
| Relay 13 | Zone 13      |
| Relay 14 | Zone 14      |
| Relay 15 | Zone 15      |

---

## HW-IO-003

Future Expansion

The MCP23017 interrupt outputs shall remain available for future hardware revisions.

Potential future applications include:

* Digital inputs
* Cabinet alarms
* Expansion boards

---

# 6.6 Relay Driver Stage

## HW-DRV-001

Approved Device

ULN2803A

Purpose:

Provide current amplification and electrical protection between the MCP23017 and the relay modules.

---

## HW-DRV-002

Responsibilities

The ULN2803A shall:

* Sink relay input current
* Protect GPIO outputs
* Improve electrical robustness
* Simplify future PCB integration

---

# 6.7 Relay Modules

## HW-RLY-001

Approved Device

HL-58S V1.2

Installed Quantity

2 modules

Channels

16 total

---

## HW-RLY-002

Electrical Characteristics

* 5 V logic
* Optocoupler isolated
* Active-low inputs
* NO / NC relay contacts

The firmware shall therefore define:

```text
RELAY_ACTIVE_LOW = true
```

---

## HW-RLY-003

Relay Allocation

Relay 0 is permanently reserved for the Master Valve.

Relays 1–15 are assigned to irrigation zones.

This allocation shall not change without a major hardware revision.

---

# 6.8 Valve Power Supply

## HW-PSU-001

Approved Transformer

Hager ST315

Specifications:

* 230 VAC input
* 24 VAC output
* 63 VA
* DIN rail mounting

---

## HW-PSU-002

Design Capacity

The transformer shall support:

* Master valve
* Three simultaneous irrigation valves

Design margin shall exceed 20%.

---

## HW-PSU-003

Future Expansion

Provision shall be made for an optional relay-controlled transformer disconnect, although the transformer will remain permanently energized in Version 5.0.

---

# 6.9 Logic Power Supply

## HW-PSU-004

Approved Power Supply

Mean Well HDR-30-5

Specifications

* 230 VAC input
* 5 VDC output
* 3 A
* DIN rail mounting

---

## HW-PSU-005

Powered Devices

The 5 V supply shall power:

* ESP32 display
* MCP23017
* ULN2803A
* Relay logic
* ADS1115
* MCP9808
* Sensor interfaces

Future expansion capacity shall remain available.

---

# 6.10 Flow Measurement System

## HW-FLOW-001

Approved Sensor

DN50 G2 Turbine Flow Meter

Measurement Range

10–200 L/min

---

## HW-FLOW-002

Measured Values

The controller shall calculate:

* Instantaneous flow
* Average flow
* Daily consumption
* Monthly consumption
* Lifetime consumption

---

## HW-FLOW-003

Interface

Sensor pulses shall be connected to an ESP32 pulse counter (PCNT) input.

Signal conditioning shall include:

* Surge protection
* Noise filtering
* Shielded cable

---

# 6.11 Pressure Measurement

## HW-PRES-001

Approved Sensor

Industrial pressure transmitter

Range

0–10 bar

Output

0.5–4.5 V

---

## HW-PRES-002

ADC

Approved ADC

ADS1115

Resolution

16-bit

Purpose

Provide stable and repeatable pressure measurements exceeding the performance of the ESP32 internal ADC.

---

# 6.12 Cabinet Monitoring

## HW-CAB-001

Temperature

Approved Device

MCP9808

Purpose

Monitor internal cabinet temperature.

Alarm limits:

* Warning: 45°C
* Critical: 55°C

---

## HW-CAB-002

Door Switch

Approved Device

Magnetic Reed Switch

Purpose

Detect:

* Cabinet opening
* Service access
* Tamper events

---

# 6.13 24 VAC Monitoring

## HW-MON-001

The controller shall continuously monitor the presence of the 24 VAC irrigation supply.

Purpose:

* Detect transformer failure
* Detect blown fuse
* Detect disconnected supply

Loss of 24 VAC shall generate a Critical Alarm.

---

# 6.14 Reserved Interfaces

The following interfaces are reserved for future expansion.

### RS485

Purpose:

* Remote valve controllers
* Modbus devices
* Weather stations

---

### CAN Bus

Reserved for future distributed Zmartify devices.

---

### Additional I²C

Reserved for:

* Soil moisture sensors
* Environmental sensors
* Future display peripherals

---

# 6.15 Mechanical Platform

Recommended enclosure:

* Polycarbonate
* IP65
* DIN rail mounted
* Transparent front door preferred

Recommended minimum dimensions:

400 × 300 × 200 mm

Internal layout shall separate:

* 230 VAC
* 24 VAC
* SELV circuits

---

# 6.16 Wi-Fi Design Requirements

Reliable wireless communication is critical for MQTT operation.

The following requirements shall be observed:

* Polycarbonate enclosure
* No metal directly behind the ESP32 antenna
* Minimum 50 mm clearance around antenna
* Transformer mounted away from antenna
* Relay modules separated from antenna region

Target Wi-Fi signal:

RSSI better than **−65 dBm**

If this cannot be achieved, installation of an additional Wi-Fi access point or mesh node is recommended.

---

# 6.17 Hardware Life Expectancy

Design targets:

| Component      |           Target Service Life |
| -------------- | ----------------------------: |
| Controller     |                     >10 years |
| Power Supplies |                     >10 years |
| Relay Modules  | >100,000 operations (minimum) |
| Transformer    |                     >15 years |
| Cabinet        |                     >20 years |

The hardware platform shall be designed to permit replacement of individual modules without replacement of the complete controller.

---

# 6.18 Hardware Baseline Approval

The hardware defined in this chapter constitutes the official baseline for:

**ZIC-S3 Rev.B**

Future hardware changes shall be controlled through Engineering Change Requests (ECRs) and reflected in subsequent revisions of the Master Engineering Package.

---

# 6.19 Chapter Summary

This chapter establishes the approved hardware platform for Version 5.0 of the Zmartify Irrigation Controller.

The selected components provide a modular, serviceable and industrially robust foundation for firmware development and future migration to a fully integrated custom PCB while maintaining compatibility with the approved architecture defined in Chapter 5.

---

# End of Chapter 6

**Next Chapter**

**Chapter 7 – System Software Baseline & Firmware Platform Specification**

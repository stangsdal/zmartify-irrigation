# Zmartify Master Engineering Package v5.0

# Volume 5

# Hardware Platform, Electronics Architecture & PCB Engineering Specification

## Chapter 3

# Power Supply Architecture, Protection Circuits & Power Distribution Network

---

# 3.1 Purpose

This chapter defines the complete power architecture for the Zmartify Irrigation Controller.

The power subsystem is one of the most critical elements of the entire platform. It must reliably power the controller, display, communications, sensors and valve drivers while remaining robust against electrical disturbances commonly encountered in irrigation installations.

The design prioritizes:

* High reliability
* Electrical safety
* Low EMI
* High efficiency
* Long operational life
* Field serviceability

---

# 3.2 Design Objectives

The power subsystem shall:

* Accept industry-standard irrigation supply voltages
* Provide stable regulated outputs
* Protect against electrical faults
* Support simultaneous valve activation
* Minimize conducted and radiated emissions
* Recover automatically after power failures
* Provide diagnostic information to firmware
* Support future hardware expansion

---

# 3.3 Power Architecture

Overall architecture:

```text id="power-architecture"
24 VAC / 24 VDC Input

            │

            ▼

Input Protection

            │

            ▼

Bridge Rectifier

            │

            ▼

Bulk Filtering

            │

            ▼

Primary DC Bus

            │

      ┌─────┼─────────────┐

      ▼     ▼             ▼

12V Rail 5V Rail      Valve Supply

            │

            ▼

3.3V Digital Rail

            │

      ┌─────┼─────────────┐

      ▼     ▼             ▼

ESP32 Display Sensors
```

Each voltage rail shall be independently protected.

---

# 3.4 Input Supply

Primary supply:

| Parameter       | Requirement |
| --------------- | ----------: |
| Nominal Input   |      24 VAC |
| Alternative     |      24 VDC |
| Frequency       |    50/60 Hz |
| Operating Range |   20–30 VAC |

The controller shall automatically detect AC or DC input without user configuration.

---

# 3.5 Input Protection

Protection devices:

* Input fuse
* Reverse polarity protection
* TVS diode
* Surge suppression
* Inrush limiting
* EMI filter

The protection network shall survive typical lightning-induced transients encountered in irrigation systems.

---

# 3.6 AC Rectification

When operating from AC:

```text id="rectification"
24 VAC

↓

Bridge Rectifier

↓

Bulk Capacitor

↓

Primary DC Bus
```

Recommended bridge:

* Low forward voltage
* High surge capability
* Industrial temperature rating

---

# 3.7 Primary DC Bus

Target DC bus voltage:

```text id="primary-bus"
Approximately

33 VDC

(from 24 VAC RMS)
```

Bulk capacitance shall support:

* Valve inrush current
* Display startup
* Brownout immunity

---

# 3.8 Voltage Rails

The controller shall generate the following rails.

| Rail             | Typical Load |
| ---------------- | -----------: |
| Valve Bus        | High Current |
| 12 V             |    Expansion |
| 5 V              | Display, USB |
| 3.3 V            |        Logic |
| Analog Reference |      Sensors |

Voltage ripple shall remain within component specifications.

---

# 3.9 Power Tree

```text id="power-tree"
24 VAC

↓

Rectifier

↓

33V DC

↓

Buck Converter

↓

12V

↓

Buck Converter

↓

5V

↓

LDO / Buck

↓

3.3V
```

High-efficiency synchronous converters are recommended.

---

# 3.10 12V Rail

Applications:

* Expansion modules
* Future Ethernet
* Relay outputs
* External peripherals

Target specifications:

| Parameter | Target |
| --------- | -----: |
| Output    |   12 V |
| Accuracy  |    ±3% |
| Ripple    | <50 mV |

---

# 3.11 5V Rail

Primary loads:

* RGB display
* USB
* Touch controller
* Future accessories

Recommended capacity:

```text id="5v-capacity"
3–5 A

continuous
```

The display backlight shall be supplied from the 5 V rail unless otherwise required.

---

# 3.12 3.3V Rail

Primary loads:

* ESP32-S3
* Sensors
* RS-485 transceiver
* Logic ICs

Recommended specifications:

| Parameter | Target |
| --------- | -----: |
| Output    |  3.3 V |
| Accuracy  |    ±2% |
| Ripple    | <20 mV |

Noise-sensitive analog circuitry may receive additional filtering.

---

# 3.13 Analog Supply

Analog sensors shall receive:

* Filtered supply
* Low-noise reference
* Dedicated decoupling
* Ground isolation where appropriate

Analog performance shall not degrade due to valve switching.

---

# 3.14 Converter Selection

Recommended topology:

| Rail      | Converter                                             |
| --------- | ----------------------------------------------------- |
| 33V → 12V | Synchronous Buck                                      |
| 12V → 5V  | Synchronous Buck                                      |
| 5V → 3.3V | Buck or LDO (depending on efficiency/noise trade-off) |

Converter selection shall prioritize efficiency and long-term availability.

---

# 3.15 Efficiency Targets

Overall conversion efficiency:

| Stage     |                                   Target |
| --------- | ---------------------------------------: |
| 33V → 12V |                                     >90% |
| 12V → 5V  |                                     >92% |
| 5V → 3.3V | >90% (buck) / acceptable LDO dissipation |

Overall system efficiency shall minimize enclosure heating.

---

# 3.16 Power Sequencing

Recommended startup order:

```text id="power-sequencing"
Input Voltage

↓

12V

↓

5V

↓

3.3V

↓

ESP32 Reset Release

↓

Display

↓

Application
```

Power sequencing shall prevent undefined processor startup conditions.

---

# 3.17 Brownout Handling

The power subsystem shall cooperate with firmware.

Brownout sequence:

```text id="brownout-sequence"
Supply Drop

↓

Voltage Detection

↓

Valve Outputs Disabled

↓

Configuration Protected

↓

System Reset

↓

Automatic Restart
```

Brownout events shall be recorded in diagnostics.

---

# 3.18 Power Monitoring

Measured parameters:

* Input Voltage
* 12V Rail
* 5V Rail
* 3.3V Rail
* Controller Current
* Temperature

These measurements shall be accessible through the Diagnostics interface.

---

# 3.19 Valve Supply Isolation

Valve drivers shall be electrically isolated from logic where practical.

Benefits:

* Reduced conducted noise
* Improved EMC
* Improved surge immunity
* Lower reset risk during valve switching

Isolation strategy is further detailed in the Valve Driver chapter.

---

# 3.20 PCB Power Distribution

Guidelines:

* Wide copper pours
* Separate analog and digital supplies
* Dedicated return paths
* Star distribution where appropriate
* Minimize high-current loop areas

Power routing shall be considered before signal routing.

---

# 3.21 Grounding Strategy

Ground domains:

```text id="ground-domains"
Power Ground

↓

Digital Ground

↓

Analog Ground

↓

Shield Ground
```

Ground connections shall be controlled to avoid ground loops and excessive return currents.

---

# 3.22 Decoupling Strategy

Every IC shall include local decoupling.

Typical arrangement:

* 100 nF ceramic close to supply pins
* Bulk capacitor per subsystem
* Additional high-frequency decoupling where required

Placement is more important than capacitance value.

---

# 3.23 Thermal Design

Major heat sources:

* Primary buck converter
* Secondary buck converter
* Valve drivers
* Display backlight supply

Thermal management shall include:

* Copper planes
* Thermal vias
* Airflow consideration
* Component spacing

No heatsinks shall normally be required.

---

# 3.24 EMC Considerations

Power subsystem layout shall minimize:

* Switching noise
* Ground bounce
* EMI radiation
* Conducted emissions

Recommended techniques:

* Short switching loops
* Proper input filtering
* Shielded inductors
* Controlled return paths
* Snubber networks where required

---

# 3.25 Service Features

Power subsystem diagnostics shall support:

* Voltage measurement
* Rail status LEDs (optional)
* Test points
* Current monitoring
* Converter enable signals

Critical rails shall be easily measurable during servicing.

---

# 3.26 Manufacturing Considerations

Production testing shall verify:

* Input polarity
* Converter startup
* Rail voltages
* Ripple
* Protection circuits
* Brownout detection
* Thermal behavior

Power verification shall be completed before firmware programming.

---

# 3.27 Reliability Requirements

Design targets:

| Requirement        |                         Target |
| ------------------ | -----------------------------: |
| Converter MTBF     |                     >100,000 h |
| Input Protection   |               Industrial grade |
| Operating Life     |                      >10 years |
| Capacitor Lifetime | >10 years @ design temperature |
| Brownout Recovery  |                      Automatic |

Long-life capacitors (105°C or higher) are recommended.

---

# 3.28 Relationship to Other Chapters

This power architecture directly supports:

| Chapter   | Relationship        |
| --------- | ------------------- |
| Chapter 2 | ESP32-S3 Core       |
| Chapter 4 | Display Electronics |
| Chapter 5 | Valve Drivers       |
| Chapter 6 | Sensor Interfaces   |
| Chapter 8 | Communications      |
| Volume 7  | EMC & Verification  |

---

# 3.29 Future Expansion

The power architecture has been designed to support future additions including:

* PoE power input
* Solar-powered variants
* Battery backup
* Supercapacitor ride-through
* Redundant power inputs
* Higher-current valve banks
* Intelligent power management
* Energy metering

These features shall integrate without requiring redesign of the core power distribution philosophy.

---

# 3.30 Engineering Notes

The power subsystem forms the electrical backbone of the Zmartify platform. Its design emphasizes robustness against unstable field power, electrical transients and inductive loads while maintaining high efficiency and low thermal dissipation.

By using a staged conversion architecture with dedicated voltage domains, comprehensive protection circuits and careful PCB layout practices, the platform achieves both high reliability and excellent EMC performance. This architecture also provides the flexibility required for future product variants with increased processing capability, additional communication interfaces and expanded I/O capacity.

---

# 3.31 Chapter Summary

This chapter defines the complete power supply architecture for the Zmartify controller, including input protection, voltage conversion, power distribution, grounding strategy and thermal design.

It establishes a robust electrical foundation for the remaining hardware subsystems and ensures reliable operation under the demanding environmental conditions typical of professional irrigation installations.

---

# End of Chapter 3

**Next Chapter**

**Chapter 4 – RGB Display Electronics, GT911 Touch Interface & High-Speed PCB Design**

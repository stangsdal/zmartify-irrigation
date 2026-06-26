# Zmartify Master Engineering Package v5.0

# Volume 5

# Hardware Platform, Electronics Architecture & PCB Engineering Specification

## Chapter 6

# Sensor Interfaces, Analog Front-End, Flow Measurement & Environmental Inputs

---

# 6.1 Purpose

This chapter defines the complete sensor interface architecture for the Zmartify Irrigation Controller.

The sensor subsystem provides the controller with real-time information regarding hydraulic performance, environmental conditions and controller health. These measurements enable intelligent irrigation scheduling, leak detection, predictive maintenance and future AI-assisted optimization.

Unlike traditional irrigation controllers that primarily use binary rain sensors, the Zmartify platform has been designed around a modular sensor architecture capable of supporting both simple digital inputs and precision analog instrumentation.

---

# 6.2 Design Objectives

The sensor subsystem shall:

* Support industrial-grade sensors
* Maintain high measurement accuracy
* Minimize electrical noise
* Detect sensor failures
* Support automatic calibration
* Provide future expansion capability
* Be resistant to outdoor electrical environments
* Support predictive diagnostics

---

# 6.3 Sensor Architecture

```text id="sensor-architecture"
                 Sensors

                     │

      ┌──────────────┼──────────────┐

      ▼              ▼              ▼

 Digital        Analog        Pulse Inputs

      │              │              │

      ▼              ▼              ▼

 Input Filters   Analog Front-End   Counters

             │

             ▼

          ADC Manager

             │

             ▼

      Sensor Manager

             │

             ▼

     Application Managers
```

Only the Sensor Manager shall expose measurements to the application layer.

---

# 6.4 Supported Sensor Types

| Sensor            | Version 5 |  Future  |
| ----------------- | :-------: | :------: |
| Flow Meter        |     ✔     | Enhanced |
| Pressure Sensor   |     ✔     | Enhanced |
| Rain Sensor       |     ✔     |     ✔    |
| Soil Moisture     |  Optional |     ✔    |
| Water Temperature |  Optional |     ✔    |
| Air Temperature   |     ✔     |     ✔    |
| Humidity          |     ✔     |     ✔    |
| Wind Speed        |     ✔     |     ✔    |
| Solar Radiation   |   Future  |     ✔    |
| Tank Level        |  Optional |     ✔    |
| Conductivity (EC) |   Future  |     ✔    |
| pH Sensor         |   Future  |     ✔    |

---

# 6.5 Input Categories

The hardware supports four fundamental input types.

| Interface       | Typical Sensors           |
| --------------- | ------------------------- |
| Digital         | Rain switch, float switch |
| Analog Voltage  | Pressure, level           |
| Pulse/Frequency | Flow meter                |
| I²C             | Environmental sensors     |

Future revisions may support RS-485/Modbus sensors.

---

# 6.6 Analog Front-End Philosophy

The analog subsystem shall:

* Reject electrical noise
* Protect the ADC
* Provide stable measurements
* Minimize temperature drift
* Detect wiring faults

Signal conditioning shall occur before ADC conversion.

---

# 6.7 Analog Input Specifications

Recommended specifications:

| Parameter            |           Target |
| -------------------- | ---------------: |
| Resolution           |   12-bit minimum |
| Effective Resolution |         ≥11 bits |
| Sample Rate          |     Configurable |
| Accuracy             | ±1% FS or better |
| Temperature Drift    |        Minimized |

Oversampling may be employed to improve effective resolution.

---

# 6.8 Recommended Analog Channels

Reference allocation:

| Channel | Function               |
| ------- | ---------------------- |
| ADC1    | Pressure               |
| ADC2    | Supply Voltage         |
| ADC3    | Controller Temperature |
| ADC4    | Expansion              |

Future products may utilize external ADCs for higher precision.

---

# 6.9 Analog Input Circuit

Reference architecture:

```text id="analog-front-end"
Sensor

↓

Transient Protection

↓

RC Filter

↓

Voltage Divider

↓

Buffer (optional)

↓

ADC

↓

ESP32
```

All external analog inputs shall include input protection.

---

# 6.10 Input Protection

Recommended protection devices:

* TVS diode
* Series resistor
* RC low-pass filter
* Clamp diodes
* Reverse polarity protection

The analog front-end shall tolerate accidental wiring faults.

---

# 6.11 Pressure Sensor Interface

Supported pressure sensors:

* 0–5 V
* 0–10 V (with divider)
* 4–20 mA (future option)

Typical range:

| Parameter          |    Value |
| ------------------ | -------: |
| Operating Pressure | 0–10 bar |
| Accuracy           | ±0.5% FS |

Pressure measurements support:

* Leak detection
* Pump monitoring
* Hydraulic health analysis

---

# 6.12 4–20 mA Support (Future)

Industrial installations may use current-loop transmitters.

Reference interface:

```text id="current-loop"
4–20 mA

↓

Precision Shunt

↓

Low-pass Filter

↓

ADC
```

Galvanic isolation is recommended for remote sensors.

---

# 6.13 Flow Measurement

Supported technologies:

| Type              | Support |
| ----------------- | :-----: |
| Hall-effect pulse |    ✔    |
| Reed switch       |    ✔    |
| Turbine           |    ✔    |
| Ultrasonic        |  Future |
| Electromagnetic   |  Future |

Pulse-based flow meters shall be the reference implementation.

---

# 6.14 Pulse Counter Architecture

```text id="pulse-counter"
Flow Meter

↓

Pulse Filter

↓

Schmitt Trigger

↓

ESP32 Pulse Counter

↓

Flow Manager
```

Hardware pulse counting minimizes CPU load.

---

# 6.15 Flow Measurement Accuracy

Design targets:

| Parameter               |       Target |
| ----------------------- | -----------: |
| Resolution              |    0.1 L/min |
| Accuracy                |          ±2% |
| Maximum Frequency       |        5 kHz |
| Minimum Detectable Flow | Configurable |

Calibration factors shall be stored in non-volatile memory.

---

# 6.16 Rain Sensor Interface

Supported devices:

* Normally Open
* Normally Closed
* Resistive rain sensors (future)

Rain inputs shall include:

* Debouncing
* Surge protection
* Cable fault detection (future)

---

# 6.17 Environmental Sensors

Typical interfaces:

| Sensor      | Interface      |
| ----------- | -------------- |
| Temperature | I²C            |
| Humidity    | I²C            |
| Pressure    | I²C            |
| Light       | I²C            |
| Wind        | Pulse / RS-485 |

Environmental measurements supplement weather API data and continue operating if cloud connectivity is unavailable.

---

# 6.18 Soil Moisture Sensors

Supported technologies:

* Capacitive
* SDI-12 (future)
* RS-485 Modbus (future)

Resistive probes are not recommended due to electrode corrosion.

---

# 6.19 Water Temperature

Applications include:

* Frost protection
* Irrigation optimization
* Diagnostics

Typical sensor:

```text id="temperature-sensor"
DS18B20

or

I²C Digital Sensor
```

---

# 6.20 Sensor Power Distribution

Sensor supplies:

| Voltage | Purpose                     |
| ------- | --------------------------- |
| 3.3 V   | Digital sensors             |
| 5 V     | Legacy sensors              |
| 12 V    | Industrial sensors (future) |

Unused sensor power may be disabled for energy savings.

---

# 6.21 Noise Reduction

Recommended techniques:

* RC filtering
* Shielded cables
* Differential routing
* Star grounding
* Twisted pair wiring
* Ferrite beads

Analog signals shall remain physically separated from valve outputs.

---

# 6.22 Connector Strategy

Sensor connectors shall be:

* Polarized
* Numbered
* Replaceable
* Field accessible
* Clearly labeled

Recommended connector family:

* Phoenix Contact
* Würth
* TE Connectivity

Final selection shall depend on enclosure design.

---

# 6.23 Calibration

Supported calibration methods:

* Factory calibration
* Installer calibration
* Automatic zero calibration
* Flow calibration
* Pressure calibration

Calibration values shall be versioned and backed up.

---

# 6.24 Fault Detection

Detectable sensor faults:

* Open circuit
* Short circuit
* Out-of-range values
* Stuck readings
* Communication timeout
* Calibration failure

Sensor health shall be reported to the Diagnostics Manager.

---

# 6.25 Diagnostic Measurements

The controller shall expose:

* Raw ADC values
* Filtered values
* Calibration factors
* Update rate
* Sensor status
* Signal quality

These values are available in Service Mode.

---

# 6.26 PCB Layout Guidelines

Recommendations:

* Separate analog and digital grounds
* Short analog traces
* Ground guard traces where practical
* Dedicated analog reference area
* Shield high-impedance nodes

The analog front-end shall be positioned away from switch-mode power supplies.

---

# 6.27 Manufacturing Verification

Production testing shall verify:

* ADC accuracy
* Input protection
* Flow pulse counting
* Pressure input
* Sensor power rails
* Connector integrity
* Calibration storage

Automated fixtures shall inject simulated sensor signals.

---

# 6.28 Reliability Requirements

| Requirement             |           Target |
| ----------------------- | ---------------: |
| Analog Accuracy         |           ±1% FS |
| Pulse Counting          |             100% |
| Calibration Retention   | Product lifetime |
| Sensor Supply Stability |              ±2% |

The subsystem shall operate continuously throughout the controller's service life.

---

# 6.29 Future Expansion

The sensor architecture supports future additions including:

* SDI-12 agricultural sensors
* NMEA 2000 environmental sensors
* CAN bus sensor networks
* LoRa wireless sensors
* Bluetooth sensor gateways
* Smart pressure transmitters
* AI-assisted sensor validation
* Distributed sensor modules

The Sensor Manager defined in Volume 2 shall abstract these additions from the application layer.

---

# 6.30 Engineering Recommendations

For production hardware, the following practices are recommended:

* Use precision 0.1% resistors in voltage divider networks
* Employ low-drift reference components for analog measurements
* Provide individual TVS protection on all external sensor inputs
* Reserve PCB space for future 4–20 mA interfaces
* Include configurable filtering in both hardware and firmware
* Design connectors to permit field replacement without PCB removal

These recommendations improve long-term measurement stability and facilitate future product evolution.

---

# 6.31 Engineering Notes

The sensor subsystem transforms the Zmartify controller from a simple timer into a measurement-driven irrigation platform. By combining high-quality analog design, modular digital interfaces and robust signal conditioning, the controller gains the ability to monitor hydraulic performance, environmental conditions and equipment health with engineering-grade reliability.

This architecture also provides a clear migration path toward precision agriculture and commercial irrigation applications through support for industrial sensors and intelligent diagnostics.

---

# 6.32 Chapter Summary

This chapter defines the complete sensor interface architecture, including analog front-end design, pulse counting, environmental sensing, calibration and diagnostic capabilities.

The subsystem provides accurate, reliable and extensible measurement capabilities while maintaining electrical robustness and compatibility with future sensor technologies.

---

# End of Chapter 6

**Next Chapter**

**Chapter 7 – Communication Interfaces, RS-485, Expansion Bus & External Device Connectivity**

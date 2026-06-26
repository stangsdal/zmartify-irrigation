# Zmartify Master Engineering Package v5.0

# Volume 5

# Hardware Platform, Electronics Architecture & PCB Engineering Specification

## Chapter 5

# Valve Driver Electronics, Solenoid Outputs & High-Current Switching Architecture

---

# 5.1 Purpose

This chapter defines the complete design of the valve driver subsystem used by the Zmartify Irrigation Controller.

The valve driver hardware is responsible for controlling industry-standard **24 VAC irrigation solenoid valves** while maintaining complete electrical isolation between the high-energy switching circuitry and the controller electronics.

Unlike many residential irrigation controllers that use simple TRIAC switching, the Zmartify platform adopts a modular driver architecture designed for diagnostics, future current monitoring, predictive maintenance and expansion to commercial installations.

The subsystem supports:

* Individual valve control
* Master valve control
* Pump relay output
* Output diagnostics
* Electrical fault detection
* Future current sensing
* Safe shutdown
* High EMC immunity

---

# 5.2 Design Objectives

The valve driver subsystem shall:

* Support standard 24 VAC irrigation valves
* Operate continuously for more than 10 years
* Survive inductive switching transients
* Prevent controller resets during switching
* Detect output faults
* Support future current monitoring
* Minimize conducted emissions
* Be field serviceable

---

# 5.3 System Architecture

```text id="valve-architecture"
                ESP32-S3

                    │

          Valve Driver Manager

                    │

           Digital Output Bus

                    │

        ┌───────────┴────────────┐

        ▼                        ▼

 Driver Electronics        Diagnostics

        │

        ▼

 Isolation Stage

        │

        ▼

 High Voltage Switching

        │

        ▼

 Valve Connector

        │

        ▼

24 VAC Solenoid Valve
```

Application firmware communicates only with the Driver Manager.

---

# 5.4 Supported Outputs

The reference platform supports:

| Output            | Quantity |
| ----------------- | -------: |
| Irrigation Zones  |        8 |
| Master Valve      |        1 |
| Pump Relay        |        1 |
| Auxiliary Outputs |   Future |

Expansion modules shall increase available zone outputs without modifying the CPU hardware.

---

# 5.5 Electrical Characteristics

Reference valve specifications:

| Parameter       |    Typical |
| --------------- | ---------: |
| Voltage         |     24 VAC |
| Inrush Current  | 250–400 mA |
| Holding Current | 180–300 mA |
| Frequency       |   50/60 Hz |

The design shall accommodate the highest expected inrush current.

---

# 5.6 Switching Philosophy

The switching architecture shall:

* Electrically isolate logic from field wiring
* Protect against inductive transients
* Minimize switching losses
* Prevent unintended activation
* Allow deterministic timing

Valve outputs shall default to OFF during reset.

---

# 5.7 Recommended Switching Topology

The preferred implementation is based on **solid-state switching**, with the exact device selected according to the final valve power architecture.

Supported implementations include:

* TRIAC (24 VAC native systems)
* Solid-State Relay (SSR)
* Optically isolated TRIAC driver
* MOSFET H-bridge (future DC variants)

The production design shall prioritize long-term reliability and EMC performance over minimum BOM cost.

---

# 5.8 Output Circuit Architecture

Reference output structure:

```text id="output-circuit"
ESP32 GPIO

↓

Opto-Isolation

↓

Gate Driver

↓

Power Switch

↓

Protection Network

↓

Valve Connector
```

Each output channel shall be electrically independent.

---

# 5.9 Isolation Strategy

Electrical isolation protects:

* ESP32
* Display
* Sensors
* Communications

Isolation options:

* Optocouplers
* Digital isolators
* Isolated gate drivers (future)

The minimum isolation voltage shall comply with applicable safety standards.

---

# 5.10 Output Protection

Each valve channel shall include:

* Over-voltage protection
* Surge suppression
* Snubber network
* Current limiting (future)
* Thermal protection
* Reverse energy protection

Protection devices shall survive repeated inductive switching.

---

# 5.11 Snubber Network

Recommended protection:

```text id="snubber"
Valve Output

↓

RC Snubber

↓

MOV / TVS

↓

Connector
```

The final values shall be optimized during EMC verification.

---

# 5.12 Connector Design

Valve connector requirements:

* Screw terminals
* Clearly numbered
* Replaceable
* Finger-safe
* Suitable for outdoor installations

Recommended conductor size:

```text id="wire-size"
0.5 – 2.5 mm²
```

---

# 5.13 Master Valve Output

Dedicated master valve output.

Characteristics:

* Independent driver
* Configurable enable
* Configurable delay
* Diagnostic monitoring

The master valve output shall not share hardware with standard zones.

---

# 5.14 Pump Relay Output

Pump output characteristics:

| Parameter          | Requirement |
| ------------------ | ----------- |
| Isolated           | Yes         |
| Relay Contact      | Dry Contact |
| Configurable Delay | Yes         |
| Diagnostic Status  | Future      |

Pump output timing is defined by the firmware architecture in Volume 2.

---

# 5.15 Current Monitoring (Future)

Future hardware revisions may include current sensing.

Measured values:

* Inrush current
* Holding current
* Leakage current
* Open circuit
* Short circuit

Example:

```text id="current-monitoring"
Valve

↓

Current Sensor

↓

ADC

↓

Diagnostics

↓

Firmware
```

Current monitoring enables predictive maintenance.

---

# 5.16 Fault Detection

Detectable faults include:

* Open circuit
* Short circuit
* Stuck valve
* Wiring fault
* Driver failure
* Over-temperature

Detected faults shall be reported through the Diagnostics framework.

---

# 5.17 Timing Requirements

Valve activation timing:

| Parameter                    | Target |
| ---------------------------- | -----: |
| Turn-On Delay                | <20 ms |
| Turn-Off Delay               | <20 ms |
| Channel-to-Channel Variation |  <2 ms |

Timing shall remain deterministic across all operating conditions.

---

# 5.18 Simultaneous Operation

The controller shall support:

* One active irrigation zone
* Master valve
* Pump relay

Future commercial controllers may support multiple simultaneous zones.

Power distribution shall be designed accordingly.

---

# 5.19 EMC Considerations

Valve switching generates significant electrical noise.

Recommended practices:

* Separate power planes
* RC snubbers
* MOV protection
* Short switching loops
* Ground segregation
* Filtered power input

High-current traces shall remain isolated from analog circuitry.

---

# 5.20 PCB Layout

Guidelines:

* Wide copper traces
* High-current polygon pours
* Thermal reliefs
* Creepage and clearance compliance
* Physical separation from ESP32

Power routing shall precede signal routing.

---

# 5.21 Thermal Design

Heat-producing components:

* Power switches
* Snubber resistors
* Protection devices

Thermal management:

* Copper pours
* Thermal vias
* Air circulation
* Conservative component derating

Component junction temperatures shall remain within manufacturer specifications.

---

# 5.22 Status Indicators

Optional status LEDs:

| LED          | Meaning |
| ------------ | ------- |
| Zone Active  | Green   |
| Master Valve | Blue    |
| Pump         | White   |
| Fault        | Red     |

LEDs shall be visible during servicing.

---

# 5.23 Diagnostic Test Points

Recommended test points:

* Valve Supply
* Driver Output
* Logic Input
* Isolation Output
* Ground Reference

Test points shall support automated production testing.

---

# 5.24 Manufacturing Verification

Production testing shall verify:

* Output switching
* Isolation
* Leakage current
* Protection circuitry
* Connector integrity
* Output timing
* Thermal behavior

Each channel shall be tested individually.

---

# 5.25 Reliability Requirements

| Requirement        |           Target |
| ------------------ | ---------------: |
| Switching Cycles   |      >10 million |
| Service Life       |        >10 years |
| Isolation Lifetime | Product lifetime |
| Output Accuracy    |    Deterministic |

Components shall be selected with generous derating.

---

# 5.26 Safety Considerations

Valve outputs shall fail safely.

During:

* Reset
* Firmware update
* Brownout
* Watchdog reset
* Bootloader

All outputs shall default to the OFF state unless explicitly controlled by the irrigation engine.

Unexpected valve activation shall be prevented by hardware as well as firmware.

---

# 5.27 Relationship to Firmware

This subsystem interfaces directly with:

| Volume   | Relationship                    |
| -------- | ------------------------------- |
| Volume 2 | Irrigation Engine               |
| Volume 4 | Diagnostics & Service Interface |
| Volume 7 | Electrical Verification         |
| Volume 9 | Installation                    |

The Driver Manager provides the only firmware interface to valve hardware.

---

# 5.28 Future Expansion

The architecture supports future enhancements including:

* Latching solenoid support
* 12 VDC valve variants
* Variable-current valve diagnostics
* Automatic wiring identification
* Smart valve modules
* Distributed remote valve controllers
* CAN-based output expansion
* Per-channel energy metering

These additions shall integrate without changing the Application Manager interfaces.

---

# 5.29 Engineering Recommendations

For the production controller, the following design practices are recommended:

* Optically isolated outputs for field wiring
* Replaceable terminal blocks
* Individually protected output channels
* Conservative component derating (≥50% where practical)
* Industrial-grade surge suppression
* PCB spacing exceeding minimum creepage requirements
* Comprehensive diagnostic access for manufacturing and service

Although these recommendations slightly increase the bill of materials, they substantially improve field reliability, EMC robustness and serviceability.

---

# 5.30 Engineering Notes

The valve driver subsystem represents the primary interface between the controller and the external irrigation infrastructure. It must therefore tolerate harsh electrical environments while providing deterministic switching performance and protecting the controller from field-induced disturbances.

The modular architecture defined in this chapter also prepares the platform for future diagnostic capabilities, including per-valve current monitoring and predictive maintenance, allowing the controller to identify deteriorating valves or wiring before complete failures occur.

---

# 5.31 Chapter Summary

This chapter defines the valve driver electronics, switching architecture and protection strategy for the Zmartify controller.

By combining electrically isolated outputs, robust protection networks, deterministic timing and a scalable architecture, the subsystem provides a reliable interface to irrigation valves while supporting future commercial expansion and advanced diagnostic capabilities.

---

# End of Chapter 5

**Next Chapter**

**Chapter 6 – Sensor Interfaces, Analog Front-End, Flow Measurement & Environmental Inputs**

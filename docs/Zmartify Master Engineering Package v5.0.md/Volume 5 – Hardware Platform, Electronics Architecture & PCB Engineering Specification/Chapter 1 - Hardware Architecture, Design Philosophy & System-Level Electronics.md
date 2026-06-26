# Zmartify Master Engineering Package v5.0

# Volume 5

# Hardware Platform, Electronics Architecture & PCB Engineering Specification

## Chapter 1

# Hardware Architecture, Design Philosophy & System-Level Electronics

---

# 1.1 Purpose

This volume defines the complete hardware architecture for the Zmartify Irrigation Controller platform.

While previous volumes defined the software, firmware and HMI architectures, this volume specifies the physical electronics required to realize those designs in a reliable, manufacturable and serviceable product.

The hardware platform has been designed around a modular architecture capable of supporting multiple controller variants ranging from residential installations to professional irrigation systems.

This chapter establishes the engineering philosophy, architectural principles and system-level electronics upon which the remaining hardware design is based.

---

# 1.2 Scope

This volume specifies:

* Hardware architecture
* PCB design
* Power distribution
* ESP32-S3 integration
* Display electronics
* RS-485 interface
* Ethernet (future)
* Expansion interfaces
* Valve drivers
* Sensor interfaces
* Protection circuitry
* Manufacturing considerations
* EMC design
* Reliability engineering

---

# 1.3 Design Objectives

The hardware platform shall:

* Operate continuously for more than 10 years
* Support unattended outdoor installations
* Be service friendly
* Be modular
* Minimize component count
* Support OTA firmware
* Be manufacturable at scale
* Support future controller variants

---

# 1.4 System Philosophy

The Zmartify controller is designed as an industrial embedded controller rather than a consumer appliance.

Every subsystem shall prioritize:

* Reliability
* Maintainability
* Deterministic behavior
* EMC robustness
* Field serviceability
* Manufacturing repeatability

Performance optimization shall never compromise operational reliability.

---

# 1.5 Hardware Architecture

Overall architecture:

```text id="system-architecture"
                 Power Input

                      │

                      ▼

              Power Management

                      │

      ┌───────────────┼────────────────┐

      ▼               ▼                ▼

 ESP32-S3        Display PCB      Expansion Bus

      │               │                │

      ▼               ▼                ▼

 Communications    Touch LCD      External Modules

      │

      ▼

 Valve Drivers

      │

      ▼

 Solenoid Valves

      │

      ▼

 Irrigation System
```

Each functional block shall remain electrically independent where practical.

---

# 1.6 Functional Subsystems

The controller consists of the following major subsystems.

| Subsystem      | Responsibility            |
| -------------- | ------------------------- |
| CPU            | Control and processing    |
| Display        | Human interface           |
| Power          | Voltage regulation        |
| Communications | Wi-Fi, MQTT, RS-485       |
| Valve Drivers  | Solenoid control          |
| Sensors        | Analog and digital inputs |
| Expansion      | Future peripherals        |
| Diagnostics    | Service interface         |

Subsystem interfaces shall be well-defined and documented.

---

# 1.7 Modular PCB Architecture

The hardware is divided into logical modules.

```text id="pcb-modules"
Main Controller PCB

│

├── CPU Module

├── Power Module

├── Driver Module

├── Sensor Module

├── Communications

└── Expansion
```

Future revisions may separate the Display PCB.

---

# 1.8 Hardware Layers

The design follows layered abstraction similar to the firmware architecture.

```text id="hardware-layers"
Application

↓

Firmware

↓

HAL

↓

Drivers

↓

Electronics

↓

Physical Interfaces
```

Each layer minimizes dependencies on adjacent layers.

---

# 1.9 Product Variants

The architecture supports multiple controller models.

| Model      | Zones | Display  | Expansion |
| ---------- | ----: | -------- | --------- |
| Home       |     8 | Yes      | Basic     |
| Home+      |    16 | Yes      | Standard  |
| Pro        |    24 | Yes      | Advanced  |
| Commercial |   48+ | External | Full      |

All variants shall share the same software architecture where practical.

---

# 1.10 Hardware Platform

Initial production platform:

| Component      | Selection     |
| -------------- | ------------- |
| MCU            | ESP32-S3      |
| Display        | 7" RGB TFT    |
| Touch          | GT911         |
| Flash          | Integrated    |
| PSRAM          | Integrated    |
| Communications | Wi-Fi 2.4 GHz |
| Expansion      | RS-485        |

Future processor upgrades shall require minimal PCB redesign.

---

# 1.11 Electrical Domains

The PCB shall separate electrical domains.

```text id="power-domains"
24V Input

↓

Power Stage

↓

12V Domain

↓

5V Domain

↓

3.3V Digital

↓

Analog Reference
```

Ground separation shall be maintained where required.

---

# 1.12 Voltage Rails

Primary voltage rails:

| Rail             | Purpose                 |
| ---------------- | ----------------------- |
| 24 V             | Valve power             |
| 12 V             | Expansion & peripherals |
| 5 V              | Display & USB           |
| 3.3 V            | Digital electronics     |
| Analog Reference | Sensors                 |

Each rail shall be individually protected.

---

# 1.13 PCB Design Philosophy

The PCB shall emphasize:

* Short signal paths
* Low EMI
* Thermal efficiency
* Mechanical robustness
* Ease of assembly
* Automated testing
* Service accessibility

High-current and low-level analog signals shall be physically separated.

---

# 1.14 Connector Philosophy

Connectors shall be:

* Clearly labeled
* Keyed where possible
* Locking where appropriate
* Field replaceable
* Accessible after installation

Connector orientation shall simplify field wiring.

---

# 1.15 Expansion Strategy

The controller shall support future expansion modules.

Examples:

* Additional valve outputs
* Ethernet
* LoRa
* Cellular
* CAN Bus
* RS-232
* Additional sensor inputs
* AI accelerator

Expansion shall not require redesign of the main controller PCB.

---

# 1.16 Environmental Requirements

Operating conditions:

| Parameter             |                Requirement |
| --------------------- | -------------------------: |
| Operating Temperature |             -20°C to +60°C |
| Storage Temperature   |             -40°C to +85°C |
| Relative Humidity     |     5–95% (non-condensing) |
| Installation          | Indoor / Protected Outdoor |

The hardware shall tolerate typical irrigation controller environments.

---

# 1.17 Mechanical Design

PCB design shall support:

* DIN rail mounting (future)
* Wall mounting
* Service replacement
* Adequate ventilation
* Cable strain relief

Component placement shall facilitate manufacturing and servicing.

---

# 1.18 Reliability Requirements

Design targets:

| Requirement      |               Target |
| ---------------- | -------------------: |
| Service Life     |            >10 years |
| MTBF             |       >100,000 hours |
| Boot Reliability |                 100% |
| Power Recovery   |            Automatic |
| EEPROM Wear      | Within specification |

Long-term reliability shall take precedence over minimum BOM cost.

---

# 1.19 EMC Design Principles

The hardware shall be designed for compliance with applicable EMC standards.

Key practices include:

* Ground planes
* Controlled return paths
* Decoupling capacitors
* Ferrite filtering
* ESD protection
* Surge protection
* Separation of noisy and sensitive circuits

Formal EMC verification is defined in Volume 7.

---

# 1.20 Serviceability

Field service shall support:

* Firmware recovery
* Diagnostic connectors
* Test points
* Module replacement
* Visual status indicators

Critical components shall remain accessible without complete disassembly.

---

# 1.21 Manufacturing Strategy

The PCB shall be optimized for automated production.

Requirements:

* SMT assembly
* AOI compatibility
* Flying probe testing
* ICT test points
* Functional testing
* Automated firmware programming

Manufacturing documentation is covered in Volume 6.

---

# 1.22 Relationship to Other Volumes

This volume interfaces with:

| Volume   | Relationship                |
| -------- | --------------------------- |
| Volume 1 | Overall system architecture |
| Volume 2 | Firmware platform           |
| Volume 3 | Communications              |
| Volume 4 | Display subsystem           |
| Volume 6 | Manufacturing               |
| Volume 7 | Verification                |
| Volume 9 | Installation                |

The hardware defined here provides the physical implementation for all preceding software architectures.

---

# 1.23 Hardware Roadmap

The architecture has been designed to support future hardware evolution including:

* ESP32-P4 migration
* Gigabit Ethernet
* Secure Elements
* Matter over Thread
* Cellular connectivity
* Edge AI accelerators
* Higher-resolution displays
* Modular I/O expansion

Future enhancements shall preserve the existing hardware abstraction layer wherever practical.

---

# 1.24 Engineering Notes

The Zmartify hardware platform has been conceived as a professional embedded control system rather than a fixed-function irrigation controller. By emphasizing modularity, electrical isolation and long-term maintainability, the architecture provides a stable foundation for multiple product generations while minimizing redesign effort.

The close alignment between the hardware abstraction defined in Volume 2 and the physical architecture specified in this volume ensures that future processor upgrades, communication technologies and expansion modules can be incorporated with limited impact on application software.

---

# 1.25 Chapter Summary

This chapter establishes the system-level hardware architecture and engineering philosophy for the Zmartify platform.

It defines the modular electronics architecture, electrical domains, PCB organization, environmental requirements and long-term design principles that guide the detailed hardware specifications presented in the remaining chapters of this volume.

---

# End of Chapter 1

**Next Chapter**

**Chapter 2 – ESP32-S3 Core Module, Clock Architecture, Memory & Boot Configuration**

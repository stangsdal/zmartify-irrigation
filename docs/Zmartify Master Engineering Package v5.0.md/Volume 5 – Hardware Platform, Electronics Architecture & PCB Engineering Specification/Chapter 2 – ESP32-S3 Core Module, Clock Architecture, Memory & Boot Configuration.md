# Zmartify Master Engineering Package v5.0

# Volume 5

# Hardware Platform, Electronics Architecture & PCB Engineering Specification

## Chapter 2

# ESP32-S3 Core Module, Clock Architecture, Memory & Boot Configuration

---

# 2.1 Purpose

This chapter defines the complete design of the Zmartify CPU subsystem based on the **Espressif ESP32-S3**.

The CPU subsystem forms the computational heart of the controller and has been selected to provide sufficient processing performance, memory capacity, connectivity and long-term software support while maintaining low power consumption and excellent manufacturing availability.

The design presented here establishes the reference hardware for all future Zmartify controller variants.

---

# 2.2 Design Objectives

The CPU subsystem shall:

* Execute deterministic irrigation control
* Drive a 7" RGB display at 60 Hz
* Support Wi-Fi and MQTT simultaneously
* Execute LVGL with smooth animations
* Provide OTA firmware updates
* Support secure boot
* Support flash encryption
* Remain software compatible for future hardware revisions

---

# 2.3 Processor Selection

Primary processor:

| Parameter           | Selection            |
| ------------------- | -------------------- |
| MCU                 | ESP32-S3             |
| Architecture        | Xtensa LX7 Dual Core |
| CPU Frequency       | Up to 240 MHz        |
| Wi-Fi               | 802.11 b/g/n         |
| Bluetooth           | BLE 5                |
| USB                 | Native USB OTG       |
| LCD                 | RGB LCD Peripheral   |
| Camera              | Supported            |
| Vector Instructions | Yes                  |

The ESP32-S3 provides sufficient computational resources for the current controller generation while allowing future software expansion.

---

# 2.4 CPU Responsibilities

Core responsibilities include:

* Irrigation control
* Hydraulic analysis
* Display rendering
* MQTT communications
* OTA updates
* Data logging
* Diagnostics
* Weather processing
* Security
* Configuration management

No single subsystem shall monopolize CPU resources.

---

# 2.5 Core Assignment

Recommended FreeRTOS core affinity:

| Core   | Responsibilities                                |
| ------ | ----------------------------------------------- |
| Core 0 | Networking, MQTT, Weather, OTA                  |
| Core 1 | Irrigation, Display, Hydraulics, User Interface |

Interrupt balancing shall be optimized during implementation.

---

# 2.6 Clock Architecture

Reference clock system:

```text id="clock-architecture"
40 MHz Crystal

↓

PLL

↓

CPU Clock

240 MHz

↓

Peripheral Clocks

↓

LCD

SPI

I²C

UART

USB

PWM
```

Clock generation shall remain stable across the full operating temperature range.

---

# 2.7 Crystal Oscillator

Primary oscillator requirements:

| Parameter             |                     Value |
| --------------------- | ------------------------: |
| Frequency             |                    40 MHz |
| Tolerance             |                   ±10 ppm |
| Load Capacitance      | Per crystal specification |
| Temperature Stability |          Industrial grade |

Crystal placement shall minimize trace length and electrical noise.

---

# 2.8 Boot Configuration

Boot sequence:

```text id="boot-sequence"
Power Applied

↓

Reset

↓

ROM Bootloader

↓

Flash Verification

↓

Second Stage Bootloader

↓

Application

↓

Display Initialization

↓

Dashboard
```

Target boot time:

```text id="boot-time"
<5 seconds
```

---

# 2.9 Flash Memory

Recommended flash configuration:

| Parameter    |                   Value |
| ------------ | ----------------------: |
| Size         |                   16 MB |
| Interface    |               Octal SPI |
| Speed        |                  80 MHz |
| Manufacturer | Qualified supplier list |

Flash shall support:

* OTA
* Encryption
* Wear leveling
* File system

---

# 2.10 PSRAM

Recommended configuration:

| Parameter |       Value |
| --------- | ----------: |
| Capacity  |        8 MB |
| Interface | Octal PSRAM |
| Frequency |      80 MHz |

PSRAM shall primarily support:

* LVGL frame buffers
* Image caches
* Charts
* Dynamic widgets
* Temporary data structures

---

# 2.11 Memory Map

Reference memory allocation:

```text id="memory-map"
Internal RAM

↓

FreeRTOS

Application

Stacks

↓

PSRAM

Frame Buffers

Charts

Images

↓

Flash

Firmware

Assets

Languages

Themes
```

Memory allocation shall be optimized for deterministic execution.

---

# 2.12 Bootloader Configuration

The bootloader shall support:

* Dual OTA partitions
* Secure Boot
* Rollback protection
* Factory firmware
* Diagnostic mode
* Recovery mode

Bootloader configuration shall remain independent of the application firmware.

---

# 2.13 Partition Layout

Recommended flash partition table:

| Partition       | Purpose                  |
| --------------- | ------------------------ |
| Bootloader      | Boot firmware            |
| Partition Table | Flash map                |
| NVS             | Configuration            |
| OTA Data        | Active firmware tracking |
| Factory         | Recovery firmware        |
| OTA Slot A      | Active firmware          |
| OTA Slot B      | Update firmware          |
| SPIFFS/LittleFS | Assets & logs            |

The partition layout shall support future expansion without disrupting OTA compatibility.

---

# 2.14 Reset Sources

Supported reset sources:

* Power-On Reset
* Brownout Reset
* Software Reset
* Watchdog Reset
* External Reset
* Deep Sleep Wake

The reset reason shall be recorded and exposed through the Diagnostics interface.

---

# 2.15 Watchdog Strategy

Multiple watchdogs shall be employed.

| Watchdog          | Purpose             |
| ----------------- | ------------------- |
| RTC Watchdog      | Boot protection     |
| Task Watchdog     | FreeRTOS monitoring |
| Hardware Watchdog | System recovery     |

Critical irrigation tasks shall periodically service the watchdog.

---

# 2.16 Brownout Protection

Brownout detector settings:

| Threshold        |                     Recommendation |
| ---------------- | ---------------------------------: |
| Brownout Voltage | ~3.0 V (per ESP-IDF configuration) |

Brownout events shall:

* Halt valve activation
* Preserve configuration
* Record diagnostics
* Restart automatically after power recovery

---

# 2.17 GPIO Allocation Strategy

GPIO assignment shall prioritize:

1. RGB Display
2. Touch Controller
3. Valve Outputs
4. RS-485
5. I²C
6. SPI
7. Expansion
8. User LEDs
9. Future peripherals

Unused GPIOs shall remain reserved where practical.

---

# 2.18 Peripheral Allocation

Major peripherals:

| Peripheral | Usage                |
| ---------- | -------------------- |
| RGB LCD    | Display              |
| I²C        | GT911, sensors       |
| SPI        | Future expansion     |
| UART0      | Programming          |
| UART1      | RS-485               |
| USB        | Debug & provisioning |
| PWM        | Backlight            |
| GPIO       | Valves & status      |

Peripheral conflicts shall be avoided through centralized pin planning.

---

# 2.19 Interrupt Architecture

Interrupt priorities:

| Priority | Function               |
| -------- | ---------------------- |
| Highest  | Safety-critical timers |
| High     | Valve timing           |
| Medium   | Display DMA            |
| Medium   | Touch                  |
| Low      | Communications         |
| Lowest   | Background diagnostics |

Interrupt handlers shall remain short and defer processing to tasks.

---

# 2.20 DMA Utilization

DMA channels shall support:

* RGB display refresh
* SPI transfers
* Memory operations (where applicable)

DMA transfers shall minimize CPU overhead and improve GUI responsiveness.

---

# 2.21 Security Features

The hardware shall support:

* Secure Boot V2
* Flash Encryption
* Hardware RNG
* Cryptographic acceleration
* Secure OTA
* TLS acceleration

Security features shall be enabled in production firmware.

---

# 2.22 Power Modes

Supported operating modes:

| Mode                         | Description      |
| ---------------------------- | ---------------- |
| Normal                       | Full operation   |
| Idle                         | Reduced activity |
| Display Sleep                | Backlight off    |
| Maintenance                  | Diagnostics      |
| Deep Sleep (future products) | Ultra-low power  |

The residential controller is expected to operate continuously in Normal mode.

---

# 2.23 Debug Interface

Development interfaces include:

* USB-C
* UART Console
* JTAG (where available)
* Serial Logging
* OTA Debug

Production units may disable certain debug functions while retaining service diagnostics.

---

# 2.24 Thermal Considerations

Expected CPU operating range:

| Condition |                   Requirement |
| --------- | ----------------------------: |
| Ambient   |                -20°C to +60°C |
| Junction  | Within ESP32-S3 specification |

PCB copper planes shall assist with thermal dissipation.

No active cooling shall be required.

---

# 2.25 Reliability Requirements

Design targets:

| Requirement           |               Target |
| --------------------- | -------------------: |
| Continuous Operation  |            >10 years |
| Unexpected Reset Rate |          <1 per year |
| Boot Success          |                 100% |
| OTA Recovery          |            Automatic |
| Flash Lifetime        | Within specification |

---

# 2.26 Manufacturing Requirements

CPU subsystem shall support:

* Automated programming
* Automated verification
* MAC address registration
* Secure key injection
* Functional testing
* Factory diagnostics

Programming shall be possible without removing the PCB from the production fixture.

---

# 2.27 Relationship to Firmware

This hardware directly supports the software architecture defined in:

| Volume   | Relationship          |
| -------- | --------------------- |
| Volume 2 | Firmware Architecture |
| Volume 3 | Communications        |
| Volume 4 | HMI                   |
| Volume 6 | Manufacturing         |
| Volume 7 | Validation            |

Hardware abstraction shall isolate application software from future processor revisions.

---

# 2.28 Future Processor Roadmap

The CPU architecture has been selected to facilitate migration to future platforms including:

* ESP32-P4
* ESP32-C6 (gateway variants)
* Multi-MCU systems
* AI co-processors
* Secure elements

Migration shall preserve the firmware architecture wherever possible.

---

# 2.29 Engineering Notes

The ESP32-S3 provides an excellent balance between computational performance, integrated peripherals and long-term software support. Its native RGB LCD controller, substantial internal memory and support for external PSRAM make it particularly well suited for a modern embedded HMI while simultaneously executing deterministic irrigation control.

By adopting disciplined partitioning, secure boot, dual OTA partitions and a structured peripheral allocation strategy, the CPU subsystem provides a robust foundation for future Zmartify controller generations without requiring significant changes to higher software layers.

---

# 2.30 Chapter Summary

This chapter defines the ESP32-S3 CPU subsystem, including processor selection, memory architecture, clock generation, boot configuration, peripheral allocation and security features.

It establishes the reference processing platform upon which all subsequent hardware subsystems—including power management, display electronics, communications and valve control—are built.

---

# End of Chapter 2

**Next Chapter**

**Chapter 3 – Power Supply Architecture, Protection Circuits & Power Distribution Network**

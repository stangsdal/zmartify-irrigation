# Zmartify Master Engineering Package v5.0

# Volume 5

# Hardware Platform, Electronics Architecture & PCB Engineering Specification

## Chapter 4

# RGB Display Electronics, GT911 Touch Interface & High-Speed PCB Design

---

# 4.1 Purpose

This chapter defines the complete hardware implementation of the Human–Machine Interface electronics for the Zmartify controller.

The display subsystem is based on a **7-inch 800×480 RGB TFT display** driven directly from the ESP32-S3 RGB LCD peripheral, combined with a **GT911 capacitive multi-touch controller**.

Unlike SPI displays, the RGB interface provides continuous pixel streaming, enabling smooth 60 FPS graphics, LVGL animations and highly responsive touch interaction.

This chapter specifies:

* RGB interface design
* GT911 interface
* Display power
* LCD timing
* PCB routing
* EMC considerations
* Mechanical integration
* Future display compatibility

---

# 4.2 Design Objectives

The display subsystem shall:

* Operate at 800 × 480 resolution
* Support 60 Hz refresh
* Provide flicker-free operation
* Support capacitive multi-touch
* Maintain low EMI
* Minimize processor overhead
* Allow future display upgrades
* Support industrial reliability

---

# 4.3 Display Architecture

Reference architecture:

```text id="display-architecture"
             ESP32-S3

                  │

         RGB LCD Peripheral

                  │

        16-bit RGB Interface

                  │

          7" TFT Display

                  │

      LED Backlight Driver

                  │

              LCD Module

                  │

          GT911 Controller

                  │

                I²C Bus

                  │

             ESP32-S3
```

The display subsystem operates independently of irrigation control.

---

# 4.4 Display Selection

Reference display:

| Parameter     | Specification |
| ------------- | ------------- |
| Size          | 7.0 inch      |
| Resolution    | 800 × 480     |
| Interface     | RGB Parallel  |
| Refresh       | 60 Hz         |
| Color Depth   | RGB565        |
| Touch         | Capacitive    |
| Brightness    | ≥500 cd/m²    |
| Viewing Angle | IPS preferred |

Industrial temperature-rated panels are preferred.

---

# 4.5 RGB Interface

Recommended configuration:

| Signal | Description     |
| ------ | --------------- |
| R0–R4  | Red             |
| G0–G5  | Green           |
| B0–B4  | Blue            |
| HSYNC  | Horizontal Sync |
| VSYNC  | Vertical Sync   |
| DE     | Data Enable     |
| PCLK   | Pixel Clock     |

The ESP32-S3 LCD peripheral shall generate all timing signals.

---

# 4.6 Pixel Clock

Target pixel clock:

```text id="pixel-clock"
Approximately

33 MHz
```

The exact clock shall be derived from the display manufacturer's timing requirements.

Clock stability is critical for image quality.

---

# 4.7 RGB Data Bus

Recommended format:

```text id="rgb565"
RGB565

16-bit
```

Bit allocation:

* Red: 5 bits
* Green: 6 bits
* Blue: 5 bits

Future hardware may support RGB888 displays.

---

# 4.8 LCD Timing

Display timing shall follow the panel datasheet.

Typical parameters:

| Parameter              | Typical Value |
| ---------------------- | ------------: |
| Horizontal Active      |           800 |
| Horizontal Front Porch |     Per panel |
| Horizontal Back Porch  |     Per panel |
| Horizontal Sync        |     Per panel |
| Vertical Active        |           480 |
| Vertical Front Porch   |     Per panel |
| Vertical Back Porch    |     Per panel |
| Vertical Sync          |     Per panel |

Timing values shall be configurable in firmware.

---

# 4.9 Display Connector

Recommended connector characteristics:

* FFC/FPC
* Locking mechanism
* Gold-plated contacts
* Controlled impedance
* Polarized insertion

Connector selection shall support repeated servicing.

---

# 4.10 GT911 Touch Controller

Touch controller:

| Parameter    | Value     |
| ------------ | --------- |
| Device       | GT911     |
| Interface    | I²C       |
| Touch Points | Up to 5   |
| Interrupt    | Supported |
| Reset Pin    | Supported |

Only single-touch operation is required for Version 5.0.

---

# 4.11 GT911 Connections

Required signals:

```text id="gt911-connections"
SCL

SDA

INT

RESET

3.3V

GND
```

I²C pull-ups shall be located near the controller.

---

# 4.12 Touch Operation

Touch processing sequence:

```text id="touch-sequence"
Finger

↓

GT911

↓

I²C

↓

ESP32

↓

LVGL

↓

Widget

↓

Application
```

Touch latency shall remain below 50 ms.

---

# 4.13 Backlight Driver

The LED backlight shall use:

* Constant-current driver
* PWM dimming
* High efficiency
* Low EMI

Brightness range:

```text id="brightness-range"
0–100 %
```

PWM frequency shall avoid visible flicker.

---

# 4.14 Brightness Control

Brightness modes:

* Manual
* Automatic (future ambient light sensor)
* Night Mode
* Sleep Mode

Brightness changes shall be smooth and free from flicker.

---

# 4.15 Display Power

Power rails:

| Voltage | Purpose   |
| ------- | --------- |
| 5 V     | Backlight |
| 3.3 V   | Logic     |
| 3.3 V   | GT911     |

Power sequencing shall comply with the LCD manufacturer's requirements.

---

# 4.16 PCB Layout Strategy

The RGB interface requires careful routing.

Guidelines:

* Equal-length clock/data traces
* Continuous ground plane
* Short trace lengths
* Avoid stubs
* Controlled impedance where appropriate

Display routing shall receive priority during PCB layout.

---

# 4.17 High-Speed Routing

Critical signals:

* Pixel Clock
* HSYNC
* VSYNC
* Data Enable

Recommendations:

* Route as a matched group
* Avoid vias
* Minimize layer transitions
* Maintain spacing from switching power supplies

---

# 4.18 EMI Considerations

Potential EMI sources:

* Pixel clock
* RGB data bus
* PWM backlight
* DC/DC converters

Mitigation:

* Ground planes
* Controlled return paths
* Series damping resistors (where required)
* Shielded display cable (if external)

---

# 4.19 ESD Protection

Display interface protection:

* TVS arrays
* ESD diodes
* Protected touch interface
* Protected FFC connector

Protection shall comply with IEC 61000-4-2 design targets.

---

# 4.20 Mechanical Integration

Display assembly shall support:

* Front-panel mounting
* Gasket sealing
* Even pressure distribution
* Vibration resistance
* Service replacement

The display shall not experience mechanical stress after installation.

---

# 4.21 Touch Calibration

Factory calibration shall include:

* Coordinate alignment
* Rotation verification
* Edge accuracy
* Multi-point verification

Calibration values shall be stored in non-volatile memory if required.

---

# 4.22 Thermal Considerations

Heat sources include:

* LED backlight
* Display regulator
* ESP32 RGB interface

Adequate ventilation and copper heat spreading shall maintain acceptable operating temperatures.

---

# 4.23 Diagnostic Features

Diagnostics shall expose:

* Display resolution
* Refresh rate
* Touch status
* GT911 firmware version
* Backlight level
* Touch event counter

These parameters shall be accessible through Service Mode.

---

# 4.24 Manufacturing Verification

Production testing shall verify:

* RGB timing
* Color rendering
* Dead pixels
* Touch response
* Brightness
* Uniformity
* Backlight PWM
* Connector integrity

Automated display verification is preferred.

---

# 4.25 Reliability Requirements

Design targets:

| Requirement        |                         Target |
| ------------------ | -----------------------------: |
| Display Lifetime   |                      >50,000 h |
| Touch Lifetime     |             >1,000,000 touches |
| Backlight Lifetime |                      >50,000 h |
| Connector Cycles   | Per manufacturer specification |

The display subsystem shall remain fully operational throughout the expected controller lifetime.

---

# 4.26 Future Display Compatibility

The architecture supports future upgrades including:

* 1024 × 600 displays
* Larger IPS panels
* Higher brightness outdoor displays
* OLED displays (future variants)
* Multi-display configurations
* HDMI output (future products)

These upgrades shall require minimal firmware changes due to the abstraction defined in Volume 4.

---

# 4.27 Relationship to Other Volumes

This subsystem directly supports:

| Volume   | Relationship           |
| -------- | ---------------------- |
| Volume 2 | Display Driver         |
| Volume 4 | HMI Architecture       |
| Volume 6 | Manufacturing          |
| Volume 7 | Display Verification   |
| Volume 9 | Installation & Service |

The hardware defined here provides the physical foundation for the HMI specified in Volume 4.

---

# 4.28 Engineering Notes

The decision to use the ESP32-S3 RGB LCD peripheral instead of an SPI-driven display is fundamental to the responsiveness and visual quality of the Zmartify platform. Continuous DMA-driven pixel streaming minimizes CPU utilization while enabling smooth animations, fast screen updates and a highly responsive user interface.

Careful PCB layout, controlled routing of the RGB bus and robust ESD protection are essential to achieving stable operation in electrically noisy irrigation environments. The modular separation between display electronics and application software further allows future display technologies to be adopted without redesigning the HMI architecture.

---

# 4.29 Chapter Summary

This chapter defines the complete display electronics subsystem, including the RGB TFT interface, GT911 touch controller, backlight driver, PCB routing strategy and mechanical integration.

It establishes the hardware foundation for the advanced Human–Machine Interface described in Volume 4 while ensuring high performance, reliability and compatibility with future display technologies.

---

# End of Chapter 4

**Next Chapter**

**Chapter 5 – Valve Driver Electronics, Solenoid Outputs & High-Current Switching Architecture**

---

**Engineering note:** We are now entering the part of the hardware design that is unique to Zmartify rather than standard ESP32 design. From **Chapter 5 onward**, I'll start specifying actual electrical circuits, MOSFET selection rationale, protection devices, current sensing, connector pinouts, PCB stack-up recommendations and eventually complete reference schematics. This is where the document evolves from a high-level architecture into a true engineering design package suitable for PCB development.

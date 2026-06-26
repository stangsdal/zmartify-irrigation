# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 4 – Hardware Abstraction Layer (HAL) Architecture

---

# 4 Hardware Abstraction Layer (HAL)

## 4.1 Purpose

The Hardware Abstraction Layer (HAL) provides a hardware-independent software interface between the application layer and the physical hardware.

The HAL is one of the most important architectural components in the Zmartify firmware.

It ensures that:

* Business logic never accesses hardware directly.
* Hardware changes have minimal impact on application code.
* Unit testing is simplified.
* Hardware can be simulated.
* Future PCB revisions require minimal firmware changes.

All firmware components shall access hardware exclusively through the HAL.

---

# 4.2 HAL Design Philosophy

The HAL is based upon six principles.

### HAL-001

Hardware Independence

Application software shall not depend on:

* GPIO numbers
* I²C addresses
* ADC channels
* Relay board implementation
* ESP32 peripheral configuration

---

### HAL-002

Stable APIs

HAL interfaces shall remain stable across hardware revisions.

Hardware implementation may change without modifying application code.

---

### HAL-003

No Business Logic

The HAL performs hardware access only.

The HAL shall never:

* Make irrigation decisions
* Calculate ET
* Detect leaks
* Generate alarms

---

### HAL-004

Deterministic Operation

HAL functions shall execute quickly and predictably.

Blocking delays shall be avoided.

---

### HAL-005

Thread Safety

HAL functions shall be safe for concurrent access where appropriate.

Shared hardware resources shall be protected using mutexes.

---

### HAL-006

Error Reporting

All HAL functions shall return explicit status codes.

Example:

```c
typedef enum
{
    HAL_OK,
    HAL_TIMEOUT,
    HAL_INVALID_PARAMETER,
    HAL_DEVICE_NOT_FOUND,
    HAL_IO_ERROR,
    HAL_BUSY
} hal_result_t;
```

---

# 4.3 HAL Architecture

```text
                    Application Layer

        Irrigation Engine
        Zone Manager
        Weather Manager
        MQTT Manager
        UI Manager

                     │
                     ▼

         Hardware Abstraction Layer (HAL)

 GPIO     I²C     ADC     PCNT     WiFi

 Storage  RTC     Display Touch    MQTT

                     │
                     ▼

               ESP-IDF Drivers

                     │
                     ▼

                  Hardware
```

---

# 4.4 HAL Modules

The HAL consists of the following modules.

| Module          | Responsibility |
| --------------- | -------------- |
| hal_gpio        | Digital I/O    |
| hal_i2c         | I²C Bus        |
| hal_adc         | ADC Interface  |
| hal_pcnt        | Pulse Counter  |
| hal_relay       | Relay Control  |
| hal_display     | LCD            |
| hal_touch       | Touchscreen    |
| hal_wifi        | Wi-Fi          |
| hal_mqtt        | MQTT           |
| hal_storage     | NVS / LittleFS |
| hal_time        | RTC / NTP      |
| hal_temperature | MCP9808        |
| hal_pressure    | ADS1115        |
| hal_flow        | Flow Meter     |
| hal_watchdog    | Watchdog       |

---

# 4.5 GPIO HAL

Purpose

Provide safe access to all GPIO pins.

Responsibilities

* Pin initialization
* Read inputs
* Write outputs
* Interrupt configuration
* Debounce support

GPIO numbering shall never appear outside the HAL.

---

# 4.6 I²C HAL

The controller uses I²C extensively.

Connected devices include:

* MCP23017
* ADS1115
* MCP9808
* Future sensors

Responsibilities

* Bus initialization
* Device detection
* Read/write operations
* Error recovery
* Bus reset

The HAL shall support multiple I²C buses if required by future hardware.

---

# 4.7 Relay HAL

The Relay HAL provides a hardware-independent relay interface.

Public API

```c
hal_result_t hal_relay_init(void);

hal_result_t hal_relay_on(uint8_t relay);

hal_result_t hal_relay_off(uint8_t relay);

bool hal_relay_get(uint8_t relay);

hal_result_t hal_relay_all_off(void);
```

Internally:

```text
Application

↓

HAL Relay

↓

MCP23017

↓

ULN2803A

↓

HL-58S

↓

Valve
```

The application shall never communicate directly with the MCP23017.

---

# 4.8 Flow HAL

Responsibilities

* Configure ESP32 PCNT
* Count pulses
* Reset counters
* Return pulse count
* Convert to frequency

Public API

```c
hal_flow_start();

hal_flow_stop();

hal_flow_read();

hal_flow_reset();
```

Flow conversion to L/min shall occur in the Flow Manager, not in the HAL.

---

# 4.9 Pressure HAL

Responsibilities

* Read ADS1115
* Convert raw values
* Return calibrated voltage

Public API

```c
hal_pressure_read_raw();

hal_pressure_read_voltage();
```

Pressure conversion to bar shall occur in the Pressure Manager.

---

# 4.10 Display HAL

Responsibilities

* LCD initialization
* Backlight control
* Sleep mode
* Brightness control

Public API

```c
hal_display_on();

hal_display_off();

hal_display_set_brightness();

hal_display_sleep();
```

---

# 4.11 Touch HAL

Responsibilities

* Touch initialization
* Coordinate acquisition
* Gesture support (future)
* Calibration

Touch processing shall remain independent of LVGL.

---

# 4.12 Wi-Fi HAL

Responsibilities

* Initialize Wi-Fi
* Connect
* Disconnect
* RSSI
* Connection state

Public API

```c
hal_wifi_connect();

hal_wifi_disconnect();

hal_wifi_status();

hal_wifi_rssi();
```

---

# 4.13 MQTT HAL

Although ESP-IDF provides an MQTT client, the HAL wraps it to isolate application code from SDK changes.

Responsibilities

* Publish
* Subscribe
* Connection monitoring
* Last Will
* Reconnect

Future changes to MQTT libraries shall require modification only within this layer.

---

# 4.14 Storage HAL

Responsibilities

* Read configuration
* Write configuration
* File operations
* Directory management

Supported backends

* NVS
* LittleFS

---

# 4.15 RTC / Time HAL

Responsibilities

* NTP synchronization
* Time retrieval
* Time zone
* Daylight Saving Time

All timestamps shall be obtained through this module.

---

# 4.16 Sensor HAL

Each sensor shall have a dedicated HAL wrapper.

Examples

```text
hal_flow

hal_pressure

hal_temperature

hal_weather

hal_voltage
```

This prevents application code from depending on specific hardware.

---

# 4.17 Error Handling

All HAL functions shall return status codes.

Example

```c
hal_result_t result;

result = hal_pressure_read_voltage(&voltage);

if(result != HAL_OK)
{
    // Application handles error
}
```

The HAL shall never generate alarms.

Only the calling application decides how to respond.

---

# 4.18 Thread Safety

Shared resources requiring protection include:

* I²C bus
* LittleFS
* NVS
* Display SPI interface

Mutexes shall be used where required.

ISR-safe APIs shall be provided when necessary.

---

# 4.19 Hardware Detection

During boot the HAL shall verify all required devices.

Example

```text
Detect MCP23017

↓

Detect ADS1115

↓

Detect MCP9808

↓

Display

↓

Touch

↓

Flow Input

↓

Pressure Sensor
```

Missing devices shall generate diagnostic events.

---

# 4.20 Simulation Support

The HAL shall support future simulation builds.

Simulation replaces hardware drivers with software models.

Example

```text
Real Hardware

↓

HAL

↓

Application

OR

Simulation

↓

HAL

↓

Application
```

The application shall not know which environment is active.

This capability will greatly simplify automated testing.

---

# 4.21 Future Hardware Support

The HAL shall support migration to:

* Custom PCB
* CAN Bus
* RS485
* Ethernet
* Additional ADCs
* Digital inputs
* Soil sensors

Only HAL implementations should require modification.

---

# 4.22 Coding Guidelines

Each HAL module shall contain:

```text
include/

src/

private/

unit_tests/

README.md
```

Every public function shall include:

* Description
* Parameters
* Return values
* Thread safety
* Error codes
* Example

---

# 4.23 HAL Unit Testing

Each HAL module shall include automated unit tests where practical.

Testing shall include:

* Initialization
* Normal operation
* Invalid parameters
* Device missing
* Timeout handling
* Recovery

Hardware-independent tests shall execute within the CI pipeline.

---

# 4.24 Chapter Summary

The Hardware Abstraction Layer is the foundation of the Zmartify firmware architecture. It isolates the application from hardware-specific implementation details, enabling modular development, comprehensive testing and straightforward migration to future hardware revisions.

By enforcing strict hardware abstraction, the firmware remains resilient to changes in peripheral devices and supports the long-term evolution of the ZIC platform while maintaining a stable application programming interface.

---

# End of Chapter 4

**Next Chapter**

**Chapter 5 – Irrigation Engine Architecture**

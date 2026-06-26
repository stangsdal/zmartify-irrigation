# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 17 – Diagnostics Manager Architecture

---

# 17 Diagnostics Manager

---

# 17.1 Purpose

The Diagnostics Manager provides continuous monitoring of the operational health of the Zmartify Irrigation Controller.

It functions as the controller's internal "health monitoring system", collecting information from every firmware module and hardware subsystem to detect abnormal conditions before they become failures.

Unlike the Alarm Manager, which reacts to faults, the Diagnostics Manager focuses on:

* Health monitoring
* Performance analysis
* Preventive maintenance
* System optimization
* Engineering support
* Service diagnostics

The Diagnostics Manager shall never directly control irrigation hardware.

---

# 17.2 Design Objectives

The Diagnostics Manager shall:

* Monitor firmware health
* Monitor hardware health
* Detect degradation
* Collect runtime statistics
* Support predictive maintenance
* Assist troubleshooting
* Publish diagnostic information
* Record long-term trends
* Minimize performance impact

---

# 17.3 Architectural Position

```text
                 Every Firmware Module

        Flow Manager

        Pressure Manager

        Relay Manager

        MQTT Manager

        Weather Manager

        Storage Manager

        OTA Manager

        HAL

               │

               ▼

        Diagnostics Manager

               │

      ┌────────┼────────┐

      ▼                 ▼

 Logging          MQTT Manager

               │

               ▼

           User Interface
```

The Diagnostics Manager subscribes to events from all major firmware components.

---

# 17.4 Diagnostic Philosophy

Diagnostics shall be:

* Continuous
* Non-intrusive
* Lightweight
* Historical
* Actionable

Diagnostics shall never significantly affect irrigation timing or controller responsiveness.

---

# 17.5 Diagnostic Categories

The following diagnostic groups shall be maintained.

| Category  | Description                 |
| --------- | --------------------------- |
| System    | Overall controller health   |
| CPU       | Processor performance       |
| Memory    | RAM usage                   |
| Tasks     | FreeRTOS health             |
| Network   | Wi-Fi and MQTT              |
| Storage   | Flash health                |
| Sensors   | Flow, Pressure, Temperature |
| Relay     | Output subsystem            |
| Display   | LCD and Touch               |
| Power     | Supply monitoring           |
| Hydraulic | System performance          |

---

# 17.6 System Health Score

The Diagnostics Manager shall calculate an overall System Health Score.

Example:

|  Score | Status    |
| -----: | --------- |
| 95–100 | Excellent |
|  85–94 | Good      |
|  70–84 | Attention |
|  50–69 | Poor      |
|    <50 | Critical  |

The score shall be visible on the dashboard and published via MQTT.

---

# 17.7 CPU Diagnostics

Collected parameters:

* CPU utilization
* Idle percentage
* Interrupt load
* Task switching rate
* Watchdog activity
* Average execution latency

Target CPU utilization:

**<60%**

Maximum acceptable:

**80%**

Above this threshold, a diagnostic warning shall be generated.

---

# 17.8 Memory Diagnostics

The controller shall monitor:

* Free heap
* Minimum free heap
* Largest free block
* Fragmentation estimate
* Allocation failures
* Stack high-water marks

Memory shall be sampled every 10 seconds.

---

# 17.9 FreeRTOS Task Diagnostics

Each task shall report:

* Running state
* CPU time
* Stack usage
* Priority
* Last execution
* Watchdog status

Tasks exceeding configured execution limits shall generate diagnostics.

---

# 17.10 Hardware Diagnostics

The Diagnostics Manager shall supervise:

* MCP23017 communication
* ADS1115 communication
* Display interface
* Touch controller
* Flow sensor
* Pressure sensor
* Temperature sensor
* I²C bus
* Internal RTC

Future hardware modules shall register automatically during initialization.

---

# 17.11 Power Supply Diagnostics

Although the controller does not initially include voltage sensing, the architecture shall support future monitoring of:

* 5 V supply
* 24 VAC transformer
* ESP32 supply voltage
* Brownout events
* Power interruptions

Future hardware revisions may include ADC-based voltage monitoring.

---

# 17.12 Network Diagnostics

Network monitoring includes:

Wi-Fi

* RSSI
* Signal quality
* Reconnect count
* DHCP status

MQTT

* Connection status
* Latency
* Publish failures
* Subscription status

Time Synchronization

* NTP status
* Last synchronization
* Clock drift

---

# 17.13 Storage Diagnostics

Collected information:

* Flash usage
* Free space
* File count
* Wear estimate
* Write failures
* Read failures
* CRC failures

Storage health shall contribute to the overall Health Score.

---

# 17.14 Hydraulic Diagnostics

Hydraulic health shall be continuously evaluated using:

* Flow stability
* Pressure stability
* Learned baseline deviation
* Leak probability
* Water efficiency

These values support predictive maintenance and intelligent alarm correlation.

---

# 17.15 Performance Metrics

The Diagnostics Manager shall record:

* Boot duration
* Average task latency
* MQTT throughput
* Display refresh time
* Sensor update latency
* Event Bus latency
* Queue utilization

Historical performance trends shall be retained.

---

# 17.16 Event Subscription

The Diagnostics Manager subscribes to:

```text
EVT_SYSTEM_*

EVT_FLOW_*

EVT_PRESS_*

EVT_RELAY_*

EVT_STORAGE_*

EVT_MQTT_*

EVT_WIFI_*

EVT_OTA_*

EVT_ALARM_*
```

Publishes:

```text
EVT_DIAG_WARNING

EVT_DIAG_ERROR

EVT_DIAG_HEALTH

EVT_DIAG_REPORT
```

---

# 17.17 Self-Test

At startup the Diagnostics Manager shall coordinate a complete controller self-test.

Sequence:

```text
Boot

↓

Memory Test

↓

Storage Test

↓

Relay Test

↓

Display Test

↓

Touch Test

↓

Sensor Detection

↓

Network Initialization

↓

Ready
```

Failures shall be reported to the Alarm Manager.

---

# 17.18 Continuous Self-Monitoring

Periodic diagnostic intervals:

| Item              | Interval |
| ----------------- | -------: |
| CPU               |      5 s |
| Memory            |     10 s |
| Tasks             |      5 s |
| Network           |     30 s |
| Storage           |    5 min |
| Sensors           |      1 s |
| Hydraulic Health  |      5 s |
| Full Health Score |     60 s |

---

# 17.19 Historical Diagnostics

Historical records shall include:

* CPU utilization
* Memory usage
* Wi-Fi quality
* MQTT latency
* Alarm frequency
* Hydraulic stability
* Storage health
* System Health Score

Minimum retention:

5 years or until storage limits require rotation.

---

# 17.20 MQTT Integration

Diagnostic topics:

```text
zmartify/diagnostics/system

zmartify/diagnostics/cpu

zmartify/diagnostics/memory

zmartify/diagnostics/tasks

zmartify/diagnostics/network

zmartify/diagnostics/storage

zmartify/diagnostics/health
```

Diagnostics shall be published periodically and upon significant state changes.

---

# 17.21 User Interface

The Diagnostics pages shall include:

### System

* Health Score
* Uptime
* Firmware Version
* Hardware Revision

### Performance

* CPU
* RAM
* Task List
* Event Bus Statistics

### Hardware

* I²C Devices
* Sensor Status
* Relay Status
* Display Status

### Network

* Wi-Fi
* MQTT
* NTP

### Storage

* Flash Usage
* Backups
* Logs
* Filesystem Health

---

# 17.22 Service Mode

The Diagnostics Manager shall support an Engineering Service Mode.

Capabilities include:

* Manual relay operation
* Sensor simulation
* Event injection
* Live variable inspection
* I²C scan
* Hardware tests
* Calibration tools

Access shall require administrator authentication.

---

# 17.23 Remote Diagnostics

Through MQTT (and future cloud services), authorized users may retrieve:

* Diagnostic snapshot
* System report
* Event history
* Alarm history
* Hardware inventory
* Performance statistics

Remote diagnostics shall be read-only unless explicitly authorized.

---

# 17.24 Public API

Example interface:

```c
diagnostics_init();

diagnostics_start();

diagnostics_update();

diagnostics_get_health();

diagnostics_get_report();

diagnostics_run_selftest();

diagnostics_export();

diagnostics_reset_statistics();
```

---

# 17.25 Unit Testing

Automated tests shall verify:

* Health Score calculation
* Self-test
* Historical statistics
* Event processing
* MQTT publication
* Memory monitoring
* CPU monitoring
* Storage diagnostics
* Hardware diagnostics
* Service Mode

Minimum code coverage:

**95%**

---

# 17.26 Future Enhancements

The Diagnostics architecture supports:

* AI-assisted fault prediction
* Automatic service recommendations
* Cloud diagnostics portal
* Fleet health monitoring
* Predictive hardware replacement
* Digital Twin integration
* Machine-learning anomaly detection

These features shall integrate through the existing Diagnostics API.

---

# 17.27 Engineering Notes

The Diagnostics Manager is intended to evolve into one of the defining features of the Zmartify platform.

Rather than merely reporting failures, it continuously evaluates system health, identifies degradation trends and supports predictive maintenance. Combined with the Flow Manager, Pressure Manager and Alarm Manager, it enables a proactive maintenance philosophy that minimizes downtime and extends the operational life of the irrigation system.

The Engineering Service Mode further supports commissioning, troubleshooting and field service without requiring firmware modifications.

---

# 17.28 Chapter Summary

The Diagnostics Manager provides comprehensive health monitoring for every aspect of the Zmartify Irrigation Controller.

By continuously collecting operational metrics, coordinating self-tests and calculating an overall System Health Score, it enables preventive maintenance, efficient troubleshooting and long-term reliability.

Its modular architecture and extensive diagnostic capabilities establish a solid foundation for future cloud services, predictive analytics and AI-assisted maintenance, making it a key component of the Zmartify ecosystem.

---

# End of Chapter 17

**Next Chapter**

**Chapter 18 – User Interface (LVGL) Architecture**

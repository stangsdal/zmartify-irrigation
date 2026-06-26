# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 14

# Node-RED, Grafana & Third-Party Integration Guide

---

# 14.1 Purpose

This chapter defines how third-party software integrates with the Zmartify Irrigation Controller using the public MQTT interface.

Where previous chapters focused on smart-home platforms, this chapter addresses engineering tools, automation frameworks and data visualization systems including:

* Node-RED
* Grafana
* InfluxDB
* Prometheus
* SCADA Systems
* SQL Databases
* Custom Applications
* Mobile Applications
* Industrial Monitoring Systems

The objective is to ensure that every external application can interact with the controller without requiring firmware modifications or proprietary protocols.

---

# 14.2 Design Objectives

The third-party integration architecture shall:

* Use only public MQTT topics
* Remain platform independent
* Support engineering dashboards
* Support long-term historical storage
* Support enterprise integration
* Support multiple controllers
* Support future Zmartify products
* Preserve API compatibility

---

# 14.3 Integration Philosophy

The MQTT namespace defined in this specification is the **canonical interface** for all external integrations.

External applications shall never depend upon:

* Internal firmware variables
* Memory layouts
* Internal task names
* Source code implementation
* Proprietary APIs

All integrations shall communicate exclusively through documented MQTT topics.

---

# 14.4 Integration Architecture

```text
                Zmartify Controller

                        │

                 MQTT Manager

                        │

                  MQTT Broker

        ┌───────────────┼────────────────┐

        ▼               ▼                ▼

     Node-RED        Grafana        Custom Apps

        │               │                │

     InfluxDB      Prometheus       SQL Database

```

The MQTT Broker serves as the single integration point for all external software.

---

# 14.5 Recommended MQTT Subscriptions

General monitoring:

```text
zmartify/controller/#

zmartify/system/#

zmartify/events/#

zmartify/alarms/#
```

Hydraulic monitoring:

```text
zmartify/flow/#

zmartify/pressure/#

zmartify/hydraulics/#
```

Weather monitoring:

```text
zmartify/weather/#
```

Irrigation monitoring:

```text
zmartify/irrigation/#

zmartify/zones/#
```

Diagnostics:

```text
zmartify/diagnostics/#
```

---

# 14.6 Node-RED Integration

Node-RED is particularly well suited for automation workflows and protocol bridging.

Typical applications include:

* Custom notifications
* SMS gateways
* E-mail alerts
* PLC integration
* REST API gateways
* Dashboard generation
* Rule engines

The recommended MQTT nodes shall subscribe using wildcard topics to minimize maintenance as the API evolves.

---

# 14.7 Example Node-RED Flow – Leak Detection

```text
MQTT Input

↓

Alarm Filter

↓

Leak Detected?

↓

Yes

↓

Stop Irrigation Command

↓

Send Notification

↓

Log Event

↓

Create Maintenance Ticket
```

This flow illustrates the event-driven architecture of the Zmartify platform.

---

# 14.8 Example Node-RED Flow – Water Budget Adjustment

```text
Weather Recommendation

↓

Runtime Factor

↓

Compare Threshold

↓

Adjust Schedule

↓

Publish Configuration

↓

Receive Acknowledgement
```

All configuration changes shall use the transactional command interface described in Chapter 10.

---

# 14.9 Grafana Integration

Grafana is recommended for visualization of long-term operational data.

Typical dashboards include:

* Water consumption
* Flow trends
* Pressure trends
* Weather history
* ET trends
* Alarm statistics
* Hydraulic health
* Controller diagnostics

Grafana should retrieve data from a time-series database rather than subscribing directly to MQTT.

---

# 14.10 Recommended Time-Series Database

Recommended databases include:

| Database        | Status      |
| --------------- | ----------- |
| InfluxDB        | Recommended |
| TimescaleDB     | Supported   |
| Prometheus      | Supported   |
| VictoriaMetrics | Supported   |

These databases provide efficient storage for historical engineering telemetry.

---

# 14.11 Recommended Grafana Dashboards

### Controller Dashboard

Displays:

* Controller status
* Active program
* Active zone
* Uptime
* Firmware version
* Health score

---

### Hydraulic Dashboard

Displays:

* Current flow
* Current pressure
* Learned flow
* Learned pressure
* Leak probability
* Restriction probability
* Hydraulic health

---

### Water Usage Dashboard

Displays:

* Daily water usage
* Weekly water usage
* Monthly water usage
* Seasonal usage
* Lifetime usage

---

### Weather Dashboard

Displays:

* Temperature
* Humidity
* Rainfall
* Solar radiation
* UV Index
* ET
* Wind speed

---

### Diagnostics Dashboard

Displays:

* CPU utilization
* Heap memory
* Wi-Fi RSSI
* MQTT latency
* Storage utilization
* Task count
* Health score

---

# 14.12 Prometheus Integration

Prometheus is suitable for monitoring controller health in enterprise environments.

Recommended metrics include:

* Controller availability
* CPU usage
* Memory usage
* MQTT latency
* Wi-Fi signal
* Active alarms
* Active irrigation
* OTA status

Metrics should be exported via a bridge service that converts MQTT messages into Prometheus-compatible metrics.

---

# 14.13 SQL Database Integration

For relational databases, the following logical tables are recommended:

| Table                 | Purpose                       |
| --------------------- | ----------------------------- |
| controller            | Device metadata               |
| irrigation_runs       | Completed irrigation programs |
| zone_statistics       | Historical zone performance   |
| flow_measurements     | Flow telemetry                |
| pressure_measurements | Pressure telemetry            |
| weather_data          | Environmental conditions      |
| alarms                | Alarm history                 |
| diagnostics           | Controller diagnostics        |

Each record should include:

* Device ID
* Timestamp
* Measurement
* Units
* Quality indicator

---

# 14.14 REST API Bridge

A REST gateway may expose MQTT data to web applications.

Typical endpoints include:

```text
GET /api/controller

GET /api/zones

GET /api/weather

GET /api/flow

GET /api/pressure

GET /api/alarms

POST /api/commands/start

POST /api/commands/manual

POST /api/commands/stop
```

The REST gateway shall translate HTTP requests into validated MQTT transactions.

---

# 14.15 WebSocket Integration

A WebSocket gateway may provide real-time updates to browser applications.

Recommended message types:

* Controller state
* Active zone
* Flow
* Pressure
* Weather
* Alarms
* Diagnostics

WebSocket messages should reuse the JSON payloads defined in this specification.

---

# 14.16 Mobile Application Integration

Native mobile applications shall communicate through the public MQTT API or a REST gateway.

Recommended features:

* Live dashboard
* Push notifications
* Manual irrigation
* Alarm acknowledgement
* Water usage reports
* Weather overview

Applications shall not require knowledge of internal firmware architecture.

---

# 14.17 SCADA Integration

The MQTT interface has been designed to support industrial SCADA systems.

Recommended mappings:

| SCADA Object      | MQTT Topic        |
| ----------------- | ----------------- |
| Controller Status | controller/status |
| Active Zone       | irrigation/state  |
| Flow              | flow/current      |
| Pressure          | pressure/current  |
| Alarm             | alarms/current    |
| Hydraulic Health  | hydraulics/health |

SCADA systems may bridge MQTT to OPC UA or Modbus if required.

---

# 14.18 Multi-Controller Architecture

Enterprise installations may include multiple controllers.

Recommended namespace:

```text
zmartify/

    ZIC-S3-0001/

    ZIC-S3-0002/

    ZIC-S3-0003/

        controller/

        irrigation/

        flow/

        pressure/

        alarms/
```

This structure supports centralized monitoring without namespace conflicts.

---

# 14.19 Performance Recommendations

Recommended polling intervals for applications that do not use event subscriptions:

| Data Type         | Maximum Polling Interval |
| ----------------- | -----------------------: |
| Controller Status |                     30 s |
| Weather           |                    5 min |
| Diagnostics       |                     60 s |
| Statistics        |                    5 min |
| Configuration     |                On demand |

Whenever possible, applications should rely on MQTT subscriptions rather than polling.

---

# 14.20 Security Considerations

Third-party applications shall:

* Use authenticated MQTT sessions
* Support TLS when available
* Validate JSON payloads
* Respect transaction acknowledgements
* Never bypass the command interface
* Never publish directly to state topics

Only documented command topics shall be used to request controller actions.

---

# 14.21 Engineering Notes

The Zmartify integration architecture deliberately separates **transport**, **protocol** and **application logic**.

MQTT provides reliable message transport, while the documented topic hierarchy defines a stable protocol. External applications are therefore insulated from firmware implementation details and can evolve independently.

This layered approach also allows future transports—such as WebSockets, REST or OPC UA—to reuse the same logical API, ensuring long-term compatibility across the expanding Zmartify ecosystem.

---

# 14.22 Chapter Summary

This chapter defines best practices for integrating the Zmartify Irrigation Controller with Node-RED, Grafana, time-series databases, SCADA systems and custom software.

By standardizing on the documented MQTT interface, all third-party applications can exchange data with the controller in a consistent, secure and maintainable manner. The resulting architecture is scalable from single residential installations to multi-controller enterprise deployments and provides a robust foundation for future protocol gateways and visualization platforms.

---

# End of Chapter 14

**Next Chapter**

**Chapter 15 – API Versioning, Compatibility Strategy & Long-Term Interface Stability**

# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 2

# Complete MQTT Namespace & Topic Hierarchy

---

# 2.1 Purpose

This chapter defines the complete MQTT namespace used throughout the Zmartify ecosystem.

The namespace constitutes the public communication interface between the controller and external systems and therefore represents one of the most important long-term compatibility commitments of the project.

Topic names defined within this chapter are considered stable public interfaces.

Future firmware versions shall preserve these topics whenever practical.

---

# 2.2 Design Objectives

The MQTT namespace has been designed to provide:

* Consistent naming
* Human readability
* Predictable hierarchy
* Easy wildcard subscription
* Platform independence
* Future expansion
* Multi-controller support
* Backward compatibility

---

# 2.3 Namespace Philosophy

Every MQTT topic begins with:

```text
zmartify/
```

This root namespace reserves the entire communication space for Zmartify products.

Example:

```text
zmartify/controller/status

zmartify/weather/current

zmartify/zone/03/state

zmartify/alarm/current
```

No other root namespaces shall be used.

---

# 2.4 Namespace Hierarchy

The complete namespace hierarchy is illustrated below.

```text
zmartify/

├── controller/

├── system/

├── weather/

├── irrigation/

├── zones/

├── flow/

├── pressure/

├── relay/

├── sensors/

├── alarms/

├── diagnostics/

├── configuration/

├── statistics/

├── ota/

├── discovery/

├── events/

├── commands/

└── integration/
```

Each branch has a clearly defined ownership and purpose.

---

# 2.5 Topic Naming Rules

The following rules are mandatory.

### MQTT-001

Lowercase only.

Correct

```text
zmartify/flow/current
```

Incorrect

```text
Zmartify/Flow/Current
```

---

### MQTT-002

Use forward slashes only.

---

### MQTT-003

Avoid abbreviations unless universally accepted.

Preferred:

```text
pressure
```

Avoid:

```text
press
```

---

### MQTT-004

Topic names shall describe data rather than implementation.

Correct:

```text
zone/state
```

Avoid:

```text
relay/status
```

when the data logically belongs to the irrigation zone.

---

# 2.6 Controller Namespace

Controller-wide information.

```text
zmartify/controller/
```

Topics include:

```text
status

version

uptime

identity

serial

hardware

firmware

ip

hostname

reboot

heartbeat
```

---

# 2.7 System Namespace

Overall controller state.

```text
zmartify/system/
```

Topics:

```text
state

time

health

load

memory

cpu

restart

shutdown
```

---

# 2.8 Weather Namespace

```text
zmartify/weather/
```

Topics:

```text
current

forecast

hourly

daily

rain

wind

temperature

humidity

pressure

solar

uv

et

recommendation

provider
```

---

# 2.9 Irrigation Namespace

```text
zmartify/irrigation/
```

Topics:

```text
state

program

runtime

remaining

manual

automatic

queue

history

pause

resume
```

---

# 2.10 Zone Namespace

Every irrigation zone receives its own namespace.

Example:

```text
zmartify/zones/01/
```

Subtopics:

```text
state

runtime

remaining

enabled

flow

pressure

statistics

water

budget

soil

plant

schedule

configuration

alarm
```

Zone numbers shall always be two digits.

Example:

```text
01

02

03

...

15
```

---

# 2.11 Flow Namespace

```text
zmartify/flow/
```

Topics:

```text
current

average

today

week

month

year

lifetime

learning

calibration

sensor
```

---

# 2.12 Pressure Namespace

```text
zmartify/pressure/
```

Topics:

```text
current

average

minimum

maximum

learning

calibration

sensor

health
```

---

# 2.13 Relay Namespace

Relay diagnostics.

```text
zmartify/relay/
```

Topics:

```text
status

outputs

fault

board

diagnostics

statistics
```

This namespace is primarily intended for service diagnostics rather than routine automation.

---

# 2.14 Sensor Namespace

Future sensor support.

```text
zmartify/sensors/
```

Examples:

```text
temperature

humidity

soil

rain

flow

pressure

light

leaf

tank
```

The namespace allows future expansion without changing the API structure.

---

# 2.15 Alarm Namespace

```text
zmartify/alarms/
```

Topics:

```text
current

active

critical

warning

information

history

count
```

Critical alarms shall always be published immediately.

---

# 2.16 Diagnostics Namespace

```text
zmartify/diagnostics/
```

Topics:

```text
cpu

memory

storage

tasks

i2c

network

wifi

mqtt

display

health

statistics

selftest
```

---

# 2.17 Configuration Namespace

Configuration exchange.

```text
zmartify/configuration/
```

Topics:

```text
system

zones

programs

weather

network

display

security

backup

restore
```

Configuration updates shall require authentication.

---

# 2.18 Statistics Namespace

Historical information.

```text
zmartify/statistics/
```

Topics:

```text
water

runtime

et

flow

pressure

weather

system

zones
```

Statistics are intended primarily for dashboards and reporting systems.

---

# 2.19 OTA Namespace

```text
zmartify/ota/
```

Topics:

```text
status

progress

available

version

history

rollback

update
```

---

# 2.20 Discovery Namespace

Automatic discovery.

```text
zmartify/discovery/
```

Topics:

```text
controller

entities

capabilities

hardware

software
```

Future products will use the same discovery mechanism.

---

# 2.21 Event Namespace

Controller-generated events.

```text
zmartify/events/
```

Topics:

```text
zone

alarm

weather

system

ota

configuration

diagnostics
```

Events are instantaneous notifications and are generally **not retained**.

---

# 2.22 Command Namespace

Incoming commands.

```text
zmartify/commands/
```

Topics:

```text
start

stop

pause

resume

manual

configuration

reboot

ota
```

Commands are validated before execution.

---

# 2.23 Integration Namespace

Reserved for platform-specific enhancements.

```text
zmartify/integration/
```

Subtopics:

```text
homeio

homeassistant

homey

nodered

grafana
```

This namespace avoids polluting the generic controller API with platform-specific features.

---

# 2.24 Wildcard Subscription Examples

Subscribe to all controller data:

```text
zmartify/#
```

Subscribe to all zone information:

```text
zmartify/zones/+/#
```

Subscribe to all alarms:

```text
zmartify/alarms/#
```

Subscribe to hydraulic data:

```text
zmartify/flow/#

zmartify/pressure/#
```

Subscribe to diagnostics:

```text
zmartify/diagnostics/#
```

These wildcard patterns simplify integration with dashboards and monitoring tools.

---

# 2.25 Topic Ownership

Each namespace has a single owning firmware module.

| Namespace     | Owner                 |
| ------------- | --------------------- |
| controller    | System Manager        |
| weather       | Weather Manager       |
| irrigation    | Irrigation Engine     |
| zones         | Zone Manager          |
| flow          | Flow Manager          |
| pressure      | Pressure Manager      |
| alarms        | Alarm Manager         |
| diagnostics   | Diagnostics Manager   |
| configuration | Configuration Manager |
| ota           | OTA Manager           |

This ownership model prevents conflicting publications.

---

# 2.26 Future Expansion Policy

New functionality shall extend the namespace by **adding** topics rather than renaming existing ones.

Breaking changes shall only occur in conjunction with a new API version.

For example:

```text
API v1

zmartify/flow/current
```

Future additions:

```text
zmartify/flow/efficiency

zmartify/flow/leak_probability

zmartify/flow/prediction
```

Existing integrations remain fully functional.

---

# 2.27 Engineering Notes

The namespace has been designed to support not only the current Zmartify Irrigation Controller but also the long-term vision of a complete outdoor automation ecosystem.

By reserving logical namespaces for future products and enforcing strict ownership rules, the architecture minimizes the risk of incompatibilities while allowing new capabilities to be introduced incrementally.

The use of predictable hierarchies and wildcard-friendly topic structures also simplifies integration with MQTT clients, dashboards and automation engines, reducing implementation complexity for third-party developers.

---

# 2.28 Chapter Summary

This chapter defines the complete MQTT namespace hierarchy used by the Zmartify platform.

The hierarchy establishes a stable, scalable and extensible communication model that supports current firmware functionality while providing ample room for future expansion. Each namespace has a clearly defined purpose and owner, ensuring deterministic behaviour and long-term compatibility across the Zmartify ecosystem.

---

# End of Chapter 2

**Next Chapter**

**Chapter 3 – MQTT Topic Specification: Controller, System & Status Topics**

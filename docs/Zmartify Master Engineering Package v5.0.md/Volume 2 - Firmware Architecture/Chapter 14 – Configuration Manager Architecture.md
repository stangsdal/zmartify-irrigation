# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 14 – Configuration Manager Architecture

---

# 14 Configuration Manager

---

# 14.1 Purpose

The Configuration Manager is responsible for the complete lifecycle of all persistent configuration data within the Zmartify Irrigation Controller.

It provides a centralized, version-controlled and validated configuration service for all firmware modules.

The Configuration Manager ensures that every subsystem always operates with consistent and verified parameters while protecting against configuration corruption, incompatible firmware upgrades and invalid user input.

No firmware component shall directly access persistent storage.

---

# 14.2 Design Objectives

The Configuration Manager shall:

* Maintain all system configuration
* Validate every configuration item
* Support firmware migration
* Maintain configuration versioning
* Protect against corruption
* Provide factory defaults
* Support backup and restore
* Support import/export
* Detect invalid configuration
* Maintain transactional integrity

---

# 14.3 Architectural Position

```text
                  User Interface

                      MQTT

                    HOMEIO

                 Configuration API

                         │

                         ▼

             Configuration Manager

                         │

         ┌───────────────┼───────────────┐

         ▼                               ▼

       NVS                          LittleFS

                         │

                         ▼

                  Firmware Modules
```

The Configuration Manager is the only component permitted to access configuration storage.

---

# 14.4 Configuration Philosophy

Configuration is divided into three classes.

## Static Configuration

Changes rarely.

Examples:

* Wi-Fi
* MQTT
* Hardware mapping
* Display settings

---

## Operational Configuration

Changes occasionally.

Examples:

* Irrigation programs
* Zone configuration
* Seasonal adjustment
* Weather settings

---

## Runtime Configuration

Generated automatically.

Examples:

* Learned flow
* Learned pressure
* ET statistics
* Runtime counters

Runtime data is not directly editable by the user.

---

# 14.5 Configuration Categories

The following categories shall exist.

| Category    | Description         |
| ----------- | ------------------- |
| System      | Controller settings |
| Hardware    | GPIO, relay mapping |
| Network     | Wi-Fi, MQTT         |
| Weather     | Weather services    |
| Irrigation  | Programs            |
| Zones       | Zone definitions    |
| Flow        | Calibration         |
| Pressure    | Calibration         |
| Display     | UI preferences      |
| Diagnostics | Logging             |
| Security    | Authentication      |
| OTA         | Update settings     |

---

# 14.6 Configuration Hierarchy

```text
System

├── Hardware

├── Network

├── Display

├── Irrigation

│     ├── Programs

│     ├── Zones

│     ├── ET

│     ├── Weather

│     └── Hydraulics

├── Diagnostics

└── Security
```

---

# 14.7 Configuration Versioning

Every configuration database shall contain:

* Major Version
* Minor Version
* Build Number
* Schema Version
* Migration Version

Example

```text
Configuration Version

5.0.0

Schema

12
```

Firmware shall automatically migrate older schemas whenever possible.

---

# 14.8 Factory Defaults

Factory defaults shall be stored separately from user configuration.

Factory Reset restores:

* Network
* Irrigation programs
* Zones
* Weather
* Display
* Diagnostics

Factory Reset shall **not** erase:

* Lifetime statistics (optional, configurable)
* Hardware serial number
* Manufacturing information

---

# 14.9 Configuration Validation

Every configuration update shall be validated before being committed.

Validation includes:

* Range checking
* Type checking
* Dependency validation
* Duplicate detection
* Cross-reference validation

Invalid configurations shall be rejected.

---

# 14.10 Transaction Model

Configuration updates shall be atomic.

```text
Receive Update

↓

Validate

↓

Temporary Copy

↓

Commit

↓

Publish Event
```

Partial writes shall never occur.

---

# 14.11 Persistent Storage

Storage strategy:

| Data Type    | Storage  |
| ------------ | -------- |
| Settings     | NVS      |
| Programs     | LittleFS |
| Statistics   | LittleFS |
| Logs         | LittleFS |
| Certificates | NVS      |

Future hardware may replace these storage backends without affecting application code.

---

# 14.12 Configuration API

All firmware modules shall use the Configuration API.

Example:

```c
config_get();

config_set();

config_commit();

config_restore_defaults();

config_import();

config_export();

config_validate();
```

Modules shall never manipulate storage directly.

---

# 14.13 Configuration Locking

To prevent corruption:

Configuration updates shall be protected using mutexes.

Rules:

* One writer
* Multiple readers

Read operations shall never block unnecessarily.

---

# 14.14 Configuration Events

Published events:

```text
EVT_CFG_CHANGED

EVT_CFG_COMMITTED

EVT_CFG_RESET

EVT_CFG_IMPORTED

EVT_CFG_EXPORTED

EVT_CFG_MIGRATED
```

Subscribers include:

* UI Manager
* MQTT Manager
* Irrigation Engine
* Weather Manager
* Diagnostics Manager

---

# 14.15 Zone Configuration

Each zone stores:

* Name
* Relay
* Runtime
* Area
* Plant Type
* Soil Type
* Sprinkler Type
* Flow Baseline
* Pressure Baseline
* ET Settings
* Seasonal Factor

Zone configuration shall be independently editable.

---

# 14.16 Irrigation Program Configuration

Each program includes:

* Name
* Schedule
* Enabled
* Priority
* Start Times
* Assigned Zones
* Runtime Rules
* Cycle & Soak
* Seasonal Behaviour

Unlimited logical programs shall be supported within available storage.

---

# 14.17 Hardware Configuration

Hardware settings include:

* MCP23017 addresses
* ADS1115 configuration
* Flow calibration
* Pressure calibration
* Relay polarity
* Display brightness
* Display timeout
* Touch calibration

Hardware configuration shall normally be hidden from standard users.

---

# 14.18 Weather Configuration

Configurable parameters:

* Weather provider
* API key
* Update interval
* Rain delay
* Freeze threshold
* Wind limits
* Forecast confidence
* ET calculation method

---

# 14.19 Network Configuration

Network settings include:

* SSID
* Password
* Static/DHCP
* MQTT Broker
* Username
* TLS
* Certificates
* DNS
* Time Server

Passwords shall always be stored securely.

---

# 14.20 Import & Export

Configuration shall support export to JSON.

Example:

```text
Export

↓

JSON File

↓

USB

MQTT

Web

Future Cloud
```

Import shall validate:

* Firmware compatibility
* Schema version
* Required fields
* Checksums

---

# 14.21 Backup & Restore

Automatic backup:

* Before firmware update
* Before migration
* Before Factory Reset

Manual backup:

* UI
* MQTT
* USB (future)

Backups shall include integrity verification.

---

# 14.22 Security

Sensitive configuration:

* Passwords
* Certificates
* API Keys

Shall be:

* Encrypted where practical
* Hidden from UI
* Never published through MQTT
* Never written to logs

---

# 14.23 MQTT Integration

Configuration topics:

```text
zmartify/config/system

zmartify/config/network

zmartify/config/weather

zmartify/config/zones

zmartify/config/programs

zmartify/config/display
```

Configuration updates received via MQTT shall undergo the same validation process as local changes.

---

# 14.24 User Interface

Configuration pages include:

* System
* Irrigation
* Zones
* Weather
* Network
* Diagnostics
* Security
* Maintenance

The UI shall indicate whether pending changes require a reboot.

---

# 14.25 Diagnostics

Diagnostic information:

* Schema version
* Configuration age
* Last backup
* Last restore
* Validation failures
* Storage utilization
* Corruption detection
* Migration history

---

# 14.26 Unit Testing

Automated tests shall verify:

* Read/write operations
* Validation
* Migration
* Factory reset
* Backup
* Restore
* Import/export
* Concurrent access
* Corruption recovery

Minimum code coverage:

**95%**

---

# 14.27 Future Enhancements

The architecture supports:

* Cloud configuration
* Multi-controller synchronization
* Git-based configuration history
* Remote deployment
* Configuration templates
* Role-based configuration
* Enterprise fleet management

These enhancements shall not require changes to the Configuration API.

---

# 14.28 Engineering Notes

The Configuration Manager is one of the most critical infrastructure components of the firmware.

By enforcing schema versioning, transactional updates and strict validation, it minimizes the risk of controller malfunction due to invalid configuration while simplifying firmware upgrades and long-term maintenance.

The separation of user-editable configuration from learned runtime data also supports intelligent features such as Flow Learning, Pressure Learning and future AI-assisted optimization without risking accidental data loss.

---

# 14.29 Chapter Summary

The Configuration Manager provides a robust, centralized and future-proof configuration framework for the Zmartify Irrigation Controller.

Its transactional design, strict validation, version control and secure storage strategy ensure reliable controller operation across firmware updates and hardware revisions. As the central authority for all persistent settings, it plays a vital role in system stability, maintainability and long-term product evolution.

---

# End of Chapter 14

**Next Chapter**

**Chapter 15 – Storage Manager Architecture**

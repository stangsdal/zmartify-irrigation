# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 15 – Storage Manager Architecture

---

# 15 Storage Manager

---

# 15.1 Purpose

The Storage Manager provides a unified abstraction for all persistent and semi-persistent data within the Zmartify Irrigation Controller.

It is responsible for ensuring that configuration, statistics, logs, learned hydraulic data and historical information are stored safely, efficiently and reliably throughout the lifetime of the controller.

The Storage Manager provides:

* Persistent configuration
* Historical logging
* Runtime statistics
* Learned hydraulic baselines
* Backup and restore
* Firmware compatibility
* Data integrity verification

All access to persistent storage shall occur through the Storage Manager.

---

# 15.2 Design Objectives

The Storage Manager shall:

* Protect against data corruption
* Minimize Flash wear
* Support transactional writes
* Support automatic recovery
* Support future storage technologies
* Provide version control
* Detect storage failures
* Support backup and restore
* Maintain high performance

---

# 15.3 Architectural Position

```text
               Configuration Manager

                Diagnostics Manager

                  Logging Manager

                 Irrigation Engine

                   Zone Manager

                        │

                        ▼

                 Storage Manager

                        │

        ┌───────────────┼───────────────┐

        ▼                               ▼

        NVS                         LittleFS

                        │

                        ▼

                   ESP-IDF Flash
```

The Storage Manager isolates firmware modules from the underlying storage implementation.

---

# 15.4 Storage Philosophy

Different types of information have different storage requirements.

Therefore storage is divided into independent classes.

| Class              | Storage  |
| ------------------ | -------- |
| Configuration      | NVS      |
| Runtime Statistics | LittleFS |
| Historical Logs    | LittleFS |
| Learning Database  | LittleFS |
| Firmware Metadata  | NVS      |
| Certificates       | NVS      |

This separation minimizes flash wear and simplifies firmware migration.

---

# 15.5 Storage Layers

```text
Application

↓

Configuration Manager

↓

Storage Manager

↓

NVS / LittleFS

↓

Flash Driver

↓

Flash Memory
```

Application modules shall never directly access NVS or LittleFS.

---

# 15.6 Flash Partition Layout

Recommended partition table.

| Partition  | Purpose           |
| ---------- | ----------------- |
| Bootloader | ESP-IDF           |
| OTA Data   | OTA metadata      |
| OTA App 0  | Active firmware   |
| OTA App 1  | Update firmware   |
| NVS        | Configuration     |
| LittleFS   | Logs & Statistics |
| Core Dump  | Diagnostics       |

Additional partitions may be added for future revisions.

---

# 15.7 Data Categories

## Configuration

Stored in NVS.

Examples:

* Wi-Fi
* MQTT
* Zones
* Programs
* Weather

---

## Statistics

Stored in LittleFS.

Examples:

* Runtime
* Water consumption
* ET history
* Alarm statistics

---

## Logs

Stored in LittleFS.

Examples:

* Event Log
* Alarm Log
* Diagnostic Log

---

## Learning Database

Stored in LittleFS.

Examples:

* Flow Learning
* Pressure Learning
* Hydraulic fingerprints
* AI models (future)

---

# 15.8 File Structure

Recommended directory layout.

```text
/config

/statistics

/logs

/learning

/backups

/diagnostics

/temp
```

Future modules may add additional directories.

---

# 15.9 File Formats

Preferred formats:

Configuration:

JSON

Historical logs:

CSV (export)

Binary database (internal)

Backups:

Compressed JSON archive

Future:

SQLite (optional)

---

# 15.10 Write Policy

Flash wear shall be minimized.

Examples:

Configuration

Immediately after modification.

Statistics

Buffered.

Logs

Buffered.

Historical data

Periodic flush.

Frequent values shall never be written directly after every update.

---

# 15.11 Transaction Model

Every write operation shall be atomic.

```text
Prepare

↓

Validate

↓

Write Temporary

↓

Verify

↓

Commit

↓

Delete Previous
```

Power loss during writing shall never corrupt previously committed data.

---

# 15.12 Wear Levelling

LittleFS wear levelling shall be enabled.

Additional strategies:

* Buffered writes
* Log rotation
* Batch updates
* Delayed commits

Flash endurance shall support continuous operation for at least ten years.

---

# 15.13 Data Integrity

Every stored object shall include:

* Version
* CRC32 checksum
* Timestamp
* Size
* Object type

Corrupted records shall be rejected.

---

# 15.14 Automatic Recovery

Upon boot:

```text
Open Storage

↓

Integrity Check

↓

Corruption?

↓

No

↓

Continue

OR

Yes

↓

Restore Backup

↓

Log Event

↓

Continue
```

Recovery shall be automatic whenever possible.

---

# 15.15 Backup Strategy

Automatic backups occur:

* Before firmware update
* Before schema migration
* Before factory reset
* Before major configuration changes

Manual backups may be initiated through:

* User Interface
* MQTT
* Future USB export

---

# 15.16 Restore Strategy

Restore sequence:

```text
Select Backup

↓

Validate Version

↓

Validate CRC

↓

Restore

↓

Verify

↓

Restart Modules
```

Restore shall not require controller reboot unless firmware compatibility demands it.

---

# 15.17 Log Management

Three primary logs are maintained.

Event Log

Alarm Log

Diagnostic Log

Each log shall support:

* Rotation
* Filtering
* Export
* Search

---

# 15.18 Log Rotation

Log rotation prevents uncontrolled storage growth.

Example policy:

| Log         | Maximum Size |
| ----------- | -----------: |
| Event       |        10 MB |
| Alarm       |         5 MB |
| Diagnostics |        20 MB |

Oldest records shall be removed first.

Retention policies shall be configurable.

---

# 15.19 Historical Statistics

Historical information includes:

Daily

* Water usage
* Runtime
* ET

Weekly

Monthly

Yearly

Lifetime

Historical data shall support graphical presentation in the user interface.

---

# 15.20 Learning Database

Learning records include:

Per Zone:

* Learned Flow
* Learned Pressure
* Hydraulic Signature
* Confidence Score
* Learning Date

Future:

* AI optimization data
* Predictive maintenance models

---

# 15.21 Public API

Example interface.

```c
storage_init();

storage_read();

storage_write();

storage_delete();

storage_backup();

storage_restore();

storage_verify();

storage_statistics();

storage_format();
```

All storage operations shall return explicit status codes.

---

# 15.22 Event Subscription

Storage Manager subscribes to:

```text
EVT_CFG_COMMITTED

EVT_ZONE_UPDATED

EVT_FLOW_LEARNED

EVT_PRESS_LEARNED

EVT_ALARM_NEW

EVT_LOG_EVENT
```

Publishes:

```text
EVT_STORAGE_ERROR

EVT_STORAGE_FULL

EVT_STORAGE_BACKUP

EVT_STORAGE_RESTORE
```

---

# 15.23 MQTT Integration

Storage-related MQTT topics:

```text
zmartify/storage/status

zmartify/storage/statistics

zmartify/storage/backup

zmartify/storage/restore

zmartify/storage/health
```

Sensitive file contents shall never be transmitted automatically.

---

# 15.24 Diagnostics

Diagnostic information:

* Flash utilization
* Free space
* Wear estimate
* File count
* Last backup
* Last restore
* CRC failures
* Read errors
* Write errors

Diagnostics shall be available through:

* LVGL
* MQTT
* Diagnostics Manager

---

# 15.25 Performance Targets

| Operation           |  Target |
| ------------------- | ------: |
| Configuration Read  |  <10 ms |
| Configuration Write | <100 ms |
| Statistics Read     |  <20 ms |
| Statistics Write    | <100 ms |
| Backup              |    <5 s |
| Restore             |   <10 s |

These values apply to the reference ESP32-S3 hardware platform.

---

# 15.26 Security

Protected objects include:

* MQTT credentials
* Wi-Fi credentials
* TLS certificates
* API keys

Requirements:

* Stored securely
* Never logged
* Never exported without authorization
* Protected during backup

Future revisions may use hardware-backed secure storage.

---

# 15.27 Unit Testing

Automated tests shall verify:

* Read/write
* Backup
* Restore
* Transaction rollback
* CRC validation
* Corruption recovery
* Flash wear handling
* Log rotation
* Storage limits
* Concurrent access

Minimum code coverage:

**95%**

---

# 15.28 Future Enhancements

The Storage Manager architecture supports:

* External SD card storage
* USB export/import
* Cloud backup
* SQLite database
* Encrypted filesystem
* Remote diagnostics archive
* Automatic maintenance reports
* Multi-controller synchronization

These enhancements shall not require changes to the public Storage Manager API.

---

# 15.29 Engineering Notes

The Storage Manager is designed to ensure that the controller remains reliable over many years of continuous operation.

By separating frequently changing runtime information from infrequently modified configuration data, flash wear is minimized while preserving excellent performance.

The combination of transactional writes, CRC validation, automatic backups and integrity checking provides a highly resilient storage subsystem suitable for industrial-grade embedded applications.

---

# 15.30 Chapter Summary

The Storage Manager provides a robust, scalable and hardware-independent persistence layer for the Zmartify Irrigation Controller.

It safeguards all configuration, historical data and learned hydraulic information while minimizing flash wear and ensuring compatibility across firmware revisions.

As the foundation for configuration management, logging, diagnostics and future analytics, the Storage Manager plays a central role in the long-term reliability and maintainability of the Zmartify platform.

---

# End of Chapter 15

**Next Chapter**

**Chapter 16 – OTA Manager Architecture**

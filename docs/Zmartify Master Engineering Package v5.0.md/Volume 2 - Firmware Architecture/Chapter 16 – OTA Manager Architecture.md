# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 16 – OTA Manager Architecture

---

# 16 OTA Manager

---

# 16.1 Purpose

The OTA (Over-the-Air) Manager is responsible for safely updating the firmware of the Zmartify Irrigation Controller without requiring physical access to the controller.

The OTA Manager shall ensure that firmware updates are:

* Safe
* Reliable
* Recoverable
* Secure
* Fully traceable

Under no circumstances shall an unsuccessful firmware update render the controller inoperable.

The OTA subsystem shall always support rollback to the previous verified firmware image.

---

# 16.2 Design Objectives

The OTA Manager shall:

* Support secure firmware updates
* Validate downloaded firmware
* Support automatic rollback
* Preserve user configuration
* Preserve historical data
* Prevent incompatible firmware installation
* Support staged deployments
* Report update progress
* Maintain update history

---

# 16.3 Architectural Position

```text
                 Firmware Server

                    GitHub

               Zmartify Update Server

                  Local HTTP Server

                        │

                        ▼

                  OTA Manager

                        │

      ┌─────────────────┼────────────────┐

      ▼                                  ▼

 ESP-IDF OTA                    Configuration Manager

      │                                  │

      ▼                                  ▼

 OTA Partition                     Persistent Data
```

The OTA Manager shall be the only component authorized to write firmware partitions.

---

# 16.4 ESP-IDF OTA Architecture

The firmware shall utilize the native ESP-IDF OTA framework.

Recommended partition layout:

| Partition | Purpose                      |
| --------- | ---------------------------- |
| Factory   | Recovery firmware (optional) |
| OTA_0     | Active firmware              |
| OTA_1     | Update firmware              |
| OTA Data  | Boot selection               |
| NVS       | Configuration                |
| LittleFS  | Statistics & Logs            |

This layout supports fully reversible firmware updates.

---

# 16.5 Firmware Update Workflow

```text
Check for Update

↓

Download Metadata

↓

Verify Compatibility

↓

Download Firmware

↓

Verify SHA256

↓

Verify Signature

↓

Write OTA Partition

↓

Verify Flash

↓

Request Restart

↓

Boot New Firmware

↓

Self-Test

↓

Mark Valid

↓

Complete
```

If any step fails, the controller shall remain on the current firmware.

---

# 16.6 Update Sources

Version 5.0 supports:

* GitHub Releases
* Local HTTP Server
* HTTPS URL
* MQTT Update Notification

Future support:

* USB update
* SD Card
* Cloud Device Manager
* Enterprise Update Server

---

# 16.7 Update Policies

Supported update modes:

### Manual

User initiates update.

---

### Notification Only

Controller informs user that an update is available.

---

### Scheduled

Updates occur only during configured maintenance windows.

---

### Automatic

Automatically installs approved updates.

Recommended only after extensive field validation.

---

# 16.8 Firmware Validation

Before installation, firmware shall be validated.

Validation includes:

* Version check
* Hardware compatibility
* SHA256 checksum
* Digital signature
* Image integrity
* Partition size
* Manifest validation

Invalid firmware shall be rejected.

---

# 16.9 Hardware Compatibility

Firmware shall verify compatibility with:

* ESP32-S3
* Hardware Revision
* Display Revision
* Relay Hardware
* MCP23017
* ADS1115

Unsupported hardware shall prevent installation.

---

# 16.10 Configuration Preservation

Configuration shall survive OTA updates.

Preserved items include:

* Wi-Fi
* MQTT
* Zones
* Programs
* Weather settings
* User preferences
* Learned flow
* Learned pressure
* Historical statistics

Configuration migration shall occur automatically if required.

---

# 16.11 Schema Migration

Firmware may introduce new configuration schemas.

Migration sequence:

```text
Boot New Firmware

↓

Read Existing Schema

↓

Migration Required?

↓

Yes

↓

Backup Configuration

↓

Migrate

↓

Validate

↓

Commit

↓

Continue
```

Migration failures shall initiate rollback.

---

# 16.12 Safe Update Conditions

Firmware updates shall not begin if:

* Irrigation is active
* Emergency alarm exists
* Storage is corrupted
* Battery backup is low (future)
* Critical diagnostics are active

The OTA Manager shall postpone updates until conditions are safe.

---

# 16.13 Rollback Strategy

If the new firmware fails:

```text
Boot

↓

Self-Test

↓

Failure?

↓

Yes

↓

Mark Invalid

↓

Boot Previous Firmware

↓

Publish Alarm

↓

Continue Operation
```

Rollback shall occur automatically without user intervention.

---

# 16.14 Firmware Self-Test

Following every update:

Verify:

* Configuration
* Relay subsystem
* Display
* Touch
* MCP23017
* ADS1115
* Flow Manager
* Pressure Manager
* Event Bus
* MQTT

Only after successful verification shall the firmware be marked as valid.

---

# 16.15 Update Progress

Progress stages:

| Stage        | Progress |
| ------------ | -------: |
| Download     |    0–50% |
| Verification |   50–70% |
| Flashing     |   70–90% |
| Restart      |   90–95% |
| Self-Test    |  95–100% |

Progress shall be visible through the display and MQTT.

---

# 16.16 OTA State Machine

```text
Idle

↓

Check

↓

Available

↓

Downloading

↓

Validating

↓

Flashing

↓

Restart Required

↓

Self-Test

↓

Complete
```

Failure states:

* Download Failed
* Verification Failed
* Flash Failed
* Rollback

---

# 16.17 MQTT Integration

Topics:

```text
zmartify/ota/status

zmartify/ota/progress

zmartify/ota/version

zmartify/ota/check

zmartify/ota/update
```

Remote firmware updates shall require authentication.

---

# 16.18 User Interface

OTA pages shall display:

Current firmware

Available firmware

Release notes

Update progress

Estimated remaining time

Rollback status

Update history

Confirmation dialogs shall be presented before major firmware updates.

---

# 16.19 Firmware History

The controller shall maintain:

* Current Version
* Previous Version
* Installation Date
* Installation Result
* Rollback Count
* Update Source

History shall survive firmware updates.

---

# 16.20 Security

Firmware security requirements:

* HTTPS only
* SHA256 validation
* Digital signatures
* Certificate validation
* Secure boot compatible
* Flash encryption compatible

Unsigned firmware shall not be accepted in production mode.

---

# 16.21 Public API

Example interface.

```c
ota_manager_init();

ota_manager_check();

ota_manager_download();

ota_manager_install();

ota_manager_abort();

ota_manager_progress();

ota_manager_status();

ota_manager_rollback();
```

---

# 16.22 Event Subscription

OTA Manager subscribes to:

```text
EVT_SYSTEM_IDLE

EVT_NETWORK_CONNECTED

EVT_STORAGE_READY

EVT_USER_UPDATE_REQUEST
```

Publishes:

```text
EVT_OTA_AVAILABLE

EVT_OTA_STARTED

EVT_OTA_PROGRESS

EVT_OTA_COMPLETED

EVT_OTA_FAILED

EVT_OTA_ROLLBACK
```

---

# 16.23 Diagnostics

Diagnostic information:

* Active firmware
* Previous firmware
* Boot count
* Rollback count
* Last update
* Download speed
* Verification status
* Flash status

---

# 16.24 Unit Testing

Automated tests shall verify:

* Download
* Verification
* Flashing
* Rollback
* Migration
* Interrupted update
* Invalid firmware
* Corrupted firmware
* Network interruption
* Power-loss recovery

Minimum code coverage:

**95%**

---

# 16.25 Future Enhancements

Future capabilities include:

* Differential OTA updates
* Delta patch installation
* Fleet management
* Canary deployments
* Signed enterprise firmware
* Automatic nightly builds
* Remote diagnostics bundle
* Remote recovery mode

The OTA architecture shall remain compatible with these enhancements.

---

# 16.26 Engineering Notes

The OTA subsystem is fundamental to the long-term maintainability of the Zmartify platform.

By leveraging the ESP-IDF OTA framework together with dual firmware partitions, automatic rollback and configuration migration, firmware can be updated safely throughout the controller's operational lifetime.

For production deployments, Secure Boot and Flash Encryption should be enabled to prevent unauthorized firmware installation. The OTA Manager has therefore been designed to remain fully compatible with these ESP32 security features from the outset.

---

# 16.27 Chapter Summary

The OTA Manager provides a secure, resilient and fully recoverable firmware update mechanism for the Zmartify Irrigation Controller.

Its architecture combines ESP-IDF's proven OTA capabilities with robust validation, rollback protection, configuration preservation and comprehensive diagnostics, ensuring that firmware can evolve throughout the product's lifetime without compromising system reliability or user data.

---

# End of Chapter 16

**Next Chapter**

**Chapter 17 – Diagnostics Manager Architecture**

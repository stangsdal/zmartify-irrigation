# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 8

# Configuration Interface, Settings Architecture & System Administration

---

# 8.1 Purpose

This chapter defines the complete configuration architecture of the Zmartify Human–Machine Interface.

The Configuration Interface provides structured access to all controller settings while protecting critical operational parameters through authentication, validation and confirmation workflows.

The interface has been designed for three distinct user groups:

* Homeowners
* Professional Installers
* Service Technicians

Each user shall only have access to configuration functions appropriate for their authorization level.

---

# 8.2 Design Objectives

The Configuration Interface shall:

* Be logically organized
* Minimize configuration errors
* Protect critical parameters
* Support guided configuration
* Provide engineering diagnostics
* Support future controller capabilities
* Maintain consistent navigation
* Support rollback where appropriate

---

# 8.3 Configuration Philosophy

Configuration follows four engineering principles.

### CFG-001

Safe by Default

Every default configuration shall result in safe controller operation.

---

### CFG-002

Guided Configuration

The interface shall guide inexperienced users toward correct settings.

---

### CFG-003

Protected Engineering Settings

Critical engineering parameters shall require elevated authorization.

---

### CFG-004

Immediate Validation

Invalid values shall be detected before they are committed.

---

# 8.4 Configuration Hierarchy

```text id="config-hierarchy"
Settings

│

├── General

├── Irrigation

├── Zones

├── Programs

├── Weather

├── Hydraulics

├── Display

├── Notifications

├── Network

├── MQTT

├── Security

├── Backup

├── Diagnostics

├── Firmware

└── Service
```

Each category shall be represented by a dedicated screen.

---

# 8.5 Settings Home Screen

The Settings Home Screen provides access to all configuration categories.

Reference layout:

```text id="settings-home"
+------------------------------------------------+

General

Irrigation

Weather

Display

Network

Security

Firmware

Service

+------------------------------------------------+
```

Each category shall display a descriptive icon.

---

# 8.6 General Settings

General configuration includes:

* Controller Name
* Language
* Time Zone
* Date Format
* Time Format
* Units
* Regional Settings

Example:

```text id="general-settings"
Controller Name

Garden Controller

Language

English

Units

Metric
```

Changes shall take effect immediately where possible.

---

# 8.7 Irrigation Settings

Available settings:

* Default Runtime
* Water Budget
* Rain Delay
* Seasonal Adjustment
* Pump Delay
* Master Valve Delay
* Maximum Simultaneous Zones (future)

All changes shall be validated by the Application Manager.

---

# 8.8 Zone Configuration

Each irrigation zone supports:

* Zone Name
* Enable/Disable
* Valve Type
* Runtime Limits
* Water Budget
* Flow Calibration
* Pressure Calibration

Example:

```text id="zone-config"
Zone

Front Lawn

Enabled

Yes

Maximum Runtime

30 Minutes
```

---

# 8.9 Program Configuration

Program settings include:

* Program Name
* Schedule
* Start Time
* Active Days
* Assigned Zones
* Runtime
* Weather Adjustment
* ET Adjustment

A visual schedule editor shall be provided.

---

# 8.10 Weather Settings

Configurable parameters:

* Weather Provider
* Update Interval
* ET Calculation Method
* Rain Delay Threshold
* Wind Threshold
* Freeze Threshold

Future weather providers shall integrate without modifying the existing interface.

---

# 8.11 Hydraulic Settings

Hydraulic configuration includes:

* Flow Sensor Calibration
* Pressure Sensor Calibration
* Leak Detection Sensitivity
* Restriction Sensitivity
* Learning Mode
* Alarm Thresholds

Engineering settings shall be protected by authorization.

---

# 8.12 Display Settings

Display configuration includes:

* Brightness
* Automatic Brightness
* Day Theme
* Night Theme
* Screen Timeout
* Sleep Mode
* Screen Saver

Example:

```text id="display-settings"
Brightness

75 %

Screen Timeout

5 Minutes

Theme

Automatic
```

Changes shall be previewed immediately.

---

# 8.13 Notification Settings

Users may configure:

* Alarm Notifications
* Informational Messages
* Push Notifications
* Email (future)
* MQTT Notifications
* Sound
* Vibration (future)

Notification priorities shall remain fixed by firmware.

---

# 8.14 Network Settings

Network configuration includes:

* Wi-Fi SSID
* Static/DHCP
* IP Address
* Gateway
* DNS
* Hostname

Example:

```text id="network-settings"
Wi-Fi

Connected

IP

192.168.1.40

DHCP

Enabled
```

Network changes shall require confirmation before activation.

---

# 8.15 MQTT Settings

MQTT configuration includes:

* Broker Address
* Port
* Username
* TLS
* Client ID
* Discovery
* Topic Prefix

Passwords shall never be displayed in plaintext.

Connection status shall be verified before saving.

---

# 8.16 Security Settings

Security configuration includes:

* User Accounts
* Password Policy
* Administrator PIN
* Automatic Lock
* TLS Certificates
* Secure Boot Status
* Flash Encryption Status

Critical security changes shall require administrator authentication.

---

# 8.17 Backup & Restore

Available functions:

* Backup Configuration
* Restore Configuration
* Export Settings
* Import Settings
* Factory Defaults

Example workflow:

```text id="backup-workflow"
Backup

↓

Create Archive

↓

Verify

↓

Store

↓

Complete
```

Restores shall always require confirmation.

---

# 8.18 Firmware Settings

Displays:

* Current Version
* Available Update
* Build Number
* Hardware Revision
* Release Notes

Actions:

* Check for Updates
* Install Update
* View History

Firmware installation shall invoke the OTA Manager described in Volume 2.

---

# 8.19 Service Settings

Restricted functions include:

* Calibration
* Diagnostic Logging
* Factory Test
* Hardware Information
* Engineering Parameters
* Event Log Export

Service Mode shall remain clearly indicated.

---

# 8.20 Configuration Validation

Every modified parameter shall pass through the following sequence:

```text id="config-validation"
User Input

↓

Range Validation

↓

Dependency Validation

↓

Application Validation

↓

Confirmation

↓

Commit

↓

Save

↓

Notify
```

Invalid values shall never be committed.

---

# 8.21 Confirmation Dialogs

Confirmation is required for:

* Factory Reset
* Restore Backup
* Network Changes
* Security Changes
* Firmware Update
* Delete Program
* Delete Zone

Example:

```text id="confirmation-dialog"
Apply Network Changes?

Connection will restart.

[Cancel]

[Apply]
```

---

# 8.22 Search Function

The Settings interface shall include a searchable configuration index.

Search examples:

* MQTT
* Brightness
* Rain Delay
* Flow Calibration
* Password

Search results shall navigate directly to the relevant configuration page.

---

# 8.23 User Roles

Configuration visibility depends on user role.

| Function    | User | Installer | Service | Administrator |
| ----------- | :--: | :-------: | :-----: | :-----------: |
| Display     |   ✔  |     ✔     |    ✔    |       ✔       |
| Weather     |   ✔  |     ✔     |    ✔    |       ✔       |
| Network     |   ✖  |     ✔     |    ✔    |       ✔       |
| MQTT        |   ✖  |     ✔     |    ✔    |       ✔       |
| Calibration |   ✖  |     ✖     |    ✔    |       ✔       |
| Security    |   ✖  |     ✖     |    ✖    |       ✔       |

Role changes take effect immediately after authentication.

---

# 8.24 Performance Targets

| Operation           |  Target |
| ------------------- | ------: |
| Open Settings       | <150 ms |
| Apply Configuration | <250 ms |
| Search              | <200 ms |
| Save Configuration  | <500 ms |
| Validation          | <100 ms |

Configuration changes shall not interrupt ongoing irrigation unless explicitly required.

---

# 8.25 Future Extensions

The Configuration Framework supports future capabilities including:

* Cloud synchronization
* Remote configuration
* Configuration templates
* Multi-controller management
* QR-code setup
* NFC provisioning
* Bluetooth commissioning
* AI configuration advisor

These features shall integrate without changing the established configuration hierarchy.

---

# 8.26 Engineering Notes

The Configuration Interface has been designed to balance simplicity for everyday users with the flexibility required by installers and service personnel. By organizing settings into clearly defined functional categories and enforcing validation before configuration changes are committed, the interface minimizes the risk of misconfiguration while maintaining a professional engineering workflow.

Role-based access control further ensures that advanced calibration and security parameters remain protected without complicating routine operation for residential users.

---

# 8.27 Chapter Summary

This chapter defines the Configuration Interface, including settings organization, validation workflows, role-based access control and system administration functions.

The architecture provides a scalable and secure framework for configuring all aspects of the Zmartify Irrigation Controller while maintaining consistency with the overall HMI design and preserving the controller's autonomous operation.

---

# End of Chapter 8

**Next Chapter**

**Chapter 9 – Diagnostics, Alarm Management & Service Interface**

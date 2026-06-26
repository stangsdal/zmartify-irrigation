# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 8

# MQTT Topic Specification – Configuration, OTA & Controller Management

---

# 8.1 Purpose

This chapter defines the MQTT interface used for configuration management, firmware updates (OTA) and controller administration.

Unlike telemetry topics, these interfaces modify controller behaviour and therefore require authentication and authorization before execution.

The Configuration Manager, OTA Manager and System Manager together provide a transactional management interface that supports both local operation and remote administration.

---

# 8.2 Design Objectives

The management interface shall:

* Allow secure remote configuration
* Support transactional configuration updates
* Support firmware management
* Enable configuration backup and restore
* Report controller capabilities
* Maintain complete auditability
* Prevent unauthorized modifications
* Preserve operational safety during updates

---

# 8.3 Namespace

Configuration namespace:

```text
zmartify/configuration/
```

OTA namespace:

```text
zmartify/ota/
```

Management namespace:

```text
zmartify/controller/
```

Command namespace:

```text
zmartify/commands/
```

Primary topics:

```text
configuration/system
configuration/network
configuration/zones
configuration/programs
configuration/weather
configuration/display
configuration/security
configuration/backup
configuration/restore

ota/status
ota/progress
ota/version
ota/update
ota/history
ota/rollback

controller/reboot
controller/restart
controller/factory_reset
controller/maintenance
controller/capabilities
```

---

# 8.4 Configuration Philosophy

Configuration changes shall never modify the controller immediately.

Every configuration update follows a transactional workflow.

```text
Receive Configuration

↓

Authenticate

↓

Validate

↓

Apply to Staging

↓

Commit

↓

Publish Result
```

If validation fails, the existing configuration shall remain active.

---

# 8.5 System Configuration

Topic

```text
zmartify/configuration/system
```

QoS

```
1
```

Retained

```
Yes
```

Example

```json
{
  "payload":
  {
      "controller_name":"Garden Controller",
      "timezone":"Europe/Copenhagen",
      "language":"en",
      "units":"metric",
      "display_sleep":600
  }
}
```

---

# 8.6 Network Configuration

Topic

```text
zmartify/configuration/network
```

Example

```json
{
  "payload":
  {
      "hostname":"zmartify-controller",
      "mqtt_server":"192.168.1.10",
      "mqtt_port":1883,
      "ntp_server":"pool.ntp.org",
      "dhcp":true
  }
}
```

Sensitive information such as passwords shall never be published.

---

# 8.7 Zone Configuration

Topic

```text
zmartify/configuration/zones
```

Example

```json
{
  "payload":
  {
      "zone":4,
      "name":"Front Lawn",
      "enabled":true,
      "runtime":900,
      "soil":"Loam",
      "plant":"Turf",
      "sprinkler":"MP Rotator"
  }
}
```

---

# 8.8 Program Configuration

Topic

```text
zmartify/configuration/programs
```

Example

```json
{
  "payload":
  {
      "program":"Morning",
      "enabled":true,
      "start":"06:00",
      "days":[
          "Mon",
          "Wed",
          "Fri"
      ]
  }
}
```

---

# 8.9 Weather Configuration

Topic

```text
zmartify/configuration/weather
```

Example

```json
{
  "payload":
  {
      "provider":"OpenWeather",
      "automatic_et":true,
      "rain_delay":6,
      "freeze_threshold":2.0
  }
}
```

---

# 8.10 Display Configuration

Topic

```text
zmartify/configuration/display
```

Example

```json
{
  "payload":
  {
      "theme":"Dark",
      "brightness":70,
      "sleep_timeout":600,
      "language":"en"
  }
}
```

---

# 8.11 Security Configuration

Topic

```text
zmartify/configuration/security
```

Published values shall never include confidential credentials.

Example

```json
{
  "payload":
  {
      "authentication":"Enabled",
      "secure_boot":true,
      "flash_encryption":true,
      "tls":true
  }
}
```

---

# 8.12 Configuration Backup

Topic

```text
zmartify/configuration/backup
```

Purpose

Initiates or reports configuration backups.

Example

```json
{
  "payload":
  {
      "backup_id":"CFG-20260714-001",
      "timestamp":"2026-07-14T18:00:00Z",
      "status":"Completed"
  }
}
```

Backups shall include:

* Zones
* Programs
* Weather settings
* Network settings
* User preferences
* Learned hydraulic values

---

# 8.13 Configuration Restore

Command

```text
zmartify/commands/configuration/restore
```

Example

```json
{
  "backup_id":"CFG-20260714-001"
}
```

The Configuration Manager shall validate compatibility before restoration.

---

# 8.14 OTA Status

Topic

```text
zmartify/ota/status
```

Example

```json
{
  "payload":
  {
      "state":"Idle",
      "current_version":"5.0.0",
      "update_available":true
  }
}
```

---

# 8.15 OTA Progress

Topic

```text
zmartify/ota/progress
```

Published during firmware updates.

Example

```json
{
  "payload":
  {
      "stage":"Downloading",
      "progress":42,
      "remaining_seconds":26
  }
}
```

Stages:

* Checking
* Downloading
* Verifying
* Flashing
* Restarting
* Self-Test
* Completed
* Failed

---

# 8.16 OTA Version

Topic

```text
zmartify/ota/version
```

Example

```json
{
  "payload":
  {
      "installed":"5.0.0",
      "available":"5.1.0",
      "mandatory":false
  }
}
```

---

# 8.17 OTA Update Command

Command

```text
zmartify/commands/ota/update
```

Example

```json
{
    "version":"5.1.0"
}
```

The OTA Manager shall verify:

* Authentication
* Firmware compatibility
* Digital signature
* SHA-256 checksum
* Available storage
* Safe operating state

Only after successful validation shall installation begin.

---

# 8.18 OTA Rollback

Command

```text
zmartify/commands/ota/rollback
```

Example

```json
{
    "rollback":true
}
```

Rollback requires administrator authorization.

---

# 8.19 Controller Restart

Command

```text
zmartify/commands/reboot
```

Example

```json
{
    "reason":"Maintenance"
}
```

Before rebooting the controller shall:

1. Stop irrigation safely
2. Save runtime data
3. Flush pending logs
4. Publish reboot event

---

# 8.20 Factory Reset

Command

```text
zmartify/commands/factory_reset
```

Example

```json
{
    "confirm":true
}
```

Factory Reset requires:

* Administrator authentication
* Confirmation
* Audit logging

The following shall not be erased:

* Serial number
* Hardware identification
* Bootloader
* Manufacturing data

---

# 8.21 Controller Capabilities

Topic

```text
zmartify/controller/capabilities
```

Example

```json
{
  "payload":
  {
      "zones":15,
      "supports_ota":true,
      "supports_weather":true,
      "supports_flow_learning":true,
      "supports_pressure_learning":true,
      "supports_homeio":true,
      "supports_homeassistant":true
  }
}
```

This topic allows clients to adapt dynamically to controller capabilities.

---

# 8.22 Publish Rates

| Topic           |          Interval |
| --------------- | ----------------: |
| configuration/* |         On change |
| ota/status      |              60 s |
| ota/progress    | 1 s during update |
| ota/version     |      Boot / Check |
| capabilities    |              Boot |
| reboot          |             Event |

---

# 8.23 QoS Policy

| Topic         | QoS | Retained |
| ------------- | :-: | :------: |
| configuration |  1  |    Yes   |
| ota/status    |  1  |    Yes   |
| ota/progress  |  1  |    No    |
| ota/version   |  1  |    Yes   |
| commands      |  2  |    No    |
| reboot        |  1  |    No    |

All management commands shall use **QoS 2**.

---

# 8.24 Security Requirements

Every incoming management command shall require:

* Authentication
* Authorization
* Payload validation
* Schema validation
* Audit logging

Unauthorized commands shall be rejected without revealing sensitive system information.

---

# 8.25 Transaction Response

Every management command shall generate a response.

Example

```json
{
  "payload":
  {
      "status":"Accepted",
      "transaction_id":"TX-000124",
      "message":"Configuration committed successfully."
  }
}
```

Failed transactions shall include a descriptive error code.

---

# 8.26 Engineering Notes

The Configuration and Management interfaces are designed around the principle of **transactional integrity**. Configuration changes are staged, validated and committed as atomic operations, ensuring that incomplete or invalid updates cannot leave the controller in an inconsistent state.

Separating configuration, firmware management and operational control also allows future cloud services and supervisory systems to manage controllers safely without bypassing internal validation or safety mechanisms.

---

# 8.27 Chapter Summary

This chapter defines the MQTT interface for controller configuration, firmware management and administrative operations.

The interface provides secure, transactional management capabilities while preserving controller reliability, auditability and safety. By enforcing authentication, validation and rollback support, it enables robust remote administration suitable for both residential and professional deployments.

---

# End of Chapter 8

**Next Chapter**

**Chapter 9 – MQTT Topic Specification: Events, Discovery & Smart-Home Integration**

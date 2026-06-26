# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 18

# Security Architecture, Authentication & MQTT Hardening

---

# 18.1 Purpose

This chapter defines the security architecture protecting all external communications with the Zmartify Irrigation Controller.

The objective is to ensure that the controller remains secure while preserving the simplicity and reliability of its MQTT-based communication model.

Security is implemented as a layered architecture spanning:

* Device identity
* Network security
* MQTT authentication
* Transport encryption
* Authorization
* Command validation
* Firmware integrity
* Audit logging

The controller shall continue operating safely even when communication security is compromised.

---

# 18.2 Security Design Objectives

The communication security architecture shall:

* Protect controller integrity
* Prevent unauthorized control
* Protect configuration data
* Support encrypted communication
* Support local-first operation
* Prevent replay attacks
* Protect firmware authenticity
* Maintain complete auditability

---

# 18.3 Security Philosophy

The Zmartify controller follows the **Defense in Depth** principle.

Security shall never depend upon a single mechanism.

Protection layers include:

```text
Application Layer

↓

Authorization

↓

Authentication

↓

MQTT Security

↓

TLS Encryption

↓

Wi-Fi Security

↓

Physical Security
```

Failure of one layer shall not compromise the entire system.

---

# 18.4 Security Principles

The following principles apply throughout the platform.

### SEC-001

Least Privilege

Users and applications shall receive only the permissions necessary to perform their intended functions.

---

### SEC-002

Fail Safe

Authentication failures shall result in command rejection.

The controller shall continue autonomous irrigation according to its programmed schedules.

---

### SEC-003

Secure by Default

Security features shall be enabled by default.

Users may reduce security only through explicit administrative configuration.

---

### SEC-004

No Security Through Obscurity

Security shall rely on cryptographic mechanisms rather than hidden implementation details.

---

# 18.5 Threat Model

The controller is designed to mitigate:

* Unauthorized MQTT clients
* Spoofed MQTT messages
* Replay attacks
* Malformed JSON payloads
* Rogue firmware
* Unauthorized configuration changes
* Wi-Fi interception
* Broker compromise
* Denial-of-Service attempts

The controller is not intended to defend against physical hardware attacks with unrestricted device access.

---

# 18.6 MQTT Authentication

The controller supports authenticated MQTT sessions.

Supported mechanisms:

| Method                 | Status                         |
| ---------------------- | ------------------------------ |
| Username/Password      | Mandatory                      |
| TLS Client Certificate | Recommended                    |
| Anonymous Access       | Optional (disabled by default) |

Anonymous access shall never permit management commands.

---

# 18.7 MQTT Authorization

Permissions shall be assigned by topic namespace.

Example authorization matrix:

| Topic                      | Read |          Write         |
| -------------------------- | :--: | :--------------------: |
| `zmartify/controller/#`    |   ✔  |            ✖           |
| `zmartify/weather/#`       |   ✔  |            ✖           |
| `zmartify/flow/#`          |   ✔  |            ✖           |
| `zmartify/events/#`        |   ✔  |            ✖           |
| `zmartify/commands/#`      |   ✖  | ✔ (Authorized Clients) |
| `zmartify/configuration/#` |   ✔  |   Administrator Only   |

The MQTT broker should enforce topic-level access control where supported.

---

# 18.8 User Roles

The controller recognizes four logical roles.

| Role          | Permissions                           |
| ------------- | ------------------------------------- |
| Viewer        | Read-only telemetry                   |
| Operator      | Manual irrigation, pause/resume       |
| Administrator | Configuration, OTA, backup, restore   |
| Service       | Diagnostics, calibration, maintenance |

Role assignment is external to the MQTT protocol and may be implemented by the broker or an identity provider.

---

# 18.9 TLS Support

MQTT over TLS is recommended for all production installations.

Supported versions:

* TLS 1.2
* TLS 1.3 (preferred)

Older SSL protocols shall not be supported.

---

# 18.10 Certificate Validation

When TLS is enabled, the controller shall validate:

* Server certificate chain
* Certificate expiration
* Hostname (when applicable)
* Trusted Certificate Authority

Connections with invalid certificates shall be rejected.

---

# 18.11 Secure MQTT Topics

Sensitive operations include:

```text
zmartify/commands/

zmartify/configuration/

zmartify/ota/

zmartify/security/
```

These topics shall require authenticated and authorized sessions.

---

# 18.12 Replay Protection

Every command shall include:

* Transaction ID
* UTC timestamp

The controller shall reject:

* Duplicate transaction IDs
* Expired timestamps
* Commands outside the allowable execution window

Default timeout:

```text
60 seconds
```

---

# 18.13 Payload Validation

Incoming payloads shall pass the following sequence:

```text
UTF-8 Validation

↓

JSON Parsing

↓

JSON Schema Validation

↓

Authorization

↓

Business Rules

↓

Execution
```

Any failure shall terminate processing immediately.

---

# 18.14 Rate Limiting

The MQTT Manager shall protect against excessive command traffic.

Recommended limits:

| Operation               |        Limit |
| ----------------------- | -----------: |
| Commands                |       10/sec |
| Configuration Changes   |        1/sec |
| OTA Requests            |        1/min |
| Authentication Failures | Configurable |

Rate limits shall be configurable.

---

# 18.15 Audit Logging

Security-relevant events shall be logged.

Examples:

* Successful login
* Failed authentication
* Configuration change
* Firmware update
* Factory reset
* User role change
* Command rejection
* Certificate validation failure

Each audit record shall include:

* Timestamp
* Device ID
* User (if known)
* Origin
* Event
* Result

---

# 18.16 Firmware Integrity

OTA firmware shall be verified before installation.

Verification shall include:

* SHA-256 checksum
* Digital signature
* Hardware compatibility
* Firmware compatibility
* Image integrity

Unsigned firmware shall not be installed.

---

# 18.17 Secure Boot

The ESP32-S3 Secure Boot feature is recommended for production hardware.

Benefits include:

* Verified bootloader
* Verified application image
* Protection against unauthorized firmware
* Prevention of persistent malware

Secure Boot shall be enabled for production releases where manufacturing processes support key provisioning.

---

# 18.18 Flash Encryption

ESP32-S3 Flash Encryption is recommended for production devices.

Protected information includes:

* Configuration
* Credentials
* Certificates
* Calibration data
* Stored secrets

Sensitive data shall not be stored in plaintext.

---

# 18.19 Credential Management

Credentials shall never be:

* Published over MQTT
* Logged in plaintext
* Included in diagnostic payloads
* Exported in backups without encryption

When credentials must be stored, they shall be protected using appropriate cryptographic mechanisms supported by the ESP32-S3 platform.

---

# 18.20 Denial-of-Service Protection

The controller shall remain operational during communication attacks.

If MQTT communication becomes unavailable:

* Autonomous irrigation shall continue.
* Scheduled programs shall continue.
* Hydraulic safety shall remain active.
* Local user interface shall remain operational.

The communication subsystem shall never become a single point of failure.

---

# 18.21 Security Event Topics

Security-related events shall be published under:

```text
zmartify/events/security
```

Examples:

```json
{
  "payload":
  {
      "event":"Authentication Failed",
      "origin":"MQTT",
      "severity":"Warning"
  }
}
```

These events shall not expose sensitive credential information.

---

# 18.22 Recommended Deployment

Recommended production architecture:

```text
                 Internet
                     │
              Firewall / Router
                     │
              MQTT Broker (TLS)
                     │
             Authenticated Clients
                     │
         ┌───────────┴───────────┐
         │                       │
 HOMEIO / Home Assistant     Homey
         │
     Zmartify Controller
```

The controller should reside on a trusted local network segment.

---

# 18.23 Security Compliance

The communication architecture has been designed to align with the principles of:

* IEC 62443 (Industrial Automation Security)
* OWASP IoT Top 10
* NIST Cybersecurity Framework (general guidance)
* MQTT Security Best Practices

This document is not intended as a formal certification statement.

---

# 18.24 Engineering Notes

The Zmartify security architecture reflects the platform's local-first philosophy. Rather than depending on external cloud authentication services, the controller remains fully operational within a secure local network while supporting industry-standard security mechanisms such as TLS, authenticated MQTT sessions, Secure Boot and Flash Encryption.

Separating authentication, authorization and command validation into distinct layers minimizes the impact of individual failures and provides a robust foundation for future enterprise deployments and cloud gateways.

---

# 18.25 Chapter Summary

This chapter defines the security architecture protecting the Zmartify MQTT API and management interfaces.

By combining authenticated MQTT communication, layered authorization, payload validation, firmware integrity verification and comprehensive audit logging, the platform provides a secure yet flexible communication framework suitable for residential, commercial and industrial irrigation applications.

---

# End of Chapter 18

**Next Chapter**

**Chapter 19 – REST API & WebSocket Architecture (Future Interface Specification)**

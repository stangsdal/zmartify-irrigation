# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 19 – Security Architecture

---

# 19 Security Architecture

---

# 19.1 Purpose

The Security Architecture defines the technical and operational security framework for the Zmartify Irrigation Controller.

Although the controller is primarily an irrigation appliance, it is also a permanently connected IoT device that communicates with external systems through Wi-Fi and MQTT. Consequently, security shall be considered a fundamental system requirement rather than an optional feature.

The objectives are to:

* Protect the irrigation system
* Protect user configuration
* Protect network credentials
* Prevent unauthorized operation
* Ensure firmware integrity
* Protect smart-home integrations
* Support secure future cloud services

Security shall be implemented using a **defense-in-depth** strategy, ensuring that failure of a single mechanism does not compromise the entire controller.

---

# 19.2 Security Objectives

The controller shall ensure:

* Confidentiality
* Integrity
* Availability
* Authenticity
* Accountability

These principles apply equally to:

* Firmware
* Configuration
* Communications
* User authentication
* Diagnostics
* OTA updates

---

# 19.3 Security Architecture

```text
                 User

                  │

                  ▼

          Authentication Layer

                  │

                  ▼

          Authorization Layer

                  │

                  ▼

        Application Services

                  │

        ┌─────────┼─────────┐

        ▼                   ▼

 Secure Storage      Secure Communication

        │                   │

        ▼                   ▼

 ESP32 Security      TLS / MQTT / HTTPS

        │

        ▼

 Secure Boot & Flash Encryption
```

---

# 19.4 Security Philosophy

The Zmartify controller follows six security principles.

### SEC-001

Least Privilege

Every software component shall have only the permissions required to perform its task.

---

### SEC-002

Secure by Default

All security-sensitive features shall be enabled by default where practical.

---

### SEC-003

Fail Secure

Security failures shall deny access rather than permit it.

---

### SEC-004

Separation of Duties

Authentication, authorization and application logic shall remain independent.

---

### SEC-005

No Plaintext Secrets

Passwords and API keys shall never be exposed through:

* MQTT
* Log files
* Debug output
* User Interface

---

### SEC-006

Auditability

Security-relevant events shall always be logged.

---

# 19.5 Threat Model

Primary threats include:

| Threat                   | Protection           |
| ------------------------ | -------------------- |
| Unauthorized MQTT access | Authentication + TLS |
| Wi-Fi compromise         | WPA2/WPA3            |
| Firmware tampering       | Secure Boot          |
| Malicious firmware       | Signed OTA           |
| Configuration corruption | CRC + Validation     |
| Physical reset           | Authentication       |
| Replay attacks           | TLS                  |
| Credential theft         | Secure Storage       |

---

# 19.6 Authentication

Supported authentication mechanisms:

### Local Display

Administrator PIN

Future:

Password

Future:

RFID

Future:

Biometric

---

### MQTT

Username

Password

TLS Certificates

Future OAuth

---

### OTA

HTTPS

Digital Signature

Certificate Validation

---

# 19.7 User Roles

Supported roles:

| Role          | Permissions                    |
| ------------- | ------------------------------ |
| Guest         | View only                      |
| Operator      | Manual irrigation              |
| Administrator | Configuration                  |
| Installer     | Service Mode                   |
| Developer     | Debug (disabled in production) |

Each user action shall be checked against assigned permissions.

---

# 19.8 Authorization

Every security-sensitive operation shall require authorization.

Examples:

Administrator required:

* Factory Reset
* OTA Installation
* Network Configuration
* Hardware Configuration
* Firmware Rollback

Operator access:

* Manual irrigation
* Pause
* Resume
* Rain Delay

Guest access:

* Dashboard
* Statistics
* Weather
* Water Usage

---

# 19.9 Secure Storage

Sensitive information includes:

* Wi-Fi credentials
* MQTT credentials
* API keys
* TLS certificates
* Administrator PIN

Storage requirements:

* Never stored in plaintext where practical
* Never exported without authorization
* Never transmitted over unsecured channels

Future hardware revisions may leverage ESP32 hardware-backed key storage.

---

# 19.10 Secure Communication

External communication shall use encrypted protocols whenever available.

Supported:

* HTTPS
* TLS 1.2+
* MQTT over TLS
* NTP with certificate validation (future)

Plain HTTP shall only be permitted in explicitly enabled development mode.

---

# 19.11 Secure Boot

Production firmware should enable:

ESP32 Secure Boot

Benefits:

* Prevents unauthorized firmware execution
* Verifies firmware authenticity
* Protects against firmware replacement attacks

Development firmware may disable Secure Boot for debugging.

---

# 19.12 Flash Encryption

Production firmware should enable:

ESP32 Flash Encryption

Benefits:

* Protects stored credentials
* Protects firmware image
* Protects configuration
* Prevents offline extraction

Development builds may disable encryption to simplify debugging.

---

# 19.13 OTA Security

OTA updates shall require:

* HTTPS transport
* SHA256 validation
* Firmware signature verification
* Certificate validation
* Version compatibility

Unsigned firmware shall be rejected in production mode.

---

# 19.14 Session Management

Authenticated UI sessions shall automatically expire.

Default timeout:

15 minutes

Configurable:

* 5 min
* 10 min
* 15 min
* 30 min
* Never (Installer Mode only)

---

# 19.15 Audit Logging

Security events shall be recorded.

Examples:

* Login
* Logout
* Failed login
* Configuration change
* Firmware update
* Factory Reset
* OTA rollback
* Administrator actions

Audit logs shall be retained independently from operational logs.

---

# 19.16 MQTT Security

MQTT requirements:

* Client authentication
* Optional client certificates
* TLS encryption
* Unique Client ID
* Topic authorization (broker dependent)

The MQTT Manager shall reject unauthenticated command messages.

---

# 19.17 Configuration Protection

Configuration changes shall require:

1. Authentication
2. Validation
3. Transaction commit
4. Audit logging

Critical configuration changes shall optionally require user confirmation.

---

# 19.18 Factory Reset Protection

Factory Reset shall require:

Administrator authentication

Confirmation dialog

Optional long-press hardware button

Factory Reset shall never erase:

* Hardware serial number
* Manufacturing information
* Bootloader
* Security certificates (optional policy)

---

# 19.19 Service Mode Security

Engineering Service Mode shall require:

* Administrator role
* Authentication
* Audit logging

Optional automatic timeout:

30 minutes

All Service Mode actions shall be logged.

---

# 19.20 Network Security

Network protections include:

* WPA2/WPA3
* Static IP (optional)
* DNS validation
* Secure NTP
* TLS certificates
* MQTT authentication

Future support:

* VLAN awareness
* IPv6
* mTLS

---

# 19.21 API Security

Future REST or WebSocket APIs shall require:

* Authentication
* Authorization
* HTTPS
* Rate limiting
* Request validation

API versioning shall be mandatory.

---

# 19.22 Engineering Mode

Development firmware may expose:

* Debug console
* Verbose logging
* JTAG
* Memory inspection

Production firmware shall disable or restrict these features.

---

# 19.23 Security Diagnostics

Diagnostic parameters include:

* Secure Boot status
* Flash Encryption status
* TLS status
* Certificate validity
* Failed login count
* Session count
* OTA signature verification
* Firmware authenticity

---

# 19.24 Event Integration

Security-related events:

```text
EVT_SECURITY_LOGIN

EVT_SECURITY_LOGOUT

EVT_SECURITY_FAILURE

EVT_SECURITY_LOCKOUT

EVT_SECURITY_CONFIGURATION

EVT_SECURITY_OTA

EVT_SECURITY_FACTORY_RESET
```

Subscribers include:

* Alarm Manager
* Diagnostics Manager
* Logging Manager

---

# 19.25 Public API

Example interface.

```c
security_init();

security_login();

security_logout();

security_authorize();

security_verify_signature();

security_get_role();

security_is_authenticated();

security_lock();

security_unlock();
```

The Security Manager shall provide a single interface for authentication and authorization services.

---

# 19.26 Unit Testing

Automated tests shall verify:

* Authentication
* Authorization
* Session timeout
* Secure storage
* TLS configuration
* OTA signature validation
* Role enforcement
* Audit logging
* Factory Reset protection

Minimum code coverage:

**95%**

---

# 19.27 Future Enhancements

The architecture supports:

* Multi-factor authentication
* Hardware security modules
* FIDO2 authentication
* Cloud identity providers
* Secure remote commissioning
* Zero-touch provisioning
* Enterprise certificate management

These features shall integrate without breaking the existing Security API.

---

# 19.28 Engineering Notes

The Security Architecture has been designed to balance usability with robust protection.

For most residential installations, username/password authentication combined with TLS and secure OTA updates will provide an appropriate level of protection. For commercial or municipal deployments, enabling Secure Boot, Flash Encryption and certificate-based authentication will significantly increase resilience against unauthorized access and firmware tampering.

The modular security framework also positions the Zmartify platform for future cloud connectivity and enterprise-scale device management.

---

# 19.29 Chapter Summary

The Security Architecture provides a comprehensive framework for protecting the Zmartify Irrigation Controller against unauthorized access, firmware tampering and data compromise.

By combining authentication, authorization, secure storage, encrypted communications, firmware integrity verification and detailed audit logging, the architecture supports both residential and professional deployments while remaining scalable for future cloud services and enterprise integrations.

---

# End of Chapter 19

**Next Chapter**

**Chapter 20 – Coding Standards, Development Guidelines & Software Quality Assurance**

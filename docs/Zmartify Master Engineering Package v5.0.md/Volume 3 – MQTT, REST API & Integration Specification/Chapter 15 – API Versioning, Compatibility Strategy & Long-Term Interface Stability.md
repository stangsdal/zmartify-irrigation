# Zmartify Master Engineering Package v5.0

# Volume 3

# MQTT, API & Integration Specification

## Chapter 15

# API Versioning, Compatibility Strategy & Long-Term Interface Stability

---

# 15.1 Purpose

This chapter defines the long-term compatibility strategy governing the Zmartify MQTT API and all future communication interfaces.

The objective is to ensure that integrations developed today continue functioning across future firmware releases with minimal or no modification.

API stability is considered a fundamental engineering requirement of the Zmartify ecosystem.

---

# 15.2 Design Objectives

The compatibility strategy shall:

* Preserve existing integrations
* Minimize breaking changes
* Support incremental feature additions
* Allow multiple firmware generations
* Maintain predictable behavior
* Enable future protocol expansion
* Support long-term commercial deployments
* Simplify third-party development

---

# 15.3 Compatibility Philosophy

The MQTT namespace defined in Volume 3 represents the **canonical public API** of the Zmartify platform.

Internal firmware architecture may evolve without affecting:

* Topic names
* Payload structures
* JSON schemas
* Command semantics
* Discovery behavior

This separation allows firmware refactoring while maintaining interface stability.

---

# 15.4 Public vs. Internal Interfaces

Only documented interfaces are considered public.

| Interface          | Status   |
| ------------------ | -------- |
| MQTT Topics        | Public   |
| JSON Payloads      | Public   |
| Discovery Messages | Public   |
| Command Interface  | Public   |
| Internal Event Bus | Internal |
| FreeRTOS Tasks     | Internal |
| Firmware Managers  | Internal |
| Driver APIs        | Internal |

Internal interfaces may change without notice.

---

# 15.5 Version Numbering

The API follows **Semantic Versioning (SemVer)**.

Format:

```text id="semver-format"
MAJOR.MINOR.PATCH
```

Example:

```text id="semver-example"
1.0.0
```

Meaning:

| Component | Description                       |
| --------- | --------------------------------- |
| Major     | Breaking API changes              |
| Minor     | Backward-compatible functionality |
| Patch     | Bug fixes and clarifications      |

---

# 15.6 Firmware vs. API Version

Firmware and API versions are independent.

Example:

| Firmware | API |
| -------- | --- |
| 5.0.0    | 1.0 |
| 5.1.0    | 1.0 |
| 5.2.0    | 1.1 |
| 6.0.0    | 2.0 |

Firmware improvements do not necessarily imply API changes.

---

# 15.7 Backward Compatibility Rules

The following changes are considered backward compatible:

* Adding new MQTT topics
* Adding optional JSON fields
* Adding new event types
* Adding discovery metadata
* Adding capabilities
* Increasing limits
* Improving diagnostics

Existing clients shall continue functioning without modification.

---

# 15.8 Breaking Changes

Breaking changes include:

* Renaming MQTT topics
* Removing topics
* Removing mandatory JSON fields
* Changing field types
* Changing command semantics
* Changing authentication requirements
* Modifying retained-message behavior

Breaking changes shall only occur in a new **Major API Version**.

---

# 15.9 Topic Stability

Once published, MQTT topic names are considered permanent.

Example:

```text id="stable-topic"
zmartify/flow/current
```

shall never become

```text id="unstable-topic"
zmartify/hydraulics/current_flow
```

Such a change would require a new major API version.

---

# 15.10 Payload Evolution

Payloads may evolve by adding optional fields.

Example Version 1:

```json
{
    "flow":24.3
}
```

Future Version:

```json
{
    "flow":24.3,
    "confidence":98,
    "quality":"Normal"
}
```

Older clients continue functioning because required fields remain unchanged.

---

# 15.11 JSON Schema Compatibility

Every schema shall distinguish between:

* Required properties
* Optional properties

Only optional properties may be added within the same major API version.

Required properties shall not be removed or renamed.

---

# 15.12 Discovery Compatibility

Discovery payloads shall include:

```json
{
    "api_version":"1.0",
    "firmware":"5.0.0",
    "hardware":"RevB"
}
```

Client applications should use the API version—not the firmware version—to determine compatibility.

---

# 15.13 Capability Negotiation

Clients shall inspect:

```text
zmartify/controller/capabilities
```

rather than assuming functionality.

Example:

```json
{
    "supports_weather":true,
    "supports_flow_learning":true,
    "supports_pressure_learning":true,
    "supports_modbus":false,
    "supports_rest":false
}
```

This enables feature detection without version-specific logic.

---

# 15.14 Deprecation Policy

Deprecated interfaces shall remain available for at least one full major firmware generation.

Example timeline:

| Firmware | Status                       |
| -------- | ---------------------------- |
| 5.x      | Fully supported              |
| 6.x      | Deprecated (warning issued)  |
| 7.x      | Removed (new major API only) |

Deprecation notices shall be documented in release notes.

---

# 15.15 Multi-Version Support

Where practical, the controller may expose multiple API versions simultaneously.

Example:

```text
zmartify/v1/...

zmartify/v2/...
```

This allows gradual migration for enterprise deployments.

---

# 15.16 Extension Strategy

New functionality shall be introduced through extensions rather than modifications.

Example:

Existing:

```text
zmartify/weather/
```

Future additions:

```text
zmartify/weather/air_quality

zmartify/weather/pollen

zmartify/weather/lightning
```

Existing clients remain unaffected.

---

# 15.17 Future Transport Layers

The logical API defined in this document shall be reusable across future transports including:

* REST
* WebSocket
* OPC UA
* Matter
* Thread
* Modbus TCP
* gRPC

The transport layer may change while preserving the logical interface.

---

# 15.18 Compatibility Testing

Each firmware release shall verify:

* Topic compatibility
* Payload compatibility
* Command compatibility
* Discovery compatibility
* Home Assistant compatibility
* HOMEIO compatibility
* Homey compatibility

Automated regression testing is recommended.

---

# 15.19 Release Documentation

Every firmware release shall include:

* API version
* New features
* Deprecated interfaces
* Compatibility notes
* Migration guidance (if applicable)

This information shall accompany the firmware package.

---

# 15.20 Long-Term Stability Goals

The Zmartify MQTT API is intended to remain stable for many years.

Engineering targets include:

| Target                  | Goal      |
| ----------------------- | --------- |
| Topic stability         | >10 years |
| JSON schema stability   | >10 years |
| Command compatibility   | >10 years |
| Discovery compatibility | >10 years |

These goals reflect the expected service life of irrigation installations.

---

# 15.21 Engineering Notes

Long-term API stability is essential for automation systems, where integrations often outlive the hardware on which they were originally developed.

By treating the MQTT namespace as a contractual interface rather than an implementation detail, Zmartify enables third-party software, dashboards and automation rules to remain functional across multiple hardware and firmware generations. This approach reduces maintenance costs, increases user confidence and supports the platform's evolution into a broader ecosystem of interconnected products.

---

# 15.22 Chapter Summary

This chapter defines the compatibility principles governing the Zmartify MQTT API.

Through semantic versioning, strict backward compatibility rules, capability-based feature detection and a disciplined deprecation strategy, the platform provides a stable and predictable interface for both residential and professional deployments. These principles ensure that the public API can evolve responsibly while preserving existing integrations and protecting long-term engineering investments.

---

# End of Chapter 15

**Next Chapter**

**Chapter 16 – Complete MQTT Topic Reference & Interface Compliance Matrix**

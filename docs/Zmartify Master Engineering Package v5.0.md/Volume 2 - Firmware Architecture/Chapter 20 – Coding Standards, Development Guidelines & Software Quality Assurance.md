# Zmartify Master Engineering Package v5.0

## Volume 2 – Firmware Architecture

# Chapter 20 – Coding Standards, Development Guidelines & Software Quality Assurance

---

# 20 Coding Standards, Development Guidelines & Software Quality Assurance

---

# 20.1 Purpose

This chapter defines the mandatory software engineering standards for the Zmartify Irrigation Controller firmware.

Its objectives are to ensure that the firmware remains:

* Maintainable
* Readable
* Testable
* Modular
* Deterministic
* Safe
* Scalable

All source code contributed to the project shall comply with these standards.

---

# 20.2 Software Engineering Philosophy

The firmware shall follow modern embedded software engineering principles.

Primary objectives:

* Safety before features
* Simplicity before optimization
* Readability before cleverness
* Modular design
* Test-driven development where practical
* Continuous refactoring
* Comprehensive documentation

The project shall favour long-term maintainability over short-term implementation speed.

---

# 20.3 Programming Language

Primary language:

**C++17**

Platform:

**ESP-IDF 5.x**

Permitted use of C:

* Hardware abstraction
* Third-party drivers
* ESP-IDF libraries

Application logic shall be implemented in modern C++ wherever practical.

---

# 20.4 Coding Style

The project shall follow a consistent coding style.

### Indentation

* 4 spaces
* No tabs

### Braces

Opening brace on next line.

Example:

```cpp
if (condition)
{
    execute();
}
```

### Maximum Line Length

120 characters

---

# 20.5 Naming Conventions

## Classes

PascalCase

Example:

```cpp
FlowManager
```

---

## Methods

camelCase

```cpp
calculateFlow()
```

---

## Variables

camelCase

```cpp
currentPressure
```

---

## Constants

UPPER_CASE

```cpp
MAX_ZONES
```

---

## Enums

PascalCase

```cpp
enum class AlarmSeverity
```

---

## Files

snake_case

```text
flow_manager.cpp

flow_manager.hpp
```

---

# 20.6 Directory Structure

```text
src/

├── app/

├── managers/

├── drivers/

├── hal/

├── ui/

├── mqtt/

├── storage/

├── diagnostics/

├── common/

├── config/

├── tests/

└── third_party/
```

Each module shall be self-contained.

---

# 20.7 File Organization

Each module shall contain:

```text
flow_manager.hpp

flow_manager.cpp

flow_manager_internal.hpp (optional)

flow_manager_test.cpp
```

No module shall exceed approximately **1,500 lines** without documented justification.

---

# 20.8 Header Files

Header files shall contain:

* Public API
* Forward declarations
* Public constants
* Public types

Implementation details shall remain in `.cpp` files.

Use include guards or `#pragma once`.

---

# 20.9 Documentation

Every public class shall include:

```cpp
/**
 * @brief Flow Manager
 *
 * Responsible for hydraulic flow measurement,
 * validation and diagnostics.
 */
```

Every public function shall include:

* Description
* Parameters
* Return value
* Exceptions (if applicable)

---

# 20.10 Comments

Comments shall explain **why**, not **what**.

Good example:

```cpp
// Delay required to allow hydraulic pressure to stabilize
```

Poor example:

```cpp
// Increment i
i++;
```

Dead or obsolete comments shall be removed immediately.

---

# 20.11 Error Handling

All functions shall return explicit status information where appropriate.

Preferred:

```cpp
enum class Result
{
    Ok,
    InvalidParameter,
    Timeout,
    IOError
};
```

Exceptions shall generally not be used in embedded application code.

---

# 20.12 Logging

Logging levels:

| Level    | Purpose                   |
| -------- | ------------------------- |
| TRACE    | Development               |
| DEBUG    | Debugging                 |
| INFO     | Normal operation          |
| WARNING  | Recoverable issue         |
| ERROR    | Serious fault             |
| CRITICAL | Immediate action required |

Production firmware shall disable TRACE logging by default.

---

# 20.13 Assertions

Assertions shall be used for programmer errors.

Example:

```cpp
assert(zoneId < MAX_ZONES);
```

Assertions shall not replace runtime validation of user input.

---

# 20.14 Memory Management

Dynamic memory allocation shall be minimized.

Guidelines:

* Prefer stack allocation
* Prefer fixed-size containers where practical
* Avoid heap allocation in time-critical paths
* Avoid memory fragmentation

All memory leaks shall be treated as critical defects.

---

# 20.15 Concurrency

FreeRTOS tasks shall communicate using:

* Queues
* Event Groups
* Mutexes
* Semaphores

Shared global data shall be protected.

Busy waiting shall not be used.

---

# 20.16 Event-Driven Design

Application modules shall communicate through the Event Bus.

Direct dependencies between managers shall be minimized.

Preferred architecture:

```text
Module A

↓

Event Bus

↓

Module B
```

This improves modularity and testability.

---

# 20.17 Hardware Abstraction

Application code shall never access hardware registers directly.

All hardware interaction shall occur through the HAL.

Example:

```text
Application

↓

Relay Manager

↓

HAL

↓

ESP-IDF Driver

↓

Hardware
```

---

# 20.18 Configuration Access

Modules shall never access NVS or LittleFS directly.

All configuration shall be accessed via:

```cpp
ConfigurationManager
```

This ensures validation, version control and migration support.

---

# 20.19 Thread Safety

Public APIs shall clearly specify thread-safety guarantees.

Rules:

* Immutable data preferred
* Protect mutable shared state
* Avoid long mutex hold times
* Never block inside interrupt context

---

# 20.20 Unit Testing

Every manager shall have dedicated unit tests.

Minimum coverage targets:

| Module        | Coverage |
| ------------- | -------: |
| Core Managers |      95% |
| HAL           |      90% |
| UI            |      85% |
| Utilities     |      95% |

Unit tests shall be executable on the host development environment where practical.

---

# 20.21 Integration Testing

Integration tests shall verify:

* Event Bus communication
* MQTT integration
* Irrigation sequencing
* Hydraulic learning
* Configuration persistence
* OTA updates
* Alarm propagation
* Diagnostics reporting

Integration tests shall be part of the continuous integration pipeline.

---

# 20.22 Static Analysis

Mandatory tools:

* **clang-format** (code formatting)
* **clang-tidy** (static analysis)
* **cppcheck**
* ESP-IDF compiler warnings (`-Wall -Wextra`)

New code shall compile without warnings.

---

# 20.23 Continuous Integration

Every commit shall automatically perform:

1. Source formatting validation
2. Static analysis
3. Compilation
4. Unit testing
5. Integration testing
6. Documentation verification

A build shall not be merged if any mandatory check fails.

---

# 20.24 Version Control

Git shall be used.

Recommended branching model:

```text
main

develop

feature/*

release/*

hotfix/*
```

Every commit message shall follow a consistent convention.

Example:

```text
feat(flow): add adaptive flow learning

fix(mqtt): reconnect after broker timeout

docs(volume2): update relay architecture
```

---

# 20.25 Code Review

All changes shall undergo peer review.

Review checklist:

* Architecture compliance
* Coding standards
* Test coverage
* Documentation
* Error handling
* Security implications
* Performance considerations

No code shall be merged without review.

---

# 20.26 Performance Targets

General software targets:

| Metric           |       Target |
| ---------------- | -----------: |
| Boot Time        |        <10 s |
| UI Response      |       <50 ms |
| Event Dispatch   |       <10 ms |
| MQTT Publish     |      <250 ms |
| Relay Activation |      <100 ms |
| Memory Usage     |     <75% RAM |
| CPU Utilization  | <60% Average |

Performance regressions shall be investigated before release.

---

# 20.27 Release Process

Each official release shall include:

* Version number
* Release notes
* Updated documentation
* Test results
* Known issues
* Migration notes (if applicable)

Semantic Versioning (SemVer) shall be used.

Example:

```text
v5.0.0

Major.Minor.Patch
```

---

# 20.28 Engineering Notes

The long-term success of the Zmartify platform depends as much on software quality as on hardware design.

By enforcing consistent coding standards, modular architecture, comprehensive testing and automated quality assurance, the project will remain maintainable as it grows from a single irrigation controller into a complete ecosystem of smart irrigation products.

The emphasis on event-driven design, hardware abstraction and continuous integration also makes the firmware well suited for future contributions from additional developers.

---

# 20.29 Chapter Summary

This chapter establishes the engineering practices that govern all firmware development for the Zmartify Irrigation Controller.

By defining coding conventions, architectural principles, testing requirements, quality assurance processes and release management, it ensures that the software remains reliable, maintainable and scalable throughout the lifetime of the project.

These standards provide the foundation for professional-grade embedded software development and support the long-term vision of Zmartify as a robust, extensible and commercially viable smart irrigation platform.

---

# End of Chapter 20

**Next Chapter**

**Chapter 21 – Firmware Roadmap, Future Architecture & Volume 2 Summary**

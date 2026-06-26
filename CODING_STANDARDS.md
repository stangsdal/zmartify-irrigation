# Zmartify Coding Standards

**Document:** Zmartify Irrigation Controller Coding Standards  
**Version:** 5.0  
**Based on:** Volume 2, Chapter 20 – Coding Standards & Development Guidelines  
**Status:** Active (v5.0 Baseline)

---

## 1 Purpose

This document defines mandatory software engineering standards for the Zmartify Irrigation Controller firmware. All source code shall comply with these standards.

**Objectives:**
- Maintain readability and consistency across the codebase
- Ensure code is testable and modular
- Support long-term maintainability
- Facilitate knowledge transfer and code review

---

## 2 Software Engineering Philosophy

### 2.1 Core Principles

1. **Safety before features** – Safety-critical functions take priority
2. **Simplicity before optimization** – Clear code over clever code
3. **Readability before cleverness** – Future maintainers matter
4. **Modular design** – Components shall be independent and reusable
5. **Test-driven development** – Tests written before or with implementation
6. **Continuous refactoring** – Improve code quality incrementally
7. **Comprehensive documentation** – Every public API documented

### 2.2 Design Approach

- Event-driven architecture (event bus based)
- Layered design (HAL → Logic → Application)
- Loose coupling between components
- High cohesion within components
- No global state (use dependency injection)

---

## 3 Programming Language

| Aspect | Standard |
|--------|----------|
| Primary Language | C99/C11 (with modern practices) |
| Framework | ESP-IDF 5.x |
| Target | ESP32-S3 Dual-Core |
| Compiler | GCC (arm-none-eabi) |

### 3.1 C Language Features

**Permitted:**
- C99 features (restrict pointers, designated initializers, inline)
- Variable declarations anywhere in function
- Loop declarations in `for` statement
- Modern function prototypes

**Prohibited:**
- K&R function prototypes
- Global mutable state (use static module scope)
- Unbounded buffers (always define max size)
- Implicit integer conversions

---

## 4 File Organization

### 4.1 Directory Structure

```
components/
├── component_name/
│   ├── CMakeLists.txt          # Build configuration
│   ├── idf_component.yml       # Component manifest
│   ├── include/
│   │   └── component_name.h    # Public API
│   ├── src/
│   │   └── component_name.c    # Implementation
│   ├── private/
│   │   └── *.h                 # Internal headers (not exported)
│   └── test/
│       └── test_*.c            # Component unit tests
├── event_bus/
├── hal/
├── ...
main/
├── CMakeLists.txt
├── idf_component.yml
├── main.c                      # Application entry point
└── README.md                   # Main app documentation
test/
├── integration/
└── unit/
docs/
├── DEVELOPMENT-PLAN-v5.0.md
├── CODING_STANDARDS.md         # This file
├── ARCHITECTURE.md
└── Zmartify Master Engineering Package v5.0.md/
scripts/
├── build.sh
├── flash.sh
├── monitor.sh
└── size-report.sh
```

### 4.2 Header Files (.h)

**Purpose:** Define public API, constants, and type definitions

**Structure:**
```c
/*
 * component_name.h
 * Brief description of component
 *
 * Copyright and license notice
 */

#ifndef COMPONENT_NAME_H
#define COMPONENT_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * Maximum zones supported by the controller
 */
#define MAX_ZONES 16

/**
 * Zone identification type
 */
typedef uint8_t zone_id_t;

/**
 * Initialize the component
 *
 * @return true if successful, false if error
 */
bool component_init(void);

/**
 * Process periodic task
 *
 * @param elapsed_ms Time since last call (milliseconds)
 */
void component_update(uint32_t elapsed_ms);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENT_NAME_H */
```

**Rules:**
- Always use include guards (prefer `#ifndef` over `#pragma once`)
- Include guards format: `COMPONENT_NAME_H`
- Minimal includes (only what public API needs)
- Document every public function with brief Doxygen-style comments
- Group related declarations together
- Use `_t` suffix for typedef'd types (optional but consistent)

### 4.3 Implementation Files (.c)

**Purpose:** Implement module functionality

**Structure:**
```c
/*
 * component_name.c
 * Implementation of component_name module
 *
 * Copyright and license notice
 */

#include "component_name.h"
#include "private/component_name_internal.h"
#include "event_bus.h"
#include <string.h>
#include <stdlib.h>

// Private constants
static const char *TAG = "component_name";
static const uint32_t TIMEOUT_MS = 1000;

// Private type definitions
typedef struct {
    uint32_t state;
    uint32_t timestamp;
} component_state_t;

// Private module state
static component_state_t s_state = {0};

// Private function declarations
static void component_internal_process(void);

// Public function implementations
bool component_init(void)
{
    ESP_LOGI(TAG, "Initializing component");
    memset(&s_state, 0, sizeof(s_state));
    return true;
}

void component_update(uint32_t elapsed_ms)
{
    component_internal_process();
}

// Private function implementations
static void component_internal_process(void)
{
    // Implementation here
}
```

**Rules:**
- One static context per module (module scope)
- Private functions declared static
- Static module state variables prefixed with `s_` (s for static)
- Include public header first
- Include private headers next
- Include standard library last

---

## 5 Naming Conventions

### 5.1 Types & Constants

| Element | Convention | Example |
|---------|-----------|---------|
| Typedef'd struct | `PascalCase_t` | `zone_state_t` |
| Typedef'd enum | `PascalCase_e` | `alarm_severity_e` |
| Enum values | `UPPER_CASE` | `ALARM_CRITICAL` |
| Constants | `UPPER_CASE` | `MAX_ZONES` |
| Macros | `UPPER_CASE` | `ZONE_ID_INVALID` |

### 5.2 Variables & Functions

| Element | Convention | Example |
|---------|-----------|---------|
| Global (module) | `s_camelCase` | `s_command_queue` |
| Local variable | `camelCase` | `currentPressure` |
| Function parameter | `camelCase` | `zone_id` |
| Public function | `camelCase` | `zone_manager_start()` |
| Private function | `static` + `camelCase` | `static void calculateRuntime()` |
| Boolean variable | Prefix `is_`, `has_`, `can_` | `is_running`, `has_fault` |
| Struct member | `camelCase` | `pressure_psi` |
| Pointer member | Prefix `p_` or suffix `_ptr` | `p_next`, `relay_ptr` |

### 5.3 Module Naming

Each component should have a consistent prefix:

```
Component: zone_manager
- Header: zone_manager.h
- Implementation: zone_manager.c
- Functions: zone_manager_start(), zone_manager_stop()
- Types: zone_manager_t
- Constants: ZONE_MANAGER_*
```

---

## 6 Code Style

### 6.1 Indentation & Whitespace

- **Indentation:** 4 spaces (no tabs)
- **Line length:** Maximum 120 characters
- **Blank lines:** Single blank line between functions
- **Trailing spaces:** None (configure editor to strip)

### 6.2 Braces

**Opening brace on next line:**
```c
if (condition)
{
    statement();
}
else
{
    other_statement();
}
```

**For loops and while loops:**
```c
while (condition)
{
    process();
}

for (int i = 0; i < count; i++)
{
    handle_item(i);
}
```

**Function definition:**
```c
int zone_manager_calculate_runtime(int base_runtime)
{
    // Implementation
    return result;
}
```

**Typedef struct:**
```c
typedef struct
{
    uint8_t zone_id;
    uint32_t runtime_ms;
} zone_schedule_t;
```

### 6.3 Spacing

- Space after keywords: `if`, `while`, `for`, `switch`
- Space around operators: `a = b + c` (not `a=b+c`)
- Space after commas in lists
- No space before semicolon

**Good:**
```c
if (pressure > MAX_PRESSURE)
{
    zone_stop(zone_id);
}

float flow_rate = (total_ml / elapsed_ms) * 1000.0f;
```

**Bad:**
```c
if(pressure>MAX_PRESSURE){
    zone_stop(zone_id);
}

float flow_rate=(total_ml/elapsed_ms)*1000.0f;
```

---

## 7 Documentation

### 7.1 File Headers

Every source file shall start with:
```c
/*
 * filename.c
 * Brief description of what this file does
 *
 * Detailed description if needed to understand context.
 *
 * Copyright (c) 2026 Zmartify
 * Licensed under [License Name]
 */
```

### 7.2 Function Documentation

Use Doxygen-style comments for all public functions:

```c
/**
 * @brief Start irrigation on a zone
 *
 * Activates the specified zone valve and begins counting flow/pressure.
 * The zone shall reach steady state within 5 seconds.
 *
 * @param zone_id Zone identifier (0-15)
 * @param runtime_seconds Requested runtime in seconds (1-3600)
 *
 * @return true if zone was started successfully
 * @return false if zone_id invalid or zone already running
 *
 * @note Thread-safe if called via event bus
 * @note Generates ZONE_STARTED event on success
 * @see zone_manager_stop()
 */
bool zone_manager_start(zone_id_t zone_id, uint32_t runtime_seconds);
```

### 7.3 Complex Logic

Inline comments explain complex algorithms:

```c
// Penman-Monteith ET calculation (FAO-56 standard)
// ETo = [0.408 * Sn * (Rn - G) + γ * (Cn/(T+273)) * u2 * (es - ea)] / (Δ + γ * (1 + Cd*u2))
float eto = (0.408f * sn * (rn - g) + gamma * (cn / (temp + 273.0f)) * u2 * (es - ea)) /
            (delta + gamma * (1.0f + cd * u2));
```

### 7.4 TODO Comments

Mark incomplete work clearly:

```c
// TODO: FR-IRR-025 - Add flow anomaly detection for stagnation
// Currently only checks for zero flow; should implement graduated response
// for low flow conditions
```

---

## 8 Type Definitions

### 8.1 Typedef Strategy

Use typedefs for clarity but sparingly:

**Good (use typedef):**
```c
// For opaque handles
typedef struct zone_manager_s *zone_manager_handle_t;

// For semantic clarity
typedef uint32_t timestamp_ms_t;
typedef uint16_t pressure_psi_t;

// For complex structs used in multiple places
typedef struct {
    uint8_t zone_id;
    uint32_t runtime_ms;
} zone_schedule_t;
```

**Avoid over-typedef (use direct types):**
```c
// Don't do this - obfuscates
typedef int zone_count_t;
typedef bool is_running_t;

// Do this instead
int zone_count;
bool is_running;
```

### 8.2 Structs

Keep structs simple and document members:

```c
typedef struct
{
    /**
     * Zone identifier (0-15)
     */
    uint8_t zone_id;

    /**
     * Requested runtime in milliseconds
     */
    uint32_t runtime_ms;

    /**
     * Current state (see zone_state_e)
     */
    uint8_t state;

    /**
     * Pressure reading (PSI)
     */
    float pressure_psi;
} zone_t;
```

### 8.3 Enums

Use enum classes for type safety:

```c
/**
 * Zone operational states
 */
typedef enum
{
    ZONE_STATE_IDLE = 0,           ///< Zone not active
    ZONE_STATE_REQUESTED = 1,      ///< Start request pending
    ZONE_STATE_VALVE_OPENING = 2,  ///< Valve transitioning
    ZONE_STATE_RUNNING = 3,        ///< Zone actively irrigating
    ZONE_STATE_PAUSED = 4,         ///< Paused (manual or fault)
    ZONE_STATE_STOPPING = 5,       ///< Stop in progress
} zone_state_e;
```

---

## 9 Memory Management

### 9.1 Static Allocation (Preferred)

Prefer stack and static allocation:

```c
// Good: Fixed-size array on stack
uint8_t buffer[256];

// Good: Module-scope static
static zone_t s_zones[MAX_ZONES];

// Good: Pre-allocated via heap during init
static zone_t *s_zones = NULL;

void component_init(void)
{
    s_zones = malloc(MAX_ZONES * sizeof(zone_t));
    if (s_zones == NULL) {
        ESP_LOGE(TAG, "Failed to allocate zone array");
        return false;
    }
}
```

### 9.2 Dynamic Allocation Rules

If dynamic allocation is necessary:

1. Allocate only during initialization (not in cyclic tasks)
2. Always check return value
3. Document expected lifetime
4. Free on cleanup/shutdown

```c
// Allocate once during init
if (buffer == NULL)
{
    buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Allocation failed");
        return false;
    }
}

// Free on shutdown
if (buffer != NULL)
{
    free(buffer);
    buffer = NULL;
}
```

### 9.3 Prohibited Patterns

- No dynamic allocation in interrupt handlers
- No dynamic allocation in cyclic tasks
- No frequent alloc/free patterns
- No unbounded allocations

---

## 10 Error Handling

### 10.1 Return Values

Use explicit return codes:

```c
typedef enum
{
    ZONE_OK = 0,
    ZONE_ERR_INVALID_ID,
    ZONE_ERR_ALREADY_RUNNING,
    ZONE_ERR_PRESSURE_FAULT,
    ZONE_ERR_TIMEOUT,
} zone_error_t;

zone_error_t zone_manager_start(zone_id_t zone_id)
{
    if (zone_id >= MAX_ZONES)
    {
        return ZONE_ERR_INVALID_ID;
    }
    if (s_zones[zone_id].state != ZONE_STATE_IDLE)
    {
        return ZONE_ERR_ALREADY_RUNNING;
    }
    // ... start zone
    return ZONE_OK;
}
```

### 10.2 Logging

Use ESP-IDF logging levels:

```c
ESP_LOGE(TAG, "Critical error: %d", error_code);      // ERROR
ESP_LOGW(TAG, "Warning: low battery");                // WARNING
ESP_LOGI(TAG, "Zone %d started", zone_id);            // INFO
ESP_LOGD(TAG, "Flow rate: %.2f GPM", flow_gpm);       // DEBUG
```

### 10.3 Assertions

Use assertions for invariants:

```c
// Verify precondition
assert(zone_id < MAX_ZONES);

// Verify postcondition
result = calculate_pressure();
assert(result >= 0 && result <= MAX_PRESSURE);
```

---

## 11 Concurrency & Thread Safety

### 11.1 FreeRTOS Best Practices

- Use semaphores/mutexes for shared resources
- Keep critical sections minimal
- Never call blocking functions in high-priority tasks
- Use event groups for task synchronization

### 11.2 Module Lock Pattern

```c
static SemaphoreHandle_t s_ctx_lock = NULL;

static bool acquire_lock(TickType_t timeout)
{
    return xSemaphoreTake(s_ctx_lock, timeout) == pdTRUE;
}

static void release_lock(void)
{
    xSemaphoreGive(s_ctx_lock);
}

zone_error_t zone_manager_start(zone_id_t zone_id)
{
    if (!acquire_lock(pdMS_TO_TICKS(100)))
    {
        return ZONE_ERR_TIMEOUT;
    }

    // Critical section
    zone_error_t result = zone_start_unsafe(zone_id);

    release_lock();
    return result;
}
```

---

## 12 Testing

### 12.1 Unit Test Structure

Place tests in component subdirectory:

```
components/zone_manager/
├── test/
│   ├── CMakeLists.txt
│   ├── test_zone_state_machine.c
│   ├── test_zone_validation.c
│   └── test_zone_pressure_anomaly.c
```

### 12.2 Test File Template

```c
#include <unity.h>
#include "zone_manager.h"
#include "event_bus.h"

void setUp(void)
{
    // Initialize before each test
    event_bus_init();
    zone_manager_init();
}

void tearDown(void)
{
    // Cleanup after each test
    zone_manager_cleanup();
    event_bus_cleanup();
}

void test_zone_start_valid_id(void)
{
    // Arrange
    zone_id_t zone_id = 5;
    uint32_t runtime_ms = 60000;

    // Act
    zone_error_t result = zone_manager_start(zone_id, runtime_ms);

    // Assert
    TEST_ASSERT_EQUAL(ZONE_OK, result);
    TEST_ASSERT_EQUAL(ZONE_STATE_REQUESTED, zone_manager_get_state(zone_id));
}

void test_zone_start_invalid_id(void)
{
    zone_error_t result = zone_manager_start(MAX_ZONES, 60000);
    TEST_ASSERT_EQUAL(ZONE_ERR_INVALID_ID, result);
}
```

---

## 13 Code Review Checklist

Before committing, verify:

- [ ] Compiles without warnings
- [ ] Follows naming conventions
- [ ] Properly documented (functions, complex logic)
- [ ] No global mutable state
- [ ] Error handling complete
- [ ] No memory leaks (malloc/free balanced)
- [ ] Thread-safe where needed
- [ ] Unit tests written and passing
- [ ] No hardcoded values (use constants)
- [ ] Line length ≤ 120 characters
- [ ] No trailing spaces
- [ ] Coding standard violations addressed

---

## 14 Version Control

### 14.1 Commit Messages

Follow conventional commit format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Examples:**
```
feat(zone_manager): implement zone state machine

Add state transitions for zone startup and fault handling.
Supports graceful shutdown and pressure verification.

Implements: FR-IRR-001, FR-IRR-002
Fixes: #123
```

```
fix(pressure_manager): correct ADC sampling filter

Change rolling average window from 5 to 10 samples
to reduce noise in pressure readings while maintaining
response time < 100ms.
```

```
docs(coding_standards): add concurrency guidelines

Add section on FreeRTOS best practices and module
lock pattern for thread-safe access.
```

### 14.2 Commit Types

- `feat` – New feature
- `fix` – Bug fix
- `docs` – Documentation update
- `style` – Code style (formatting, not logic)
- `refactor` – Code restructuring
- `test` – Test additions/modifications
- `build` – Build system changes
- `chore` – Other changes (dependencies, etc.)

---

## 15 Approved Tools & Configuration

### 15.1 Code Formatting

**.clang-format** configuration (LLVM style with modifications):
```yaml
BasedOnStyle: LLVM
IndentWidth: 4
UseTab: Never
ColumnLimit: 120
BreakBeforeBraces: Allman
AlignAfterOpenBracket: Align
```

Run formatting:
```bash
clang-format -i src/*.c include/*.h
```

### 15.2 Static Analysis

Enable compiler warnings:
```bash
# During build
idf.py build -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic"
```

---

## 16 Exceptions & Waivers

Deviations from these standards require:
1. Engineering review
2. Documented justification
3. Technical lead approval
4. Updated in exception log

**Exception Log:** See `docs/CODING_STANDARDS_EXCEPTIONS.md`

---

## 17 Continuous Improvement

This document is a living standard. Improvements should be:
1. Proposed to engineering team
2. Discussed in architecture review
3. Added to next revision with version bump
4. Communicated to all developers

---

## Document Control

| Item | Value |
|------|-------|
| Version | 5.0 |
| Status | Active – Baseline |
| Based On | Volume 2, Chapter 20 |
| Last Updated | 2026-06-26 |
| Next Review | End of Step 5 (Mid-implementation) |

---

**End of Coding Standards**

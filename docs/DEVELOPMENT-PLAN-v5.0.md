# Zmartify Irrigation Controller – Development Plan v5.0

**Project:** ESP-IDF Firmware for ZIC-S3 Rev.B  
**Hardware Platform:** ESP32-S3 (8MB)  
**Engineering Baseline:** Zmartify Master Engineering Package v5.0  
**Target Completion:** Production-Ready Firmware  
**Status:** Development Plan  

---

## Overview

This 10-step development plan transforms the Zmartify Master Engineering Package v5.0 requirements into a complete, modular, production-ready firmware implementation for the ZIC Irrigation Controller.

The plan emphasizes:

- **Clean Architecture:** Removing legacy code and building from requirements
- **Layered Design:** Event-driven, loosely-coupled subsystems
- **Test-Driven Development:** Testing at each integration point
- **Requirements Traceability:** Every feature linked to FRS requirements
- **Incremental Delivery:** Independent commits for each step

---

## Step 1: Project Foundation & Build System Setup

**Objective:** Establish clean project structure, build configuration, and development environment.

**Duration:** 2-3 days

### Activities

#### 1.1 Clean Repository Initialization
- **Remove Legacy Code**
  - Archive existing `main/main.c` (reference only)
  - Clean `components/` directory of incomplete implementations
  - Remove build artifacts, temporary files, and stale configurations
  - Archive `sdkconfig.old` and preserve only `sdkconfig` with baseline config
  - Document removal decisions in `MIGRATION.md`

- **Preserve Documentation**
  - Keep all `/docs` content
  - Keep existing test files as reference

#### 1.2 ESP-IDF Project Structure
- Configure `CMakeLists.txt` for ESP-IDF v5.x
- Set target to `esp32s3` with 8MB configuration
- Define component dependency graph

#### 1.3 Build Configuration
- Create `sdkconfig` baseline:
  - 2 CPU cores enabled
  - FreeRTOS configured for deterministic scheduling
  - MQTT over TLS enabled
  - OTA updates enabled
  - Partition table for OTA (bootloader + 2 app partitions)
  
- Create build scripts:
  - `scripts/build.sh` - Clean build
  - `scripts/flash.sh` - Flash via USB/OTA
  - `scripts/monitor.sh` - Serial monitoring
  - `scripts/size-report.sh` - Memory usage analysis

#### 1.4 Development Tools Setup
- Configure ESP-IDF extension in VS Code
- Set up clang-format with v5.0 coding standards
- Create `.clang-format` configuration (4-space indent, 120-char line, PascalCase/camelCase)
- Set up CMake tools integration

#### 1.5 Coding Standards & Git Configuration
- Create `CODING_STANDARDS.md` (extract from Volume 2, Chapter 20)
  - C++17 as primary language
  - Naming conventions
  - Style guidelines
  - Documentation standards
  
- Set up `.gitignore`:
  - Ignore `build/`, `sdkconfig.old`, temporary files
  - Track `sdkconfig`, `managed_components/`
  
- Create initial `ARCHITECTURE.md` referencing Volume 2

#### 1.6 Verification
- ✓ Successful build with `idf.py build`
- ✓ Memory profiling: <50% heap usage (baseline)
- ✓ All build systems functional
- ✓ Development environment documented

**Deliverable:** 
- Clean project structure with ESP-IDF v5.x integration
- Build scripts and development tooling operational
- Coding standards and documentation templates established

**Commit Message:** `Step 1: Project foundation, clean repository, and build system setup`

---

## Step 2: Event Bus & Core Communication Infrastructure

**Objective:** Implement the foundational Event Bus architecture enabling loose coupling between all subsystems.

**Duration:** 3-4 days

**Requirements Mapping:** FW-EVT-001 to FW-EVT-005, EDS-003 (Event-Driven Operation)

### Activities

#### 2.1 Event Bus Component Architecture
- Create `components/event_bus/`:
  - `event_bus.h` - Public API
  - `event_bus.c` - Implementation
  - `event_types.h` - Event definitions
  - `event_registry.h` - Subscription management

#### 2.2 Event Model Definition
- **Event Structure**
  ```c
  typedef struct {
      uint32_t event_id;              // Unique event identifier
      uint32_t producer_id;            // Who generated this
      uint64_t timestamp_us;           // System uptime in microseconds
      uint8_t priority;                // 0=low ... 15=critical
      uint8_t payload_type;            // Data interpretation guide
      uint32_t payload_size;           // Size of attached data
      void *payload;                   // Event-specific data
  } zic_event_t;
  ```

- **Event Categories**
  - Safety events (priority 14-15)
  - Control events (priority 8-10)
  - Telemetry events (priority 4-7)
  - Diagnostic events (priority 0-3)

- **Event ID Registry** (100+ event types)
  - Zone state changes
  - Flow updates
  - Pressure alerts
  - Temperature changes
  - MQTT messages
  - Alarm events
  - OTA triggers
  - UI commands

#### 2.3 Event Queue Implementation
- FreeRTOS queue-based dispatcher
- Bounded queue size (max 64 events)
- Priority-based insertion
- Event timeout handling
- Memory pool for event payloads (pre-allocated)

#### 2.4 Event Subscription System
- Publisher-Subscriber pattern
- Dynamic subscription registration
- Callback-based event delivery
- Subscription filtering by event type and producer
- Unsubscription on task deletion

#### 2.5 Synchronization Primitives
- Task-safe event dispatch
- Mutex protection for subscription database
- Atomic event counter (performance monitoring)
- Watchdog integration for stuck event handlers

#### 2.6 Logging & Diagnostics
- Event bus statistics:
  - Events processed per second
  - Queue depth (current/peak)
  - Event latency histogram
  - Dropped events counter
- Debug event replay capability

#### 2.7 Unit Tests
- `test/test_event_bus.c`
  - Event creation and validation
  - Queue insertion/retrieval
  - Subscription registration
  - Event dispatch to multiple subscribers
  - Priority ordering
  - Memory cleanup

#### 2.8 Verification
- ✓ Event bus compiles without warnings
- ✓ 100% unit test pass rate
- ✓ Event latency < 1ms (99th percentile)
- ✓ No memory leaks in long-running test (10K events)
- ✓ Performance baseline: <5% CPU at 10Hz event rate

**Deliverable:**
- Production-ready Event Bus with full API
- Event type registry aligned with firmware modules
- Complete unit test coverage
- Performance benchmarks established

**Commit Message:** `Step 2: Event Bus infrastructure and inter-task communication layer`

---

## Step 3: Hardware Abstraction Layer (HAL) & Device Drivers

**Objective:** Implement HAL for all hardware interfaces (GPIO, I²C, ADC, PWM, PCNT) according to EDS-002 (Layered Architecture).

**Duration:** 4-5 days

**Requirements Mapping:** Multiple HW/FW requirements, EDS-002

### Activities

#### 3.1 HAL Architecture
- Create `components/hal/`:
  - `gpio_hal.h/c` - GPIO abstraction
  - `i2c_hal.h/c` - I²C bus master
  - `adc_hal.h/c` - ADC interface (ADS1115)
  - `pwm_hal.h/c` - PWM for relay control
  - `pcnt_hal.h/c` - Pulse counter for flow meters
  - `rtc_hal.h/c` - RTC for persistent timekeeping
  - `nvs_hal.h/c` - Non-volatile storage wrapper

#### 3.2 GPIO Management
- Define GPIO allocation (per architecture docs)
  - Relay control pins (zone solenoids + master valve)
  - Status LED pins
  - Button inputs
  - Display CS/DC/RST pins

- Implementation:
  - GPIO initialization at startup
  - Direction and pull configuration
  - Interrupt handling for buttons
  - Level shifting verification

#### 3.3 I²C Bus Master
- I²C bus 0 configuration:
  - 400kHz standard mode
  - MCP23017 support (GPIO expander)
  - ADS1115 support (ADC)
  - ATECC608B support (crypto)
  
- Features:
  - Auto-retry on NACK
  - Timeout protection
  - Bus recovery on error
  - Clock stretching support

#### 3.4 ADC Interface (ADS1115)
- Support for 4 single-ended channels:
  - Channel 0: Pressure sensor (0-15 PSI)
  - Channel 1: Temperature sensor (−40 to +125°C)
  - Channel 2: Solar radiation sensor
  - Channel 3: Humidity sensor
  
- Features:
  - Differential measurements
  - Programmable gain (2/3x to 16x)
  - SPI/I²C interrupt configuration
  - Calibration support

#### 3.5 PWM & Relay Control
- PWM for valve duty cycle control:
  - Frequency: 1kHz for solenoid coils
  - Resolution: 10-bit (1024 steps)
  - Channels 0-15 for up to 16 relay circuits

#### 3.6 Pulse Counter (PCNT) for Flow Meters
- PCNT units 0-3 for flow meter pulse counting:
  - Pulse input from Hall-effect sensors
  - Edge counting (rising edges)
  - Overflow handling for long durations
  - Per-unit frequency calculation

#### 3.7 Device Driver Integration
- MCP23017 (16-channel GPIO expander) driver
  - I²C address 0x20, 0x21, 0x22
  - Interrupt-on-change for event generation
  - Output latch control

- ATECC608B (Crypto Accelerator) driver
  - TLS certificate storage
  - ECDH key agreement
  - SHA-256 acceleration

#### 3.8 Error Handling & Fault Recovery
- Transient error recovery (retry logic)
- Permanent error detection and reporting
- Event generation on HAL failures
- Watchdog integration

#### 3.9 Unit Tests
- `test/test_hal_gpio.c` - GPIO operations
- `test/test_hal_i2c.c` - I²C communication
- `test/test_hal_adc.c` - ADC sampling
- Mock device implementations for CI testing

#### 3.10 Verification
- ✓ All HAL interfaces compile
- ✓ 100% unit test pass rate
- ✓ GPIO toggle latency < 10µs
- ✓ I²C read/write completes within 1ms
- ✓ ADC sampling rate: 860Hz (ADS1115 native)
- ✓ No blocking calls in interrupt handlers

**Deliverable:**
- Complete Hardware Abstraction Layer for ESP32-S3
- All device drivers implemented and tested
- GPIO allocation finalized per hardware schematic
- HAL API documented with examples

**Commit Message:** `Step 3: Hardware Abstraction Layer (HAL) and device driver implementations`

---

## Step 4: Persistent Storage & Configuration Management

**Objective:** Implement NVS-based storage for configuration, persistent state, and operational history.

**Duration:** 3-4 days

**Requirements Mapping:** EDS-005 (Data Persistence), FR-SYS-001

### Activities

#### 4.1 Persistent Store Component
- Create `components/persistent_store/`:
  - `persistent_store.h/c` - Main API
  - `config_store.h/c` - Configuration persistence
  - `event_log_store.h/c` - Circular event log
  - `fault_register.h/c` - Fault history
  - `calibration_store.h/c` - Sensor calibrations

#### 4.2 NVS Partition Layout
- Partition sizes:
  - NVS: 64KB (config + state)
  - OTADATA: 8KB (OTA slot selector)
  - APP0: 1.8MB (active firmware)
  - APP1: 1.8MB (OTA update staging)
  - SPIFFS: 2MB (optional diagnostics log)

#### 4.3 Configuration Schema
```c
typedef struct {
    // System
    uint32_t controller_id;
    uint32_t uptime_seconds;
    uint8_t operational_mode;
    
    // Network
    char ssid[32];
    char mqtt_broker_uri[128];
    char mqtt_username[32];
    
    // Irrigation Program
    uint8_t num_zones;
    uint8_t num_programs;
    uint32_t program_data[...];
    
    // Alarms & Safety
    uint8_t max_pressure_psi;
    uint8_t min_pressure_psi;
    uint32_t max_runtime_seconds;
    
    // Calibration
    float pressure_offset;
    float flow_scale;
    float temperature_offset;
} zic_config_t;
```

#### 4.4 Persistent State
- Last known irrigation state
- Current zone/program execution info
- Fault counters
- Maintenance intervals
- OTA update status

#### 4.5 Event Logging
- Circular buffer design:
  - 128-entry event log
  - Timestamp, event ID, severity, context
  - Non-blocking append
  - Wrap-around capability

#### 4.6 Fault Register
- Fault history (last 64 faults):
  - Timestamp
  - Fault code
  - Fault severity
  - Zone/component affected
  - Recovery action taken

#### 4.7 Calibration Data
- Sensor calibration points:
  - Pressure calibration (min/max)
  - Flow scale factor
  - Temperature offset
  - Humidity calibration

#### 4.8 Backup & Recovery
- CRC32 protection for config
- Automatic rollback on corruption
- Factory reset capability
- Config export to JSON format

#### 4.9 Unit Tests
- `test/test_config_store.c`
  - Config read/write
  - Corruption detection
  - Recovery mechanism
  - Migration from v4.x configs

#### 4.10 Verification
- ✓ Config write completes within 100ms
- ✓ CRC corruption detection: 100%
- ✓ Circular log: no data loss at wrap
- ✓ Factory reset: < 1 second
- ✓ 1000-cycle endurance test passes

**Deliverable:**
- NVS-based persistent storage fully integrated
- Configuration schema aligned with FRS
- Event logging and fault history
- Backup/recovery mechanisms

**Commit Message:** `Step 4: Persistent storage (NVS) and configuration management`

---

## Step 5: Core Domain Model & State Machines

**Objective:** Implement the Zone, Relay, and Irrigation Engine state machines following FW-TASK-002 (Short Execution Time) and FW-EVT-001 (Events as Facts).

**Duration:** 4-5 days

**Requirements Mapping:** FR-IRR-001 to FR-IRR-010, Chapter 5 (Irrigation Engine Architecture)

### Activities

#### 5.1 Zone Manager Component
- Create `components/zone_manager/`:
  - `zone.h/c` - Zone data model
  - `zone_state_machine.h/c` - State machine
  - `zone_manager.h/c` - Manager task

- **Zone State Machine**
  ```
  IDLE -> REQUESTED -> VALVE_OPENING -> RUNNING -> 
  PAUSE -> VALVE_CLOSING -> STOPPED -> IDLE
  
  FAULT paths from all states
  ```

- **Zone Data Structure**
  ```c
  typedef struct {
      uint8_t zone_id;
      uint32_t requested_runtime_seconds;
      uint32_t actual_runtime_seconds;
      uint32_t last_run_timestamp;
      uint16_t flow_ml_per_pulse;
      float last_flow_rate_gpm;
      float max_pressure_psi;
      uint8_t state;
      uint32_t state_entry_time;
  } zone_t;
  ```

#### 5.2 Relay Manager Component
- Create `components/relay_manager/`:
  - `relay.h/c` - Relay data model
  - `relay_state_machine.h/c` - State machine
  - `relay_manager.h/c` - Manager task

- **Relay Control State Machine**
  ```
  OFF -> ON_REQUEST -> ON -> OFF_REQUEST -> OFF
  
  Fault handling for stuck relay detection
  ```

- **Relay-Zone Mapping**
  - Master valve (relay 0)
  - Zone solenoids (relays 1-15)
  - Pump control (relay 16, future)

#### 5.3 Irrigation Engine Component
- Create `components/irrigation_engine/`:
  - `irrigation_engine.h/c` - Core engine
  - `program.h/c` - Program definition
  - `runtime_calculator.h/c` - Runtime computation

- **Irrigation Engine State Machine**
  ```
  IDLE -> PROGRAM_RUNNING -> ZONE_RUNNING -> 
  ZONE_COMPLETE -> PROGRAM_COMPLETE -> IDLE
  
  Rain delay, manual override states
  ```

- **Program Structure**
  ```c
  typedef struct {
      uint8_t program_id;
      char name[32];
      uint8_t enabled;
      uint8_t days_of_week;        // Bitmask (Sun=0x01, Mon=0x02, ...)
      uint16_t start_hour;          // 0-23
      uint16_t start_minute;        // 0-59
      uint8_t num_zones;
      zone_schedule_t zones[16];    // Zone & runtime pairs
  } program_t;
  ```

#### 5.4 Runtime Calculation Engine
- **Base Runtime Calculation**
  - Program definition × ET adjustment × Season adjustment
  
- **ET-Adjusted Runtime**
  - Reference ET from weather manager
  - Crop coefficient per zone type
  - Soil type adjustment
  - Rain amount deduction
  
- **Seasonal Adjustment**
  - Growth stage curves
  - User override percentage (0-200%)

#### 5.5 Event Generation
- Zone state transitions → ZONE_STATE_CHANGED events
- Relay state transitions → RELAY_STATE_CHANGED events
- Runtime completion → ZONE_COMPLETE events
- Fault conditions → FAULT_DETECTED events
- Manual interventions → MANUAL_OVERRIDE events

#### 5.6 Fail-Safe Design (EDS-004)
- **Fault Triggers**
  - Pressure out of bounds
  - Runtime exceeds maximum
  - Relay stuck detection
  - Flow verification failure
  
- **Fail-Safe Response**
  - Close all zones
  - Close master valve
  - Generate critical alarm
  - Log fault with context
  - Notify MQTT subscribers

#### 5.7 Unit Tests
- `test/test_zone_state_machine.c`
- `test/test_relay_state_machine.c`
- `test/test_irrigation_engine.c`
  - Transitions
  - Event generation
  - Fault handling
  - Runtime calculations

#### 5.8 Verification
- ✓ All state machines implement defined transitions
- ✓ 100% unit test coverage
- ✓ Fault injection tests pass
- ✓ Event generation for every state change
- ✓ No deadlocks in concurrent state transitions

**Deliverable:**
- Complete zone, relay, and irrigation engine state machines
- Event-driven architecture functional
- Fail-safe design implemented
- Full state machine test coverage

**Commit Message:** `Step 5: Core domain model (Zone, Relay, Irrigation Engine) and state machines`

---

## Step 6: Flow & Pressure Management

**Objective:** Implement flow meter integration and pressure supervision for hydraulic safety verification.

**Duration:** 3-4 days

**Requirements Mapping:** FR-HYD-003, FR-HYD-004, Chapter 11 (Pressure Manager Architecture)

### Activities

#### 6.1 Flow Manager Component
- Create `components/flow_manager/`:
  - `flow_meter.h/c` - Flow measurement
  - `flow_manager.h/c` - Flow supervision

#### 6.2 Flow Meter Integration
- **Hardware Integration**
  - PCNT units 0-3 for pulse counting
  - Hall-effect sensors on each zone
  - Pulse-to-volume calibration

- **Flow Measurement Algorithm**
  - Pulse counting over fixed time window (1-second intervals)
  - Flow rate calculation: (pulses / calibration_factor) = gallons per minute
  - Instantaneous + rolling average (10-second window)
  - Detection of flow stagnation

#### 6.3 Pressure Manager Component
- Create `components/pressure_manager/`:
  - `pressure_sensor.h/c` - Pressure measurement
  - `pressure_manager.h/c` - Pressure supervision

#### 6.4 Pressure Monitoring
- **Sensor Integration**
  - ADS1115 ADC channel 0
  - 0-15 PSI transducer (4-20mA → voltage)
  - Sampling rate: 10Hz
  - Running average filter (5-sample window)

- **Pressure States**
  - OK (5-15 PSI during operation)
  - LOW (< 4 PSI - flow impedance)
  - HIGH (> 14 PSI - blockage)
  - NO_FLOW (pressure stable but flow = 0)

#### 6.5 Hydraulic Verification
- **Flow Verification Algorithm**
  - After zone start, expect flow within 5 seconds
  - Minimum flow rate: 0.5 GPM per zone
  - Maximum acceptable runtime without flow: 10 seconds
  
- **Pressure Verification Algorithm**
  - After zone start, expect pressure rise within 3 seconds
  - Steady-state pressure: 8±4 PSI (configurable per zone)
  - Detect pressure surge (sudden spike > 15 PSI)

#### 6.6 Event Generation
- FLOW_METER_UPDATE (every 1 second)
- FLOW_ANOMALY (stagnation/low flow detected)
- PRESSURE_UPDATE (every 100ms)
- PRESSURE_OUT_OF_BOUNDS (high/low alerts)
- VALVE_BLOCKAGE_DETECTED
- HYDRAULIC_SAFETY_FAULT

#### 6.7 Alarm Escalation
- First anomaly: Warning-level alarm
- Sustained anomaly (> 10 seconds): Critical alarm
- Response: Zone stop + master valve close + MQTT notification

#### 6.8 Unit Tests
- `test/test_flow_meter.c`
- `test/test_pressure_manager.c`
  - Pulse-to-flow conversion
  - Pressure filtering
  - Anomaly detection
  - Fault injection scenarios

#### 6.9 Verification
- ✓ Flow measurement accuracy: ±5% (vs. reference meter)
- ✓ Pressure measurement accuracy: ±0.5 PSI
- ✓ Anomaly detection latency: < 2 seconds
- ✓ Fault recovery: < 5 seconds to closed state
- ✓ No false positives on stable operation

**Deliverable:**
- Production flow and pressure monitoring
- Hydraulic safety verification fully integrated
- Event-driven anomaly detection
- Complete sensor test coverage

**Commit Message:** `Step 6: Flow and pressure management with hydraulic safety supervision`

---

## Step 7: Weather Engine & ET Adjustment

**Objective:** Implement weather data integration and ET-based irrigation adjustment.

**Duration:** 3-4 days

**Requirements Mapping:** FR-WEA-003, Chapter 8 & 9 (Weather Manager & ET Engine Architecture)

### Activities

#### 7.1 Weather Manager Component
- Create `components/weather_manager/`:
  - `weather_provider.h/c` - External data source
  - `weather_snapshot.h/c` - Current weather state
  - `weather_manager.h/c` - Manager task

#### 7.2 Weather Data Model
```c
typedef struct {
    uint32_t timestamp;
    float temperature_c;
    float humidity_percent;
    float solar_radiation_w_m2;
    float rainfall_mm;
    float wind_speed_kmh;
    uint32_t update_interval_seconds;
} weather_snapshot_t;
```

#### 7.3 Weather Data Sources
- **Primary:** National Weather Service (NWS) API via HTTP
- **Secondary:** OpenWeatherMap via MQTT
- **Tertiary:** Local sensors (future: solar radiation, rainfall gauge)

- **Update Strategy**
  - Poll external API every 30 minutes
  - Cache locally if API unavailable
  - Maximum cache age: 24 hours
  - Graceful degradation (use cached data)

#### 7.4 ET (Evapotranspiration) Engine
- Create `components/et_engine/`:
  - `et_calculator.h/c` - ET calculation
  - `et_engine.h/c` - Manager task

#### 7.5 ET Calculation Methods
- **Reference ET (ETo)**
  - Penman-Monteith equation (FAO-56 standard)
  - Inputs: Temperature, humidity, solar radiation, wind
  - Output: ETo in mm/day
  - Update frequency: Daily (midnight UTC)

- **Crop ET (ETc)**
  - ETc = ETo × Kc (crop coefficient)
  - Kc values per plant type:
    - Turf: 0.8
    - Trees: 0.9
    - Vegetables: 0.7
    - Native plants: 0.5

- **Adjusted Runtime**
  - Base runtime × (ETc / Reference ET) × Season factor
  - Capped at 200% for extreme conditions
  - Minimum: 20% of base runtime

#### 7.6 Rain Deduction
- Effective rainfall calculation:
  - Observed rainfall × infiltration factor (0.7-0.9)
  - Deduct from irrigation requirement
  - Skip irrigation if rain > daily requirement

- Rain delay feature:
  - User-selectable duration (24-72 hours)
  - Bypass all scheduled irrigation
  - Event-triggered override capability

#### 7.7 Event Generation
- WEATHER_UPDATE (every 30 minutes)
- ET_CALCULATION_COMPLETE (daily midnight)
- RAIN_DELAY_ACTIVATED/CLEARED
- WEATHER_API_ERROR (fallback to cache)

#### 7.8 Unit Tests
- `test/test_weather_manager.c`
- `test/test_et_calculator.c`
  - ET calculation accuracy
  - Rain deduction logic
  - Seasonal adjustment
  - API failure handling

#### 7.9 Verification
- ✓ ET calculation matches FAO-56 reference (< 5% error)
- ✓ Weather API integration: < 500ms HTTP call
- ✓ Cache fallback: automatic after 3 failures
- ✓ Rain deduction: accurate within ±1mm
- ✓ Offline operation: full functionality with cached data

**Deliverable:**
- Weather data integration with multiple providers
- ET-based runtime adjustment fully functional
- Rain delay and seasonal adjustment
- Weather API resilience and offline support

**Commit Message:** `Step 7: Weather manager and ET-based irrigation adjustment`

---

## Step 8: Alarm Management & Logging

**Objective:** Implement comprehensive alarm escalation and event logging for safety and diagnostics.

**Duration:** 3-4 days

**Requirements Mapping:** FR-ALM-002, FR-LOG-001+, FR-DIA-001+, Chapter 12 (Alarm Manager Architecture)

### Activities

#### 8.1 Alarm Manager Component
- Create `components/alarm_manager/`:
  - `alarm.h/c` - Alarm data model
  - `alarm_state_machine.h/c` - Alarm lifecycle
  - `alarm_manager.h/c` - Manager task
  - `alarm_log.h/c` - Persistent alarm history

#### 8.2 Alarm Model
```c
typedef enum {
    ALARM_INFO,      // Informational
    ALARM_WARNING,   // Requires attention
    ALARM_CRITICAL   // System unsafe, action required
} alarm_severity_t;

typedef struct {
    uint16_t alarm_id;
    alarm_severity_t severity;
    uint32_t timestamp;
    uint8_t zone_affected;
    char message[128];
    uint8_t auto_recovery_capable;
    uint32_t resolution_timestamp;  // When cleared
} alarm_t;
```

#### 8.3 Alarm Catalog
- **Critical Alarms (immediate action)**
  - Pressure out of bounds
  - Flow stagnation
  - Relay stuck
  - Hydraulic safety fault
  - Power anomaly
  - RTC failure
  
- **Warning Alarms (attention required)**
  - Low battery backup
  - Sensor recalibration due
  - Weather API unavailable
  - MQTT connection lost
  - OTA update available
  - Maintenance interval reached
  
- **Informational (logged)**
  - Zone started/completed
  - ET adjustment applied
  - Weather data updated
  - Program executed
  - Configuration changed

#### 8.4 Alarm State Machine
```
NEW -> ACTIVE -> ACKNOWLEDGED -> RESOLVED -> CLEARED
           ↓
      (auto-resolution check)
```

- **Escalation Paths**
  - WARNING → CRITICAL (if condition persists > threshold)
  - Auto-recovery attempt on certain faults
  - Manual acknowledgment required for some alarms

#### 8.5 Alarm Actions
- **Critical**
  - Generate immediate notification (MQTT, LED flash)
  - Execute safety shutdown (close all zones)
  - Log to persistent storage
  - Attempt recovery (retry 3 times with backoff)

- **Warning**
  - Generate notification (MQTT only)
  - Log to event log
  - Do not interrupt operation
  - Allow user acknowledgment

#### 8.6 Logging System
- Create `components/storage_manager/`:
  - `event_logger.h/c` - Event logging
  - `storage_manager.h/c` - Storage interface

#### 8.7 Event Log Schema
```c
typedef struct {
    uint32_t timestamp_unix;
    uint16_t event_code;
    uint8_t severity;
    uint8_t zone_id;
    uint32_t value;  // Context-specific
    char context[64];
} zic_log_entry_t;
```

- **Log Categories**
  - Zone events (start/stop/fault)
  - Irrigation events (program run, ET update)
  - Alarms (all severity levels)
  - Weather events (API update, rain detected)
  - System events (boot, OTA, config change)

#### 8.8 Logging Implementation
- Circular buffer in RAM: 512 entries
- Periodic flush to NVS: every 1 hour or on critical event
- Export to JSON format for diagnostics
- Retention: 30-day rolling window

#### 8.9 Diagnostics Component
- Create `components/diagnostics_manager/`:
  - `diagnostics.h/c` - System diagnostics
  - Health report generation

- **Health Indicators**
  - Uptime
  - Task execution times
  - Memory utilization
  - Event bus latency
  - Sensor reading variance
  - Fault count
  - Last reboot reason

#### 8.10 Unit Tests
- `test/test_alarm_manager.c`
- `test/test_event_logger.c`
  - Alarm state transitions
  - Escalation logic
  - Log rotation
  - Export formatting

#### 8.11 Verification
- ✓ Critical alarms trigger within 100ms
- ✓ Alarm acknowledgment reduces notification frequency
- ✓ Log rotation: no data loss at wrap
- ✓ Event export: valid JSON, all fields present
- ✓ 30-day retention: verified with timestamp validation

**Deliverable:**
- Production alarm management system
- Comprehensive event logging and diagnostics
- Alarm escalation and auto-recovery
- Health monitoring and reporting

**Commit Message:** `Step 8: Alarm management and comprehensive event logging`

---

## Step 9: MQTT Integration & Remote Control

**Objective:** Implement MQTT communication for remote control, status reporting, and cloud integration.

**Duration:** 4-5 days

**Requirements Mapping:** FR-MQTT-001+, Volume 3 (MQTT & API Specification), Chapter 13 (MQTT Manager Architecture)

### Activities

#### 9.1 MQTT Manager Component
- Create `components/mqtt_manager/`:
  - `mqtt_topic.h/c` - Topic definitions
  - `mqtt_payload.h/c` - Payload schemas
  - `mqtt_transport.h/c` - Transport layer
  - `mqtt_manager.h/c` - Manager task

#### 9.2 MQTT Topic Structure
```
zmartify/controller/{controller_id}/
├── status/                    (publish)
│   ├── system
│   ├── zones/{zone_id}
│   ├── irrigation
│   ├── weather
│   └── alarms
├── telemetry/                 (publish)
│   ├── flow/{zone_id}
│   ├── pressure
│   ├── temperature
│   └── power
├── command/                   (subscribe)
│   ├── start_zone
│   ├── stop_zone
│   ├── run_program
│   ├── stop_all
│   ├── rain_delay
│   ├── config_update
│   └── ota_trigger
└── diagnostic/                (publish)
    ├── event_log
    ├── health_report
    └── performance
```

#### 9.3 Payload Schemas (JSON)
- **Zone Status**
  ```json
  {
    "zone_id": 1,
    "state": "running",
    "runtime_requested": 900,
    "runtime_actual": 450,
    "flow_rate_gpm": 2.5,
    "pressure_psi": 9.2,
    "last_run": "2026-06-26T14:30:00Z"
  }
  ```

- **System Status**
  ```json
  {
    "controller_id": "zic-01",
    "uptime_seconds": 3600000,
    "mode": "auto",
    "active_zones": 1,
    "fault_count": 0,
    "mqtt_connected": true,
    "last_update": "2026-06-26T15:45:30Z"
  }
  ```

- **Command Format**
  ```json
  {
    "action": "start_zone",
    "zone_id": 2,
    "duration_seconds": 600,
    "priority": "manual_override"
  }
  ```

#### 9.4 Transport Layer
- MQTT 3.1.1 over TLS 1.2
- Broker: configurable URI (default: 192.168.10.2:8883)
- Client certificate authentication (ATECC608B)
- QoS levels:
  - Status: QoS 1 (at-least-once)
  - Commands: QoS 2 (exactly-once)
  - Telemetry: QoS 0 (fire-and-forget)

#### 9.5 Publish Strategy
- **Status Topics** (every 60 seconds)
  - System status
  - Zone status (all zones)
  - Irrigation engine state
  - Weather snapshot

- **Telemetry Topics** (every 10 seconds)
  - Flow rates
  - Pressure readings
  - Temperature
  - Power consumption

- **Event-Triggered**
  - Zone state changes (immediate)
  - Alarms (immediate)
  - Faults (immediate)
  - Configuration changes (immediate)

#### 9.6 Command Processing
- Subscribe to command topics
- Validate JSON schema
- Check permissions (future: user roles)
- Queue to command processor
- Publish acknowledgment or error

- **Command Handlers**
  - Start zone: priority verification, duration limits
  - Stop zone: graceful shutdown
  - Run program: validation against schedule
  - Stop all: emergency stop
  - Rain delay: validation of duration
  - Config update: schema validation, persistence
  - OTA trigger: version check, staging

#### 9.7 Connection Management
- Auto-reconnect with exponential backoff
- Connection keepalive (60-second ping)
- Offline command buffering (up to 32 commands)
- Will topic for last-will-and-testament
- Certificate renewal capability

#### 9.8 Security
- TLS certificate in ATECC608B
- Client certificate pinning
- Payload encryption (AES-128, future)
- Command signature verification (future)
- Rate limiting on commands (max 10/minute)

#### 9.9 Unit Tests
- `test/test_mqtt_payload.c`
- `test/test_mqtt_manager.c`
  - Payload schema validation
  - Topic routing
  - Command processing
  - Connection resilience
  - Offline buffering

#### 9.10 Verification
- ✓ MQTT publish latency: < 500ms (average)
- ✓ Command processing: < 200ms
- ✓ Connection recovery: < 30 seconds
- ✓ Offline buffering: no data loss
- ✓ Payload format: 100% schema compliance
- ✓ TLS handshake: successful with test broker

**Deliverable:**
- Complete MQTT implementation with TLS
- Full publish/subscribe architecture
- Command processing and acknowledgment
- Offline resilience and connection management

**Commit Message:** `Step 9: MQTT integration and remote control system`

---

## Step 10: OTA Updates, Testing & Production Release

**Objective:** Implement OTA update mechanism, complete system integration testing, and prepare for production release.

**Duration:** 5-7 days

**Requirements Mapping:** FR-MQTT-003, Chapter 16 (OTA Manager Architecture), Testing & Verification (Chapter 11, Volume 1)

### Activities

#### 10.1 OTA Manager Component
- Create `components/ota_manager/`:
  - `ota_manager.h/c` - OTA coordination
  - `ota_validator.h/c` - Firmware validation
  - `ota_rollback.h/c` - Rollback logic

#### 10.2 OTA Update Flow
```
User triggers OTA via MQTT
  ↓
Download firmware from signed URL
  ↓
Validate signature (RSA-2048)
  ↓
Validate SHA-256 hash
  ↓
Write to OTA partition (APP1)
  ↓
Validate app partition (CRC32)
  ↓
Set OTA boot flag
  ↓
Reboot
  ↓
Bootloader selects updated partition
  ↓
Boot new firmware
  ↓
Verify functionality (health check)
  ↓
Commit or rollback
```

#### 10.3 Firmware Signing & Validation
- **Build System Integration**
  - Sign firmware with RSA-2048 private key
  - Generate SHA-256 hash
  - Embed public key in bootloader

- **Runtime Validation**
  - Verify RSA signature on download
  - Validate SHA-256
  - Check firmware header
  - CRC32 of app partition

#### 10.4 OTA Partition Layout
- Bootloader: 64KB (signed)
- OTA app 0 (APP0): 1.8MB (active firmware)
- OTA app 1 (APP1): 1.8MB (staging area)
- OTA data: 8KB (slot selector)

#### 10.5 Rollback Protection
- Health check after boot:
  - All subsystems initialize successfully
  - Event bus operational
  - MQTT connectivity established
  - All managers report ready
  - Timeout: 60 seconds

- If health check fails:
  - Rollback to previous partition
  - Generate CRITICAL alarm
  - Log failure reason
  - Notify via MQTT

#### 10.6 System Integration Testing
- Create `test/integration/`:
  - `test_end_to_end.c` - Complete workflow
  - `test_fault_injection.c` - Failure scenarios
  - `test_performance.c` - Resource utilization
  - `test_mqtt_cloud.c` - Cloud integration

#### 10.7 Integration Test Scenarios
- **Irrigation Workflow**
  - Program execution from start to completion
  - Zone sequencing
  - Flow/pressure verification
  - Proper zone state transitions

- **Weather Integration**
  - ET calculation accuracy
  - Rain delay application
  - Seasonal adjustment

- **Fault Scenarios**
  - Pressure out of bounds → emergency stop
  - Flow stagnation → alarm escalation
  - MQTT disconnect → offline buffering
  - Power loss → graceful shutdown
  - OTA failure → rollback

- **Concurrent Operations**
  - Manual override while program running
  - Config update during irrigation
  - Weather API call during zone run
  - MQTT command while in fault state

#### 10.8 Performance Profiling
- Task execution times (worst-case)
  - Zone manager: < 50ms per cycle
  - Irrigation engine: < 100ms per cycle
  - Weather manager: < 200ms per cycle
  - MQTT manager: < 300ms per cycle

- Memory utilization
  - Heap: < 80% at peak
  - Stack per task: verified
  - No memory leaks (60-hour burn test)

- CPU utilization
  - Average: < 30%
  - Peak: < 70%
  - Reserve for future features: > 20%

#### 10.9 Production Build Configuration
- Enable all safety features
- Disable debug logging
- Set optimization level to -O2
- Enable firmware signing
- Configure partition sizes per spec
- Set secure boot (future)

#### 10.10 Release Artifacts
- **Firmware**
  - Signed binary: `zmartify_ic_v5.0_signed.bin`
  - Unsigned binary: `zmartify_ic_v5.0.bin`
  - Partition table: `partition_table.bin`
  - Bootloader: `bootloader.bin`

- **Documentation**
  - Release notes
  - Upgrade guide
  - Rollback procedure
  - Known limitations
  - Verification checklist

- **Tools**
  - Flash script: `scripts/flash.sh`
  - OTA upload script: `scripts/ota_upload.sh`
  - Recovery script: `scripts/recovery.sh`
  - Diagnostic tool: `tools/diag_collector.py`

#### 10.11 Pre-Release Verification Checklist
- ✓ All 10 steps implemented and tested
- ✓ Zero high-severity bugs
- ✓ 100% of critical requirements verified
- ✓ Performance targets met
- ✓ Memory utilization acceptable
- ✓ OTA update tested end-to-end
- ✓ Firmware signature validation verified
- ✓ Rollback tested successfully
- ✓ MQTT cloud integration functional
- ✓ Documentation complete and accurate
- ✓ Coding standards compliance verified
- ✓ 60-hour burn test passed
- ✓ FAT (Factory Acceptance Test) passed
- ✓ Field testing completed

#### 10.12 Release Process
1. **Create Release Branch**
   - `git checkout -b release/v5.0`
   - Update version in firmware header

2. **Build Production Firmware**
   - `idf.py build`
   - Sign firmware
   - Generate checksums

3. **Create Release Notes**
   - Feature list
   - Bug fixes
   - Known issues
   - Migration guide

4. **Tag Release**
   - `git tag -a v5.0 -m "Production release v5.0"`

5. **Archive Artifacts**
   - Save signed binaries
   - Save all test results
   - Save release notes
   - Backup private keys

6. **Prepare OTA Repository**
   - Host signed binary on secure server
   - Publish metadata
   - Enable remote OTA capability

#### 10.13 Post-Release Monitoring
- Monitor field devices for errors
- Collect telemetry from devices
- Track OTA update success rates
- Gather user feedback
- Plan v5.1 enhancement cycle

#### 10.14 Verification
- ✓ OTA update completes in < 2 minutes
- ✓ Firmware validation: 100% success
- ✓ Rollback: automatic on health check failure
- ✓ System integration: all scenarios pass
- ✓ Performance: all targets met
- ✓ Burn test: 60 hours, zero failures
- ✓ FAT documentation: complete
- ✓ Release artifacts: verified and signed

**Deliverable:**
- Production-ready firmware v5.0
- Complete OTA infrastructure
- Comprehensive system testing
- Release documentation and tools
- Production deployment ready

**Commit Message:** `Step 10: OTA infrastructure, integration testing, and production release`

---

## Implementation Timeline

| Phase | Steps | Duration | Milestone |
|-------|-------|----------|-----------|
| Foundation | 1-2 | 5-7 days | Event bus operational |
| Core Systems | 3-4 | 7-9 days | HAL + Storage complete |
| Logic & Control | 5-7 | 10-13 days | Full irrigation control |
| Integration | 8-9 | 7-9 days | Remote + logging operational |
| Release | 10 | 5-7 days | Production ready |
| **TOTAL** | **10** | **34-45 days** | **v5.0 Complete** |

---

## Success Criteria

Each step shall be considered complete only when:

1. **Code Quality**
   - Compiles without warnings
   - Passes all unit tests (100% coverage on critical paths)
   - Follows coding standards (Chapter 20, Volume 2)

2. **Functionality**
   - Implements 100% of required features per step
   - Event-driven architecture maintained
   - No blocking calls in critical sections

3. **Documentation**
   - Code comments explain non-obvious logic
   - Public API documented with examples
   - Architecture decisions recorded

4. **Testing**
   - Unit tests pass
   - Integration tests pass
   - Performance targets met
   - No memory leaks

5. **Verification**
   - Requirements traceability verified
   - Code review completed
   - Test results archived

---

## Risk Management

| Risk | Mitigation |
|------|-----------|
| Hardware changes cause API incompatibility | Rigorous HAL testing, simulation layer |
| MQTT broker unavailable | Offline buffering, local queue |
| OTA update corruption | Signature validation, rollback |
| Memory exhaustion | Pre-allocated buffers, monitoring |
| Task priority inversion | Priority ceiling locks, careful scheduling |
| Sensor failures | Graceful degradation, fallback modes |

---

## Future Enhancements (Post v5.0)

- **v5.1:** Soil moisture sensors, enhanced weather providers, analytics dashboard
- **v5.2:** Pump controller, reservoir monitoring, remote valve stations
- **v6.0:** Custom PCB, CAN Bus, distributed architecture, industrial I/O

---

## References

- [Zmartify Master Engineering Package v5.0 - Volume 1](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%201%20–%20Product%20Definition,%20Functional%20Requirements%20Specification%20(FRS)%20&%20Engineering%20Design%20Specification%20(EDS)/)
- [Zmartify Master Engineering Package v5.0 - Volume 2](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%202%20-%20Firmware%20Architecture/)
- [Zmartify Master Engineering Package v5.0 - Volume 3](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%203%20–%20MQTT,%20REST%20API%20&%20Integration%20Specification/)

---

## Document Control

| Item | Value |
|------|-------|
| Title | Zmartify Irrigation Controller - Development Plan v5.0 |
| Version | 1.0 |
| Date | 2026-06-26 |
| Author | Engineering Team |
| Status | Ready for Implementation |
| Next Review | After Step 1 Completion |

---

**End of Development Plan**

This plan is a living document. It shall be updated as implementation progresses and unforeseen challenges are addressed. All changes shall be recorded with version control.

# Zmartify Irrigation Controller – Architecture Overview v5.0

**Document:** Architecture Overview – v5.0  
**Based On:** Volume 2, Chapters 1–5 (Architecture Baseline)  
**Status:** Active – Engineering Baseline  
**Hardware Platform:** ZIC-S3 Rev.B (ESP32-S3)  

---

## 1 Architecture Philosophy

The Zmartify Irrigation Controller firmware is built on these core principles:

### 1.1 EDS-001: Modularity
- Hardware and software divided into independent functional modules
- Well-defined interfaces with minimal dependencies
- Each module responsible for one concern

### 1.2 EDS-002: Layered Architecture
- Strict separation of concerns
- Application never communicates directly with hardware
- All hardware access through HAL abstraction
- Clear dependency flow: Application → Logic → HAL → Hardware

### 1.3 EDS-003: Event-Driven Operation
- Subsystems communicate through asynchronous events
- Modules do not call unrelated modules directly
- Benefits: loose coupling, reliability, testability, scalability

### 1.4 EDS-004: Fail-Safe Design
- Any detected critical fault → hydraulically safe condition
- Safe = Master valve closed + All zones closed + Alarm + Logging + MQTT notification
- Safety-critical code paths tested separately

### 1.5 EDS-005: Data Persistence
- Configuration survives power failures, firmware updates, resets
- Operational history survives normal power interruptions
- NVS-based persistent storage with CRC protection

---

## 2 System Architecture Layers

```
┌────────────────────────────────────────────┐
│       User Interface Layer (LVGL)          │
│  Touch Screen • Local Buttons • LED        │
└────────────────────────────────────────────┘
                     ↕
┌────────────────────────────────────────────┐
│     Application Services Layer             │
│ Programs • Weather • Alarms • Diagnostics  │
└────────────────────────────────────────────┘
                     ↕
┌────────────────────────────────────────────┐
│   Irrigation Decision Layer (Core Logic)   │
│ ET Engine • Zone Manager • Flow Manager    │
│ Pressure Manager • Irrigation Engine       │
└────────────────────────────────────────────┘
                     ↕
┌────────────────────────────────────────────┐
│        Event Bus & Communication           │
│ Event Queue • Dispatcher • Subscriptions   │
└────────────────────────────────────────────┘
                     ↕
┌────────────────────────────────────────────┐
│    Hardware Abstraction Layer (HAL)        │
│ GPIO • I²C • ADC • PWM • PCNT • MQTT      │
└────────────────────────────────────────────┘
                     ↕
┌────────────────────────────────────────────┐
│         ESP-IDF / FreeRTOS / LwIP          │
│              Operating System              │
└────────────────────────────────────────────┘
```

### 2.1 User Interface Layer
- LVGL-based touchscreen interface (future)
- Local buttons for manual override
- System LED for status indication
- Never contains business logic

### 2.2 Application Services Layer
- MQTT Manager – Cloud connectivity & remote control
- OTA Manager – Firmware updates
- Alarm Manager – Escalation & user notifications
- Diagnostics Manager – System health monitoring
- Storage Manager – Data export & logging

### 2.3 Irrigation Decision Layer
- **Irrigation Engine** – Central orchestrator
  - Program execution
  - Zone sequencing
  - Runtime calculations
  - Safety supervision
  - Never controls hardware directly

- **Zone Manager** – Zone-level state machine
  - Zone state transitions
  - Valve open/close sequencing
  - Per-zone configuration

- **Flow Manager** – Flow measurement & anomaly detection
  - Pulse counting from turbine meters
  - Flow rate calculation
  - Stagnation detection
  - Minimum flow verification

- **Pressure Manager** – Pressure supervision & safety
  - ADC-based pressure reading
  - Pressure trending
  - Out-of-bounds detection
  - Hydraulic fault handling

- **Weather Manager** – Weather data integration
  - External API polling (NWS, OpenWeatherMap)
  - Local sensor integration
  - Cache management & fallback

- **ET Engine** – Evapotranspiration calculation
  - Penman-Monteith calculation (FAO-56)
  - Daily ET updates
  - Seasonal adjustment factors
  - Rain deduction logic

### 2.4 Event Bus Layer
- **Event Queue** – FreeRTOS queue for event propagation
- **Dispatcher** – Routes events to subscribers
- **Subscription Database** – Tracks event listeners
- **Priority Handling** – Safety events (14-15) before telemetry (0-3)

**Key Design:** Events are immutable facts, not commands. Producers never know subscribers. Subscribers never know producers.

### 2.5 Hardware Abstraction Layer (HAL)
Provides unified interface to all hardware:

- **GPIO** – Digital I/O (relays, buttons, LED)
- **I²C** – Multi-device communication
  - MCP23017 (GPIO expander)
  - ADS1115 (16-bit ADC)
  - MCP9808 (Temperature)
  - ATECC608B (Crypto accelerator)
- **ADC** – Analog measurements
- **PWM** – Servo/relay duty cycle control
- **PCNT** – Pulse counting (flow meters)
- **MQTT** – TLS-secured cloud connectivity
- **NVS** – Persistent configuration storage

---

## 3 Functional Subsystems

| ID | Subsystem | Responsibility | Thread |
|----|-----------|-----------------|---------
| SS-01 | Irrigation Engine | Program orchestration, decision-making | Control Task |
| SS-02 | Relay Manager | Relay state machine, activation timing | HAL Task |
| SS-03 | Zone Manager | Zone lifecycle, state transitions | Control Task |
| SS-04 | Weather Engine | ET calculation, weather integration | Weather Task |
| SS-05 | Flow Manager | Flow measurement, anomaly detection | Sensor Task |
| SS-06 | Pressure Manager | Pressure supervision, safety | Sensor Task |
| SS-07 | Alarm Manager | Alarm escalation, user notification | Service Task |
| SS-08 | MQTT Manager | Cloud communication, remote control | Telemetry Task |
| SS-09 | User Interface | Touchscreen & buttons (future) | UI Task |
| SS-10 | Configuration Manager | Settings storage & retrieval | Service Task |
| SS-11 | OTA Manager | Firmware updates, rollback | Service Task |
| SS-12 | Diagnostics Manager | Health monitoring, performance stats | Diagnostic Task |

**Independence:** Each subsystem operates independently, communicating only through events.

---

## 4 FreeRTOS Task Model

```
                 FreeRTOS Scheduler
                         │
    ┌────────────────────┼────────────────────┐
    │                    │                    │
    ▼                    ▼                    ▼

Safety Tasks      Control Tasks         Service Tasks
    │                   │                    │
    ▼                   ▼                    ▼
├─ Relay Control   ├─ Irrigation Engine ├─ Alarm Manager
├─ Pressure Guard  ├─ Zone Manager      ├─ OTA Manager
├─ Flow Guard      ├─ ET Engine         ├─ Diagnostics
└─ Watchdog        ├─ Weather Manager   ├─ Configuration
                   ├─ UI Manager        └─ Storage
                   └─ Flow/Pressure
                      Sampling
```

### 4.1 Task Priorities (0=lowest, configurable)

| Task | Priority | Purpose |
|------|----------|---------|
| Watchdog | 15 (Critical) | System health monitoring |
| Pressure Guard | 14 | Hydraulic protection |
| Flow Guard | 14 | Flow anomaly detection |
| Relay Control | 12 | Hardware actuation |
| Irrigation Engine | 10 | Decision making |
| Zone Manager | 10 | Zone sequencing |
| Weather/ET | 8 | Calculation tasks |
| Sensor Reading | 8 | ADC sampling |
| MQTT/Telemetry | 6 | Cloud communication |
| UI Manager | 5 | User interface |
| Diagnostics | 4 | Background monitoring |
| Idle | 0 | Idle/power saving |

### 4.2 Task Scheduling

- **Safety Tasks:** Highest priority (14-15)
  - Never blocked by lower tasks
  - Short execution time (< 50ms)
  - Deterministic response time

- **Control Tasks:** Medium priority (8-12)
  - Coordinate decision making
  - Moderate execution time (50-200ms)
  - May wait for events

- **Service Tasks:** Lower priority (4-7)
  - Background processing
  - Longer execution times acceptable
  - Can be preempted freely

---

## 5 Event Bus Architecture

### 5.1 Event Categories

| Category | Priority | Use Case | Subscribers |
|----------|----------|----------|-------------|
| Safety | 14-15 | Critical faults | Alarm Manager, Irrigation Engine, Logging |
| Control | 8-10 | Zone changes, irrigation updates | MQTT, UI, Diagnostics |
| Telemetry | 4-7 | Sensor readings, status updates | Data logger, MQTT, Diagnostics |
| Diagnostic | 0-3 | Debug info, performance metrics | Diagnostics only |

### 5.2 Event Lifecycle

```
Generate Event
    ↓
Validate Format
    ↓
Timestamp (system uptime)
    ↓
Assign Priority
    ↓
Insert into Queue (priority-ordered)
    ↓
Dispatcher Retrieves
    ↓
Query Subscription DB
    ↓
Deliver to All Subscribers
    ↓
Event Complete
```

### 5.3 Event Payload Design

```c
typedef struct {
    uint32_t event_id;           // Unique identifier
    uint32_t producer_id;         // Source module
    uint64_t timestamp_us;        // System uptime
    uint8_t priority;             // 0-15 (15 = critical)
    uint8_t payload_type;         // Interpretation guide
    uint32_t payload_size;        // Bytes attached
    void *payload;                // Event-specific data
} zic_event_t;
```

---

## 6 State Machine Design Pattern

Every major subsystem (Zone, Relay, Irrigation Engine) implements a state machine.

### 6.1 Example: Zone State Machine

```
┌─────────────────────────────────────────┐
│             IDLE State                  │
│  Zone not active, waiting for command   │
└──────────────────┬──────────────────────┘
                   │ Start Request
                   ▼
┌─────────────────────────────────────────┐
│          REQUESTED State                │
│  Validating parameters, preparing       │
└──────────────────┬──────────────────────┘
                   │ Validation OK
                   ▼
┌─────────────────────────────────────────┐
│       VALVE_OPENING State               │
│  Opening solenoid, waiting for feedback │
└──────────────────┬──────────────────────┘
                   │ Valve opened + Flow detected
                   ▼
┌─────────────────────────────────────────┐
│           RUNNING State                 │
│  Zone actively irrigating               │
└──────────────────┬──────────────────────┘
                   │ Runtime expired OR manual stop
                   ▼
┌─────────────────────────────────────────┐
│        VALVE_CLOSING State              │
│  Closing solenoid, verifying closure    │
└──────────────────┬──────────────────────┘
                   │ Valve closed, flow stopped
                   ▼
┌─────────────────────────────────────────┐
│         STOPPED State                   │
│  Zone shutdown complete                 │
└──────────────────┬──────────────────────┘
                   │ Return to IDLE
                   ▼
                 IDLE
```

**Key Design Rules:**
- Each state has clear entry/exit conditions
- Transitions triggered by events
- Timeout protection from stuck states
- Fault paths from any state to safe condition

---

## 7 Hardware Interface Definition

### 7.1 Relay Outputs (via MCP23017)

| Output | Function | Voltage | Type |
|--------|----------|---------|------|
| 0 | Master Valve | 24 VAC | Solenoid |
| 1-15 | Zone Solenoids | 24 VAC | Solenoid |

**Control:** Active-low (0 = energized), optically isolated

### 7.2 Sensor Inputs

| Input | Source | Resolution | Rate |
|-------|--------|-----------|------|
| Flow | DN50 Turbine (PCNT) | Pulses/sec | 1 Hz |
| Pressure | 0-10 bar (ADS1115) | 16-bit | 10 Hz |
| Temperature | MCP9808 | 16-bit | 1 Hz |

### 7.3 Network Interfaces

| Interface | Purpose | Protocol | Security |
|-----------|---------|----------|----------|
| Wi-Fi | Cloud communication | 802.11 b/g/n | WPA2 |
| MQTT | Command/Status | MQTT 3.1.1 | TLS 1.2 |
| I²C | Sensor communication | I²C 400kHz | None (local) |

---

## 8 Data Flow Examples

### 8.1 Zone Start Flow

```
User/MQTT
   │
   ├─→ Irrigation Engine (start_zone command)
   │       │
   │       ├─→ Zone Manager (zone state machine)
   │       │       │
   │       │       ├─→ Relay Manager (energize solenoid)
   │       │       │       │
   │       │       │       └─→ HAL GPIO (output pulse)
   │       │       │
   │       │       ├─→ Flow Manager (start monitoring)
   │       │       │
   │       │       └─→ Pressure Manager (start supervision)
   │       │
   │       └─→ Event Bus (ZONE_STARTED)
   │
   ├─ Alarm Manager (receives event, logs start)
   ├─ MQTT Manager (receives event, publishes status)
   └─ Diagnostics (receives event, updates telemetry)
```

### 8.2 Pressure Anomaly Response

```
Pressure Manager
   │
   └─→ Detects Pressure > MAX (15 PSI)
       │
       ├─→ Generate PRESSURE_CRITICAL event
       │
       └─→ Event Bus
           │
           ├─→ Alarm Manager (escalate to CRITICAL)
           │   │
           │   └─→ Generate ALARM_CRITICAL event
           │
           ├─→ Irrigation Engine (receives event)
           │   │
           │   └─→ Stop all zones immediately
           │
           ├─→ Relay Manager (de-energize all relays)
           │   │
           │   └─→ HAL GPIO (close all valves)
           │
           ├─→ MQTT Manager (notify cloud)
           │
           └─→ Storage Manager (log fault with timestamp)
```

---

## 9 Data Storage Model

### 9.1 Persistent Storage (NVS - 64KB)

```
NVS Partition
├── Configuration
│   ├── Controller ID
│   ├── Network settings
│   ├── Irrigation programs
│   ├── Zone configuration
│   └── Alarm thresholds
├── Persistent State
│   ├── Last known zone states
│   ├── Fault counters
│   ├── Maintenance intervals
│   └── OTA update status
└── Calibration Data
    ├── Pressure calibration
    ├── Flow scale factor
    └── Sensor offsets
```

### 9.2 Circular Event Log (RAM)

```
RAM Buffer (512 entries)
├── Entry 0: [timestamp, event_id, severity, zone_id, value]
├── Entry 1: [...]
├── ...
└── Entry 511: [...]

Periodic Flush to NVS (hourly or on critical event)
```

### 9.3 Partition Layout (8MB ESP32-S3)

| Partition | Size | Purpose |
|-----------|------|---------|
| Bootloader | 64 KB | Secure boot & OTA |
| OTA Data | 8 KB | OTA slot selector |
| NVS | 64 KB | Configuration |
| APP0 | 1.8 MB | Active firmware |
| APP1 | 1.8 MB | OTA staging |
| SPIFFS | 2 MB | Optional: diagnostics log |

---

## 10 Component Dependencies

```
irrigation_engine (core decision maker)
    ├── zone_manager
    ├── relay_manager
    ├── flow_manager
    ├── pressure_manager
    └── et_engine

et_engine
    └── weather_manager

weather_manager
    └── mqtt_manager (for API calls)

mqtt_manager
    ├── alarm_manager (notifications)
    ├── storage_manager (config)
    └── ota_manager (firmware updates)

All components
    └── event_bus (inter-communication)
    └── hal (hardware access)
    └── persistent_store (configuration)
```

---

## 11 Safety & Reliability

### 11.1 Fail-Safe Principles

1. **Detection:** All safety-critical values monitored continuously
2. **Verification:** Multiple sensors confirm state changes
3. **Response:** Immediate transition to safe condition
4. **Notification:** Alarm generated, MQTT notified, logged
5. **Recovery:** Manual intervention or automatic retry (configurable)

### 11.2 Fault Scenarios

| Fault | Detected By | Response | Recovery |
|-------|-------------|----------|----------|
| Pressure > MAX | Pressure Manager | Close all zones | Manual or automatic |
| Flow = 0 (running) | Flow Manager | Stop zone after timeout | Retry or manual |
| Relay stuck | Pressure/Flow verify | Emergency stop | Manual |
| Power loss | Watchdog | Graceful shutdown | Power restore → boot |
| MQTT down | MQTT Manager | Buffer commands | Connection restore |
| OTA corruption | Signature validation | Rollback | Automatic |

---

## 12 Performance Targets

| Metric | Target | Method |
|--------|--------|--------|
| Zone start latency | < 2 seconds | Event propagation timing |
| Pressure response | < 100ms | Sensor + event latency |
| Flow anomaly detection | < 5 seconds | Sustained low flow |
| MQTT publish latency | < 500ms | Network timing |
| Event dispatch | < 1ms (99th %ile) | Event bus benchmark |
| Memory utilization | < 80% heap | Profiling at peak load |
| CPU utilization | < 30% average | Task profiling |

---

## 13 Testing Strategy

### 13.1 Test Levels

| Level | Method | Scope |
|-------|--------|-------|
| Unit | Individual functions | Component-specific |
| Integration | Component interaction | Multiple subsystems |
| System | Full workflow | Complete controller |
| Acceptance | FAT + SAT | Hardware + field |

### 13.2 Critical Test Paths

1. **Normal Irrigation Cycle**
   - Program execution
   - Zone sequencing
   - Flow/pressure verification
   - Completion and logging

2. **Fault Handling**
   - Pressure out-of-bounds
   - Flow stagnation
   - Relay stuck
   - Emergency stop

3. **MQTT & OTA**
   - Remote command processing
   - Status reporting
   - Firmware update & rollback
   - Connection recovery

4. **Power Management**
   - Graceful shutdown
   - State preservation
   - Boot sequence
   - Watchdog recovery

---

## 14 Code Organization

### 14.1 Module Structure

Each component follows this pattern:

```
components/component_name/
├── CMakeLists.txt              # Build configuration
├── idf_component.yml           # Component metadata
├── include/
│   └── component_name.h        # Public API
├── src/
│   └── component_name.c        # Implementation
├── private/
│   └── component_name_*.h      # Internal headers
├── test/
│   └── test_*.c                # Unit tests
└── README.md                   # Component documentation
```

### 14.2 Public API Pattern

Every component exports:
- Initialization function: `component_init()`
- Cleanup function: `component_cleanup()`
- Status query function: `component_get_status()`
- Error enums and types

---

## 15 Future Architecture Evolution

### 15.1 Planned Enhancements (v5.1–v6.0)

- **v5.1:** Soil moisture sensors, custom weather API, analytics
- **v5.2:** Pump controller, reservoir monitoring, remote valve stations
- **v6.0:** Custom PCB, CAN Bus, distributed architecture

### 15.2 Architecture Extensibility

The v5.0 design supports future expansion:

- **Event Bus:** Handles new event types automatically
- **HAL:** Additional sensors/actuators plug in without core changes
- **MQTT:** New topics & commands via topic registry
- **Storage:** Versioned config schema allows migration
- **State Machines:** Extensible with new states

---

## 16 Document Control

| Item | Value |
|------|-------|
| Title | Zmartify Architecture Overview v5.0 |
| Version | 1.0 |
| Status | Active – Engineering Baseline |
| Based On | Volume 2, Chapters 1–5 |
| Date | 2026-06-26 |
| Next Review | End of Step 5 Implementation |

---

## References

- Volume 1: Product Definition, FRS & EDS
- Volume 2: Firmware Architecture (Chapters 1–5)
- Volume 3: MQTT & API Specification
- Volume 4: Hardware Platform (ZIC-S3 Rev.B)
- [CODING_STANDARDS.md](CODING_STANDARDS.md)
- [DEVELOPMENT-PLAN-v5.0.md](docs/DEVELOPMENT-PLAN-v5.0.md)

---

**End of Architecture Overview**

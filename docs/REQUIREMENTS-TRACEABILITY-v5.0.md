# Requirements Traceability Matrix - MEP v5.0

**Project:** Zmartify Irrigation Controller
**Baseline:** Master Engineering Package (MEP) v5.0 plus active WaterSensor v5.1 addendum
**RTM version:** 1.1
**Created:** 2026-07-19
**Owner:** Firmware and system engineering
**Plan:** [Implementation Plan 2](IMPLEMENTATION-PLAN-2.md), Step 1

## 1. Purpose and Use

This is the living traceability baseline for product maturation. It distinguishes:

- a requirement being described in the MEP;
- code being present;
- code being integrated into the running controller;
- automated verification being present;
- physical hardware, FAT or SAT evidence being present.

A component directory is never sufficient implementation evidence. Status must be updated in the
same commit as the implementation or verification evidence that changes it.

The table is searchable by `Domain`, `Status` and `Plan step`. Requirement text is condensed;
the linked source remains authoritative.

## 2. Status Model

| Status | Meaning |
|---|---|
| `Implemented` | Applicable behaviour is integrated and has concrete automated verification evidence. |
| `Partial` | Some applicable behaviour exists, but scope, integration or verification is incomplete. |
| `Missing` | Applicable behaviour is not implemented. |
| `Not Verified` | Design/code or physical equipment may exist, but required measured, hardware, FAT, SAT or long-term evidence is absent. |
| `Not Applicable` | Requirement is outside the approved product scope or superseded by an approved architecture decision. |

`Implemented` does not imply FAT or SAT approval. Hardware verification is recorded separately.

## 3. Source Register and Precedence

| Alias | Source | Role |
|---|---|---|
| FRS | [Volume 1, Chapter 4 - Functional Requirements](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%201%20%E2%80%93%20Product%20Definition%2C%20Functional%20Requirements%20Specification%20%28FRS%29%20%26%20Engineering%20Design%20Specification%20%28EDS%29/Chapter%204%20%E2%80%93%20Functional%20Requirements%20Specification%20%28FRS%29.md) | Primary functional requirements |
| HWB | [Volume 1, Chapter 6 - Hardware Baseline](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%201%20%E2%80%93%20Product%20Definition%2C%20Functional%20Requirements%20Specification%20%28FRS%29%20%26%20Engineering%20Design%20Specification%20%28EDS%29/Chapter%206%20%E2%80%93%20System%20Hardware%20Baseline%20%26%20Platform%20Specification.md) | Hardware requirement IDs |
| SAF | [Volume 1, Chapter 10 - Safety and Reliability](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%201%20%E2%80%93%20Product%20Definition%2C%20Functional%20Requirements%20Specification%20%28FRS%29%20%26%20Engineering%20Design%20Specification%20%28EDS%29/Chapter%2010%20%E2%80%93%20System%20Safety%2C%20Reliability%20%26%20Risk%20Management.md) | Safety objectives and validation |
| TASK | [Volume 2, Chapter 2 - FreeRTOS Task Architecture](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%202%20-%20Firmware%20Architecture/Chapter%202%20-%20FreeRTOS%20Task%20Architecture.md) | Task architecture principles |
| EVT | [Volume 2, Chapter 3 - Event Bus Architecture](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%202%20-%20Firmware%20Architecture/Chapter%203%20%E2%80%93%20Event%20Bus%2C%20Inter-Task%20Communication%20%26%20State%20Machine%20Architecture.md) | Event architecture principles |
| UIA | [Volume 2, Chapter 18 - UI Architecture](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%202%20-%20Firmware%20Architecture/Chapter%2018%20%E2%80%93%20User%20Interface%20%28LVGL%29%20Architecture.md) | UI architecture IDs |
| UID | [Volume 4, Chapter 10 - Widget Design System](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%204%20%E2%80%93%20Display%20Software%2C%20LVGL%20GUI%20%26%20Human%E2%80%93Machine%20Interface%20%28HMI%29%20Engineering%20Specification/Chapter%2010%20%E2%80%93%20Widget%20Library%2C%20Reusable%20UI%20Components%20%26%20Design%20System.md) | Duplicate `UI-001..005` design-system occurrence |
| UIT | [Volume 4, Chapter 17 - HMI Verification](Zmartify%20Master%20Engineering%20Package%20v5.0.md/Volume%204%20%E2%80%93%20Display%20Software%2C%20LVGL%20GUI%20%26%20Human%E2%80%93Machine%20Interface%20%28HMI%29%20Engineering%20Specification/Chapter%2017%20%E2%80%93%20HMI%20Testing%2C%20UI%20Verification%20%26%20Automated%20GUI%20Validation.md) | HMI test IDs |
| WSA | [Architecture v5.1 WaterSensor Addendum](ARCHITECTURE-v5.1-WATERSENSOR-ADDENDUM.md) | Supersedes direct PCNT/MCP9808 acquisition assumptions |

The MEP reuses `UI-001..005` for two different requirement sets. This RTM uses occurrence keys
`UIA-UI-xxx` and `UID-UI-xxx` while preserving the source ID.

`FR-XXX-001` is an example placeholder in the requirement-format section and is excluded.

## 4. Approved Scope Decisions

| Tracking key | Decision | Source | Status |
|---|---|---|---|
| SCOPE-RS485 | RS-485 and external expansion bus are not required for this product revision. | [Implementation Plan 2](IMPLEMENTATION-PLAN-2.md) | `Not Applicable` |
| SCOPE-CAN | Reserved CAN bus has no approved product use in this revision. | HWB 6.14 | `Not Applicable` |
| SCOPE-REST-WS | The full future REST/WebSocket interface is not part of the current product baseline; only explicitly documented local endpoints apply. | Volume 3, Chapter 19 | `Not Applicable` |
| SCOPE-WATERSENSOR | Flow pulse acquisition and normal temperature acquisition are delegated to the WaterSensor node. | WSA | Applicable, superseding baseline |

## 5. Functional Requirements

### 5.1 System and Irrigation

| ID | Condensed requirement | Status | Implementation evidence | Verification evidence | Plan step |
|---|---|---|---|---|---|
| FR-SYS-001 | Continuous operation through irrigation season | `Not Verified` | Runtime architecture and reconnect paths in [main.c](../main/main.c) | Long-term/seasonal test absent | 10 |
| FR-SYS-002 | Continue safely without Internet/weather services | `Partial` | Local scheduler; cached weather fallback in [weather_manager.c](../components/weather_manager/src/weather_manager.c) | Weather stale fallback host test; device offline soak absent | 10 |
| FR-SYS-003 | Safe startup, self-test, idle; never resume watering after power loss | `Partial` | Startup initializes managers and engine enters idle in [main.c](../main/main.c) | State-machine test; complete hardware self-test absent | 10 |
| FR-SYS-004 | Critical fault stops irrigation, closes master, logs and publishes alarm | `Partial` | Critical latch and stop path in [main.c](../main/main.c) | [Safety shutdown acceptance](../test/test_acceptance_safety_shutdown.c); immediate MQTT alarm not contract-tested | 7 |
| FR-IRR-001 | 15 zones plus one dedicated master valve | `Implemented` | Mapping in [relay_manager.h](../components/relay_manager/include/relay_manager.h) | [Irrigation engine test](../test/test_irrigation_engine.c) and relay script | - |
| FR-IRR-002 | Zone custom name, icon, colour and type | `Partial` | Name/config fields in [config_types.h](../components/config_manager/include/config_types.h) | Icon/colour/type coverage absent | 9 |
| FR-IRR-003 | Independently enabled zones never auto-run when disabled | `Implemented` | Scheduler checks zone enabled state in [main.c](../main/main.c) | Host scheduler-specific test still desirable | 10 |
| FR-IRR-004 | Manual zone start with configurable runtime | `Implemented` | Validated command path and runtime limits in [main.c](../main/main.c) | Engine sequencing test | - |
| FR-IRR-005 | Unlimited stored programs with names, starts, zones, days and seasonal factor | `Partial` | Program model and scheduler exist; limit is 8 programs in [config_types.h](../components/config_manager/include/config_types.h) | Scheduler acceptance coverage incomplete | 9 |
| FR-IRR-006 | Optional Cycle & Soak per zone | `Missing` | No cycle/soak state model | No test | 9 |
| FR-IRR-007 | Seasonal adjustment 0-300% in 1% increments | `Partial` | Program/zone seasonal factors exist but ranges differ from FRS | Weather tests cover bounded adjustment only | 4 |
| FR-IRR-008 | Persistent user rain delay for 1-30 days | `Partial` | Runtime rain-delay command exists | Persistence/range acceptance test absent | 8 |

### 5.2 Hydraulic and Weather

| ID | Condensed requirement | Status | Implementation evidence | Verification evidence | Plan step |
|---|---|---|---|---|---|
| FR-HYD-001 | Continuous active flow measurement, resolution better than 0.1 L/min | `Partial` | WaterSensor snapshots consumed by [watersensor_client.c](../components/watersensor_client/src/watersensor_client.c) | Protocol test; measurement resolution/calibration not verified | 10 |
| FR-HYD-002 | Continuous active pressure measurement detecting +/-0.1 bar deviations | `Partial` | ADS1115 pressure path, schema-v2 calibration and per-zone bounds in [config_types.h](../components/config_manager/include/config_types.h), and [pressure_manager.c](../components/pressure_manager/src/pressure_manager.c) | [Configuration boundary/migration tests](../test/test_config_validation.c) and safety rules pass; physical resolution/calibration absent | 4 |
| FR-HYD-003 | Learn per-zone average/min/max/stddev flow | `Missing` | Baseline comparison exists; statistical learning model does not | No learning test | 5 |
| FR-HYD-004 | Excess unexpected flow raises critical alarm, stops, logs and publishes | `Partial` | Configurable per-zone baseline/deviation supervision with global absolute clamp and critical shutdown | [Configuration tests](../test/test_config_validation.c) and safety rules pass; MQTT and physical fault injection incomplete | 5 |
| FR-HYD-005 | Pressure loss raises hydraulic fault and safely terminates irrigation | `Implemented` | Pressure collapse escalation and critical shutdown | Safety rules and cross-component acceptance test | - |
| FR-WEA-001 | Accept local temperature, humidity, wind, rain, solar and UV data | `Implemented` | Provider-neutral `/weather` snapshot and cache | Weather validation/cache tests | - |
| FR-WEA-002 | Optionally retrieve Internet forecast from pluggable providers | `Partial` | Provider-neutral ingestion boundary only | No controller-side provider/poll test | 7 |
| FR-WEA-003 | Consider rain, wind, temperature, humidity and ET before auto-watering | `Partial` | Scheduler weather skip and ET-adjusted runtime | Weather/ET tests; complete scheduler contract test absent | 10 |
| FR-WEA-004 | Do not auto-start under configured freeze conditions | `Partial` | Weather decision blocks at freezing temperature | Configurable threshold and scheduler acceptance test absent | 4 |

### 5.3 UI, MQTT, Alarms, Logs, Performance and Maintenance

| ID | Condensed requirement | Status | Implementation evidence | Verification evidence | Plan step |
|---|---|---|---|---|---|
| FR-UI-001 | Dashboard shows state, active zone, weather, flow, pressure, consumption and alarms | `Partial` | LVGL screens in [hmi_lvgl_port.c](../components/hmi_board/src/hmi_lvgl_port.c) | Formal screen acceptance absent | 9 |
| FR-UI-002 | Backlight off after 10 minutes; touch/button wakes immediately | `Partial` | Backlight and touch HAL exist | Timeout/wake acceptance absent | 9 |
| FR-UI-003 | Home/Wake, Manual Start, Stop and Emergency Stop buttons | `Missing` | Current hardware path is touch-centric; no four-button runtime contract | No test | 9 |
| FR-UI-004 | English and Danish with extensible translations | `Missing` | Hardcoded display text | No i18n test | 9 |
| FR-MQTT-001 | Publish operational state | `Partial` | State publication in [main.c](../main/main.c) | Broker contract/reconnect test absent | 7 |
| FR-MQTT-002 | Publish alarms immediately | `Partial` | Outcome/alarm publication paths exist | One-second delivery and QoS not verified | 7 |
| FR-MQTT-003 | Publish flow, pressure, consumption, weather and zone telemetry | `Partial` | Several v2 payloads exist | Full topic/schema compliance absent | 7 |
| FR-MQTT-004 | Subscribe to commands, configuration and OTA requests | `Partial` | Command/OTA subscriptions exist; configuration coverage incomplete | Broker contract tests absent | 7 |
| FR-ALM-001 | Information, warning and critical categories | `Implemented` | Severity model in [alarm_manager.h](../components/alarm_manager/include/alarm_manager.h) | Safety rules tests | - |
| FR-ALM-002 | Required critical catalog includes hydraulic, master, watchdog and emergency faults | `Partial` | Hydraulic codes exist | Master/watchdog/emergency catalog and injection tests incomplete | 8 |
| FR-ALM-003 | Critical alarms remain active until acknowledged | `Missing` | No acknowledged/resolved/cleared lifecycle | No lifecycle test | 8 |
| FR-LOG-001 | Persistent logs cover irrigation, alarms, weather, hydraulic, users and config | `Partial` | CRC NVS log with broad categories in [storage_manager.c](../components/storage_manager/src/storage_manager.c) | Storage tests; categories are less granular than FRS | 6 |
| FR-LOG-002 | Entry contains timestamp, source, event, severity and additional data | `Partial` | Current schema has timestamp, type and message | Schema lacks explicit source/severity/structured data | 6 |
| FR-PER-001 | Boot within 30 seconds | `Not Verified` | Boot path exists | Measured cold-boot report absent | 10 |
| FR-PER-002 | Touch response below 100 ms | `Not Verified` | Touch/LVGL runtime exists | Instrumented latency test absent | 9 |
| FR-PER-003 | MQTT alarm publication within 1 second | `Not Verified` | Alarm publication path exists | Timed broker integration test absent | 7 |
| FR-PER-004 | Flow updates every second or faster | `Not Verified` | Online WaterSensor poll target is 500 ms | Device timing/age evidence absent | 10 |
| FR-MNT-001 | Support OTA firmware updates | `Partial` | Signed direct and HTTPS command-triggered OTA paths with explicit states | Signature rejection automated; signed healthy OTA and automatic unhealthy-image rollback verified on ESP32-S3; power-interruption FAT pending | 3, 10 |
| FR-MNT-002 | Export complete controller configuration | `Missing` | Log export is not configuration export | No round-trip test | 8 |
| FR-MNT-003 | Restore controller configuration | `Missing` | Network update endpoint is not full restore | No migration/restore test | 8 |
| FR-MNT-004 | Hardware self-test available from service menu | `Partial` | Relay test script exists outside HMI | No service-menu self-test acceptance | 9 |

## 6. Hardware Requirements

Hardware requirements remain `Not Verified` until inspection, drawing, measurement, FAT or SAT
evidence is linked. Firmware support is noted but does not prove electrical compliance.

| ID | Condensed requirement | Status | Evidence or gap | Plan step |
|---|---|---|---|---|
| HW-001 | Industrial seasonal reliability | `Not Verified` | No seasonal endurance evidence | 10 |
| HW-002 | Modular commercially replaceable Rev.B design | `Not Verified` | Wiring/BOM documentation exists; inspection record absent | 10 |
| HW-003 | Physical/electrical separation of mains, 24 VAC, logic and sensors | `Not Verified` | Requires cabinet/electrical inspection | 10 |
| HW-004 | Field-replaceable modules | `Not Verified` | Requires mechanical inspection/FAT | 10 |
| HW-CPU-001 | Waveshare ESP32-S3 7-inch 1024x600 controller | `Not Verified` | Firmware targets ESP32-S3 and 1024x600; approved BOM inspection absent | 10 |
| HW-CPU-002 | ESP32 owns control, HMI, MQTT, OTA, sensors, events, logs, weather | `Partial` | Runtime performs these roles; some diagnostics/HMI scope incomplete | 6 |
| HW-IO-001 | MCP23017 provides 16 I2C outputs | `Implemented` | HAL and relay manager integrated; live relay mapping previously exercised | 10 |
| HW-IO-002 | Relay 0 master, relay 1-15 zones | `Implemented` | Mapping constants and controlled relay test | 10 |
| HW-IO-003 | MCP23017 interrupts available for future revisions | `Not Verified` | PCB/pin availability inspection absent | 10 |
| HW-DRV-001 | ULN2803A approved relay driver | `Not Verified` | Physical BOM/electrical inspection required | 10 |
| HW-DRV-002 | Driver sinks current and protects outputs | `Not Verified` | Electrical design/FAT evidence absent | 10 |
| HW-RLY-001 | Two HL-58S modules, 16 channels | `Not Verified` | Physical BOM inspection required | 10 |
| HW-RLY-002 | 5 V, optoisolated, active-low relay behaviour | `Partial` | Firmware active-low behaviour implemented; electrical characteristics unverified | 10 |
| HW-RLY-003 | Permanent master/zone allocation | `Implemented` | Mapping enforced in relay manager and tests | 10 |
| HW-PSU-001 | Hager ST315 24 VAC transformer | `Not Verified` | Physical BOM/FAT absent | 10 |
| HW-PSU-002 | Capacity for master plus three valves with >20% margin | `Not Verified` | Load calculation and measurement absent | 10 |
| HW-PSU-003 | Optional transformer disconnect provision | `Not Verified` | Hardware drawing inspection absent | 10 |
| HW-PSU-004 | Mean Well HDR-30-5 logic supply | `Not Verified` | Physical BOM/FAT absent | 10 |
| HW-PSU-005 | 5 V supply powers listed devices with expansion margin | `Not Verified` | Rail/load measurement absent | 10 |
| HW-FLOW-001 | DN50 G2 flow sensor, 10-200 L/min | `Not Verified` | WaterSensor-connected physical sensor/calibration evidence absent | 10 |
| HW-FLOW-002 | Instant, average, daily, monthly and lifetime consumption | `Partial` | WaterSensor provides instant/monotonic totals; aggregation scope incomplete | 7 |
| HW-FLOW-003 | Flow pulses connect directly to ESP32-S3 PCNT | `Not Applicable` | Superseded by WaterSensor v5.1 addendum | - |
| HW-PRES-001 | 0-10 bar, 0.5-4.5 V pressure transmitter | `Not Verified` | Physical sensor/calibration evidence absent | 10 |
| HW-PRES-002 | ADS1115 16-bit pressure ADC | `Partial` | Local HAL integration exists; repeatability/calibration unverified | 10 |
| HW-CAB-001 | MCP9808 cabinet temperature with 45/55 C alarms | `Not Applicable` | Direct MCP9808 assumption superseded; role-based WaterSensor temperature applies | - |
| HW-CAB-002 | Door switch records service/tamper access | `Missing` | No active door-switch acquisition/alarm path | 6 |
| HW-MON-001 | Continuously monitor 24 VAC and raise critical alarm on loss | `Missing` | No active 24 VAC monitor path | 5 |

## 7. Safety Objectives

| ID | Condensed requirement | Status | Implementation/verification evidence | Plan step |
|---|---|---|---|---|
| SAF-001 | Prevent uncontrolled water flow | `Partial` | Master/zone sequencing and hydraulic shutdown tested; physical faults not fully injected | 5 |
| SAF-002 | Prevent flooding from valve or pipe failure | `Partial` | Excess-flow/pressure policies exist; stuck-open diagnostics incomplete | 5 |
| SAF-003 | Protect irrigation equipment under abnormal operation | `Partial` | Runtime limits and pressure/flow alarms; electrical/power monitoring incomplete | 5 |
| SAF-004 | Protect users through mains/SELV separation | `Not Verified` | Requires electrical inspection and FAT | 10 |
| SAF-005 | Preserve complete history after safety incidents | `Partial` | CRC alarm/event persistence; schema/capacity do not prove complete incident history | 6 |
| SAF-006 | Permit rapid diagnosis and recovery | `Partial` | Logs and basic HMI exist; integrated diagnostics/service workflow incomplete | 6 |

## 8. Firmware Architecture Requirements

| Occurrence key | Source ID | Condensed requirement | Status | Evidence or gap | Plan step |
|---|---|---|---|---|---|
| TASK-FW-TASK-001 | FW-TASK-001 | Hydraulic safety tasks outrank UI/communications | `Partial` | Priorities exist; scheduling/latency not measured | 10 |
| TASK-FW-TASK-002 | FW-TASK-002 | Long operations use short non-blocking state-machine steps | `Implemented` | Irrigation engine is non-blocking and unit-tested | - |
| TASK-FW-TASK-003 | FW-TASK-003 | No busy waiting; use RTOS primitives | `Partial` | Main paths use queues/delays; complete static audit not automated | 10 |
| TASK-FW-TASK-004 | FW-TASK-004 | Predictable execution; avoid cyclic dynamic allocation | `Partial` | Safety paths are bounded; timing and allocation audit incomplete | 10 |
| TASK-FW-TASK-005 | FW-TASK-005 | Lower-priority loss cannot compromise hydraulic safety | `Partial` | Local safety is broker-independent; failure-injection breadth incomplete | 10 |
| EVT-FW-EVT-001 | FW-EVT-001 | Events represent facts, not commands | `Partial` | Event model generally factual; runtime still contains direct orchestration | 6 |
| EVT-FW-EVT-002 | FW-EVT-002 | Published events are immutable | `Implemented` | Event bus copies messages; component tests exist | - |
| EVT-FW-EVT-003 | FW-EVT-003 | Publishers do not know receivers | `Implemented` | Publish API and event-bus tests | - |
| EVT-FW-EVT-004 | FW-EVT-004 | Subscribers do not know publishers | `Implemented` | Subscription API and tests | - |
| EVT-FW-EVT-005 | FW-EVT-005 | Event bus contains no business logic | `Implemented` | Bus only queues/dispatches; tests cover behaviour | - |

## 9. UI Architecture and HMI Verification

The duplicated `UI-001..005` IDs are tracked by occurrence key.

| Occurrence key | Source ID | Condensed requirement | Status | Evidence or gap | Plan step |
|---|---|---|---|---|---|
| UIA-UI-001 | UI-001 | Routine tasks need no more than three touches | `Not Verified` | No navigation acceptance measurements | 9 |
| UIA-UI-002 | UI-002 | Frequent data visible; configuration accessible without clutter | `Partial` | Dashboard exists; workflow review incomplete | 9 |
| UIA-UI-003 | UI-003 | Capacitive-touch-first architecture | `Implemented` | GT911/LVGL touch integration active | 9 |
| UIA-UI-004 | UI-004 | Screen updates arrive through Event Bus, not module polling | `Partial` | HMI receives runtime data, but strict event-only binding not demonstrated | 9 |
| UIA-UI-005 | UI-005 | UI never directly drives hardware | `Partial` | Relay HAL ownership is separate; command-path architecture needs formal test | 9 |
| UID-UI-001 | UI-001 | Identical functions appear consistently | `Not Verified` | Design review/screenshots absent | 9 |
| UID-UI-002 | UI-002 | Compose screens from reusable widgets | `Partial` | Some shared styles; largely monolithic LVGL file | 9 |
| UID-UI-003 | UI-003 | Each widget has one responsibility | `Not Verified` | No widget architecture review | 9 |
| UID-UI-004 | UI-004 | Widgets display data without business logic | `Partial` | Rendering and callbacks coexist in one implementation file | 9 |
| UID-UI-005 | UI-005 | Widget behaviour is theme-independent | `Missing` | No theme engine/verification | 9 |
| UIT-TEST-001 | TEST-001 | Every screen has acceptance criteria | `Missing` | No screen acceptance catalogue | 9 |
| UIT-TEST-002 | TEST-002 | UI tests are repeatable | `Missing` | No active UI test harness | 9 |
| UIT-TEST-003 | TEST-003 | Automate UI tests wherever practical | `Missing` | CI contains host logic tests only | 9 |
| UIT-TEST-004 | TEST-004 | Measure graphical performance | `Missing` | No FPS/render/touch instrumentation evidence | 9 |
| UIT-TEST-005 | TEST-005 | Released UI features remain regression-tested | `Missing` | No screenshot or simulator regression suite | 9 |

## 10. Additional Explicit Design, System and Interface IDs

These IDs occur outside the primary FRS/HW/SAF tables. They are grouped only where one source
chapter defines a uniform concern and all listed IDs share the same current status and Plan 2
destination. Each source ID is written explicitly to permit machine coverage checks.

| Domain/source | Source IDs | Applicability/status | Current evidence or gap | Plan step |
|---|---|---|---|---|
| Volume 1, Chapter 3 use cases | UC-001, UC-002, UC-003, UC-004, UC-005, UC-006, UC-007, UC-008 | Applicable design input; `Partial` | Core homeowner/manual/automatic use cases exist; installer, maintenance and complete fault workflows need acceptance evidence | 9, 10 |
| Volume 1, Chapter 5 EDS | EDS-001, EDS-002, EDS-003, EDS-004, EDS-005 | `Partial` | Modular/event-driven/fail-safe architecture exists, but integration and security gaps remain | 2, 6, 7 |
| Volume 1, Chapter 7 software baseline | SW-001, SW-002, SW-003, SW-004, SW-005 | `Partial` | ESP-IDF/FreeRTOS/LVGL/MQTT/NVS baseline exists; complete quality and architecture verification is open | 6, 10 |
| Volume 1, Chapter 9 availability/performance | SYS-PER-001, SYS-PER-002, SYS-PER-003, SYS-PER-010, SYS-PER-011 | `Not Verified` | Availability, recovery and timing targets lack measured evidence | 10 |
| Volume 1, Chapter 9 UI | SYS-UI-001, SYS-UI-002, SYS-UI-003, SYS-UI-004 | `Partial` | Display/touch/HMI exist; timeout, localization and workflow requirements incomplete | 9 |
| Volume 1, Chapter 9 irrigation | SYS-IRR-001, SYS-IRR-002, SYS-IRR-003, SYS-IRR-004, SYS-IRR-005, SYS-IRR-006 | `Partial` | Core manual/automatic control exists; full program/cycle/soak scope and field proof incomplete | 4, 9, 10 |
| Volume 1, Chapter 9 hydraulic | SYS-HYD-001, SYS-HYD-002, SYS-HYD-003, SYS-HYD-004, SYS-HYD-005 | `Partial` | Flow/pressure supervision exists; learning, calibration and physical fault evidence incomplete | 4, 5, 10 |
| Volume 1, Chapter 9 weather | SYS-WEA-001, SYS-WEA-002, SYS-WEA-003 | `Partial` | Weather cache and ET policy exist; provider and site verification incomplete | 7, 10 |
| Volume 1, Chapter 9 MQTT | SYS-MQTT-001, SYS-MQTT-002, SYS-MQTT-003, SYS-MQTT-004, SYS-MQTT-005 | `Partial` | Live transport/subscriptions exist; schema, QoS, duplicate and recovery contract tests incomplete | 7 |
| Volume 1, Chapter 9 storage | SYS-STO-001, SYS-STO-002 | `Partial` | CRC event/weather persistence exists; complete retention/capacity requirements not proven | 6, 10 |
| Volume 2, Chapter 4 HAL | HAL-001, HAL-002, HAL-003, HAL-004, HAL-005, HAL-006 | `Partial` | HAL boundaries exist for active hardware; API and hardware-verification breadth incomplete | 5, 10 |
| Volume 2, Chapter 7 relay architecture | REL-001, REL-002, REL-003, REL-004, REL-005, REL-006 | `Partial` | Exclusive relay ownership and interlocks exist; electrical/response diagnostics incomplete | 5 |
| Volume 2, Chapter 13 MQTT architecture | MQTT-001, MQTT-002, MQTT-003, MQTT-004 | `Partial` | MQTT transport and v2 message subset exist; full architectural contract is open | 7 |
| Volume 2/3 security architecture | SEC-001, SEC-002, SEC-003, SEC-004, SEC-005, SEC-006 | `Partial` | Signed OTA and MQTT TLS exist; HTTP auth, authorization, replay controls, hardware secure boot and flash encryption remain open | 2, 3, 7 |
| Volume 3, Chapter 1 communications | COM-001, COM-002, COM-003, COM-004, COM-005 | `Partial` | MQTT-first integration exists; reliability/security/compliance evidence incomplete | 7 |
| Volume 3, Chapter 20 API compliance | API-001, API-002, API-003, API-004, API-005, API-006, API-007, API-008, API-009, API-010 | `Partial` | Supported subset is not yet closed by a broker-backed compliance matrix | 7 |
| Volume 4, Chapter 1 HMI principles | HMI-001, HMI-002, HMI-003, HMI-004, HMI-005 | `Partial` | Operational LVGL UI exists; full service/safety/usability evidence incomplete | 9 |
| Volume 4, Chapter 3 UX/navigation | UX-001, UX-002, UX-003, UX-004 | `Partial` | Multi-screen navigation exists; hierarchy and workflow acceptance are not measured | 9 |
| Volume 4, Chapter 8 configuration UI | CFG-001, CFG-002, CFG-003, CFG-004 | `Partial` | Configuration model exists; protected complete local administration workflow is incomplete | 2, 9 |
| Volume 4, Chapter 9 diagnostics UI | DIA-001, DIA-002, DIA-003, DIA-004 | `Partial` | Basic status screens/logs exist; authoritative diagnostics and service workflow are incomplete | 6, 9 |
| Volume 4, Chapter 11 themes | THM-001, THM-002, THM-003, THM-004 | `Missing` | No theme engine or responsive theme verification | 9 |
| Volume 4, Chapter 12 input | INP-001, INP-002, INP-003, INP-004 | `Partial` | GT911 touch works; gesture/filter/focus/input abstraction coverage incomplete | 9 |
| Volume 4, Chapter 13 languages | LNG-001, LNG-002, LNG-003, LNG-004, LNG-005 | `Missing` | Display strings are hardcoded; no translation catalogue/runtime selection | 9 |
| Volume 4, Chapter 14 animation | ANI-001, ANI-002, ANI-003, ANI-004, ANI-005 | `Missing` | No managed animation/transition framework or performance contract | 9 |
| Volume 4, Chapter 15 GUI performance | PERF-001, PERF-002, PERF-003, PERF-004, PERF-005 | `Not Verified` | No instrumented render, memory, FPS or responsiveness evidence | 9, 10 |

## 11. ID-less Normative Requirement Register

Many MEP chapters use normative `shall` statements without unique IDs. They remain applicable
through the following stable chapter-level keys until the source documents assign granular IDs.

| Tracking key | Normative domain | Applicability/status | Current evidence or gap | Plan step |
|---|---|---|---|---|
| MEP-V1-C5-ARCH | Layering, fail-safe architecture and subsystem ownership | `Partial` | Components exist; ownership drift remains in main/MQTT/diagnostics | 6, 7 |
| MEP-V1-C7-SW | ESP-IDF, coding, task, storage and software baseline | `Partial` | Build and modules exist; several architecture promises incomplete | 6, 8 |
| MEP-V1-C8-CONOPS | Auto/manual/service/fault operation and recovery | `Partial` | Auto/manual/fault paths exist; service mode incomplete | 9 |
| MEP-V1-C9-PERF | Availability, timing, storage, sensor, network and reliability targets | `Not Verified` | No consolidated performance/endurance evidence | 10 |
| MEP-V1-C10-SAFETY | Watchdogs, independent safety layers, recovery and incident handling | `Partial` | Task WDT and automatic OTA rollback verified; power/door/24 VAC gaps remain | 3, 5, 6 |
| MEP-V1-C11-VV | Full traceability, unit/integration/FAT/SAT/long-term verification | `Partial` | This RTM and host CI exist; FAT/SAT/long-term absent | 10 |
| MEP-V2-C4-HAL | HAL isolation and required peripheral services | `Partial` | I2C/relay/time/storage/sensors exist; hardware verification incomplete | 10 |
| MEP-V2-C5-IRR | Irrigation engine sequencing, authorization and safe cancellation | `Partial` | Core sequencing implemented; cycle/soak and full authorization incomplete | 2, 9 |
| MEP-V2-C6-ZONE | Zone authority, metadata, statistics and interfaces | `Partial` | Basic state/config exists; richer metadata/statistics incomplete | 9 |
| MEP-V2-C7-RELAY | Relay interlocks, diagnostics and electrical feedback | `Partial` | Exclusive actuation exists; physical feedback diagnostics missing | 5 |
| MEP-V2-C8-WEATHER | Provider, cache, decisions, events and diagnostics | `Partial` | Cache/policy exists; provider polling/events/diagnostics incomplete | 6, 7 |
| MEP-V2-C9-ET | Reference/crop ET, failure handling and diagnostics | `Partial` | FAO-56 daily model and tests; site calibration/event integration incomplete | 6, 10 |
| MEP-V2-C10-FLOW | Learning, anomaly policy and predictive diagnostics | `Partial` | Anomaly supervision exists; learning/predictive diagnostics missing | 5 |
| MEP-V2-C11-PRESS | Pressure acquisition, baseline, faults and diagnostics | `Partial` | Fault supervision exists; baseline learning/calibration incomplete | 4, 5 |
| MEP-V2-C12-ALARM | Full alarm model, lifecycle, actions and history | `Partial` | Severity/active state exists; acknowledge/resolve lifecycle missing | 8 |
| MEP-V2-C13-MQTT | TLS, topics, QoS, validation, reconnect and discovery | `Partial` | TLS transport/commands exist; full contract tests and discovery incomplete | 7 |
| MEP-V2-C14-CONFIG | Versioned schema, validation, migration and audit events | `Partial` | CRC/version/config exists; migration resets instead of converting | 4, 8 |
| MEP-V2-C15-STORAGE | Typed persistent data, retention, integrity and diagnostics | `Partial` | Event/weather persistence exists; full data model/backups incomplete | 6, 8 |
| MEP-V2-C16-OTA | Trusted download, validation, health confirmation and rollback | `Partial` | RSA-signed updates, HTTPS remote policy, live health confirmation, rollback guard and persistent audit integrated | Physical rollback, interruption and production-key FAT pending | 3, 10 |
| MEP-V2-C17-DIAG | Authoritative system health, task/resource/event/sensor metrics | `Partial` | Component exists but is not initialized from main and uses legacy/null state | 6 |
| MEP-V2-C19-SEC | Authentication, authorization, TLS, secure boot and flash encryption | `Missing` | MQTT TLS exists; HTTP auth, secure boot and flash encryption absent | 2, 3 |
| MEP-V3-MQTT-API | Namespace, schemas, transaction outcomes, QoS and integrations | `Partial` | v2 subset exists; compliance matrix and broker tests absent | 7 |
| MEP-V3-C18-SEC | MQTT/API authentication, authorization and hardening | `Partial` | Credentials/TLS available; role enforcement/replay protection incomplete | 2, 7 |
| MEP-V3-C19-REST | Future full REST/WebSocket API | `Not Applicable` | Explicit future interface, excluded from current baseline | - |
| MEP-V4-HMI | Screens, navigation, themes, i18n, service and data binding | `Partial` | Working LVGL UI; architecture and workflow breadth incomplete | 9 |
| MEP-V4-C17-VV | Automated GUI validation and performance evidence | `Missing` | No active GUI automation | 9 |
| MEP-V5-C1-C6-HW | CPU, power, display, valve driver and sensor electrical design | `Not Verified` | Firmware/wiring evidence only; PCB/electrical FAT absent | 10 |
| MEP-V5-C7-RS485 | RS-485 and expansion bus | `Not Applicable` | Approved product scope decision | - |
| MEP-V5-MFG | Manufacturing, inspection, burn-in and acceptance evidence | `Missing` | Formal production test package absent | 10 |
| ADD-V5.1-WATERSENSOR | Versioned I2C sensor acquisition with validity/age and safe loss policy | `Partial` | Client/protocol/fallback implemented; identity/config/fault policy incomplete | 4, 5, 6 |

## 12. Document Coverage Register

All 77 markdown chapters under the MEP folder were inventoried. Applicability is controlled at
volume/chapter level below; granular explicit IDs are tracked above.

| Volume | Chapters | Applicability decision |
|---|---|---|
| Volume 1 - Product/FRS/EDS | Chapters 1-11 | Applicable; Chapter 11 process is implemented by this living RTM. |
| Volume 2 - Firmware Architecture | Chapters 1-21 | Applicable except future roadmap items that receive an explicit scope decision. |
| Volume 3 - MQTT/API Integration | Chapters 1-18 and 20 applicable; Chapter 19 future REST/WebSocket excluded | MQTT subset must be explicitly closed in Plan 2 Step 7. |
| Volume 4 - HMI | Chapters 1-18 | Applicable to the 1024x600 Waveshare 7B baseline; optional themes/languages may be formally scoped in Step 9. |
| Volume 5 - Hardware | Chapters 1-6 applicable; Chapter 7 RS-485/expansion excluded | Hardware claims require inspection/FAT/SAT evidence. |

## 13. Current Summary

Counts below use requirement occurrences, so duplicated `UI-001..005` are counted separately.
Chapter-level rows are not included in these counts.

| Status | Count |
|---|---:|
| Implemented | 15 |
| Partial | 144 |
| Missing | 29 |
| Not Verified | 35 |
| Not Applicable | 2 |
| **Total explicit occurrences** | **225** |

The principal release blockers are:

1. unauthenticated HTTP control and OTA endpoints;
2. OTA rollback bootloader provisioning and physical failure/interruption evidence;
3. installed per-zone hydraulic commissioning and physical threshold/fault-injection evidence;
4. missing physical valve/relay response diagnostics;
5. disconnected/inaccurate system diagnostics;
6. incomplete MQTT schema, QoS, replay and outcome contract;
7. no formal FAT/SAT, performance or long-duration evidence.

## 14. Update Procedure

For every implementation-plan step:

1. identify affected RTM rows before code changes;
2. keep status unchanged until implementation and stated verification evidence exist;
3. add links to code, tests and hardware records;
4. record scope changes under Approved Scope Decisions;
5. update summary counts;
6. commit RTM changes with the implementation step.

A requirement may move to `Implemented` while hardware verification remains open only when the
requirement is purely software and its acceptance method is automated. Physical, timing,
reliability and safety claims requiring equipment remain `Not Verified` until measured evidence
is stored or linked.

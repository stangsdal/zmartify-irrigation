# Implementation Plan 2 - Product Maturation Against MEP v5.0

**Project:** Zmartify Irrigation Controller
**Baseline:** Zmartify Master Engineering Package v5.0
**Hardware:** ESP32-S3, Waveshare 7B display, MCP23017 relay board
**Plan status:** Steps 1-10 implemented; field release BLOCKED pending FAT/SAT and open deviations
**Created:** 2026-07-19

## 1. Purpose

Implementation Plan 1 delivered the core controller: relay sequencing, automatic scheduling,
hydraulic supervision, persistent logging, weather/ET adjustment and a host test harness.

Implementation Plan 2 closes the most important gaps between that working controller and the
production expectations in the Master Engineering Package (MEP) v5.0. Work is ordered by
safety and operational risk rather than document chapter order.

Each step shall:

- map its scope to relevant MEP requirements;
- preserve safe irrigation fallback behaviour;
- include focused automated tests where practical;
- pass the complete host suite and ESP-IDF build;
- include hardware or OTA validation when runtime behaviour changes;
- be committed independently for traceability.

## 2. Approved Scope Deviation

### RS-485 and expansion bus

RS-485 and the external expansion bus described in MEP v5.0 are **out of scope** for this
product revision. No RS-485 HAL, protocol, UI, diagnostics or acceptance testing will be
implemented.

This is an intentional product decision, not an open implementation gap. Related MEP
requirements shall be marked `Not Applicable - product scope decision` in the traceability
matrix. Existing unused pin definitions may remain until a later hardware cleanup, but no new
code shall depend on them.

## 3. Starting Point

The following capabilities are already operational:

- non-blocking master-valve and zone relay sequencing;
- automatic program scheduling after time synchronization;
- WaterSensor and optional pressure-sensor integration;
- time-qualified flow and pressure alarms with critical shutdown;
- CRC-protected persistent operational logging and HTTP export;
- provider-neutral weather ingestion, 24-hour cache and ET runtime adjustment;
- direct HTTP OTA and runtime MQTT configuration;
- eight host-side unit/acceptance tests running through CTest and CI.

Known production gaps include unauthenticated HTTP control endpoints, incomplete OTA trust and
rollback, hardcoded safety parameters, no physical relay feedback, partially disconnected
diagnostics, incomplete MQTT contract compliance, limited HMI architecture and missing formal
FAT/SAT evidence.

## 4. Delivery Steps

### Step 1 - Living Requirements Traceability Matrix

**Objective:** Establish a controlled baseline from MEP requirement to implementation and test
evidence before further product changes.

**Activities:**

- Extract applicable requirement IDs from Volumes 1-5.
- Create a machine-readable or tabular RTM with:
  - requirement ID and source chapter;
  - applicability and rationale;
  - implementation evidence;
  - verification method;
  - status: `Implemented`, `Partial`, `Missing`, `Not Applicable` or `Not Verified`.
- Record RS-485 and expansion requirements as `Not Applicable`.
- Record known hardware/document differences, including the active 1024x600 display.
- Link existing tests and operational documentation to their requirements.

**Acceptance criteria:**

- Every normative requirement has an applicability decision.
- Every `Implemented` requirement has code and verification evidence.
- No directory or component name alone counts as implementation evidence.
- Open requirements can be filtered by safety, security, firmware, HMI and verification domain.

**Deliverable:** `docs/REQUIREMENTS-TRACEABILITY-v5.0.md` or equivalent structured export.

**Commit:** `Plan 2 Step 1: Establish MEP requirements traceability`

---

### Step 2 - HTTP Control-Plane Authentication and Authorization

**Objective:** Prevent unauthorized OTA, configuration and weather changes on the local network.

**Activities:**

- Define roles and permissions for read-only diagnostics and administrative control.
- Add authentication to `/ota`, `/config/network` and `/weather`.
- Decide whether `/logs` is authenticated or exposes a deliberately reduced public view.
- Store credentials or credential verifiers without plaintext disclosure through logs or APIs.
- Add request-size, content-type, rate-limit and authentication-failure handling.
- Log security-relevant failures without logging secrets.
- Provide a secure initial provisioning and credential rotation procedure.

**Acceptance criteria:**

- Unauthenticated requests cannot upload firmware or change configuration.
- Invalid and expired credentials are rejected deterministically.
- Read-only and administrator permissions are tested separately.
- Authentication secrets never appear in logs, MQTT payloads or HTTP responses.
- Existing OTA tooling supports the selected authentication mechanism.

**Commit:** `Plan 2 Step 2: Secure HTTP control endpoints`

---

### Step 3 - OTA Trust, Validation and Rollback

**Objective:** Ensure only trusted firmware becomes permanent and recover automatically from a
bad OTA image.

**Activities:**

- Define the firmware-signing and key-management process.
- Require trusted TLS or signed-image validation for every OTA path.
- Remove any OTA path that permits an unverified remote image.
- Enable and integrate ESP-IDF application rollback.
- Repair and integrate Diagnostics Manager's post-boot health confirmation.
- Add explicit OTA states and persistent audit events.
- Document a staged secure-boot and flash-encryption provisioning procedure.

**Acceptance criteria:**

- Corrupt and unsigned/untrusted images are rejected.
- A healthy image is marked valid only after required subsystem checks.
- A deliberately unhealthy test image rolls back automatically.
- Power interruption during OTA leaves a bootable previous image.
- Secure-boot and flash-encryption enablement is performed only through an approved,
  recoverable manufacturing procedure; irreversible eFuse operations are never automated by
  ordinary development scripts.

**Hardware evidence (2026-07-19):**

- The rollback-enabled bootloader, two-slot partition table and signed baseline were provisioned
  on ESP32-S3 device `3c:0f:02:c9:8c:0c` through the physical service interface.
- A signed healthy OTA booted from the alternate slot, inhibited irrigation while unconfirmed
  and was marked valid after the 30-second health window.
- A signed fault-injection image entered `pending_confirmation`, failed its health check, was
  marked invalid and automatically returned to the previously verified slot.
- Persistent audit events and the restored HTTP service were verified after rollback. Controlled
  power-interruption trials remain pending under Step 10 FAT.

**Commit:** `Plan 2 Step 3: Enforce trusted OTA and rollback`

---

### Step 4 - Configurable Hydraulic Safety and Commissioning

**Objective:** Replace deployment-specific hardcoded hydraulic values with validated,
auditable configuration.

**Activities:**

- Inventory every flow, pressure, timing, freshness and runtime safety constant.
- Add versioned configuration fields with conservative defaults and range validation.
- Reject internally inconsistent or unsafe parameter sets.
- Add commissioning flow for baseline flow and pressure observations.
- Log configuration changes with old/new value metadata where safe.
- Preserve fail-safe behaviour when configuration is missing or corrupt.

**Acceptance criteria:**

- Runtime supervision consumes authoritative configuration rather than hardcoded site values.
- Unsafe thresholds cannot be committed.
- Corrupt or incompatible configuration falls back to documented safe behaviour.
- Boundary, migration and failure-path tests cover all safety fields.
- Commissioned values are verified on the installed hydraulic system.

**Implementation and verification (2026-07-20):**

- Configuration schema v3 owns all active hydraulic supervision values. Schema v1 and v2 are
  migrated in place and immediately persisted; corrupt, incompatible or unsafe blobs use factory
  defaults.
- Runtime and commit paths validate calibration, threshold ordering, timing/freshness bounds,
  runtime limits, per-zone deviation limits and the effective zone/global flow envelope.
- Zone commissioning records observed stable flow and a pressure envelope of +/-20 percent via
  `config_commission_zone()`. The change is auditable in the configuration log and is not durable
  until normal configuration commit succeeds.
- Runtime applies the active zone's baseline, deviation percentages and pressure envelope at zone
  start. Global absolute flow limits remain a final safety clamp.
- Host tests cover boundaries, invalid cross-field combinations, commissioning and schema-v1
  migration. The signed ESP-IDF image builds successfully.
- The signed image was OTA-deployed to the ESP32-S3 at `192.168.10.113`; the automatic health
  gate recorded `ota image confirmed valid`, and the read-only log service remained available.

**Hydraulic safety inventory and schema-v2 defaults:**

| Parameter | Default | Accepted range / relation |
|---|---:|---|
| Pressure calibration slope | 400.0 mV/bar | finite and positive |
| Pressure calibration offset | 500.0 mV | finite and non-negative |
| Global pressure limits | 0.5-7.0 bar | low < high, high <= 10.0 bar |
| Zone pressure envelope | 1.0-5.0 bar | low < high, high <= 10.0 bar |
| Pressure critical duration | 5 s | 1-300 s |
| Global flow limits | 2.0-200.0 L/min | low < high, high <= 200.0 L/min |
| Zone flow baseline | 12.0 L/min | 0.1-200.0 L/min |
| Flow warning/critical deviation | 15% / 30% | 0 < warning < critical <= 100% |
| No-flow/high-flow duration | 30 s / 10 s | 1-3600 s / 1-300 s |
| Active/idle flow freshness | 1500 / 5000 ms | active 100-10000 ms; idle >= active and <= 60000 ms |
| Valve open/close response timeout | 30 / 10 s | each 1-300 s |
| Zone/global maximum runtime | 3600 / 7200 s | positive; default <= zone max <= global max |

**Commissioning procedure:**

1. Confirm valve mapping, leak-free pipework, WaterSensor availability and calibrated pressure input.
2. Start one zone under controlled observation and wait for stable flow and pressure.
3. Record the stable observations through `config_commission_zone()` and review the old/new log.
4. Commit configuration, restart the zone and verify warning/critical behaviour using bounded
   hydraulic fault injection.
5. Repeat for every enabled zone; retain the results as SAT evidence.

Physical commissioning remains pending because the current device reports WaterSensor offline and
ADS1115 pressure unavailable. No valves were activated solely for this implementation step.

**Commit:** `Plan 2 Step 4: Externalize hydraulic safety configuration`

---

### Step 5 - Relay and Valve Diagnostics

**Objective:** Detect disagreement between commanded actuation and actual hydraulic/electrical
behaviour.

**Activities:**

- Document available physical feedback on the current hardware.
- If current or contact feedback is available, add HAL acquisition and validation.
- Otherwise implement bounded valve diagnostics using flow/pressure response correlation.
- Detect no-response, unexpected-flow, likely stuck-open and likely stuck-closed conditions.
- Avoid declaring a physical relay fault from output shadow state alone.
- Add alarm classification, persistence and safe shutdown policy.

**Acceptance criteria:**

- Fault injection distinguishes command success from observed response.
- Stuck-open/no-response scenarios produce the documented alarm and safe action.
- Sensor-unavailable states are reported as unavailable, not as a confirmed relay fault.
- Hardware tests cover all 16 outputs without unsafe simultaneous activation.

**Implementation and verification (2026-07-19):**

- The installed HL-58S/MCP23017/ULN2803A chain provides command output only. It has no relay
  contact feedback, valve position feedback or solenoid current sensing; MEP Volume 5 identifies
  current monitoring as a future hardware capability. Relay Manager shadow state is therefore
  treated only as command state and never as proof of physical actuation.
- `valve_diagnostics` correlates engine phase transitions with fresh WaterSensor flow and local
  pressure observations. Schema v3 provides dedicated validated opening/closing response times;
  global minimum flow and pressure remain the response thresholds.
- Opening is confirmed only by fresh flow. Fresh zero flow with adequate pressure is classified
  `likely stuck closed`; fresh zero flow without adequate pressure is the less-specific
  `no response`. Missing fresh flow is `sensor unavailable`, even when pressure is present.
- Flow that persists after close or appears while idle is time-qualified as `likely stuck open`.
  Pressure alone is never used to prove closure or a stuck-open valve.
- `no response`, `likely stuck closed` and `likely stuck open` raise distinct critical alarms,
  persist an alarm event and use the existing critical hydraulic shutdown/lockout path.
  `sensor unavailable` raises a warning and does not claim a physical relay or valve fault.
- Critical valve alarms intentionally remain latched with the safety lockout. Automatic clearing
  after apparent sensor recovery would erase incident state; acknowledge/resolve/clear semantics
  remain owned by Step 8 alarm lifecycle work.
- Host fault injection covers normal opening/closing, timeout boundaries, unavailable/recovery,
  generic no-response, pressure-correlated likely stuck-closed, post-close flow and idle flow.
- The isolated relay service firmware covers outputs 0-15 sequentially. It forces all outputs OFF
  before, between and after 500 ms pulses and requires disconnected loads plus explicit operator
  confirmation. It does not constitute electrical contact/current feedback.

The signed normal firmware builds successfully and was OTA-deployed to the ESP32-S3 at
`192.168.10.113`. Schema v2 migrated to v3, the read-only log service remained available and the
automatic health gate recorded `ota image confirmed valid`. Physical hydraulic fault injection
remains pending until WaterSensor and ADS1115 pressure acquisition are available. The relay service
test was not run against connected valves during this step.

**Commit:** `Plan 2 Step 5: Add relay and valve diagnostics`

---

### Step 6 - Integrated Diagnostics and System Health

**Objective:** Make Diagnostics Manager authoritative, accurate and visible in normal runtime.

**Activities:**

- Replace legacy/global alarm and null storage queries with injected live managers.
- Initialize Diagnostics Manager from application startup.
- Track uptime, reset reason, heap, task stack margins, event drops, sensor availability,
  MQTT state, time synchronization, storage health and active critical alarms.
- Define subsystem readiness and a transparent overall health result.
- Publish a bounded diagnostics snapshot through authenticated HTTP and MQTT.
- Feed diagnostics into OTA health confirmation without allowing diagnostics to control valves.

**Acceptance criteria:**

- Every reported metric is sourced from the active runtime instance.
- Health reports distinguish `healthy`, `degraded`, `unavailable` and `critical` conditions.
- Diagnostics collection does not materially affect irrigation timing.
- Host tests validate health aggregation; device tests validate heap and task metrics.

**Implementation and verification (2026-07-19):**

- Diagnostics Manager now collects from the active alarm, storage, event, MQTT, time, pressure
  and WaterSensor instances through an injected snapshot. It reports uptime, reset reason, current
  and minimum heap, event counts, active/critical alarms, persistent-log count, sensor and
  communication availability, and byte-valued stack high-water marks for the control, telemetry
  and WaterSensor tasks.
- A platform-independent aggregation policy classifies runtime, hydraulics, communications and
  storage as `healthy`, `degraded`, `unavailable` or `critical`. Overall display status remains
  distinct from OTA acceptability: unavailable optional sensors or communications degrade the
  report but do not roll back an otherwise safe image; core, heap, stack, critical-alarm and
  storage failures reject OTA confirmation.
- The same bounded JSON snapshot is served read-only at `GET /health` and retained on
  `zmartify/v2/devices/<device-id>/diagnostics/health` at QoS 1. JSON rendering fails closed when
  the caller's buffer is too small. The endpoint is temporarily unauthenticated because Step 2
  has not yet introduced the shared HTTP authentication/authorization layer; it must be protected
  by that layer before production release.
- Diagnostics has no relay, valve or irrigation-control authority. Collection uses bounded mutex
  waits and runs from the lower-priority telemetry/HTTP paths; the 30-second OTA guard consumes
  only the policy's `ota_acceptable` result.
- All 12 host tests pass, including health-state aggregation, optional-subsystem loss, critical
  alarms, storage failures, low stack margin and event drops. The signed ESP-IDF image builds at
  `0x151000` bytes with 21 percent app-partition headroom.
- The signed image was OTA-deployed to the ESP32-S3 at `192.168.10.113`. After the confirmation
  window, `/health` reported 39 percent heap utilization, 4,300/1,764/2,332 free stack bytes for
  control/telemetry/WaterSensor, zero dropped events, healthy runtime and storage, synchronized
  time, disconnected MQTT, unavailable flow/pressure, and `ota_acceptable:true`. The audit log
  recorded `ota image confirmed valid`; no valves were activated.

**Commit:** `Plan 2 Step 6: Integrate authoritative system diagnostics`

---

### Step 7 - MQTT v5.0 Contract Compliance

**Objective:** Bring the active MQTT implementation into an explicit, tested subset of Volume 3.

**Activities:**

- Resolve ownership between `mqtt_transport`, `mqtt_manager`, `zic_v2` and `main`.
- Define the supported topic and JSON schema subset in the RTM.
- Validate command schema, ranges, IDs, timestamps and authorization before queueing.
- Provide deterministic accepted/rejected/completed outcomes with correlation IDs.
- Define QoS, retained-state, last-will and reconnect policies.
- Implement required Home Assistant discovery lifecycle if retained in product scope.
- Add broker-backed contract tests for reconnect, duplicate command and malformed payload paths.

**Acceptance criteria:**

- Every supported command receives one deterministic outcome sequence.
- Duplicate/replayed commands cannot cause duplicate watering.
- Critical alarm delivery follows the documented QoS policy.
- Reported state recovers correctly after broker and controller restart.
- Unsupported Volume 3 interfaces are explicitly marked `Not Applicable` or deferred in RTM.

**Implementation and verification (2026-07-19):**

- The active MQTT ownership path is now `mqtt_transport` for connection/session/QoS,
  `zic_v2` for contract validation/serialization and `main` for queueing and execution. The legacy
  `mqtt_manager` and controller-specific command namespace are removed from the runtime dependency
  graph. The supported topics, schemas and deferred interfaces are defined in
  [MQTT-V5-CONTRACT.md](MQTT-V5-CONTRACT.md).
- ESP-MQTT explicitly negotiates MQTT v5. The stable client uses a one-hour session expiry,
  automatic five-second reconnect, receive maximum 16, 2,048-byte maximum packet size, retained
  online/offline status with QoS 1 LWT, retained reported state/diagnostics at QoS 1 and critical
  outcomes at QoS 2.
- The supported command subset is `zone/start`, `zone/stop`, `stop_all` and `rain_delay`.
  Commands require a bounded ID, strict UTC timestamp, five-minute freshness, exact integer/range
  validation and an authenticated `mqtts://` connection with broker credentials before queueing.
  Unauthenticated local brokers may receive telemetry but cannot actuate irrigation.
- Each queue element carries its own correlation ID. Successful insertion emits
  `command.accepted`, execution emits exactly one correlated completed/rejected result, malformed
  or unauthorized input emits `command.rejected`, and a fixed 16-ID replay ring emits
  `command.duplicate` without queueing. Persistent replay suppression across controller restart is
  deferred until a durable command journal is specified.
- Home Assistant discovery, legacy commands, MQTT configuration mutation, MQTT OTA, program
  control and generic management commands are outside the current product subset. Signed OTA
  remains on the controlled HTTP/HTTPS paths.
- All 13 host tests pass. The MQTT contract test covers UTC parsing, authorization, stale/future
  timestamps, malformed IDs, integer wraparound/ranges and duplicate suppression. The signed MQTT
  v5 image builds at `0x151000` bytes with 21 percent app-partition headroom.
- The MQTT v5 image was OTA-deployed to `192.168.10.113` and recorded
  `ota image confirmed valid`. The final strict-schema image (SHA-256
  `6c5d92450bd661aea41a7cd3359e9ee3e7ba7aabf39afcd19bfdc60f1fbc0dda`) was then OTA-deployed;
  after reboot it reported healthy runtime/storage, 39 percent heap utilization, synchronized
  time, zero event drops and `ota_acceptable:true`. No valve command was issued. This final boot
  did not add a new confirmation entry to the bounded audit log, so its rollback-state transition
  was not independently observed. A local Mosquitto 2.1.2 harness verified retained QoS 1
  behavior. Full device/broker reconnect, LWT and accepted duplicate sequence remains pending
  because the device subnet could not route to the temporary broker at `192.168.1.224`; the
  original `mqtt://192.168.10.2:1883` configuration was restored and verified after the test.

**Commit:** `Plan 2 Step 7: Complete MQTT contract compliance`

---

### Step 8 - Configuration Migration and Alarm Lifecycle

**Objective:** Preserve configuration across firmware upgrades and implement the required alarm
lifecycle without weakening critical lockout.

**Activities:**

- Implement schema-by-schema configuration migrations.
- Replace placeholder configuration events with typed change events.
- Add alarm states needed by the product: active, acknowledged, resolved and cleared.
- Define auto-recovery and manual-clear policies per alarm code.
- Persist alarm history separately from current active state where appropriate.
- Ensure reboot cannot silently clear a safety condition that requires inspection.

**Acceptance criteria:**

- Upgrade tests migrate representative snapshots from every supported schema.
- Failed migration preserves recoverable data and enters a documented safe mode.
- Alarm transitions reject invalid state changes.
- Critical hydraulic lockout cannot be cleared merely by acknowledging an alarm.

**Implementation and verification (2026-07-19):**

- Configuration migration now uses an atomic schema dispatcher. Schema 1 advances to schema 2,
  then schema 2 advances to schema 3; the candidate is committed only after current safety
  validation. Representative v1/v2 tests verify defaults and preservation of existing values,
  while unsupported and invalid candidates prove the source snapshot remains unchanged.
- The raw pre-migration blob is retained as `cfg_recovery` before the primary key is replaced.
  Existing invalid blobs enter a documented safe mode using validated defaults with
  `CONFIG_MODE_OFF`; normal commits are rejected. Safe-mode and recovery markers survive reboot,
  and a later boot atomically promotes a valid recovery copy to primary configuration with a
  normalized CRC. First boot without a blob remains distinct from migration failure.
- Configuration commits now publish `EVENT_CONFIG_CHANGED` with a versioned,
  `EVENT_PAYLOAD_CONFIG_CHANGE` payload and section mask instead of the placeholder system-fault
  event. The detailed policy is defined in
  [CONFIG-ALARM-LIFECYCLE.md](CONFIG-ALARM-LIFECYCLE.md).
- Alarm records implement `active`, `acknowledged`, `resolved` and `cleared` transitions. Warning
  and information conditions recover automatically; critical and safety-specific codes require
  condition resolution, acknowledgment and explicit manual clear. Acknowledgment alone retains
  lockout, critical severity cannot be downgraded, and reactivation requires fresh acknowledgment.
- Current alarms and a separate 32-transition history ring persist in a versioned CRC snapshot.
  Valid snapshots restore uncleared lockout after reboot; corrupt snapshots fail closed by raising
  a critical irrigation fault. Protected HMI acknowledge/manual-clear actions remain Step 9 scope.
- All 14 host tests pass, including new transition, invalid-state, reactivation, severity,
  snapshot round-trip/corruption and schema migration coverage. The signed ESP32-S3 image builds
  at `0x151000` with 21 percent app-partition headroom.
- Live fail-closed validation exposed two pre-existing boot defects: the persistent-configuration
  CRC included its own stored checksum, and telemetry could call diagnostics before its snapshot
  callback was initialized. CRC calculation now normalizes the checksum field, and diagnostics
  initialization precedes task startup. The full 14-test suite plus 50 repeated suite iterations
  passed after these repairs.
- The final signed image (SHA-256
  `7771296238b839eceb53dd875a04741eadcb591efe0deac1657d988a010ae7e9`) was OTA-deployed to
  `192.168.10.113` at 8 KiB/s. The device audit recorded `ota pending health confirmation`
  followed by `ota image confirmed valid`. A subsequent serial boot matched ELF SHA-256 prefix
  `b917ef06d`, loaded schema v3 from NVS with CRC OK, showed no configuration safe-mode warning,
  and passed repeated telemetry snapshots without panic. Runtime/storage were healthy, heap
  utilization was 39 percent, time was synchronized, no alarm or event drop was present and
  `ota_acceptable` was true. No valve or irrigation command was issued.

**Commit:** `Plan 2 Step 8: Add config migration and alarm lifecycle`

---

### Step 9 - HMI Product Completion

**Objective:** Align the local interface with the applicable operational and service workflows in
Volume 4 without coupling LVGL directly to valve control.

**Activities:**

- Confirm the 1024x600 Waveshare 7B as the authoritative display baseline and update conflicting
  documentation.
- Separate navigation, view models/data binding and command dispatch from screen rendering.
- Complete zone/program control, weather, hydraulic status, alarms, diagnostics and settings
  workflows.
- Add protected confirmation for safety-critical configuration and service actions.
- Decide the supported theme and language scope; mark unsupported optional features in RTM.
- Add screenshot or simulator-based regression tests for critical screens where feasible.

**Acceptance criteria:**

- All common irrigation workflows are usable without MQTT.
- HMI commands pass through the same validated command path as remote commands.
- Critical alarms remain visible until resolved according to policy.
- No screen directly manipulates relay HAL state.
- Desktop/simulator or device evidence covers navigation and critical dialogs.

**Implementation and verification (2026-07-19):**

- Added a platform-independent `hmi_controller` that owns five-screen navigation, parameter
  validation, a single protected confirmation state and command dispatch. Repeatable host tests
  cover navigation, cancellation, invalid values, immediate Stop All and confirmed zone, program,
  rain-delay and alarm actions.
- LVGL now renders runtime snapshots instead of placeholder values. Dashboard, irrigation,
  weather, hydraulics/alarms and settings show live controller, weather, hydraulic, program,
  connectivity, storage and configuration state. Alarm count or configuration safe mode remains
  visible in the footer on every screen.
- Local zone start, enabled-program run, Stop All and rain-delay actions enter the same
  `zic_runtime_command_t` queue consumed by the control task. LVGL never calls relay HAL or the
  irrigation engine. Alarm acknowledge/manual-clear runs through the Step 8 alarm manager under
  the runtime lock; clearing one alarm cannot release another critical lockout.
- Start zone, run program, rain-delay changes and alarm actions use a blocking confirmation
  overlay. Stop All remains immediate by safety policy. Credentials and safety thresholds remain
  outside the unauthenticated HMI surface.
- The active product baseline is explicitly 1024x600 Waveshare 7B with GT911, English and the
  light operational theme. The obsolete Volume 5 800x480 profile is marked as superseded. Detailed
  screen criteria and ownership are defined in [HMI-WORKFLOWS.md](HMI-WORKFLOWS.md).
- All 15 host tests pass. The signed image (SHA-256
  `e95faec8e2c910f3fb9d670346992559f4afafb1f4b92b47dd5f81915dd110d7`) builds at `0x151000`
  with 21 percent partition headroom and was OTA-deployed to `192.168.10.113`. Audit recorded
  `ota image confirmed valid`; health at 35 seconds showed healthy runtime/storage, 39 percent
  heap utilization, zero alarms and event drops, synchronized time and `ota_acceptable:true`.
  A post-confirmation serial boot matched ELF SHA-256 prefix `24e0a1993`, initialized the
  1024x600 panel, GT911 and LVGL 8.4, loaded schema v3 with CRC OK and remained stable. No valve,
  irrigation or alarm-transition command was issued during verification.
- The device build has no framebuffer export or desktop simulator. Controller-level navigation
  and critical-dialog behavior are automated; physical rendering/touch initialization are
  device-verified. Screenshot regression remains open for Step 10 and is not overclaimed here.

**Commit:** `Plan 2 Step 9: Complete controller HMI workflows`

---

### Step 10 - Verification, FAT and SAT Release Gate

**Objective:** Produce auditable evidence that the controller is ready for controlled field use.

**Activities:**

- Complete RTM verification links and close or formally defer every applicable requirement.
- Expand automated tests for HTTP security, OTA rollback, MQTT contracts, configuration
  migration and HMI logic.
- Define a Factory Acceptance Test covering all relays, sensors, display/touch, storage,
  networking, power recovery and fault injection.
- Define a Site Acceptance Test covering valve mapping, flow/pressure calibration, schedules,
  weather integration, alarms and communications.
- Run soak, reboot, network-loss, broker-loss and power-interruption tests.
- Record firmware identity, configuration baseline and test evidence for the release candidate.

**Acceptance criteria:**

- All safety/security requirements are verified or block release.
- Automated test suite and ESP-IDF build pass from a clean checkout.
- FAT passes on the production-equivalent controller.
- SAT passes at the installation or open deviations are formally accepted.
- Release and rollback procedures are documented and rehearsed.

**Implementation and verification (2026-07-19):**

- Added `scripts/release-gate.sh` with distinct software-candidate and field-release dispositions.
  From a clean checkout, `--software-only` runs the complete host suite, performs a fresh signed
  ESP32-S3 build, verifies the Secure Boot v2 signature, enforces partition headroom and emits a
  commit/artifact/security manifest. `--release` additionally requires zero open blockers.
- Added a machine-readable release deviation register. Every open security, RTM, FAT, SAT,
  hydraulic, power-loss, broker and soak blocker has a named evidence requirement. CTest and CI
  verify the register blocks release with exit code 2; the single deferred screenshot-regression
  item does not substitute for physical HMI acceptance.
- Defined auditable FAT and SAT protocols in [FAT-PROCEDURE.md](FAT-PROCEDURE.md) and
  [SAT-PROCEDURE.md](SAT-PROCEDURE.md). They require controller/artifact/configuration identity,
  calibrated measurements, per-zone and OTA trial matrices, immutable evidence, independent
  witness and explicit pass/fail. No FAT or SAT execution is claimed by this step.
- Documented candidate packaging, deployment, automatic rollback and operator rollback in
  [RELEASE-AND-ROLLBACK.md](RELEASE-AND-ROLLBACK.md). The stale MQTT URL and multipart HTTP OTA
  scripts now fail closed; USB flash remains a manufacturing/recovery operation and no longer
  automatically opens a resetting serial monitor.
- Production authorization remains **BLOCKED**. HTTP authentication/authorization, production
  Secure Boot/flash-encryption provisioning, complete RTM disposition, production-equivalent FAT,
  installation SAT, OTA power-interruption evidence, installed hydraulic calibration/fault
  injection, broker ACL evidence and long soak evidence are open blockers. No valve was activated
  and no eFuse was provisioned during this documentation/automation step.

**Commit:** `Plan 2 Step 10: Establish FAT and SAT release gate`

## 5. Execution Rules

For each step:

1. Confirm the relevant RTM entries and acceptance criteria before editing code.
2. Make the smallest coherent implementation slice.
3. Run focused tests immediately after the first substantive edit.
4. Run the complete host suite and ESP-IDF build before deployment.
5. OTA-deploy only when runtime firmware changes.
6. Avoid activating real valves during health checks unless the step explicitly requires a
   controlled hardware test.
7. Update RTM status and verification evidence in the same commit.
8. Commit and push only after validation.

## 6. Current Status

| Step | Status | Commit |
|---|---|---|
| 1. Living RTM | Complete | `Plan 2 Step 1: Establish MEP requirements traceability` |
| 2. HTTP authentication | Not started | - |
| 3. OTA trust and rollback | Signed OTA and automatic rollback hardware-verified; power-interruption FAT pending | `Plan 2 Step 3: Enforce trusted OTA and rollback` |
| 4. Hydraulic safety configuration | Firmware complete; installed hydraulic commissioning pending | `Plan 2 Step 4: Externalize hydraulic safety configuration` |
| 5. Relay and valve diagnostics | Firmware complete; hydraulic fault injection pending | `Plan 2 Step 5: Add relay and valve diagnostics` |
| 6. Integrated diagnostics | Firmware and device metrics complete; HTTP authentication pending Step 2 | `Plan 2 Step 6: Integrate authoritative system diagnostics` |
| 7. MQTT compliance | Firmware/device complete; routed authenticated broker acceptance pending | `Plan 2 Step 7: Complete MQTT contract compliance` |
| 8. Config migration and alarm lifecycle | Not started | - |
| 9. HMI completion | Not started | - |
| 10. FAT/SAT release gate | Not started | - |

## 7. Definition of Plan Completion

Implementation Plan 2 is complete when all applicable MEP v5.0 requirements are either:

- implemented and linked to passing verification evidence;
- formally deferred with an approved rationale; or
- marked not applicable because of a documented product scope decision;

and the production-equivalent controller has passed the defined FAT and SAT release gates.

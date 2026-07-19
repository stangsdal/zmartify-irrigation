# Implementation Plan 2 - Product Maturation Against MEP v5.0

**Project:** Zmartify Irrigation Controller
**Baseline:** Zmartify Master Engineering Package v5.0
**Hardware:** ESP32-S3, Waveshare 7B display, MCP23017 relay board
**Plan status:** Active
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
| 3. OTA trust and rollback | Implementation complete; hardware verification pending | `Plan 2 Step 3: Enforce trusted OTA and rollback` |
| 4. Hydraulic safety configuration | Not started | - |
| 5. Relay and valve diagnostics | Not started | - |
| 6. Integrated diagnostics | Not started | - |
| 7. MQTT compliance | Not started | - |
| 8. Config migration and alarm lifecycle | Not started | - |
| 9. HMI completion | Not started | - |
| 10. FAT/SAT release gate | Not started | - |

## 7. Definition of Plan Completion

Implementation Plan 2 is complete when all applicable MEP v5.0 requirements are either:

- implemented and linked to passing verification evidence;
- formally deferred with an approved rationale; or
- marked not applicable because of a documented product scope decision;

and the production-equivalent controller has passed the defined FAT and SAT release gates.

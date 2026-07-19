# Implementation Plan

This plan maps the v3.0 specification into incremental development steps.

This plan is complete. Product maturation against the full Master Engineering Package v5.0
continues in [Implementation Plan 2](IMPLEMENTATION-PLAN-2.md).

1. Bootstrap repository, ESP-IDF structure, and coding standards.
2. Implement shared core domain model and state machines.
3. Implement MQTT topic and payload contract layer.
4. Implement `zone_manager` and `irrigation_engine` core flows.
5. Implement flow/pressure managers and alarm escalation.
6. Implement storage, logging, and export interfaces.
7. Implement weather manager and ET runtime adjustment layer.
8. Implement test harness and acceptance scenarios.

Each step is committed independently for traceability.

## Current status

- Steps 1-4: complete.
- Step 5: complete. Flow and pressure supervision use valid sensor data, time-qualified alarm escalation, and critical fail-safe irrigation shutdown.
- Step 6: complete. Operational events use a CRC-protected NVS ring, 30-day pruning, critical/immediate and hourly flush policies, and HTTP JSON/CSV export.
- Step 7: complete. Provider-neutral weather ingestion uses a CRC-protected 24-hour NVS cache, FAO-56 reference ET, effective-rain deduction, per-zone crop coefficients, and bounded seasonal runtime adjustment with safe stale-data fallback.
- Step 8: complete. A CMake/CTest host harness runs unit and cross-component acceptance tests with warnings as errors, including critical hydraulic shutdown and persistent alarm recovery, and the suite runs in CI.

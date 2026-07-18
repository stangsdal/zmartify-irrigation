# Implementation Plan

This plan maps the v3.0 specification into incremental development steps.

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
- Steps 6-8: pending.

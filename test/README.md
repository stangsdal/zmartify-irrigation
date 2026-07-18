# Test Harness

This folder contains host-side C tests for critical logic.

## Current Tests

- `test_state_machine.c` validates controller lifecycle transitions.
- `test_safety_rules.c` validates flow and pressure alarm thresholds.
- `test_irrigation_engine.c` validates non-blocking master/zone relay sequencing and stop preemption.
- `test_storage_manager.c` validates log rotation, CRC persistence, retention and JSON escaping.
- `test_weather_manager.c` validates weather cache persistence and expiry, corruption rejection,
    rain deduction, bounded runtime adjustment and stale-data fallback.
- `test_et_engine.c` validates the daily FAO-56 reference ET calculation, weather input
    sensitivity and invalid-input handling.
- `test_watersensor_protocol.c` validates Water Sensor frame decoding, CRC,
  protocol-major rejection and malformed lengths.

## Next Step

Integrate these tests into ESP-IDF Unity test runner and CI workflow.

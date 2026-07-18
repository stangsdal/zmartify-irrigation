# Test Harness

This folder contains host-side C tests for critical logic.

## Current Tests

- `test_state_machine.c` validates controller lifecycle transitions.
- `test_safety_rules.c` validates flow and pressure alarm thresholds.
- `test_irrigation_engine.c` validates non-blocking master/zone relay sequencing and stop preemption.
- `test_watersensor_protocol.c` validates Water Sensor frame decoding, CRC,
  protocol-major rejection and malformed lengths.

## Next Step

Integrate these tests into ESP-IDF Unity test runner and CI workflow.

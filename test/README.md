# Test Harness

This folder contains host-side C tests for critical logic.

## Current Tests

- `test_state_machine.c` validates controller lifecycle transitions.
- `test_safety_rules.c` validates flow and pressure alarm thresholds.

## Next Step

Integrate these tests into ESP-IDF Unity test runner and CI workflow.

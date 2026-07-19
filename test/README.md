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
- `test_acceptance_safety_shutdown.c` validates the cross-component critical pressure
    fault path: alarm escalation, zone-before-master shutdown and persistent alarm restore.
- `test_hmi_controller.c` validates local navigation, confirmation and command dispatch policy.
- `test_release_gate.sh` verifies that open safety/security/FAT/SAT deviations block release.

## Running Host Tests

The host mode does not require ESP-IDF:

```sh
cmake -S . -B build-host-tests -DZIC_HOST_TESTS=ON
cmake --build build-host-tests
ctest --test-dir build-host-tests --output-on-failure
```

All host targets compile with `-Wall -Wextra -Werror`. GitHub Actions runs the same
configure, build and CTest sequence for every push and pull request.

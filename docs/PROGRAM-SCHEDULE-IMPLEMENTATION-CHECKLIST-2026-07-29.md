# Program And Schedule Implementation Checklist

This checklist maps the draft MQTT v2 sync contract onto concrete firmware files and tests.

## Current first slice

- Done: accept `config/programs/replace` on the existing MQTT v2 command path.
  Files: `main/main.c`, `components/zic_v2/include/zic_v2.h`
- Done: accept `config/programs/clear` on the existing MQTT v2 command path.
  Files: `main/main.c`, `components/zic_v2/include/zic_v2.h`
- Done: parse edge-authored program snapshots into `config_program_t`.
  Files: `main/main.c`, `components/config_manager/include/config_types.h`
- Done: commit the full program set atomically and restore runtime state on failure.
  Files: `components/config_manager/include/config_manager.h`, `components/config_manager/src/config_manager.c`
- Done: emit `config.programs.applied` after successful persistence.
  Files: `main/main.c`
- Done: emit scheduler revision and next-run metadata in reported state.
  Files: `main/main.c`, `components/zic_v2/include/zic_v2.h`, `components/zic_v2/src/zic_v2.c`
- Done: cover the new command action in host MQTT v2 contract tests.
  Files: `test/test_mqtt_v2_contract.c`

## Implemented constraints in this slice

- Supported now: full-snapshot replace of up to `CONFIG_MAX_PROGRAMS` programs.
- Supported now: weekly schedules that collapse into the controller model.
  Constraint: all enabled schedules inside one program must share the same weekday mask.
- Supported now: zone durations in whole minutes only.
- Supported now: zone order only when `sort_order` matches ascending controller zone order.
- Rejected for now: interval schedules, anchored dates, explicit date lists, mixed weekday masks within one program, and arbitrary zone execution order.

## Remaining contract gaps

- Todo: add optional single-program upsert.
  Likely files: `main/main.c`, `components/zic_v2/include/zic_v2.h`
- Todo: persist and surface edge program IDs and schedule IDs if reconciliation needs identity round-tripping.
  Likely files: `components/config_manager/include/config_types.h`, migration and validation sources
- Todo: add host coverage for payload translation and scheduler activation.
  Likely files: new host-testable parser helper plus `test/` coverage
- Todo: verify reboot persistence and local scheduled execution from synchronized config.
  Likely tests: host tests where feasible, plus device acceptance procedure

## Validation targets

- Host contract test: `test/test_mqtt_v2_contract.c`
- Firmware compile gate: `idf.py build`
- Follow-up acceptance: publish `config/programs/replace`, reboot, and confirm a scheduled start without edge involvement
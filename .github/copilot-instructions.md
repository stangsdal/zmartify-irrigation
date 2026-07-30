# Zmartify Irrigation Controller Copilot Instructions

Use these instructions when working in the irrigation controller firmware repository.

## Primary sources of truth

- Treat the local controller documentation as authoritative before changing code.
- Start from `ARCHITECTURE.md`, `CODING_STANDARDS.md`, and the detailed documents under `docs/`.
- When behavior spans controller autonomy, MQTT, programs, schedules, storage, alarms, or UI, verify the requirement in the engineering package docs before implementing.

## Non-negotiable product behavior

- Preserve controller autonomy. Scheduled irrigation must be able to execute locally on the controller even when the edge backend is offline.
- Do not move schedule execution responsibility to the backend. The edge may store, visualize, or synchronize configuration, but the controller is the scheduler and runtime authority.
- Keep safety first. Any uncertainty about hydraulics, pressure, relay state, or alarm handling should default to the fail-safe behavior documented in the architecture.

## Current implementation boundary to respect

- The current MQTT v2 adapter is implemented around manual irrigation control and telemetry/outcomes.
- The current `zic_v2` command action set covers zone start, zone stop, stop all, rain delay, network config, and SD-card initialization.
- Do not claim that controller-side program and schedule synchronization is complete unless code exists for:
  - receiving program and schedule definitions,
  - validating and persisting them,
  - executing them locally from controller time,
  - surviving reboot and offline periods,
  - and exposing the resulting state through telemetry and outcomes.

## Required implementation rule for programs and schedules

When working on irrigation programs or schedules, treat the feature as incomplete until all of the following are true:

1. Program and schedule data is represented in persistent controller configuration.
2. The irrigation engine can evaluate schedules from local time without edge participation.
3. Manual runs, scheduled runs, rain delay, disabled zones, weather adjustments, and fault states have defined precedence.
4. MQTT or other remote configuration paths for program and schedule data are explicitly implemented and documented.
5. Outcomes include enough information to correlate controller behavior with edge state, including `run_id` and `program_id` where applicable.
6. Reported state and diagnostics reflect whether a program is idle, scheduled, running, blocked, or faulted.
7. Documentation is updated in the same change.

## Documentation and code must stay aligned

- If you add or change controller behavior, update the matching docs in the same task.
- If documentation describes a capability that is not implemented yet, do not paper over the gap. Either implement it or mark the documentation and code comments clearly as planned or partial.
- Prefer explicit status wording such as `implemented`, `partial`, `planned`, or `not wired` instead of vague statements.

## Files and modules to check before changing behavior

- Check the controller architecture and coding standards documents first.
- Check `components/zic_v2/` for MQTT v2 command, outcome, and reported-state behavior.
- Check `components/config_manager/` for persisted settings and schema compatibility.
- Check `components/irrigation_engine/`, `components/zone_manager/`, and related managers for controller-local execution logic.
- Check HMI components before assuming the local UI already exposes a feature.

## Verification expectations

- Validate new controller behavior with the narrowest relevant test first.
- For MQTT-facing changes, verify both payload shape and runtime behavior.
- For program and schedule work, verify at minimum:
  - persistence across reboot,
  - correct local-time schedule triggering,
  - correct behavior while edge is disconnected,
  - correct precedence between manual and scheduled irrigation,
  - and correct outcome and reported-state publication.
- Do not mark a feature complete based only on UI forms, backend storage, or documentation presence.

## Build and editing guardrails

- Use ESP-IDF build flows and existing project scripts. Do not reconfigure the firmware build with generic CMake workflows.
- Follow the repository coding standards: modular components, documented public APIs, bounded buffers, and clear safety-oriented logic.
- Prefer minimal, well-scoped changes over broad refactors unless the docs require an architectural correction.

## Practical guidance for Copilot

- When asked whether a controller feature exists, verify the code path before answering.
- When implementing a documented feature, trace all required layers: configuration, engine logic, telemetry, outcomes, alarms, persistence, and documentation.
- When a backend or app feature appears present but the controller does not act on it, assume missing controller-side wiring until proven otherwise.
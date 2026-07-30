# Program And Schedule Audit - 2026-07-29

## Scope

This audit checks whether controller-local irrigation programs and schedules are both documented and fully implemented for the current `zmartify-irrigation` firmware.

## Conclusion

The repository documents controller-local schedule autonomy as a product requirement, and the firmware contains partial local program and scheduler foundations. However, the controller is not yet fully integrated with the current edge v2 program and schedule model.

The main gap is not local program storage alone. The missing boundary is remote synchronization and end-to-end verification between edge program configuration and controller-local autonomous execution.

## What is already present

### Documented intent

- Controller autonomy is required by the architecture and product documents.
- The controller is expected to continue autonomous irrigation according to programmed schedules when edge connectivity is unavailable.
- Manual irrigation is expected to take precedence over scheduled irrigation.
- Local timezone and next-run behavior are part of the documented acceptance surface.

### Local implementation evidence

- Program configuration exists in persistent config types via `config_program_t` in `components/config_manager/include/config_types.h`.
- Program selection and program-oriented HMI actions exist in `components/hmi_board/`.
- The requirements traceability document records a local scheduler as present, though partial.
- MQTT v2 outcomes already support `run_id` and `program_id` fields in `components/zic_v2/`.
- The active MQTT v2 runtime supports manual irrigation commands and publishes reported state, diagnostics, and irrigation outcomes.

## Gaps confirmed in the current repository

### 1. No remote program or schedule sync in the active MQTT v2 contract

The active contract in `docs/MQTT-V5-CONTRACT.md` explicitly limits the supported remote command subset to:

- `zone/start`
- `zone/stop`
- `stop_all`
- `rain_delay`

It also explicitly lists program start, pause, resume, skip, and broader configuration mutation as deferred interfaces.

### 2. The current `zic_v2` command adapter has no program or schedule command actions

The `zic_v2_command_action_t` enum only covers:

- zone start
- zone stop
- stop all
- rain delay
- network config
- SD-card initialization

There is no command action for:

- replacing controller programs
- replacing controller schedules
- deleting programs or schedules
- forcing a scheduler rescan
- publishing scheduler state changes

### 3. Edge program and schedule data is not proven to reach controller configuration

The edge stack currently stores irrigation programs and schedules and serves them through API routes, but that does not prove the controller receives or persists them.

This repo currently lacks a documented and implemented bridge from the edge program model to controller `config_program_t` data under the active MQTT v2 transport.

### 4. Reported state does not expose scheduler status

The current v2 reported-state builder focuses on hydraulics, power, weather, and storage.

There is no explicit scheduler/program status block covering fields such as:

- active program id
- active program name
- next scheduled run
- scheduler enabled state
- blocked reason
- rain-delay remaining

### 5. Acceptance evidence is incomplete

The requirements traceability file already marks several scheduler-related requirements as `Partial`.

Missing evidence still includes at least:

- full scheduler acceptance coverage
- end-to-end remote program/schedule synchronization coverage
- offline soak behavior for scheduled autonomy
- precedence tests between manual and scheduled irrigation
- next-run and timezone acceptance checks tied to the active v2 integration

## Operational interpretation

As of this audit, the firmware should be treated as:

- capable of local program representation,
- capable of manual v2 command execution,
- capable of publishing v2 outcomes and telemetry,
- but not yet complete for controller-synchronized scheduled execution from edge-authored program definitions.

## Required completion criteria

Program and schedule support should not be called complete until all of the following are true:

1. Remote program and schedule synchronization to controller configuration is implemented.
2. The irrigation engine evaluates schedules from local time on the controller.
3. Manual runs, scheduled runs, rain delay, disabled zones, weather gating, and fault gating have defined and tested precedence.
4. Reported state and outcomes expose scheduler-relevant state.
5. Reboot persistence and edge-offline autonomy are validated.
6. Repository documentation states the implemented status accurately.

## Recommended next implementation slice

The next slice should define and implement a controller-side v2 synchronization contract for irrigation programs and schedules, then validate it with host tests and a controller-to-edge acceptance flow.
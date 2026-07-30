# MQTT v2 Program And Schedule Sync Contract

## Status

Draft for controller implementation alignment.

This document defines the missing controller-side synchronization contract needed so edge-authored irrigation programs and schedules can be applied to controller-local autonomous scheduling.

## Goals

- Keep the controller as the scheduler and runtime authority.
- Allow edge systems to synchronize program definitions without taking over runtime execution.
- Preserve operation when the edge backend is offline after configuration has been applied.
- Keep the contract compatible with the current MQTT v2 command style used by the controller.

## Non-goals

- The backend must not become the live scheduler.
- Program execution must not require a round-trip to edge at run time.
- This contract does not replace local safety, weather, rain-delay, or hydraulic decision logic.

## Contract model

The controller remains authoritative for:

- local time evaluation,
- schedule triggering,
- runtime sequencing,
- safety stops,
- weather and rain-delay gating,
- and local recovery after reboot.

The edge remains authoritative for:

- user-authored program definitions,
- schedule editing UX,
- audit history,
- and configuration synchronization requests.

## Transport style

Use the existing MQTT v2 command namespace pattern:

- topic decides the command type,
- payload uses the compact envelope,
- command acceptance and application are reported through irrigation outcomes.

### Required envelope

```json
{
  "command_id": "cfg-20260729-001",
  "source_timestamp": "2026-07-29T14:00:00Z",
  "parameters": {}
}
```

## Proposed command topics

### Replace full program set

Topic:

```text
zmartify/v2/devices/{device_id}/commands/irrigation/config/programs/replace
```

Purpose:

- Replace the controller's complete program and schedule set with a canonical snapshot from edge.

### Delete all program configuration

Topic:

```text
zmartify/v2/devices/{device_id}/commands/irrigation/config/programs/clear
```

Purpose:

- Clear all synchronized programs and schedules while preserving unrelated controller configuration.

Suggested payload:

```json
{
  "command_id": "cfg-20260729-002",
  "source_timestamp": "2026-07-29T14:05:00Z",
  "parameters": {
    "config_revision": 13
  }
}
```

### Optional targeted upsert

Topic:

```text
zmartify/v2/devices/{device_id}/commands/irrigation/config/program/upsert
```

Purpose:

- Update one logical program without replacing the entire set.

This is optional. The full-snapshot `replace` command is the preferred first implementation because it simplifies reconciliation and persistence.

## Proposed payload for `config/programs/replace`

```json
{
  "command_id": "cfg-20260729-001",
  "source_timestamp": "2026-07-29T14:00:00Z",
  "parameters": {
    "config_revision": 12,
    "timezone": "CET-1CEST,M3.5.0/2,M10.5.0/3",
    "programs": [
      {
        "program_id": "ec994e5f-8f62-47cc-b282-76593e24f9c2",
        "name": "Test",
        "enabled": true,
        "seasonal_adjust_pct": 100,
        "weather_mode": "automatic",
        "zones": [
          {
            "zone_ref": "zone:1",
            "sort_order": 1,
            "duration_seconds": 600,
            "enabled": true
          }
        ],
        "schedules": [
          {
            "schedule_id": "15b170c4-9b64-484f-a3ab-6727fd5fd2dc",
            "name": "Test",
            "enabled": true,
            "recurrence_type": "weekdays",
            "weekdays": [1, 2, 3, 4, 5],
            "start_local_time": "15:50",
            "interval_days": null,
            "anchor_date": null,
            "dates": []
          }
        ]
      }
    ]
  }
}
```

## Controller apply rules

1. Validate the entire payload before mutating persistent configuration.
2. Reject unknown zones, invalid durations, invalid weekday sets, invalid recurrence modes, malformed local times, and duplicate program or schedule IDs.
3. Apply the configuration atomically.
4. Persist configuration before acknowledging successful application.
5. Rebuild or refresh in-memory scheduler state immediately after successful persistence.
6. Preserve local safety, rain-delay, and fault logic; synchronized configuration does not override safety interlocks.

## Outcome requirements

The controller should emit outcomes on the existing irrigation outcome topic.

### Acceptance example

```json
{
  "schema_version": "2.0",
  "source_timestamp": "2026-07-29T14:00:01Z",
  "event_type": "config.programs.accepted",
  "severity": "info",
  "result": "accepted",
  "run_id": "cfg-20260729-001"
}
```

### Applied example

```json
{
  "schema_version": "2.0",
  "source_timestamp": "2026-07-29T14:00:02Z",
  "event_type": "config.programs.applied",
  "severity": "info",
  "result": "completed",
  "run_id": "cfg-20260729-001",
  "detail": "program_count=1 schedule_count=1 revision=12"
}
```

### Rejection example

```json
{
  "schema_version": "2.0",
  "source_timestamp": "2026-07-29T14:00:01Z",
  "event_type": "command.rejected",
  "severity": "warning",
  "result": "rejected",
  "run_id": "cfg-20260729-001",
  "detail": "invalid_schedule"
}
```

## Reported-state additions

The reported-state payload should eventually include a scheduler block, for example:

```json
{
  "irrigation": {
    "scheduler": {
      "config_revision": 12,
      "active_program_id": null,
      "active_program_name": null,
      "next_run_at": "2026-07-30T13:50:00Z",
      "rain_delay_active": false,
      "blocked_reason": null
    }
  }
}
```

This lets edge verify that the controller has actually applied the synchronized configuration.

Current implementation note:

- The controller now emits a scheduler block with `config_revision`, `program_count`, `schedule_count`, `active_program_name`, `next_run_at`, `rain_delay_active`, and `blocked_reason`.
- `active_program_id` remains `null` because edge IDs are not yet persisted in the controller configuration model.

## Precedence rules

The controller implementation must document and test these rules:

1. Critical faults override everything and stop irrigation safely.
2. Manual irrigation takes precedence over scheduled irrigation.
3. Rain delay blocks automatic schedules but should not necessarily block authorized manual irrigation unless configured to do so.
4. Disabled zones never auto-run.
5. Weather and freeze rules apply to scheduled starts.
6. Reboot restores persisted schedules without edge assistance.

## Minimum test matrix

1. Full snapshot replace with valid programs and schedules.
2. Reject malformed schedule payload without partial apply.
3. Reboot and confirm synchronized schedules persist.
4. Confirm next-run calculation respects local timezone.
5. Confirm a scheduled run starts while edge is disconnected.
6. Confirm manual run suppresses or defers overlapping scheduled execution according to precedence rules.
7. Confirm reported-state exposes synchronized scheduler revision and next-run state.

## Recommended implementation order

1. Add config translation between MQTT payloads and `config_program_t` structures.
2. Add atomic persistence and scheduler refresh hooks.
3. Add outcomes for config acceptance and application.
4. Add scheduler state to reported-state.
5. Add host tests for payload validation and scheduler activation.
6. Add end-to-end documentation and acceptance procedures.
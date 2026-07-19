# MQTT v5 Supported Contract

## Ownership

The active runtime uses one MQTT path:

- `mqtt_transport` owns the ESP-MQTT v5 connection, TLS verification, credentials, persistent session, subscriptions, reconnect, QoS and last will.
- `zic_v2` owns topic construction, payload serialization, command validation, timestamp freshness and in-memory duplicate suppression.
- `main` maps accepted commands to the control queue and publishes deterministic outcomes.
- The legacy `mqtt_manager` and `zmartify/irrigation/controller_01` command namespace are not part of the active runtime.

Subsystem managers do not publish directly to MQTT.

## Supported Topics

The device ID is `zmartify-irrigation-01` in the current deployment.

| Direction | Topic | QoS | Retained |
|---|---|---:|:---:|
| Publish | `zmartify/v2/devices/{device_id}/status` | 1 | Yes |
| Publish | `zmartify/v2/devices/{device_id}/state/reported` | 1 | Yes |
| Publish | `zmartify/v2/devices/{device_id}/diagnostics/health` | 1 | Yes |
| Publish | `zmartify/v2/devices/{device_id}/events/irrigation/outcome` | 1; critical events 2 | No |
| Subscribe | `zmartify/v2/devices/{device_id}/commands/irrigation/#` | 1 | No |

The retained status is `{"status":"online"}` after every connection. The retained last will is `{"status":"offline"}` at QoS 1.

## Command Envelope

Every supported command requires:

```json
{
  "command_id": "unique-id",
  "source_timestamp": "2026-07-19T19:00:00Z",
  "parameters": {}
}
```

`command_id` is 1-39 ASCII alphanumeric, `.`, `_`, `-` or `:`. The timestamp must be UTC, no more than 300 seconds old and no more than 30 seconds in the future. Commands are accepted only on a `mqtts://` connection with configured username and password. Broker ACLs remain responsible for restricting the credential to this device namespace.

Supported command suffixes and parameters:

| Suffix | Parameters | Validation |
|---|---|---|
| `zone/start` | `zone_id`, `duration_seconds` | zone 1-15; duration 1-7200 seconds |
| `zone/stop` | `zone_id` | zone 1-15 |
| `stop_all` | none | empty `parameters` object |
| `rain_delay` | `delay_hours` | 0-8760; zero clears the delay |

A fixed 16-entry command-ID ring suppresses redelivery during reconnect and QoS retries. A duplicate returns `command.duplicate` and is never queued. The cache is intentionally RAM-only; persistent replay protection across controller restarts is deferred until the durable command journal is specified.

## Outcome Sequence

A schema-valid, authorized, fresh, nonduplicate command receives:

1. `command.accepted` after successful queue insertion.
2. Exactly one execution result with the same correlation ID: `run.started`, `run.stopped`, `rain.delay_set`, `rain.delay_cleared`, or a rejection event.

Malformed, unauthorized, stale, future, out-of-range and queue-full commands receive `command.rejected`. Duplicate IDs receive `command.duplicate`. The command ID is emitted as `run_id` in the current v2 outcome schema.

## Session Policy

The client negotiates MQTT v5, uses a one-hour session expiry, receive maximum 16, maximum packet size 2048 bytes, automatic reconnect after 5 seconds and a stable client ID. Reported state, diagnostics and online status are republished after reconnect by normal runtime publication; retained values recover subscribers across broker or consumer restarts.

## Deferred Interfaces

The following Volume 3 surfaces are outside this supported subset:

- legacy controller command topics;
- MQTT configuration mutation;
- MQTT OTA commands, because signed OTA is exposed through the existing controlled HTTP/HTTPS paths;
- program start/pause/resume/skip, reboot, backup, restore and self-test commands;
- Home Assistant discovery, which is not retained in the current product scope;
- generic REST/WebSocket interfaces already marked future/not applicable in the RTM.

TLS client credentials and broker ACL provisioning are deployment responsibilities. The device rejects remote commands when that authorization boundary is absent.

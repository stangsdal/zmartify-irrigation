# Operational Logging

The controller keeps irrigation, alarm and audit events in chronological order.
The active ESP32-S3 implementation stores the latest 32 events in a
CRC-protected, versioned NVS snapshot. This bound keeps each snapshot below the
Storage HAL's 4096-byte blob limit.

## Persistence policy

- Critical hydraulic shutdowns are flushed immediately.
- Other dirty log data is flushed at most once per hour.
- Entries older than 30 days are removed during hourly maintenance after SNTP synchronization.
- Missing or corrupt snapshots are ignored and replaced by a new log without blocking controller startup.
- High-frequency telemetry and control-loop heartbeats are not written to NVS.
- Events recorded before SNTP synchronization use timestamp `0`; subsequent events use Unix time.

## Export

The controller HTTP service exposes entries from oldest to newest:

```text
GET /logs
GET /logs?format=json
GET /logs?format=csv
```

JSON strings and CSV fields are escaped by the firmware. Export is read-only
and does not clear the retained history.
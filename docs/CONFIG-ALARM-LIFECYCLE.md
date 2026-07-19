# Configuration Migration and Alarm Lifecycle

## Configuration Upgrade Policy

The active configuration schema is version 3. Supported upgrades run one schema at a time:

| From | To | Added defaults |
|---:|---:|---|
| 1 | 2 | Alarm freshness/escalation fields and per-zone flow deviation limits |
| 2 | 3 | Valve opening and closing response timeouts |

Migration uses a temporary candidate and commits it only after current safety validation passes.
Unsupported schemas and invalid migrated candidates leave the caller's original snapshot unchanged.

Before a migrated primary blob is replaced, its exact bytes are retained under `cfg_recovery`.
If an existing blob has invalid size, magic, CRC, schema or safety values, the controller also
retains its bytes under `cfg_recovery`, loads validated defaults in `CONFIG_MODE_OFF` and reports
configuration safe mode. Separate NVS markers preserve safe mode and recovery presence across
reboot. Normal commits are rejected in safe mode. On a later boot, a valid recovery copy is
promoted to the primary key with a normalized CRC, then the recovery copy and markers are removed.
If recovery validation fails, safe mode remains active. An explicit factory restore replaces the
failed configuration, removes the recovery copy and markers, and leaves safe mode.

First boot with no configuration is not a migration failure and receives normal factory defaults.
The internal recovery copy is not the complete user-facing export/restore workflow; that remains
deferred.

Successful commits publish `EVENT_CONFIG_CHANGED` with payload type
`EVENT_PAYLOAD_CONFIG_CHANGE`. The versioned payload contains the current schema and a bit mask
for system, network, hydraulics, alarms, zones and programs. Factory restore reports all sections.

## Alarm Lifecycle

Each uncleared alarm has one visible state:

```text
CLEARED -> ACTIVE -> ACKNOWLEDGED
              |             |
              +-> RESOLVED <-+
                       |
                 manual clear
                       |
                    CLEARED
```

Warning and informational alarms use automatic recovery: removal of the condition clears the
record. Critical alarms and safety-specific hydraulic codes use manual-clear recovery. Removing
the condition moves them to `RESOLVED`, but their lockout remains until the alarm has also been
acknowledged and explicitly cleared. Acknowledgment alone never releases lockout. Reactivation
returns a resolved alarm to `ACTIVE` and requires a fresh acknowledgment. Severity can escalate
while an alarm is open but cannot be downgraded.

The manager keeps current records and a separate 32-transition history ring. Both are stored in a
versioned, CRC-protected `alarm_state` snapshot after each transition. A valid snapshot restores
uncleared critical lockout after reboot. A malformed stored snapshot raises a critical irrigation
fault and therefore fails closed.

The lifecycle API is ready for local alarm workflows. Protected HMI actions for acknowledge and
manual clear are part of Step 9; no unauthenticated network clear endpoint is exposed.
# Site Acceptance Test

## Purpose and Entry Criteria

SAT verifies the installed controller, field wiring, valves, hydraulics, schedules and routed
communications. SAT starts only after a passing FAT for the same hardware/firmware baseline and an
approved installation safety inspection. Water discharge requires site authorization and an
observer at the hydraulic isolation point.

## Site Record

| Field | Value |
|---|---|
| SAT record ID / site | |
| Date | |
| Controller and cabinet serials | |
| Firmware commit / SHA-256 | |
| FAT record ID | |
| Configuration baseline ID | |
| Installer / operator / customer witness | |
| Weather and supply conditions | |

## SAT Checklist

| ID | Procedure | Required evidence | Pass criteria | Result / deviation |
|---|---|---|---|---|
| SAT-01 | Inspect cabinet, earthing, mains/SELV separation, glands, labels and isolation | Signed electrical inspection and photos | Installation matches approved drawings and local rules | |
| SAT-02 | Verify each configured zone against the physical valve and irrigated area, one zone at a time | Zone map, observer signature and photos | No cross-wiring; master/zone sequence correct; all outputs return off | |
| SAT-03 | Measure static/dynamic pressure and flow for every zone | Traceable raw readings | Values fall within accepted zone limits or approved deviations | |
| SAT-04 | Store accepted per-zone flow baseline, deviation and pressure thresholds | Configuration baseline and review signature | Thresholds derive from measured site data and pass validation | |
| SAT-05 | Run manual zone workflow from HMI including confirmation and Stop All | HMI/serial observation | Correct zone only; Stop All closes safely; no MQTT dependency | |
| SAT-06 | Verify enabled schedules, local timezone and next-run calculation without waiting for uncontrolled watering | Schedule worksheet and dry-run calculation | Program/day/time/zone runtimes match approved irrigation plan | |
| SAT-07 | Verify weather ingestion, stale fallback, rain skip and 24-hour local rain delay | Weather payload/audit evidence | Adjustments are bounded and stale data cannot create unsafe runtime | |
| SAT-08 | Inject approved low-risk alarm stimuli; rehearse acknowledge/resolve/manual clear | Alarm history and operator sign-off | Alarm remains visible; critical lockout policy is preserved | |
| SAT-09 | Verify Wi-Fi loss/recovery, routed MQTT TLS identity/ACLs and broker loss/recovery | Network and broker logs | Local operation remains safe; unauthorized topics rejected; no replay | |
| SAT-10 | Verify signed OTA, health confirmation and documented rollback decision path | Hashes, audit and health output | Candidate confirms healthy or returns to verified image | |
| SAT-11 | Power-cycle controller and isolated peripherals in approved states | Boot/alarm/config records | Configuration and uncleared alarms persist; outputs default off | |
| SAT-12 | Run site soak through representative schedule/network conditions | Periodic health report | No unsafe activation, reset loop, event drops or unresolved critical alarms | |

## Per-Zone Commissioning

| Zone | Valve/location | Relay | Runtime | Flow min/nom/max | Pressure min/nom/max | Stop response | Accepted |
|---:|---|---:|---:|---|---|---|:---:|
| 1-15 | | | | | | | |

## Deviations

Every deviation records requirement ID, observed result, safety impact, temporary control, owner,
due date and approving authority. Safety/security deviations block SAT. A non-safety deviation is
accepted only with written customer and engineering approval.

## Sign-Off

| Role | Name | Signature / date |
|---|---|---|
| Installer | | |
| Commissioning engineer | | |
| Customer/site representative | | |
| Safety/security approver (when applicable) | | |

Overall SAT result: **PASS / FAIL**. Field release requires both FAT PASS and SAT PASS, or formally
accepted non-safety deviations recorded in the release register.
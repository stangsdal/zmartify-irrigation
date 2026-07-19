# Factory Acceptance Test

## Purpose and Release Rule

This FAT verifies one production-equivalent controller before site installation. Every step needs
recorded evidence. A failed safety, security, electrical or rollback test blocks release. Open
non-safety deviations require an ID, owner, due date, risk statement and written approval in the
release deviation register.

## Test Record

| Field | Value |
|---|---|
| FAT record ID | |
| Date and location | |
| Controller serial / hardware revision | |
| Relay-board and WaterSensor serials | |
| Firmware commit | |
| Signed firmware SHA-256 | |
| Configuration export / baseline ID | |
| Test equipment and calibration dates | |
| Operator / independent witness | |

## Safety Preconditions

- A qualified person confirms mains/SELV segregation, protective earth, fusing and enclosure
  condition before energizing the fixture.
- Valve field wiring and 24 VAC loads are disconnected for logic, OTA and relay-mapping tests.
- Relay tests use labeled dummy loads or meter inputs. They never energize installed valves.
- Hydraulic fault tests use a controlled wet rig with containment, pressure relief and emergency
  isolation. They are not run on an unattended installation.
- Record the initial and final all-relays-off state.

## FAT Checklist

| ID | Procedure | Required evidence | Pass criteria | Result / evidence link |
|---|---|---|---|---|
| FAT-01 | Inspect BOM, labels, wiring, mains/SELV separation, fuses and replaceable modules | Photos, BOM revision, inspection signature | Matches approved drawings; no exposed or mixed-voltage defect | |
| FAT-02 | Run `scripts/release-gate.sh --software-only` from a clean checkout | Manifest and complete command log | All host tests, fresh ESP-IDF build, signature and size checks pass | |
| FAT-03 | Boot from cold power and record serial timestamps | Serial log | Application ready in less than 30 seconds; no panic/reset loop | |
| FAT-04 | Inspect 1024x600 display and exercise every tab, selector and confirmation cancel path | Screen photos/video and touch log | No clipping; touch target selects intended control; cancel dispatches nothing | |
| FAT-05 | With valve power disconnected, map master plus zone relays 1-15 using the controlled relay fixture | Channel-by-channel measurement sheet | Exactly one requested output plus master policy; all outputs return off | |
| FAT-06 | Verify WaterSensor identity, flow channels, pressure ADC, optional sensors and stale/offline behavior | Device logs and calibrated stimulus results | Correct identity/scaling; unavailable sensors degrade safely | |
| FAT-07 | Calibrate flow and pressure with traceable reference instruments at low/mid/high points | Raw readings, coefficients, residual error | Accepted calibration tolerance documented and met | |
| FAT-08 | Inject no-flow, high-flow, unexpected-flow, pressure collapse and valve no-response/post-close-flow faults on wet rig | Serial/event/alarm history and shutdown timing | Correct alarm; zone then master close; lockout survives reboot | |
| FAT-09 | Acknowledge, resolve and manually clear each critical alarm | HMI record and persisted alarm snapshot evidence | Acknowledge alone never releases lockout; other alarms retain lockout | |
| FAT-10 | Verify authenticated HTTP allow/deny cases and MQTT TLS/ACL allow/deny cases | Request/broker logs | Unauthorized requests denied; authorized bounded requests accepted | |
| FAT-11 | Install a signed healthy OTA image and a signed forced-unhealthy rollback image | Artifact hashes, serial and audit logs | Healthy image confirms; unhealthy image automatically rolls back | |
| FAT-12 | Interrupt OTA/power at defined upload, reboot and pending-confirmation points | Trial matrix with pre/post versions | Device always boots a complete verified image | |
| FAT-13 | Remove/recover Wi-Fi, broker, WaterSensor and pressure input independently | Health/audit timeline | Irrigation safety remains local; reconnect has no command replay | |
| FAT-14 | Corrupt/reject configuration and alarm persistence on test fixture | Recovery logs and retained blob hashes | Config enters OFF safe mode; alarm corruption fails closed | |
| FAT-15 | Run controlled endurance soak with periodic health capture | Soak report | No unexpected reset, critical alarm, event drop trend or unsafe heap trend | |

## Mandatory Trial Matrices

### Relay Mapping

| Logical output | Physical channel | Master state | Measured active level | Off verified | Result |
|---|---|---|---|---|---|
| Master | | | | | |
| Zone 1-15 (one row each) | | | | | |

### OTA Interruption

| Trial | Interruption point | Previous version | Candidate hash | Booted version | Rollback/audit result | Pass |
|---|---|---|---|---|---|---|
| 1 | Early upload | | | | | |
| 2 | Late upload | | | | | |
| 3 | First candidate boot | | | | | |
| 4 | Pending health confirmation | | | | | |

## Disposition and Sign-Off

| Gate | Result |
|---|---|
| Safety and electrical tests | PASS / FAIL |
| Security tests | PASS / FAIL |
| Firmware and rollback tests | PASS / FAIL |
| Open deviations | None / IDs |
| Overall FAT | PASS / FAIL |

Operator and independent witness sign the immutable FAT record. A FAIL result cannot be converted
to PASS by a note; the failed step must be rerun after corrective action.
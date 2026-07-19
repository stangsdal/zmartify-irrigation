# Release and Rollback Procedure

## Release Candidate Gate

Run from a clean checkout with the approved ESP-IDF environment and signing key available:

```sh
source ~/.espressif/v6.0.1/esp-idf/export.sh
./scripts/release-gate.sh --release
```

`--release` passes only when the fresh host suite, fresh signed ESP32-S3 build, signature check,
artifact-size check and all registered release blockers pass. `--software-only` creates the same
manifest but deliberately does not authorize field release. Development use of `ALLOW_DIRTY=1`
must never be treated as release evidence.

Archive together:

- `build-release/zmartify_irrigation.bin`;
- `build-release/release-manifest.txt`;
- bootloader, partition table and OTA data artifacts from the same build;
- immutable host/build command log;
- approved configuration baseline/export;
- signed FAT and SAT records;
- approved deviation register;
- public verification key fingerprint and protected-key custody reference.

Never archive or distribute the private signing key with release artifacts.

## Deployment

1. Confirm controller identity, current version, idle state and all outputs off.
2. Confirm the candidate SHA-256 equals the approved manifest.
3. Upload only the signed image through the approved authenticated OTA path.
4. Do not reset the controller during pending health confirmation.
5. Require `ota pending health confirmation` followed by `ota image confirmed valid`.
6. Capture `/health`, audit log and serial application identity after confirmation.
7. Confirm configuration schema/CRC, time, storage, alarms and command authorization state.

## Automatic Rollback

The bootloader keeps the previous verified slot. A candidate remains pending until the 30-second
health guard accepts diagnostics policy. Failed health calls `esp_ota_mark_app_invalid_rollback()`
and reboots into the previous verified image. Follow the controlled unhealthy-image rehearsal in
[OTA-SECURITY-PROVISIONING.md](OTA-SECURITY-PROVISIONING.md); valve power must be disconnected.

## Operator-Initiated Rollback

1. Stop irrigation and electrically verify all outputs off.
2. Record the incident, active firmware hash, health, alarms and audit log.
3. Select the last approved signed artifact and verify its archived SHA-256/signature.
4. Upload it through the same authenticated OTA path as a new candidate.
5. Let health confirmation finish without serial/DTR reset or power interruption.
6. Verify the restored configuration schema and compatibility. Do not factory-reset to solve a
   migration failure without preserving the recovery blob and obtaining engineering approval.
7. Re-run the affected FAT/SAT regression subset before returning to automatic operation.

If OTA and USB recovery are both unavailable, isolate valve power, quarantine the controller and
follow the approved manufacturing recovery instruction. Do not burn eFuses, erase NVS or bypass
signature checks as an ad hoc recovery action.
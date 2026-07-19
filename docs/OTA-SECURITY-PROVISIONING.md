# OTA Security and Provisioning

## Security Boundary

All installable application images use ESP-IDF Secure Boot V2 RSA-3072 signature blocks.
`CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT` makes ESP-IDF OTA APIs reject images that are
unsigned, corrupt or signed by a different key. Remote URL updates additionally require HTTPS
with a configured certificate or the ESP-IDF root certificate bundle.

This software-only mode protects network OTA against an attacker who does not control the
device physically. It does not prevent replacement of the bootloader through physical flash
access. Hardware Secure Boot and flash encryption are separate manufacturing controls.

## Key Management

- Development keys are generated locally with `scripts/setup-ota-signing-key.sh` and stored at
  `keys/ota_signing_key.pem`, which is excluded from Git.
- Production private keys shall be generated and retained offline or in an approved HSM. They
  shall not be copied into the repository, CI logs, firmware artifacts or controller storage.
- Release signing access shall be limited, audited and revocable. Public verification keys may
  be distributed to release tooling and manufacturing fixtures.
- Development and production devices shall use different keys. Key rotation requires an
  approved bootloader transition plan signed by a currently trusted key.

## Development Build and Upload

```sh
source ~/.espressif/v6.0.1/esp-idf/export.sh
./scripts/setup-ota-signing-key.sh
idf.py build
./scripts/ota-direct.sh 192.168.10.113 build/zmartify_irrigation.bin
```

The deployable file is `build/zmartify_irrigation.bin`. The
`build/zmartify_irrigation-unsigned.bin` intermediate must never be uploaded. The direct upload
script verifies the signature with `OTA_VERIFY_KEY` before sending any bytes.

## Rollback Provisioning

Application rollback is a bootloader capability. A controller whose existing bootloader was
built without `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` cannot gain rollback through application
OTA alone. Before field rollout, a controlled fixture shall physically flash and verify the
signed bootloader, partition table, initial OTA data and signed application from the same
approved release configuration.

The fixture procedure shall:

1. record device identity, flash ID and current recoverable image;
2. keep valve power disconnected;
3. flash the complete release image through the physical service interface;
4. verify the two-OTA partition layout and boot the baseline application;
5. install a healthy signed OTA image, verify irrigation remains inhibited while unconfirmed,
   and confirm the `ota image confirmed valid` audit event;
6. retain the previous verified slot until the new image passes its 30-second health check.

## Rollback and Interruption Tests

Build the deliberately unhealthy, still correctly signed test image separately:

```sh
idf.py -B build-ota-rollback-test -DZIC_OTA_FORCE_HEALTH_FAILURE=ON build
```

With valve power disconnected, upload that image to a rollback-provisioned controller. It must
enter `pending_confirmation`, emit `ota health failed; rollback requested`, reboot, and return
to the previously verified version. Never release or deploy this test image outside the
controlled test fixture.

For interruption testing, remove power at several points during upload and reboot. Each trial
must return either the unchanged verified image or the completely written signed candidate; a
partially written partition must never become the boot target. Store serial logs and observed
versions as FAT evidence.

## Secure Boot and Flash Encryption

Do not enable or burn eFuses from ordinary build, flash or OTA scripts. Production enablement
requires a separately approved manufacturing work instruction that includes disposable-device
rehearsal, key backup, recovery verification, dual authorization, eFuse readback and device
quarantine on any mismatch. Secure Boot and flash encryption remain disabled until that process
has passed its release gate.
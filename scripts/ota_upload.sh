#!/usr/bin/env bash

set -euo pipefail

cat >&2 <<'EOF'
ERROR: scripts/ota_upload.sh is retired.

The legacy MQTT URL command is not part of the current OTA contract. It could report success
without proving upload, boot, health confirmation or rollback. Follow docs/RELEASE-AND-ROLLBACK.md
and use only the approved authenticated OTA transport after the HTTP-AUTH release blocker closes.
EOF
exit 64

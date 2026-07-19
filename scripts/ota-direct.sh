#!/bin/bash

set -euo pipefail

DEVICE_IP="${1:-}"
FIRMWARE="${2:-build/zmartify_irrigation.bin}"

if [[ -z "$DEVICE_IP" ]]; then
    echo "Usage: $0 DEVICE_IP [FIRMWARE_BIN]"
    exit 1
fi
if [[ ! -f "$FIRMWARE" ]]; then
    echo "Error: firmware not found: $FIRMWARE"
    exit 1
fi

VERIFY_KEY="${OTA_VERIFY_KEY:-keys/ota_signing_key.pem}"
if [[ ! -f "$VERIFY_KEY" ]]; then
    echo "Error: OTA verification key not found: $VERIFY_KEY"
    echo "Set OTA_VERIFY_KEY to the matching public or private PEM key."
    exit 1
fi
if ! command -v espsecure >/dev/null 2>&1; then
    echo "Error: espsecure is unavailable; source the ESP-IDF export script first."
    exit 1
fi

verify_dir=$(mktemp -d)
trap 'rm -rf "$verify_dir"' EXIT
PUBLIC_KEY="$VERIFY_KEY"
if grep -q "PRIVATE KEY" "$VERIFY_KEY"; then
    PUBLIC_KEY="$verify_dir/public.pem"
    espsecure extract-public-key --version 2 --keyfile "$VERIFY_KEY" "$PUBLIC_KEY" >/dev/null
fi
if ! espsecure verify-signature --version 2 --keyfile "$PUBLIC_KEY" "$FIRMWARE"; then
    echo "Error: unsigned, corrupt, or untrusted firmware: $FIRMWARE"
    exit 1
fi

echo "Signature verified; uploading $FIRMWARE to http://$DEVICE_IP/ota"
curl --fail --show-error --connect-timeout 10 --max-time 180 \
    -H "Expect:" \
    -H "Content-Type: application/octet-stream" \
    --data-binary "@$FIRMWARE" \
    "http://$DEVICE_IP/ota"

echo "Waiting for controller to reboot..."
went_offline=false
for attempt in {1..15}; do
    if ! ping -c 1 -W 1000 "$DEVICE_IP" >/dev/null 2>&1; then
        went_offline=true
        break
    fi
    sleep 1
done

if [[ "$went_offline" != true ]]; then
    echo "Warning: reboot offline window was not observed"
fi

for attempt in {1..45}; do
    http_code=$(curl --silent --output /dev/null --max-time 2 \
        --write-out '%{http_code}' "http://$DEVICE_IP/logs" || true)
    if [[ "$http_code" == "200" ]]; then
        echo "Controller application is healthy at $DEVICE_IP"
        exit 0
    fi
    sleep 1
done

echo "Error: controller application did not return at $DEVICE_IP"
exit 1
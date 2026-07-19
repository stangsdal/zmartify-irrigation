#!/bin/bash

set -euo pipefail

KEY_PATH="${1:-keys/ota_signing_key.pem}"

if ! command -v espsecure >/dev/null 2>&1; then
    echo "Error: espsecure is unavailable; source the ESP-IDF export script first."
    exit 1
fi
if [[ -e "$KEY_PATH" ]]; then
    echo "Refusing to overwrite existing key: $KEY_PATH"
    exit 1
fi

mkdir -p "$(dirname "$KEY_PATH")"
umask 077
espsecure generate-signing-key --version 2 "$KEY_PATH"
chmod 600 "$KEY_PATH"
echo "Created local development OTA signing key: $KEY_PATH"
echo "This private key is ignored by Git. Do not use it as a production signing key."
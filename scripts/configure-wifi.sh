#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
IDF_EXPORT="${IDF_EXPORT:-$HOME/.espressif/v6.0.1/esp-idf/export.sh}"
PORT="${1:-}"
CREDENTIALS_FILE="$PROJECT_ROOT/main/wifi_credentials.local.h"

cd "$PROJECT_ROOT"

if [[ ! -f "$IDF_EXPORT" ]]; then
    echo "Error: ESP-IDF export script not found: $IDF_EXPORT"
    exit 1
fi

if [[ -z "$PORT" ]]; then
    shopt -s nullglob
    ports=(/dev/cu.usbmodem* /dev/cu.usbserial-* /dev/ttyUSB*)
    shopt -u nullglob
    if (( ${#ports[@]} != 1 )); then
        echo "Error: expected one USB serial port, found ${#ports[@]}."
        echo "Usage: $0 /dev/cu.usbmodem..."
        exit 1
    fi
    PORT="${ports[0]}"
fi

read -r -p "Wi-Fi SSID: " wifi_ssid
read -r -s -p "Wi-Fi password: " wifi_password
echo

if [[ -z "$wifi_ssid" ]]; then
    echo "Error: SSID cannot be empty"
    exit 1
fi
if (( ${#wifi_ssid} > 32 )); then
    echo "Error: SSID exceeds 32 bytes"
    exit 1
fi
if (( ${#wifi_password} < 8 || ${#wifi_password} > 63 )); then
    echo "Error: WPA2 password must be 8-63 bytes"
    exit 1
fi

bytes_to_c_array() {
    local value="$1"
    local bytes
    bytes=$(printf '%s' "$value" | od -An -v -tu1 | tr -s '[:space:]' ' ' | sed 's/^ //;s/ $//;s/ /, /g')
    printf '%s, 0' "$bytes"
}

umask 077
{
    echo "#pragma once"
    echo "#include <stdint.h>"
    echo "static const uint8_t zic_wifi_ssid[] = {$(bytes_to_c_array "$wifi_ssid")};"
    echo "static const uint8_t zic_wifi_password[] = {$(bytes_to_c_array "$wifi_password")};"
} > "$CREDENTIALS_FILE"

unset wifi_password

source "$IDF_EXPORT"
idf.py -B build clean
idf.py -B build build
idf.py -B build -p "$PORT" -b 921600 flash

echo "Wi-Fi firmware flashed. Credentials remain only in the ignored local file:"
echo "  $CREDENTIALS_FILE"
echo "Run './scripts/monitor.sh $PORT' to view the assigned IP address."
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
IDF_EXPORT="${IDF_EXPORT:-$HOME/.espressif/v6.0.1/esp-idf/export.sh}"
PORT="${1:-}"

cd "$PROJECT_ROOT"

if [[ ! -f "$IDF_EXPORT" ]]; then
    echo "Error: ESP-IDF export script not found: $IDF_EXPORT"
    exit 1
fi

source "$IDF_EXPORT"

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

if [[ ! -e "$PORT" ]]; then
    echo "Error: serial port does not exist: $PORT"
    exit 1
fi

echo "Relay test safety check"
echo "  - Disconnect pumps, valves, and other loads."
echo "  - Each relay will turn on alone for 500 ms."
echo "  - All relays are forced off before, between, and after pulses."
echo "  - Press Ctrl+] after the test completes to restore normal firmware."
echo
read -r -p "Type TEST to continue: " confirmation
if [[ "$confirmation" != "TEST" ]]; then
    echo "Relay test cancelled."
    exit 1
fi

echo "Building normal firmware..."
idf.py -B build build

echo "Building isolated relay-test firmware..."
idf.py -B build-relay-test -D ZIC_RELAY_SELF_TEST=ON build

restore_normal_firmware() {
    echo
    echo "Restoring normal firmware..."
    idf.py -B build -p "$PORT" -b 921600 flash
    echo "Normal firmware restored."
}

trap restore_normal_firmware EXIT

echo "Flashing relay-test firmware..."
idf.py -B build-relay-test -p "$PORT" -b 921600 flash

echo "Monitoring relay test. Press Ctrl+] after 'all relays OFF'."
idf.py -B build-relay-test -p "$PORT" monitor
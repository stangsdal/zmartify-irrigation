#!/bin/bash
# ota_upload.sh - Upload firmware to a live device via MQTT OTA command
#
# Usage: ./scripts/ota_upload.sh [MQTT_BROKER] [FIRMWARE_URL]
#
# Publishes to: zmartify/irrigation/controller_01/command/ota
# Payload:      {"url":"<firmware_url>"}
#
# Requirements: mosquitto_pub (brew install mosquitto)
#               or use the MQTT broker web UI / MQTT Explorer

set -euo pipefail

BROKER="${1:-mqtt://192.168.10.2:1883}"
FIRMWARE_URL="${2:-}"

if [[ "$FIRMWARE_URL" != https://* ]]; then
    echo "ERROR: a trusted HTTPS firmware URL is required."
    echo "Usage: $0 [MQTT_BROKER] https://updates.example/firmware.bin"
    exit 1
fi

TOPIC="zmartify/irrigation/controller_01/command/ota"
PAYLOAD="{\"url\":\"${FIRMWARE_URL}\"}"

echo "================================="
echo "Zmartify OTA Upload"
echo "================================="
echo "Broker:  $BROKER"
echo "Topic:   $TOPIC"
echo "URL:     $FIRMWARE_URL"
echo ""

if ! command -v mosquitto_pub &>/dev/null; then
    echo "ERROR: mosquitto_pub not found."
    echo "Install: brew install mosquitto"
    exit 1
fi

echo "Publishing OTA command..."
mosquitto_pub -L "${BROKER}/${TOPIC}" \
              -m "$PAYLOAD" \
              -q 1 \
              -d

echo ""
echo "OTA command sent. Monitor device serial output for progress."
echo "The device will reboot automatically after successful update."

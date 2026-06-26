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

# If no URL given, try to construct one from a local HTTP server
if [ -z "$FIRMWARE_URL" ]; then
    LOCAL_IP=$(ipconfig getifaddr en0 2>/dev/null || ip route get 1 | awk '{print $NF;exit}' 2>/dev/null || echo "192.168.10.1")
    FIRMWARE_URL="http://${LOCAL_IP}:8070/zmartify_irrigation.bin"
    echo "No URL given – defaulting to: $FIRMWARE_URL"
    echo ""
    echo "Start a local HTTP server first:"
    echo "  cd build && python3 -m http.server 8070"
    echo ""
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

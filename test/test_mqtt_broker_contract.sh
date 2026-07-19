#!/bin/sh
set -eu

: "${MQTT_HOST:?Set MQTT_HOST to the authenticated test broker}"
: "${MQTT_USER:?Set MQTT_USER}"
: "${MQTT_PASSWORD:?Set MQTT_PASSWORD}"
: "${MQTT_CA:?Set MQTT_CA to the broker CA file}"

MQTT_PORT="${MQTT_PORT:-8883}"
DEVICE_ID="${DEVICE_ID:-zmartify-irrigation-01}"
ROOT="zmartify/v2/devices/${DEVICE_ID}"
OUTCOME_TOPIC="${ROOT}/events/irrigation/outcome"
COMMAND_TOPIC="${ROOT}/commands/irrigation/rain_delay"
COMMAND_ID="broker-test-$(date -u +%Y%m%dT%H%M%SZ)"
TIMESTAMP="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

mqtt_sub() {
    mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" --cafile "$MQTT_CA" \
        -u "$MQTT_USER" -P "$MQTT_PASSWORD" "$@"
}

mqtt_pub() {
    mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" --cafile "$MQTT_CA" \
        -u "$MQTT_USER" -P "$MQTT_PASSWORD" "$@"
}

mqtt_sub -t "${ROOT}/status" -C 1 -W 10 | grep -q '"status":"online"'
mqtt_sub -t "${ROOT}/state/reported" -C 1 -W 10 | grep -q '"schema_version":"2.0"'

mqtt_sub -t "$OUTCOME_TOPIC" -C 1 -W 10 >"$TMP_DIR/malformed.out" &
subscriber_pid=$!
mqtt_pub -q 1 -t "$COMMAND_TOPIC" -m '{'
wait "$subscriber_pid"
grep -q '"event_type":"command.rejected"' "$TMP_DIR/malformed.out"
grep -q '"detail":"invalid_json"' "$TMP_DIR/malformed.out"

mqtt_sub -t "$OUTCOME_TOPIC" -C 3 -W 15 >"$TMP_DIR/duplicate.out" &
subscriber_pid=$!
PAYLOAD="{\"command_id\":\"${COMMAND_ID}\",\"source_timestamp\":\"${TIMESTAMP}\",\"parameters\":{\"delay_hours\":0}}"
mqtt_pub -q 1 -t "$COMMAND_TOPIC" -m "$PAYLOAD"
mqtt_pub -q 1 -t "$COMMAND_TOPIC" -m "$PAYLOAD"
wait "$subscriber_pid"
grep -q '"event_type":"command.accepted"' "$TMP_DIR/duplicate.out"
grep -q '"event_type":"rain.delay_cleared"' "$TMP_DIR/duplicate.out"
grep -q '"event_type":"command.duplicate"' "$TMP_DIR/duplicate.out"

printf '%s\n' "MQTT broker contract passed without valve activation"

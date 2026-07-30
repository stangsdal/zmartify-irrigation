#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"
source "$SCRIPT_DIR/esp-idf-env.sh"

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

firmware_size_bytes=$(wc -c < "$FIRMWARE")
expected_firmware_version="${ZIC_EXPECTED_FIRMWARE_VERSION:-$(sed -n 's/^CONFIG_APP_PROJECT_VER="\(.*\)"$/\1/p' "$PROJECT_ROOT/sdkconfig.defaults" | head -n 1)}"

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
auth_args=()
if [[ -n "${ZIC_HTTP_ADMIN_TOKEN:-}" ]]; then
    auth_args=(-H "Authorization: Bearer ${ZIC_HTTP_ADMIN_TOKEN}")
else
    echo "Error: set ZIC_HTTP_ADMIN_TOKEN for authenticated OTA upload"
    exit 1
fi
min_upload_rate_bytes_per_s=${ZIC_OTA_MIN_UPLOAD_RATE_BPS:-1024}
if [[ "$min_upload_rate_bytes_per_s" -le 0 ]]; then
    echo "Error: ZIC_OTA_MIN_UPLOAD_RATE_BPS must be > 0"
    exit 1
fi
calculated_upload_timeout_seconds=$(( ((firmware_size_bytes + min_upload_rate_bytes_per_s - 1) / min_upload_rate_bytes_per_s) + 300 ))
default_upload_timeout_seconds=$(( calculated_upload_timeout_seconds > 1800 ? calculated_upload_timeout_seconds : 1800 ))
upload_timeout_seconds=${ZIC_OTA_MAX_TIME:-$default_upload_timeout_seconds}
echo "Using OTA upload timeout: ${upload_timeout_seconds}s for ${firmware_size_bytes} bytes"
curl --fail --show-error --connect-timeout 10 --max-time "$upload_timeout_seconds" \
    -H "Expect:" \
    -H "Content-Type: application/octet-stream" \
    "${auth_args[@]}" \
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
        break
    fi
    sleep 1
done

if [[ "${http_code:-}" != "200" ]]; then
    echo "Error: controller application did not return at $DEVICE_IP"
    exit 1
fi

health_retry_count=${ZIC_OTA_HEALTH_RETRIES:-60}
for ((attempt=1; attempt<=health_retry_count; attempt++)); do
    health_json=$(curl --silent --show-error --max-time 3 "http://$DEVICE_IP/health" || true)
    if [[ -n "$health_json" ]]; then
        check_result=$(HEALTH_JSON="$health_json" EXPECTED_FW="$expected_firmware_version" python3 - <<'PY'
import json
import os

payload = os.environ.get("HEALTH_JSON", "")
expected = os.environ.get("EXPECTED_FW", "")

try:
    data = json.loads(payload)
except json.JSONDecodeError:
    print("wait")
    print("invalid health json")
    raise SystemExit(0)

problems = []
firmware_version = str(data.get("firmware_version") or "")
if expected and firmware_version != expected:
    problems.append(f"firmware_version={firmware_version or 'missing'} expected={expected}")
if data.get("runtime") not in (None, "healthy"):
    problems.append(f"runtime={data.get('runtime')}")
if data.get("communications") not in (None, "healthy"):
    problems.append(f"communications={data.get('communications')}")
if not bool(data.get("mqtt_connected")):
    problems.append("mqtt_connected=false")
if not bool(data.get("time_synchronized")):
    problems.append("time_synchronized=false")

if problems:
    print("wait")
    print("; ".join(problems))
else:
    print("ok")
    print(firmware_version)
PY
)
        result_status=$(printf '%s\n' "$check_result" | sed -n '1p')
        result_detail=$(printf '%s\n' "$check_result" | sed -n '2p')
        if [[ "$result_status" == "ok" ]]; then
            echo "Controller OTA verified at $DEVICE_IP"
            echo "Firmware version: ${result_detail:-unknown}"
            curl --silent --max-time 3 "http://$DEVICE_IP/logs" | tail -n 20
            exit 0
        fi
    fi
    sleep 1
done

echo "Error: controller health did not reach the expected post-OTA state"
if [[ -n "${result_detail:-}" ]]; then
    echo "Last health mismatch: $result_detail"
fi
exit 1
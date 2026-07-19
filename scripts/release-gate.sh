#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BLOCKERS_FILE="$PROJECT_ROOT/release/open-deviations.tsv"
HOST_BUILD_DIR="${HOST_BUILD_DIR:-$PROJECT_ROOT/build-host-release}"
FIRMWARE_BUILD_DIR="${FIRMWARE_BUILD_DIR:-$PROJECT_ROOT/build-release}"
SIGNING_KEY="${SIGNING_KEY:-$PROJECT_ROOT/keys/ota_signing_key.pem}"
MANIFEST_PATH="${MANIFEST_PATH:-$FIRMWARE_BUILD_DIR/release-manifest.txt}"
APP_PARTITION_SIZE=$((0x1a9000))

mode="release"
case "${1:-}" in
    --software-only) mode="software" ;;
    --check-blockers) mode="blockers" ;;
    --release|"") mode="release" ;;
    *)
        echo "Usage: $0 [--software-only|--check-blockers|--release]" >&2
        exit 64
        ;;
esac

cd "$PROJECT_ROOT"

check_blockers()
{
    if [[ ! -f "$BLOCKERS_FILE" ]]; then
        echo "BLOCKED: deviation register missing: $BLOCKERS_FILE" >&2
        return 2
    fi

    local open_count
    open_count="$(awk -F '\t' 'NR > 1 && $2 == "OPEN" && $3 == "BLOCKER" { count++ } END { print count + 0 }' "$BLOCKERS_FILE")"
    if [[ "$open_count" -gt 0 ]]; then
        echo "RELEASE BLOCKED: $open_count open blocker(s)"
        awk -F '\t' 'NR > 1 && $2 == "OPEN" && $3 == "BLOCKER" {
            printf "  %-22s %s\n", $1, $4
        }' "$BLOCKERS_FILE"
        return 2
    fi

    echo "Release deviations: no open blockers"
}

if [[ "$mode" == "blockers" ]]; then
    check_blockers
    exit $?
fi

if [[ "$mode" == "release" ]]; then
    check_blockers
fi

if [[ "${ALLOW_DIRTY:-0}" != "1" && -n "$(git status --porcelain)" ]]; then
    echo "BLOCKED: working tree is not clean (set ALLOW_DIRTY=1 only for development rehearsal)" >&2
    exit 2
fi

if [[ ! -f "$SIGNING_KEY" ]]; then
    echo "BLOCKED: signing key unavailable: $SIGNING_KEY" >&2
    exit 2
fi
if ! command -v idf.py >/dev/null 2>&1 || ! command -v espsecure >/dev/null 2>&1; then
    echo "BLOCKED: source the ESP-IDF environment before running the release gate" >&2
    exit 2
fi

echo "[1/6] Patch integrity"
git diff --check

echo "[2/6] Fresh host configure/build/test"
cmake -E remove_directory "$HOST_BUILD_DIR"
cmake -S . -B "$HOST_BUILD_DIR" -DZIC_HOST_TESTS=ON
cmake --build "$HOST_BUILD_DIR"
ctest --test-dir "$HOST_BUILD_DIR" --output-on-failure

echo "[3/6] Fresh signed ESP32-S3 build"
cmake -E remove_directory "$FIRMWARE_BUILD_DIR"
idf.py -B "$FIRMWARE_BUILD_DIR" build

firmware="$FIRMWARE_BUILD_DIR/zmartify_irrigation.bin"
if [[ ! -f "$firmware" ]]; then
    echo "BLOCKED: signed firmware artifact missing" >&2
    exit 2
fi

echo "[4/6] Signing and security configuration"
grep -qx 'CONFIG_IDF_TARGET="esp32s3"' sdkconfig
grep -qx 'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y' sdkconfig
grep -qx 'CONFIG_PARTITION_TABLE_TWO_OTA_LARGE=y' sdkconfig
grep -qx 'CONFIG_SPIRAM=y' sdkconfig
grep -qx 'CONFIG_MQTT_PROTOCOL_5=y' sdkconfig
grep -qx 'CONFIG_SECURE_SIGNED_ON_UPDATE=y' sdkconfig
grep -qx 'CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME=y' sdkconfig
grep -qx '# CONFIG_SECURE_BOOT is not set' sdkconfig
grep -qx '# CONFIG_FLASH_ENCRYPTION_ENABLED is not set' sdkconfig
espsecure verify-signature --version 2 --keyfile "$SIGNING_KEY" "$firmware"

echo "[5/6] Artifact size and identity"
firmware_size="$(stat -f%z "$firmware" 2>/dev/null || stat -c%s "$firmware")"
if (( firmware_size >= APP_PARTITION_SIZE )); then
    echo "BLOCKED: firmware size $firmware_size exceeds app partition $APP_PARTITION_SIZE" >&2
    exit 2
fi
firmware_sha256="$(shasum -a 256 "$firmware" | awk '{print $1}')"
headroom_bytes=$((APP_PARTITION_SIZE - firmware_size))
headroom_percent=$((headroom_bytes * 100 / APP_PARTITION_SIZE))

mkdir -p "$(dirname "$MANIFEST_PATH")"
{
    echo "release_gate_version=1"
    echo "git_commit=$(git rev-parse HEAD)"
    echo "git_dirty=$([[ -n "$(git status --porcelain)" ]] && echo true || echo false)"
    echo "idf_target=esp32s3"
    echo "firmware_path=${firmware#$PROJECT_ROOT/}"
    echo "firmware_sha256=$firmware_sha256"
    echo "firmware_size_bytes=$firmware_size"
    echo "app_partition_size_bytes=$APP_PARTITION_SIZE"
    echo "app_partition_headroom_bytes=$headroom_bytes"
    echo "app_partition_headroom_percent=$headroom_percent"
    echo "signed_on_update=true"
    echo "secure_boot_enabled=false"
    echo "flash_encryption_enabled=false"
} > "$MANIFEST_PATH"
echo "Manifest: $MANIFEST_PATH"

echo "[6/6] Release disposition"
if [[ "$mode" == "software" ]]; then
    echo "SOFTWARE GATE PASSED: $firmware_sha256"
    exit 0
fi

check_blockers
echo "RELEASE GATE PASSED: $firmware_sha256"
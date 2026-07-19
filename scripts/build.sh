#!/bin/bash
# build.sh - Clean build for Zmartify Irrigation Controller
#
# Usage: ./scripts/build.sh [target]
# Examples:
#   ./scripts/build.sh          # Build with current target
#   ./scripts/build.sh esp32s3  # Set target to esp32s3 first, then build

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

if [[ ! -f "keys/ota_signing_key.pem" ]]; then
    echo "Error: OTA signing key not found: keys/ota_signing_key.pem"
    echo "Create a local development key with: ./scripts/setup-ota-signing-key.sh"
    exit 1
fi

TARGET="${1:-}"
CLEAN="${CLEAN:-0}"

echo "================================"
echo "Zmartify Irrigation Controller"
echo "Clean Build Script v5.0"
echo "================================"
echo ""

# Show target
if [ -z "$TARGET" ]; then
    echo "Building with current target..."
    TARGET=$(grep "^CONFIG_IDF_TARGET=" sdkconfig 2>/dev/null | cut -d'=' -f2)
    if [ -z "$TARGET" ]; then
        echo "Error: No target specified and sdkconfig not configured"
        echo "Usage: $0 [target]"
        echo "Example: $0 esp32s3"
        exit 1
    fi
else
    echo "Setting target to: $TARGET"
    idf.py set-target "$TARGET" || exit 1
fi

echo "Target: $TARGET"
echo ""

# Clean if requested
if [ "$CLEAN" = "1" ]; then
    echo "Cleaning build directory..."
    rm -rf build/
    echo ""
fi

# Build
echo "Building project..."
idf.py build

# Report size
echo ""
echo "Build Complete!"
echo ""
echo "Build outputs:"
ls -lh build/zmartify_irrigation.bin 2>/dev/null || echo "Binary: build/*.bin"
echo ""

# Memory usage
echo "Checking binary size..."
if [ -f "build/zmartify_irrigation.bin" ]; then
    SIZE=$(stat -f%z "build/zmartify_irrigation.bin" 2>/dev/null || stat -c%s "build/zmartify_irrigation.bin" 2>/dev/null)
    echo "Firmware size: $((SIZE / 1024)) KB (target: < 1800 KB)"
fi

echo ""
echo "Next steps:"
echo "  1. Flash device:   ./scripts/flash.sh"
echo "  2. Monitor output: ./scripts/monitor.sh"
echo "  3. Size analysis:  ./scripts/size-report.sh"

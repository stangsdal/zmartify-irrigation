#!/usr/bin/env bash
# flash.sh - Manufacturing/recovery USB flash for Zmartify Irrigation Controller
#
# Usage: ./scripts/flash.sh usb [port]
# Examples:
#   ./scripts/flash.sh usb          # Flash via USB (auto-detect port)
#   ./scripts/flash.sh usb /dev/ttyUSB0

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

METHOD="${1:-usb}"
PARAM="${2:-}"

echo "================================"
echo "Zmartify Irrigation Controller"
echo "Flash Script v5.0"
echo "================================"
echo ""

if [ ! -f "build/zmartify_irrigation.bin" ]; then
    echo "Error: Firmware not built!"
    echo "Run: ./scripts/build.sh"
    exit 1
fi

case "$METHOD" in
    usb)
        echo "Flashing via USB..."
        
        if [ -z "$PARAM" ]; then
            # Auto-detect port
            if command -v ls &> /dev/null; then
                PORTS=($(ls /dev/tty.usbserial-* 2>/dev/null || ls /dev/ttyUSB* 2>/dev/null || echo ""))
                if [ ${#PORTS[@]} -eq 0 ]; then
                    echo "Error: No USB device found"
                    echo "Please connect device and ensure USB driver is installed"
                    exit 1
                elif [ ${#PORTS[@]} -eq 1 ]; then
                    PORT="${PORTS[0]}"
                else
                    echo "Multiple USB devices found:"
                    for i in "${!PORTS[@]}"; do
                        echo "  [$((i+1))] ${PORTS[$i]}"
                    done
                    read -p "Select port (1-${#PORTS[@]}): " PORT_NUM
                    PORT="${PORTS[$((PORT_NUM-1))]}"
                fi
            fi
        else
            PORT="$PARAM"
        fi
        
        echo "Using port: $PORT"
        echo ""
        echo "Flashing firmware..."
        
        idf.py -p "$PORT" -b 921600 flash
        ;;
        
    ota)
        echo "Error: legacy multipart OTA is retired." >&2
        echo "Follow docs/RELEASE-AND-ROLLBACK.md after the HTTP-AUTH blocker closes." >&2
        exit 64
        ;;
        
    *)
        echo "Error: Unknown method '$METHOD'"
        echo ""
        echo "Usage: $0 usb [port]"
        echo ""
        echo "Methods:"
        echo "  usb [port]      Flash via USB (auto-detect if no port specified)"
        echo ""
        echo "Examples:"
        echo "  $0 usb"
        echo "  $0 usb /dev/ttyUSB0"
        exit 1
        ;;
esac

echo ""
echo "Flash Complete!"

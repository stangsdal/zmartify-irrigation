#!/bin/bash
# flash.sh - Flash Zmartify Irrigation Controller firmware
#
# Usage: ./scripts/flash.sh [method] [port]
# Methods: usb, ota
# Examples:
#   ./scripts/flash.sh usb          # Flash via USB (auto-detect port)
#   ./scripts/flash.sh usb /dev/ttyUSB0
#   ./scripts/flash.sh ota http://192.168.10.57/ota

set -e

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
        
        idf.py -p "$PORT" -b 921600 flash monitor
        ;;
        
    ota)
        echo "Flashing via OTA..."
        
        if [ -z "$PARAM" ]; then
            echo "Error: OTA URL required"
            echo "Usage: $0 ota <url>"
            echo "Example: $0 ota http://192.168.10.57/ota"
            exit 1
        fi
        
        OTA_URL="$PARAM"
        FIRMWARE="build/zmartify_irrigation.bin"
        
        echo "OTA URL: $OTA_URL"
        echo "Firmware: $FIRMWARE"
        echo ""
        
        # Extract host and path
        HOST=$(echo "$OTA_URL" | cut -d'/' -f3)
        PATH=$(echo "$OTA_URL" | cut -d'/' -f4-)
        
        echo "Uploading firmware to $HOST..."
        
        # Check if endpoint is available
        if curl --connect-timeout 2 -s "http://$HOST/" > /dev/null 2>&1; then
            # Upload firmware
            curl -X POST \
                --form "firmware=@$FIRMWARE" \
                --form "action=update_firmware" \
                "http://$HOST/ota" \
                -v
            
            echo ""
            echo "OTA update initiated"
            echo "Device will reboot and apply update"
        else
            echo "Error: Cannot reach OTA endpoint at $OTA_URL"
            echo "Verify device is online and endpoint is correct"
            exit 1
        fi
        ;;
        
    *)
        echo "Error: Unknown method '$METHOD'"
        echo ""
        echo "Usage: $0 [method] [param]"
        echo ""
        echo "Methods:"
        echo "  usb [port]      Flash via USB (auto-detect if no port specified)"
        echo "  ota <url>       Flash via OTA to specified URL"
        echo ""
        echo "Examples:"
        echo "  $0 usb"
        echo "  $0 usb /dev/ttyUSB0"
        echo "  $0 ota http://192.168.10.57/ota"
        exit 1
        ;;
esac

echo ""
echo "Flash Complete!"

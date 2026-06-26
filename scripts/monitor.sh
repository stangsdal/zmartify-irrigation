#!/bin/bash
# monitor.sh - Monitor serial output from Zmartify device
#
# Usage: ./scripts/monitor.sh [port] [baud]
# Examples:
#   ./scripts/monitor.sh              # Auto-detect port, 115200 baud
#   ./scripts/monitor.sh /dev/ttyUSB0 # Specific port, 115200 baud
#   ./scripts/monitor.sh /dev/ttyUSB0 921600

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

PORT="${1:-}"
BAUD="${2:-115200}"

echo "================================"
echo "Zmartify Serial Monitor"
echo "================================"
echo ""

if [ -z "$PORT" ]; then
    # Auto-detect port
    if [[ "$OSTYPE" == "darwin"* ]]; then
        PORTS=($(ls /dev/tty.usbserial-* 2>/dev/null || echo ""))
    else
        PORTS=($(ls /dev/ttyUSB* 2>/dev/null || ls /dev/ttyACM* 2>/dev/null || echo ""))
    fi
    
    if [ ${#PORTS[@]} -eq 0 ]; then
        echo "Error: No USB device found"
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

echo "Connecting to: $PORT @ $BAUD baud"
echo "Press Ctrl+C to exit"
echo ""

idf.py -p "$PORT" -b "$BAUD" monitor

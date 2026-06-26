#!/bin/bash
# size-report.sh - Generate binary size analysis report
#
# Usage: ./scripts/size-report.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

echo "================================"
echo "Zmartify Binary Size Analysis"
echo "================================"
echo ""

if [ ! -f "build/zmartify_irrigation.elf" ]; then
    echo "Error: Firmware ELF file not found"
    echo "Run: ./scripts/build.sh"
    exit 1
fi

echo "Generating size report..."
echo ""

# Total sizes
BINSIZE=$(stat -f%z "build/zmartify_irrigation.bin" 2>/dev/null || stat -c%s "build/zmartify_irrigation.bin" 2>/dev/null)
ELFSIZE=$(stat -f%z "build/zmartify_irrigation.elf" 2>/dev/null || stat -c%s "build/zmartify_irrigation.elf" 2>/dev/null)

echo "Binary Sizes:"
echo "  Firmware binary: $((BINSIZE / 1024)) KB"
echo "  ELF file: $((ELFSIZE / 1024)) KB"
echo "  Target limit: 1800 KB"
echo ""

# Detailed analysis
echo "Detailed Analysis (top 20 symbols by size):"
echo ""

arm-none-eabi-nm -S --size-sort "build/zmartify_irrigation.elf" | tail -20 | while read addr size name; do
    if [ ! -z "$size" ]; then
        printf "  %8s bytes: %s\n" "$size" "$name"
    fi
done

echo ""
echo "Section breakdown:"
echo ""

arm-none-eabi-size -A "build/zmartify_irrigation.elf" | grep -E "^(\.text|\.data|\.bss|\.rodata)" | awk '{
    printf "  %-12s %10d bytes\n", $1, $2
}'

echo ""
echo "Memory estimate for ESP32-S3 (8MB):"
echo "  Flash available: ~1800 KB (APP partition)"
echo "  RAM available: 512 KB"
echo ""

# Utilization percentage
PERCENT=$((BINSIZE * 100 / (1800 * 1024)))
echo "Flash utilization: $PERCENT% of APP partition"

if [ $PERCENT -gt 90 ]; then
    echo "⚠️  WARNING: Approaching flash limit!"
elif [ $PERCENT -gt 95 ]; then
    echo "🚨 CRITICAL: Exceeding recommended flash utilization!"
fi

echo ""
echo "Recommendations:"
echo "  - Review object files in build/CMakeFiles/app.dir"
echo "  - Check for large data tables or strings"
echo "  - Consider code optimization if > 80% utilization"

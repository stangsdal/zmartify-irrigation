#!/usr/bin/env bash

IDF_EXPORT="${IDF_EXPORT:-$HOME/.espressif/v6.0.1/esp-idf/export.sh}"

if [[ ! -f "$IDF_EXPORT" ]]; then
    echo "Error: ESP-IDF export script not found: $IDF_EXPORT" >&2
    echo "Set IDF_EXPORT to the ESP-IDF export.sh path." >&2
    return 1 2>/dev/null || exit 1
fi

# Keep firmware builds on ESP-IDF's Python/esptool/Ninja toolchain, not any
# Python environment that happens to be first in PATH.
source "$IDF_EXPORT" >/dev/null
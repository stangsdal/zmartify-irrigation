#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_FILE="$PROJECT_ROOT/main/http_auth.local.h"

token="${ZIC_HTTP_ADMIN_TOKEN:-}"
if [[ $# -gt 0 ]]; then
    token="$1"
fi

if [[ -z "$token" ]]; then
    echo "Usage: ZIC_HTTP_ADMIN_TOKEN=... $0" >&2
    echo "   or: $0 ADMIN_TOKEN" >&2
    exit 64
fi
if [[ ${#token} -gt 96 ]]; then
    echo "Error: admin token must be at most 96 bytes" >&2
    exit 64
fi
case "$token" in
    *[[:space:]]*)
        echo "Error: admin token must not contain whitespace" >&2
        exit 64
        ;;
esac

digest_hex="$(printf '%s' "$token" | shasum -a 256 | awk '{print $1}')"
array=""
for ((index = 0; index < ${#digest_hex}; index += 2)); do
    if [[ -n "$array" ]]; then
        array+=", "
    fi
    array+="0x${digest_hex:index:2}"
done

umask 077
cat > "$OUTPUT_FILE" <<EOF
#pragma once

#include <stdint.h>

static const uint8_t zic_http_admin_token_sha256[32] = {
    $array
};
EOF

echo "Wrote HTTP admin verifier to ${OUTPUT_FILE#$PROJECT_ROOT/}"
echo "Keep the token in your password manager; it is not recoverable from this file."
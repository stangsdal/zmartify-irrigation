#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

set +e
output="$($PROJECT_ROOT/scripts/release-gate.sh --check-blockers 2>&1)"
exit_code=$?
set -e

if [[ "$exit_code" -ne 2 ]]; then
    echo "Expected blocker check to exit 2, got $exit_code" >&2
    echo "$output" >&2
    exit 1
fi

grep -q "HTTP-AUTH" <<<"$output"
grep -q "FAT-PRODUCTION" <<<"$output"
grep -q "SAT-INSTALLATION" <<<"$output"
grep -q "RELEASE BLOCKED" <<<"$output"
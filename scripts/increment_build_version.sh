#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VERSION_TXT="$BUILD_DIR/version.txt"
GENERATED_DIR="$BUILD_DIR/generated"
VERSION_HEADER="$GENERATED_DIR/build_version.h"

mkdir -p "$BUILD_DIR" "$GENERATED_DIR"

current_version=0
if [[ -f "$VERSION_TXT" ]]; then
    raw_value="$(tr -d '[:space:]' < "$VERSION_TXT")"
    if [[ "$raw_value" =~ ^[0-9]+$ ]]; then
        current_version="$raw_value"
    fi
fi

next_version=$((current_version + 1))
printf '%s\n' "$next_version" > "$VERSION_TXT"

cat > "$VERSION_HEADER" <<EOF
#pragma once
#define BOWLING_BUILD_VERSION "${next_version}"
EOF

printf 'Build version: %s\n' "$next_version"

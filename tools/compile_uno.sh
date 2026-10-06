#!/bin/sh
# Build only. Never detect a port or upload to a connected feeder.
set -eu
cd "$(dirname "$0")/.."
uno_build=$(mktemp -d "${TMPDIR:-/tmp}/feeder-uno.XXXXXX")
trap 'rm -rf "$uno_build"' EXIT HUP INT TERM
# A unique path prevents normal/experimental build flags from sharing outputs.
arduino-cli compile --fqbn arduino:avr:uno --warnings all \
  --build-path "$uno_build" "$@" AutoFeeder

#!/bin/sh
# Compile only. No port discovery, upload, reset or hardware I/O.
set -eu
cd "$(dirname "$0")/.."
case "${1:-default}" in
  default) ./tools/compile_uno.sh ;;
  experimental)
    ./tools/compile_uno.sh --build-property \
      'compiler.cpp.extra_flags=-DENABLE_PLATE_JIGGLE=1 -DRUN_PLATE_JIGGLE_ONCE_AT_HOME=1 -DENABLE_DEBUG_PIN_TRACE=1'
    ;;
  uart-rejection)
    log=$(mktemp "${TMPDIR:-/tmp}/feeder-uart.XXXXXX")
    trap 'rm -f "$log"' EXIT HUP INT TERM
    if ./tools/compile_uno.sh --build-property \
        'compiler.cpp.extra_flags=-DENABLE_UART_TELEMETRY=1' >"$log" 2>&1; then
      cat "$log"
      echo 'FAIL: UART-enabled build unexpectedly succeeded' >&2
      exit 1
    fi
    if ! grep -F 'UART TX conflicts with the existing D1 mode switch; keep UART disabled.' "$log" >/dev/null; then
      cat "$log"
      echo 'FAIL: build failed without the expected D1 UART conflict' >&2
      exit 1
    fi
    echo 'PASS: UART-enabled build rejected for the expected D1 conflict'
    ;;
  *) echo 'Usage: check_uno.sh [default|experimental|uart-rejection]' >&2; exit 2 ;;
esac

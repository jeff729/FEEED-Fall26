#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
git diff --check 3fa818db2e62f5d08bb78fadfa861dc6b18c0f4f HEAD
git diff --check
git diff --cached --check
for script in tests/run_tests.sh tools/*.sh; do sh -n "$script"; done
if git ls-files | grep -E '\.(elf|hex|o|bin|exe)$|(^|/)\.env($|\.)'; then
  echo 'FAIL: generated binary or environment file is tracked' >&2
  exit 1
fi
echo 'PASS: whitespace, shell syntax, and tracked artifact checks'

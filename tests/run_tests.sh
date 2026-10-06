#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_build=$(mktemp -d "${TMPDIR:-/tmp}/fee ed-tests.XXXXXX")
trap 'rm -rf "$test_build"' EXIT HUP INT TERM
${CXX:-c++} -std=c++11 -Wall -Wextra -Itests/stubs -IAutoFeeder \
  tests/test_kinematics.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp \
  -o "$test_build/kinematics"
"$test_build/kinematics"

#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_build=$(mktemp -d "${TMPDIR:-/tmp}/feeder-tests.XXXXXX")
trap 'rm -rf "$test_build"' EXIT HUP INT TERM
${CXX:-c++} -std=c++11 -Wall -Wextra -Itests/stubs -IAutoFeeder \
  tests/test_kinematics.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp \
  -o "$test_build/kinematics"
"$test_build/kinematics"

${CXX:-c++} -std=c++11 -Wall -Wextra -Werror -IAutoFeeder tests/test_control.cpp AutoFeeder/Control.cpp AutoFeeder/kinematics.cpp -o "$test_build/control"
"$test_build/control"

${CXX:-c++} -std=c++11 -Wall -Wextra -Werror -Itests/stubs -IAutoFeeder tests/test_firmware.cpp AutoFeeder/Firmware.cpp AutoFeeder/Control.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp AutoFeeder/DCMotor.cpp AutoFeeder/Joystick.cpp -o "$test_build/firmware"
"$test_build/firmware"

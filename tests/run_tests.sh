#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_build=$(mktemp -d "${TMPDIR:-/tmp}/feeder-tests.XXXXXX")
trap 'rm -rf "$test_build"' EXIT HUP INT TERM
if [ "${SANITIZE:-0}" = 1 ]; then
  export CXXFLAGS="${CXXFLAGS:-} -fsanitize=address,undefined -fno-omit-frame-pointer"
fi
${CXX:-c++} ${CXXFLAGS:-} -std=c++11 -Wall -Wextra -Werror -Itests/stubs -IAutoFeeder \
  tests/test_kinematics.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp \
  -o "$test_build/kinematics"
"$test_build/kinematics"

${CXX:-c++} ${CXXFLAGS:-} -std=c++11 -Wall -Wextra -Werror -IAutoFeeder tests/test_control.cpp AutoFeeder/Control.cpp AutoFeeder/kinematics.cpp -o "$test_build/control"
"$test_build/control"

${CXX:-c++} ${CXXFLAGS:-} -std=c++11 -Wall -Wextra -Werror -Itests/stubs -IAutoFeeder tests/test_firmware.cpp AutoFeeder/Firmware.cpp AutoFeeder/Control.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp AutoFeeder/DCMotor.cpp AutoFeeder/Joystick.cpp -o "$test_build/firmware"
"$test_build/firmware"

${CXX:-c++} ${CXXFLAGS:-} -std=c++11 -Wall -Wextra -Werror -DENABLE_PLATE_JIGGLE=1 -DENABLE_DEBUG_PIN_TRACE=1 -Itests/stubs -IAutoFeeder tests/test_firmware.cpp AutoFeeder/Firmware.cpp AutoFeeder/Control.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp AutoFeeder/DCMotor.cpp AutoFeeder/Joystick.cpp -o "$test_build/jiggle"
"$test_build/jiggle"

${CXX:-c++} ${CXXFLAGS:-} -std=c++11 -Wall -Wextra -Werror -DENABLE_PLATE_JIGGLE=1 -DRUN_PLATE_JIGGLE_ONCE_AT_HOME=1 -Itests/stubs -IAutoFeeder tests/test_firmware.cpp AutoFeeder/Firmware.cpp AutoFeeder/Control.cpp AutoFeeder/kinematics.cpp AutoFeeder/Profile.cpp AutoFeeder/DCMotor.cpp AutoFeeder/Joystick.cpp -o "$test_build/lab-boot"
"$test_build/lab-boot"

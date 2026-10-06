# F.E.E.E.D. — Fall 2026 existing-hardware firmware

UMD's assistive feeder uses its existing two-servo spoon arm, DC plate motor,
main button, joystick/button, four-profile potentiometer, mode switch and LED.
This branch improves the existing Arduino Uno software. It adds no hardware,
changes no pin assignments and does not add vision, networking or a companion
computer. `main` remains the original file-import baseline; work is on
`fall26-development`.

## Build and software checks

Arduino CLI 1.5.1, AVR core 1.8.8 and Servo 1.2.2 were used for verification.
Install with your package manager or the official CLI installer, then:

```sh
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.8
arduino-cli lib install Servo@1.2.2
./tools/compile_uno.sh
./tests/run_tests.sh
SANITIZE=1 ./tests/run_tests.sh
```

See [Arduino CLI setup](https://docs.arduino.cc/arduino-cli/getting-started/).
The scripts build in temporary directories and never upload firmware or
select a physical port. For manual IDE builds, open `AutoFeeder/AutoFeeder.ino`
and select Arduino Uno. Keep the entire AutoFeeder folder together.

## Existing connections

`AutoFeeder/Config.h` is the authoritative unchanged pin/tuning reference.
D1 is the mode switch, D2 main button, D5/D6 the two servo signals, D7 the
joystick button, D10 LED, and D8/D11/D13 plate brake/PWM/direction. D3/D12
control existing servo supply enable/polarity. A0 is servo current, A1
is the existing unused plate-current input, A2/A3 are joystick X/Y, A4
is battery voltage and A5 is profile selection.

**Old documentation assigned the analog pins differently.** This branch
preserves the actual source assignments, not the obsolete A2 voltage /
A3,A4 joystick table. Do not rewire the feeder to fit old documentation.
Verify existing wiring in the lab. Hardware UART TX conflicts with D1;
Serial remains disabled. [Pinout.txt](AutoFeeder/Pinout.txt) lists every pin.

## Operation

Start with a neutral joystick and clear the utensil path. The inherited
startup pose can snap because there is no position feedback. The arm lifts
through its inherited startup path, then automatically returns home.

- **Advanced / HIGH mode switch:** quick main press-release scoops; main
  hold >=500 ms rotates the plate while held. Release stops PWM immediately,
  then settles. A long press never starts a scoop. At delivery, press to return.
- **Simple / LOW mode switch:** one press-release rotates for 600 ms,
  stops, settles for 400 ms, scoops, delivers, waits 6500 ms and returns.
- A short joystick press-release retains the scoop shortcut. New presses
  during approach, scoop or lift request a clearance retract and home.
  A held press does not repeatedly start cycles. Starting mode/profile
  remain fixed throughout a cycle.
- **Calibration:** joystick hold-release between 1 and 10 seconds enters
  ENTRY, BOTTOM, MIDDLE, FRONT, END. Move with joystick; short release confirms.
  LED counts identify the next point; no physical confirmation nod. Plate
  stays stopped by default. Long joystick hold or main press cancels.
  The save slot is locked when calibration starts; other slots are unchanged.
- **Explicit reset:** >=10-second joystick hold-release resets all four
  EEPROM profiles. Invalid data never automatically resets other profiles.
- **Low power:** plate stops, servo supply turns off and LED blinks 500 ms
  on/off; power-cycle recovery only.
- **Fault LED counts:** 1 kinematics, 2 profile, 3 overload, 4 contact limit,
  5 storage. Stop and inspect; movement holds its last valid command rather
  than driving against a suspected obstruction. Six calibration flashes
  reject invalid completion without saving.

The five-point EEPROM layout, link lengths, pulses, joint limits and raw
current/battery thresholds are preserved. Bounded contact recovery raises
by 1 mm per retry, at most 3 retries / 3 mm. Motion uses timed, synchronized,
velocity-limited easing through existing points. Initial rad/s speeds need
lab measurements: the old speed was expressed per loop, with no stable timebase.
Software checks cannot establish physical food retention or face safety.

## Project guides

- [Inherited firmware architecture](docs/CURRENT_FIRMWARE_ARCHITECTURE.md)
- [Implementation plan and decisions](docs/FALL26_IMPLEMENTATION_PLAN.md)
- [All tuning parameters and fault behavior](docs/TUNING_GUIDE.md)
- [Exact first physical test and tuning sequence](docs/PHYSICAL_TUNING_GUIDE.md)
- [Movement and food trial checklist](docs/MOVEMENT_TEST_CHECKLIST.md)

Normal feeding excludes jiggle and braking. For an explicitly cleared lab
experiment only, build with:

```sh
./tools/compile_uno.sh --build-property \
  'compiler.cpp.extra_flags=-DENABLE_PLATE_JIGGLE=1 -DRUN_PLATE_JIGGLE_ONCE_AT_HOME=1'
```

That lab build runs one forward/pause/reverse/pause experiment on first home;
restore both flags to 0 for normal operation. An optional existing-D4 scope
trace is available with ENABLE_DEBUG_PIN_TRACE=1. `debug_snapshot()` exposes
values to the host harness. UART telemetry intentionally cannot be enabled
with the current D1 wiring.

## Source layout

`AutoFeeder.ino` wraps setup/loop; `Firmware.cpp` owns the explicit states,
inputs, safety and calibration; `Control.cpp` supplies pure debounce/motion/
retry logic; `kinematics.cpp` validates geometry; `Profile.cpp` validates and
reads/writes the unchanged EEPROM structs. `DCMotor.cpp` and `Joystick.cpp`
use only existing pins. `tests/` provides a lightweight C++ harness with
simulated Arduino I/O. No simulator result is a physical hardware trial.

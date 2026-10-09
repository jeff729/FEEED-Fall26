# F.E.E.E.D. - UMD Fall 2026

**Experimental firmware. Software verification does not establish
physical feeding performance or suitability for use with a person.**

This assistive feeder uses the existing Arduino Uno, two-servo spoon arm,
time-based DC plate motor, controls and power system. Current work is on
[`fall26-development`](https://github.com/jeff729/FEEED-Fall26/tree/fall26-development).
`main` remains the inherited file-import baseline. No new hardware is added.

## Build and test

Use Arduino CLI **1.5.1**, AVR core **1.8.8**, Servo **1.2.2**, and board
`arduino:avr:uno`. Host CI uses GCC **14.2.0** in a digest-pinned official GCC
container on Ubuntu 24.04; action commit references are pinned in
[the workflow](.github/workflows/verify.yml).

```sh
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.8
arduino-cli lib install Servo@1.2.2
./tests/run_tests.sh
SANITIZE=1 ./tests/run_tests.sh
./tools/compile_uno.sh
./tools/check_uno.sh experimental
./tools/check_uno.sh uart-rejection
./tools/check_repository.sh
```

Use a POSIX shell and C++11 compiler. The scripts use temporary build paths
and never detect ports or upload. Git Bash works for local Uno compilation.
This Windows review environment has no WSL; Application Control blocked its
portable host linker, so host/sanitizer results come from Linux GitHub Actions.
See the [verification report](docs/VERIFICATION_REPORT.md) for exact revisions,
results, warnings, simulated phase times and local blockers. Software checks
include default and experimental builds; experimental firmware is not the
normal bench configuration. October 9 verification passed 6,652 host
assertions in normal/sanitizer runs and both Uno builds; physical testing
remains pending.

## What changed

The earlier Fall 2026 work added checked kinematics/profiles, synchronized timed
motion, debounced controls, bounded current-based recovery, locked cycle/profile
selection, validated cancellation, LED calibration feedback and save readback.
This completion pass fixes overload priority during profile-reset recovery and
Cartesian paths crossing the existing inner workspace, and adds CI, focused
regressions and phase timing. See [CHANGELOG.md](CHANGELOG.md).

## Before the first bench session

- Use [Pinout.txt](AutoFeeder/Pinout.txt) and [Config.h](AutoFeeder/Config.h).
  The source uses A2/A3 for joystick and A4 for voltage; old wiring tables
  conflict. Verify existing wires in the lab. A1 plate-current wiring and
  calibration remain unconfirmed; no protection uses it. D1 is the mode
  switch, so UART must stay disabled.
- Follow the [movement checklist](docs/MOVEMENT_TEST_CHECKLIST.md) and
  [physical tuning guide](docs/PHYSICAL_TUNING_GUIDE.md), with people outside
  utensil reach. Upload manually only after operator approval.
- Read [controls, settings and fault codes](docs/TUNING_GUIDE.md) and the
  [current state machine](docs/FIRMWARE_STATE_MACHINE.md). The inherited Simple
  eating timeout remains **6500 ms** and needs caregiver/supervisor review
  before any use with a person.
- Physical testing is pending. Commanded joint positions are not feedback;
  PWM zero is not proof that the plate has stopped coasting. Startup snap,
  bowl clearance, tracking under load, ADC calibration and fault hold versus
  power-off consequences still require supervised measurement. Keep braking,
  jiggle, one-shot boot experiments, rotating calibration and UART disabled.

## Provenance and course report

This continuation credits the prior UMD F.E.E.E.D. teams and
[original project](https://github.com/Ibrahimtourepe/F.E.E.E.D.). This repository
begins with a file import, not preserved upstream commit history. The
[inherited architecture document](docs/CURRENT_FIRMWARE_ARCHITECTURE.md)
describes that baseline, including its defects; it is not current operating
instructions. No project license was found in this repository or the original
repository metadata. Public visibility does not supply a reuse license; none
has been added.

[PR4 software update](docs/progress/PR4_SOFTWARE_UPDATE.md) contains the two
course-progress slides for October 3-9, 2026. Implementation and documentation
in this pass were prepared with AI assistance; human team review and physical
testing remain pending.

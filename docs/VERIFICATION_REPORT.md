# Fall 2026 software verification report

Verified on 2026-10-06, on fall26-development. No firmware was uploaded and
no physical feeder, food trial or person-delivery test was performed.

## Inherited behavior and defects

The Uno controlled a 100/100 mm two-servo arm and time-based DC bowl motor.
Four EEPROM profiles held entry/bottom/middle/front/end coordinates.
Advanced quick presses scooped; holds rotated while held. Simple presses
rotated briefly, scooped, delivered, waited 6500 ms and returned. The joystick
manually calibrated five points with a physical nod and rotating plate.
Current >400 raised scoop offset and rewound a segment without a bound;
>500 requested cancellation. Raw voltage <662 entered low power, but checks
were absent from several states and blocked during input/LED waits.

Verified defects now fixed:

- Profile endpoint used end_y twice, corrupting/ignoring the endpoint pair.
- bool FK omitted its return; IK accepted nonfinite/domain-invalid inputs.
- Workspace projection divided by zero at origin/shoulder-circle centre.
- Failed IK/FK was ignored and could leave stale servo targets active.
- Blocking holds, settle/LED delays and calibration nod delayed safety input.
- Exact millis modulo scheduling could miss profile polling entirely.
- Contact rewind/offset could repeat indefinitely without a recovery bound.
- A constrained return target could reset all saved EEPROM profiles.
- Cancellation could still deliver; calibration could use the preceding bowl
  for clearance and overwrite/save without successful readback verification.
- Input held across return or begun just before idle could start another cycle.
- Above-clearance joint cancellation could dip down toward the bowl.
- Near-singular Cartesian paths could exceed configured acceleration.

## Implemented behavior

Config.h groups pins, inherited limits, speeds, plate timings, input timing,
ADC thresholds, battery, calibration, eating wait and disabled experiments.
An enum state machine centralizes transitions; every transition stops plate
PWM and discards stale trajectories. Global elapsed safety polling precedes
state work. Faults stop movement and hold the last valid servo command;
severe overload is not followed by blind retreat. Low power stops plate and
disables the existing servo supply, latched until power cycle.

Timed quintic joint trajectories synchronize arrival. Eased Cartesian segments
preserve all five calibrated corners rather than rounding unknown bowl walls.
Candidate commands enforce velocity/discrete acceleration bounds before servo
writes. Near-singular Cartesian targets are rejected without changing physical
joint limits; straight delivery remains supported in joint space. Return from
straight delivery uses a clearance-preflighted partial inherited joint route,
then Cartesian return. Above-clearance cancellation maintains commanded height.

Moderate contact uses 1 mm relief, a 200 ms settle and retry of the same segment,
limited to three total retries / 3 mm per cycle. Overload >500 or exhausted
recovery faults. Thresholds remain 400/500 raw ADC. Plate PWM/ramp/600 ms
rotation and 400 ms settle remain inherited; direction changes start at zero
PWM. Braking remains disabled. Jiggle is disabled and excluded from normal
feeding; a lab-only flag enables one timed forward/pause/reverse/pause at home.

Both buttons and mode input are debounced. Stable release is required to arm
idle gestures; held/long presses cannot become a subsequent unintended scoop.
Profiles and operating mode are captured per cycle. Calibration locks its slot,
stores achieved command coordinates, names all five steps, uses LED counts
instead of a physical nod, stops plate by default and verifies saved readback.
EEPROM remains four raw 40-byte structs, without a format migration. Invalid
slots use RAM defaults with a visible fault; no automatic EEPROM writes or
reset-all on a motion constraint. Only the exact inherited bowl template gets
its original effective entry projection in RAM. Explicit ten-second reset
still deliberately resets all four slots.

Pin documentation is corrected to source: A2/A3 joystick, A4 voltage. No pin
assignment, link length, pulse range, joint travel or ADC cutoff changed.
UART cannot be enabled because D1 remains mode selection. Host debug snapshots
and optional existing-D4 pulse tracing do not require a new feeder input.

## Build and tests

Toolchain: Arduino CLI 1.5.1, arduino:avr core 1.8.8, Servo 1.2.2; board
arduino:avr:uno. Build scripts use unique temporary paths and never upload.

| Verification | Result |
|---|---|
| Default Uno firmware | PASS; 19882/32256 bytes flash, 595/2048 bytes static SRAM |
| Jiggle + one-shot lab boot + D4 trace Uno build | PASS; 20128 bytes flash, 601 bytes static SRAM |
| UART enabled intentionally | PASS guard test: rejected with D1 conflict error |
| FK/IK/workspace/profiles | 471 checks, zero failures |
| Button/clock/motion/retry helpers | 5874 checks, zero failures |
| Default full firmware integration | 108 checks, zero failures |
| Jiggle + D4 trace integration | 119 checks, zero failures |
| Lab one-shot boot integration | 5 checks, zero failures |
| All host checks with ASan + UBSan | Same 6577 checks, zero failures |
| Whitespace/diff check | PASS |
| Source invariants versus main | Pins, link geometry, pulses/trims and ADC thresholds match; no blocking delay/while or hardware Serial calls |

The Arduino core emits four existing unused-parameter warnings in new.cpp;
firmware-owned files produced no warnings in these builds. SRAM figures are
static allocation, not measured runtime stack headroom. Host tests run real
control/profile/state/motor/joystick code with simulated Arduino I/O, current,
voltage, EEPROM, servo outputs and time; delay() fails the harness. They check
both default bowl/plate paths, cancellation, mode/profile locking, held input,
clock rollover, fault priority, motor stopping, low-power latch/LED, calibration
save/rejection, legacy EEPROM compatibility, failed writes and bounded contact.

One independent read-only whole-branch review found three material edge cases:
idle-edge input, clearance dip, Cartesian singularity acceleration. Each had a
failing regression before its fix; the final suite passes. Review did not
validate hardware or certify person safety.

## Remaining physical validation and deliberate omissions

Use PHYSICAL_TUNING_GUIDE.md in its exact first-session order and record every
trial in MOVEMENT_TEST_CHECKLIST.md. First tune home/acceleration, then descend,
scoop, lift, return, plate PWM/duration, measured current thresholds, recovery
and eating wait. Initial rad/s rates are conservative software starting points;
old per-loop rates cannot be converted reliably from source alone.

Measure loaded servo tracking, spoon bounce, bowl clearance/contact force,
ADC-to-current/voltage calibration, voltage sag, bowl start/coast/rotation and
filled-spoon rate. Confirm reverse/controller behavior before jiggle, and brake
support/thermal behavior before changing USE_PLATE_BRAKE. Startup can still
snap to the inherited initial pose because no joint position feedback exists.
Fault-hold versus servo power-off has mechanical consequences requiring lab
assessment. Command-space preflight is not collision sensing.

No voltage speed compensation, current-threshold retuning, corner-rounding
splines, EEPROM migration/checksum, UART logging, normal-flow jiggle/braking,
new sensors, hardware, pins, vision, AI or network service was implemented.
Unknown measurements, bowl geometry, controller identity, physical interface
compatibility or wiring conflicts prevent those choices in this pass. Existing
EEPROM saves are read back but not power-loss atomic: plausible interrupted
writes remain undetectable without a format change. Numerical Cartesian
conditioning rejects unsupported paths while preserving full joint travel.

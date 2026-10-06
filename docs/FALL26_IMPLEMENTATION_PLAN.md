# Existing-hardware firmware reliability plan

Goal: preserve the current Uno, pins, two-link geometry, five-point EEPROM
format, thresholds, physical joint limits and recognizable Simple/Advanced
interface while implementing the user's P0..P5 requirements on
fall26-development. The user explicitly requests autonomous implementation,
compilation, testing and separate commits; additional design approval gates
are superseded by that instruction. Work stays in the requested existing
checkout rather than creating a different development branch/worktree.

## Design and decisions

Keep small Arduino modules. Harden existing kinematics and profiles first.
Centralize tuning in Config.h; pure timing/motion logic in Control.h/.cpp.
Use an enum state machine because immediate fault priority and guaranteed
plate stopping require a single transition owner. All inputs and LEDs are
polled; all durations use uint32_t subtraction. No heap allocation.

Use synchronized quintic joint trajectories and eased Cartesian segments
through the existing calibrated points, with joint-rate checks. Stop at
each calibrated corner rather than rounding an unknown bowl wall. The
original speed is loop-dependent: initial 0.3 rad/s caps are conservative
provisional settings, not a measured conversion; lift/delivery 0.225 rad/s.
Startup pose and home/delivery poses remain inherited. Severe overload or
untrustworthy kinematics latches a hold fault; low power disables servo
supply. User cancel and bounded moderate-contact recovery use verified
Cartesian clearance. Neither is a certified collision avoidance system.

No EEPROM migration. Validate all points including adjusted exit/return
targets. Invalid slots get RAM defaults and a visible fault, never automatic
EEPROM writes. Explicit reset or confirmed calibration can save. Calibration
locks its slot, keeps plate stopped by default (existing rotating calibration
can be enabled only for lab comparison), uses LED confirmation instead of
an unsafe physical nod. Preserve joystick short-press cycle shortcut.

UART remains disabled because D1 is wired. Debug snapshot function can be
inspected in the host harness; optional UART code must fail compilation with
current mode wiring. Jiggle is disabled by default and accessed only through
a lab-only explicit trigger, never through ordinary feeding.

## Tasks and verification

1. Record inherited architecture before code changes (this document and
   CURRENT_FIRMWARE_ARCHITECTURE.md). Install Arduino CLI/core/Servo;
   compile the unchanged sketch to record baseline defects.
2. Write host regression tests for FK/IK including nonfinite/inner/outer
   boundaries and zero workspace inputs, and endpoint/EEPROM corruption.
   Observe failure, fix kinematics/Profile with transactional outputs and
   slot-local defaults, run host tests and Uno compile; commit.
3. Test debounced press/release/hold across clock rollover, synchronized
   velocity-limited easing and bounded recovery. Implement shared control
   helpers and configuration. Compile/test; commit motion/control group.
4. Write simulated Arduino integration tests reproducing blocking/cancel,
   plate-exit, overload, low-power, one-cycle-held-input and EEPROM issues.
   Replace function-pointer flow with explicit states, central stop/fault,
   nonblocking mode/input/calibration, bounded contact recovery, plate ramp,
   and disabled lab jiggle. Run tests and Uno build; commit safety/state and
   calibration/recovery changes as logical groups.
5. Add tuning, physical tuning and movement checklist docs, update pin
   documentation and both READMEs to actual firmware behavior; add repeatable
   build/test scripts and pinned build instructions. Compile/test; commit.
6. Review full main...HEAD diff, preserve baseline main, run tests plus normal
   and experimental Uno builds; request one independent whole-branch review
   using the executing-plans/requesting-code-review skills. Fix material
   findings with reproducing tests. Commit, verify clean status, report SHAs
   and precise lab sequence. Never upload automatically.

## Review focus

- Held/bouncing buttons through return and startup must not start another cycle.
- Mode/profile changes during a cycle must not change its saved path/timing.
- Fault/low voltage must override motor activity and pending normal transitions.
- Nonfinite, invalid and partially-written EEPROM must not change servo targets.
- No commanded clearance path is guaranteed physically safe without position
  feedback: validate commanded path, hold on failed validation/overload, test
  with an empty bowl and no person in reach before any food/delivery trial.

## Execution ledger

- Initial checkout clean, branch fall26-development, main c9ea8ac.
- Ruling: preserve source analog assignments despite old documentation;
  incorrect physical assumptions would require lab wiring verification.
- Ruling: execute the user's fully specified scope inline; no intermediate
  permission gates or additional worktree. Request independent review at end.

- Task 1 complete: architecture recorded before code, commit 6103fbe; inherited
  Uno compile passed with missing-return/function-pointer warnings.
- Task 2 complete: 919dd76, finite transactional FK/IK and profile validation;
  regression failures observed before fixes.
- Task 3 complete: 35bb44a, debounced timing, synchronized eased trajectories
  and bounded recovery; velocity/acceleration/rollover tests passing.
- Task 4 complete: 88cb1bc, centralized states/fault/motor stop and nonblocking
  inputs/motion/calibration. 3f3369c adds confirmed save/readback and feedback.
- Task 5 complete: 22eaffe, disabled jiggle with zero-PWM direction changes;
  63ac01d expands corruption/mode/profile/calibration/low-power regressions
  and isolated build tooling. Tuning/physical/checklist/state docs added.
- Ruling: only exact whole-template legacy bowl defaults receive the original
  known entry projection; general invalid saved coordinates are rejected.
  This preserves effective factory behavior without accepting arbitrary
  unreachable user profiles. Cost if wrong: an old user profile differing
  from that exact template requires explicit inspection/recalibration.
- Ruling: cancel returns home without delivery; severe overload/retry limit
  holds last valid command instead of blind retreat. Cost: operator may need
  to inspect/reposition after a fault; command position is not feedback.
- Ruling: replace blocking calibration nod with LED counts; keep calibration
  plate stopped by default. Cost: existing calibration feel changes, but no
  added wiring or unsafe elbow overshoot; rotating calibration is lab-only.
- Task 6 in progress: independent whole-branch review, then final green suite,
  normal/lab Uno builds, UART conflict rejection, clean main and branch push.

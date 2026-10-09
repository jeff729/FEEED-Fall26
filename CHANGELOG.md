# Changelog

## October 9, 2026 completion pass

- Fix overload priority during profile-reset recovery and recheck safety
  after EEPROM writes before startup commands/power enable (`f06cc9a`).
- Reject Cartesian chords crossing the inherited inner workspace (`f06cc9a`).
- Add pinned host/sanitizer/Uno CI, specific UART-conflict rejection, hardware
  invariants, storage/input/fault regressions and simulated phase timing
  (`ec2b71f`, `d8f24d7`, `6cab55d`).
- Reconcile guides and wiring uncertainty, prepare PR4 course slides, and
  document publication review and the local host-toolchain policy blocker.

## Already present at the start of this pass

October 6 commits `919dd76` through `3fa818d` supplied validated IK/FK and
compatible profiles, timed synchronized motion, debounced state/input logic,
cycle/profile locking, bounded recovery, validated cancellation/return,
calibration LED feedback and save readback. The plate stays stopped during
calibration by default. Normal braking, jiggle, one-shot boot and UART remain
disabled. These changes were inspected and preserved, not implemented again.

## Physical work pending

No hardware upload or physical test was performed. Verify existing wiring,
startup behavior, actual bowl clearance, loaded tracking, ADC calibration,
plate coast and fault power behavior using the supervised movement checklist.
Review the simulated 116-128 second movement cycles and unchanged 6500 ms
Simple eating timeout with the team/caregiver before considering person use.
Pins, geometry, pulse ranges/trims, joint limits, thresholds and EEPROM layout
remain unchanged. Public visibility does not imply a software reuse license
or physical validation.

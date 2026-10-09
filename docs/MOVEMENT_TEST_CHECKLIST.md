# Existing feeder movement checklist

Run after reviewing PHYSICAL_TUNING_GUIDE.md. No person within utensil reach
for dry, empty-bowl or initial food tests. Stop on unexpected motion or a
fault. Mark a test not performed if safe conditions/equipment are missing.

Session: ____ Date/operator: ____ Commit/build flags: ____ Bowl/profile: ____
Battery voltage / ADC: ____ Food / amount / consistency: ____
Changed parameter (one at a time), old -> new: ____

## Before power and manual upload

- [ ] Record the exact commit/build flags; compile the normal Uno configuration.
- [ ] Keep brake, jiggle, one-shot boot, UART and rotating calibration disabled.
- [ ] Verify existing wiring against the source pin table, especially D1 and
      A2/A3/A4; A1 sensing/calibration is unconfirmed and unused by firmware.
- [ ] Clear the mechanism; keep people outside utensil reach and the existing
      power disconnect accessible. Software cancel is not an emergency cutoff.
- [ ] Lab operator approves manual upload before anyone uploads or energizes.
- [ ] Preserve existing calibration records before any deliberate reset-all.

## Dry tests

- [ ] Power on: inherited initial pose, lift, automatic home; no powered plate.
- [ ] Home: synchronized arrival, no jump or interference; neutral joystick at boot.
- [ ] Profile selection: four bins/LED counts; no boundary chatter.
- [ ] Manual calibration: ENTRY, BOTTOM, MIDDLE, FRONT, END; LED counts; single confirmation each.
- [ ] Confirmed save survives a power cycle; other profile slots remain unchanged.
- [ ] Pot change during calibration does not redirect save; invalid completion is rejected visibly.
- [ ] Joystick long cancel and main-button cancel leave EEPROM unchanged and plate stopped.
- [ ] Advanced quick press: exactly one scoop cycle and wait for return input.
- [ ] Advanced long press: plate only, stops on release/mode change, no subsequent scoop.
- [ ] Simple: automatic rotate, PWM zero, observed physical coast stop, settle,
      scoop, delivery, timed return. The unchanged 6500 ms eating wait needs
      caregiver/supervisor review before any use with a person.
- [ ] Hold main input through an entire return: no second cycle.
- [ ] Press either button just before reaching idle: no cycle until release and a fresh press.
- [ ] New presses cancel descend/scoop/lift; no delivery after canceled scoop.
- [ ] Delivery press returns early; mode/profile changes during a cycle do not redirect it.
- [ ] Low voltage, **only if safely testable unloaded**: PWM zero, servo supply off,
      500 ms LED halves, no new cycles/recovery until power cycle.
- [ ] Fault inspection: existing LED code understood; no automatic feeding restart.

## Empty bowl

- [ ] Descend gently to calibrated entry; no impact or spoon bounce.
- [ ] Bottom/middle/front traversal stays within the measured bowl.
- [ ] Corner slowing and synchronized joints are smooth.
- [ ] Gentle contact response is bounded; no repeated backward-segment oscillation.
- [ ] If a contact retry occurs naturally, current decreases after safe offset.
- [ ] Overload is **not** deliberately induced against a rigid obstruction/person.
- [ ] User cancel clears before returning; shoulder-limit cases either clear safely or fault.
- [ ] Normal staged return from straight delivery and home remain above bowl rim; plate stays stopped.
- [ ] Cancel while already above clearance: horizontal retreat does not dip toward the bowl.
- [ ] Near-straight invalid Cartesian calibration/paths are rejected; do not widen physical limits to override.
- [ ] Spoon exit +5/+20 and return +30 inherited clearances are physically adequate.

## Food trials

Proceed only after the dry and empty-bowl checks pass under supervision. No
person is an overload-test subject and no deliberate jam is permitted.

Use several repeats per food at a controlled quantity; record each cycle,
including empty scoops and aborts. Existing five-point calibration may
need adjustment for the same bowl's food depth. Do not assume these foods
behave alike or claim successful performance without trial data.

| Trial | Food | Successful scoop | Empty spoon | Spill | Bowl scrape | Current retry/count | Abort/fault | Cycle time (s) | Notes / interventions |
|---|---|---|---|---|---|---|---|---|---|
| 1 | Yogurt | | | | | | | | |
| 2 | Yogurt | | | | | | | | |
| 3 | Applesauce | | | | | | | | |
| 4 | Applesauce | | | | | | | | |
| 5 | Oatmeal | | | | | | | | |
| 6 | Oatmeal | | | | | | | | |
| 7 | Cereal | | | | | | | | |
| 8 | Cereal | | | | | | | | |
| 9 | Mac and cheese | | | | | | | | |
| 10 | Mac and cheese | | | | | | | | |

For every dry/empty-bowl trial use the same outcome columns:

| Test / repeat | Successful scoop / N/A | Empty spoon / N/A | Spill | Bowl scrape | Current retry/count | Abort/fault | Cycle time (s) | Notes / interventions |
|---|---|---|---|---|---|---|---|---|
| Dry: ____ | | | | | | | | |
| Empty bowl: ____ | | | | | | | | |

Record contact ADC and battery measurements when available; numerical
software tests do not measure bowl force, loaded servo tracking, coast
distance, spoon bounce, food retention or safety around a person.

Mark pass, fail or not performed for each step. Record the LED fault code,
phase, changed setting, power disconnect/reset, manual repositioning and
reason for stopping, including unsuccessful trials. The repository contains
no completed hardware test records as of October 9, 2026.

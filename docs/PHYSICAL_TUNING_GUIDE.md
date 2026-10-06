# Physical tuning of the existing feeder

Use the existing hardware only. No autonomous upload has been performed.
Begin with no person in utensil reach, no food, a clear mechanism, and
ready access to the existing power disconnect. The software button is a
cancel request, not an independent emergency power cutoff. Verify the
actual existing A2/A3 joystick and A4 voltage wires against source before
energizing; do not swap wires or pin numbers to fit the old documentation.

## Recommended first session, in exact order

1. Build the default Uno configuration and review Config.h/Pinout.txt.
   Check the current thresholds remain 400/500 and cutoff 662 ADC. Keep
   brake, jiggle, UART and rotating calibration disabled. Only manually
   upload when the lab operator has cleared the mechanism.
2. Keep the utensil unloaded and observe startup snap, inherited lift,
   return waypoint and home. Keep joystick neutral during startup. Stop
   on unexpected direction, interference, buzzing or fault; do not simply
   increase a threshold to continue. Record measured boot/cycle times.
3. At home, check the profile potentiometer and LED selection counts.
   For a bad-EEPROM fault, inspect first; a >=10-second joystick hold-release
   explicitly resets all four saved profiles. Save/copy old calibration
   data before choosing that deliberate reset if it is still needed.
4. Enter calibration with a 1..10-second joystick hold-release. With the
   plate stopped, set entry, bottom, middle, front, end for the existing
   empty bowl. Check the five LED steps, successful save and home return.
   Do not intentionally push hard into the bowl to "teach" a point.
5. In Advanced mode, test a main quick press for one cycle, a hold for
   plate-only rotation, raw release stop, and a short joystick shortcut.
   Check cancel during descend, near the bottom, during lift, and early
   return at delivery. Check a held cancel does not restart at home.
6. Run empty-bowl scoops. Check corner slowing, clearance and return. Use
   only a calibrated path; a valid numerical profile is not evidence of
   no physical collision. Stop if the spoon scrapes or the linkage stalls.
7. In Simple mode, verify rotate -> PWM stop -> settle -> scoop -> lift ->
   6.5-second wait -> home. Check one cycle per release and early return.
8. Record current/voltage with appropriate lab equipment already available.
   Do not deliberately stall against the bowl/person to provoke overload.
   Simulated ADC tests already exercise shutdown logic; physical fault
   testing should be performed only with a safe unloaded setup.
9. Add a small quantity of one food, beginning with applesauce or yogurt.
   Run several cycles and record the checklist outcomes. Tune one setting
   at a time in the sequence below, rebuilding after changes.
10. Repeat with oatmeal, cereal and mac and cheese. Only consider person
    delivery after empty/food cycles, clearance and cancel behavior have
    been reviewed by the project supervisor/operator.

## Tune in this order

All values live in AutoFeeder/Config.h. Defaults are software starting
points, not final measured settings. Change one setting, rebuild, repeat
the same bowl/food/profile trial and record the result.

| Order / settings | Controls | Too high | Too low | Observe / record |
|---|---|---|---|---|
| 1 HOME_SPEED, JOINT_ACCELERATION | Home travel and ramp shape | Spoon bounce, mechanical shocks, unexpected path dip | Slow settling or long startup/return | Joint arrival together, clearance, vibration, time; do not alter home geometry |
| 2 DESCEND_SPEED | Bowl-entry approach | Contact impact, overshoot, current spikes | Long approach, impractical cycle | First-contact location, bounce and current before food |
| 3 SCOOP_SPEED, SCOOP_CARTESIAN_SPEED | Food traversal | Scraping, spill, current spikes, poor retention | Food may drag/clump; lengthy cycle | Filled-spoon rate, scrape, trajectory through each point |
| 4 LIFT_SPEED, DELIVERY_SPEED | Exit/approach to delivery | Spill, slosh, spoon bounce | Food loss from drips, long delivery | Food held through lift, endpoint settling; DELIVERY_SPEED currently controls startup lift |
| 5 RETURN_SPEED | Return waypoint/retract | Spill of residue, abrupt retreat | Long cycle, time near user's face | Clearance at every segment and stable home |
| 6 PLATE_PWM, PLATE_START_PWM | Existing bowl motor command | Fast bowl motion, food displacement, high motor current | Motor fails to start, inconsistent rotation | Loaded/unloaded start reliability and current; preserve direction/pins |
| 7 AUTO_ROTATE_DURATION, PLATE_RAMP_TIME, PLATE_SETTLE_TIME | Rotation exposure and coast settle | Excess rotation/long pause; too-short ramp can shock | Repeated sampling of same region; short settle leaves moving bowl | Actual angle per cycle at different loads/charge; motor stopped before spoon descend |
| 8 THRESHOLD_CURRENT, OVERLOAD_CURRENT | Contact/abort thresholds | Excess bowl force, late protection | False retries or false aborts | Baseline, loaded, gentle contact and safe overload ADC distributions; retain 400/500 until measured |
| 9 CONTACT_OFFSET_MM, MAX_CONTACT_OFFSET_MM, MAX_CONTACT_RETRIES, CONTACT_SETTLE_MS | Bounded pressure relief | Empty spoon from shallow path, many repeated contacts, long recovery | Insufficient relief, frequent bounded aborts | Offset that actually reduces measured contact; retry count; safe clearance and no oscillation |
| 10 FEED_WAIT_TIME | Simple delivery hold | Long time at face, slow meal | Retreat before user finishes | User/operator observed eating time only after safe physical validation |

Do not tune pulse widths, trim or workspace boundaries to force a rejected
target to work. Inspect calibration and physical alignment first. Retain
joint travel and link lengths. Increasing retry limits is not a substitute
for correcting an overly deep calibrated bottom/front point. An empty
spoon without scraping often calls for a profile/food trial, not more speed.

## Battery characterization before compensation

No voltage-based speed compensation is implemented. Measure actual battery
and servo supply voltages versus A4 ADC; verify divider ratio and ADC
reference under load, cutoff response, ADC noise and loaded current. At
several charge levels, record unloaded and loaded segment times, observed
joint travel/settling, failed starts, bowl rotation angle and filled-spoon
rate with identical profiles. Record battery chemistry/rated safe discharge
and shield/regulator/servo ratings from the existing machine. Determine
whether slowdown is supply sag, servo torque limits, plate friction or
command-rate limits. Raising commanded speed cannot recover missing torque
and can increase contact force. Compensation needs those measurements and
a separately reviewed control change.

## Optional bench experiments last

First identify the existing motor controller and verify safe reverse/coast
behavior. Keep USE_PLATE_BRAKE=false until current/thermal/brake tests support
it. The disabled jiggle settings are in TUNING_GUIDE.md; use an empty bowl,
no person, low PWM and one cycle at home. Confirm PWM zero on every pause,
cancel and exit. Return both lab flags to 0 before normal feeding tests.

Measure current-to-force correlation and actual commanded/achieved tracking
before claiming contact-force regulation. There are no encoders or new
sensors: contact recovery is bounded heuristic relief, not force control.

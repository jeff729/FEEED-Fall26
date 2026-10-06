# Fall 2026 firmware tuning reference

All hardware and motion settings are in `AutoFeeder/Config.h`. EEPROM layout
and five-point order remain in `Profile.h`. Tune on fall26-development;
main is the untouched imported baseline. Never change pins or link lengths
to compensate for a bad measurement. The firmware has not been physically
validated by these host tests.

## Preserved settings

| Group | Parameters and defaults | Meaning |
|---|---|---|
| Geometry | ARM_L1_MM=100, ARM_L2_MM=100 | Millimetres; second link includes utensil tip |
| Home | HOME_Q1=-3.1415926, HOME_Q2=2.1817; HOME_X_MM=-42.6424, HOME_Y_MM=-81.9152 | Inherited joint pose and Cartesian approximation |
| Startup | STARTUP_Q1=-2.09, STARTUP_Q2=2.09; STARTUP_SETTLE_MS=500 | Inherited initial command before power enable; boot snap remains possible |
| Joint limits | q1 -pi..0; q2 0..pi | Validated before every servo pulse; unchanged |
| Servo pulses | SERVO_MIN_PW=544, SERVO_MAX_PW=2400 us; SERVO1_TRIM=0, SERVO2_TRIM=0 | Existing pulse mapping; leave alone unless alignment is physically verified |
| Workspace | WORKSPACE_MIN_RADIUS=10; WORKSPACE_MIN_Y=-0.001; WORKSPACE_LEFT_FRACTION=0.9 | Outer reach 200 mm; shoulder exclusion circle remains centred at (-100,0), radius 100 |
| Profile clearance | SCOOP_EXIT_X_MM=5, SCOOP_EXIT_Y_MM=20; RETURN_CLEARANCE_MM=30 | Inherited offsets; calibrated five points are still used |
| Plate | PLATE_PWM=255; PLATE_START_PWM=85; PLATE_RAMP_TIME=1000 ms | Inherited maximum/minimum and ramp shape |
| Rotation/settle | AUTO_ROTATE_DURATION=600 ms; PLATE_SETTLE_TIME=400 ms | Time-based rotation; no measured angle and no encoder |
| Contact | THRESHOLD_CURRENT=400; OVERLOAD_CURRENT=500 | Raw A0 ADC counts, not mA; strict > comparisons retained |
| Battery | LOW_POWER_VOLTAGE=662 | Raw A4 ADC count cutoff; latched until power cycle |
| User hold | LONG_PRESS_MS=500 | Advanced plate rotation threshold |
| Joystick hold | CALIBRATION_HOLD_MS=1000; PROFILE_RESET_HOLD_MS=10000 | Release after 1..10 seconds enters calibration; explicit >=10-second release resets all four profiles |
| Feed wait | FEED_WAIT_TIME=6500 ms | Simple automatic eating interval; Advanced waits for a press |
| Joystick | JOYSTICK_DEADZONE=100; JOYSTICK_X_SIGN=-1; JOYSTICK_Y_SIGN=1 | Existing discrete directions and deadzones |

## Time-based movement and new safety bounds

| Parameter | Initial default | Effect |
|---|---|---|
| HOME_SPEED | 0.3 rad/s | Home and post-retract joint/Cartesian rate cap |
| DESCEND_SPEED | 0.3 rad/s | Synchronized approach to entry |
| SCOOP_SPEED | 0.3 rad/s | Joint-rate cap along Cartesian scoop and backoff |
| LIFT_SPEED | 0.225 rad/s | Controlled exit-to-delivery joint movement |
| DELIVERY_SPEED | 0.225 rad/s | Startup's inherited lift to straight pose |
| RETURN_SPEED | 0.3 rad/s | Delivery return waypoint and user-cancel clearance |
| CALIBRATION_SPEED | 0.3 rad/s | Joystick joint-rate cap, including near singularities |
| JOINT_ACCELERATION | 0.6 rad/s² | Quintic joint trajectory acceleration bound; Cartesian duration uses a conservative sampled derivative estimate |
| SCOOP_CARTESIAN_SPEED | 10 mm/s | Additional Cartesian trajectory speed cap |
| CALIBRATION_MM_PER_SECOND | 10 mm/s | Requested joystick translation; actual achieved point is stored |
| MOTION_TICK_MS | 20 ms | Servo command trajectory cadence; delayed loops slow movement instead of catching up |
| BUTTON_DEBOUNCE_MS | 30 ms | Both buttons and mode-switch debounce |
| PROFILE_CHECK_INTERVAL | 100 ms | Overflow-safe profile polling |
| PROFILE_POT_FIRST_EDGE / WIDTH / HYSTERESIS | 403 / 146 / 10 ADC | Inherited profile bins plus boundary hysteresis |
| SAFETY_CHECK_INTERVAL | 10 ms | Global current/battery sampling, including delivery wait/calibration/return |
| CONTACT_OFFSET_MM | 1 mm | Inherited lift increment per moderate-contact retry |
| MAX_CONTACT_OFFSET_MM | 3 mm | Conservative new accumulated-offset limit |
| MAX_CONTACT_RETRIES | 3 total/cycle | Bounded recovery; no segment rewind or reset of count between points |
| CONTACT_SETTLE_MS | 200 ms | Nonblocking pause after vertical relief |
| CALIBRATION_CONFIRM_MS | 180 ms | Brief command pause following point confirmation |
| PROFILE_ROUNDOFF_MM | 0.05 mm | Maximum ordinary profile coordinate correction; larger invalid points fail |
| IK_ROUNDOFF_TOLERANCE | 0.00001 dimensionless D | Only tiny acos-domain overshoot is clamped |
| LED_PULSE_MS / LED_GROUP_PAUSE_MS | 150 / 750 ms | Flash counts and grouping |
| LOW_POWER_LED_HALF_MS | 500 ms | Low battery LED on/off halves |

The old 0.0003 rad/loop speed had no stable time unit. These are provisional,
conservative rates, not a proven numerical conversion. Default home is
intentionally capped at normal approach speed rather than retaining the
old unexplained 4x multiplier. Measure cycle time, loaded movement and spoon
bounce before changing rates. Speed and acceleration must be positive;
timing intervals must remain small relative to the uint32 clock range.
Quintic segments stop gently at each calibrated corner. No splines round
unknown bowl walls. Cartesian candidates also pass a hard joint-rate check;
this is not a measured physical acceleration guarantee for hobby servos.

## Contact and fault behavior

Normal current continues the current segment. Above 400, the arm stops
advancing that segment, tries a validated 1 mm upward backoff, pauses,
and retries the same segment with the accumulated offset. Three total
retries / 3 mm are the initial bounds. Above 500, invalid IK, invalid
paths, or exhausted contact recovery latches FAULT and stops the plate.
The last valid servo command is held; overload does not trigger a blind
retract against an obstruction. User cancellation attempts a validated
vertical clearance then home without delivery. At the shoulder stop the
vertical clearance is limited by existing link geometry; a joint home
fallback is checked for a downward dip. If no valid commanded path exists,
the firmware holds and signals a fault instead of inventing a route.

q1/q2 and FK are commanded positions, not measured positions. A stalled
arm, food load, linkage flexibility or spoon vibration can invalidate a
command-only clearance assumption. The motion checklist is mandatory lab
validation, not proof that the utensil is safe around a person.

## Profiles and calibration

Four raw ten-float profiles stay at EEPROM offsets 0,40,80,120. No format
migration, checksum or automatic writes on boot. All five points and
derived exit/return points are validated. The exact inherited built-in
bowl template is recognized and receives the old entry projection in RAM:
(-75,-82.5) -> (-70.99926,-95.70244). Other substantially invalid points
are rejected. A bad slot gets its default in RAM, but selecting it for a
cycle signals a fault. Other EEPROM slots remain unchanged. Confirmed
calibration and explicit reset are the only save paths; saves are read back.
Struct writes are not power-loss atomic: plausible partial data cannot be
fully detected without changing the format. Recalibrate/inspect after an
interrupted save. No checksum is claimed.

Calibration locks its slot on entry and names ENTRY, BOTTOM, MIDDLE, FRONT,
END explicitly. Neutralize the joystick at power-on. Short joystick release
stores the achieved command point; long hold or a main-button press cancels.
Point counts 1..5 identify the next point. Six flashes reject a completed
profile; it stays at END without overwriting EEPROM. Confirmed save gives
five flashes and returns home. The physical nod is removed because it
could exceed the elbow stop. ROTATE_DURING_CALIBRATION=false keeps the
plate stopped; the inherited rotating-calibration behavior is available
only as a deliberate lab setting, with centralized stop on exits.

## Modes and LED signals

Advanced (D1 HIGH): quick main/joystick press-release starts one scoop.
Main hold >=500 ms rotates the plate; raw release immediately stops PWM,
then settles and waits. Release after a long hold never scoops. Changing
the mode switch to Simple also stops manual rotation.
Simple (D1 LOW): one press-release rotates for 600 ms, stops, settles
400 ms, scoops, delivers, waits 6500 ms and returns. Held input never
repeats cycles. A new press during approach/scoop/lift cancels; a new press
at delivery returns early. Starting mode/profile are captured for a cycle.
Startup follows the inherited snap pose and straight-arm lift, then
automatically returns home instead of waiting at the delivery pose.

Fault flash counts repeat: 1 kinematics, 2 invalid profile, 3 overload,
4 contact-retry limit, 5 EEPROM save failure. Low power is 500 ms on/off
and disables the servo supply. Inspect overload/kinematic/contact faults
before a power cycle; there is no automatic restart. Invalid-profile/storage
faults permit the inherited explicit >=10-second joystick reset gesture;
that deliberate gesture resets **all four slots**, never automatically.

## Plate experiments and debug

USE_PLATE_BRAKE=false. Repository evidence identifies a brake pin but not
a verified controller model/current/thermal response. Keep it disabled
until tested. Direction changes first command zero PWM.

ENABLE_PLATE_JIGGLE=0 and RUN_PLATE_JIGGLE_ONCE_AT_HOME=0 by default.
With the former enabled, `start_plate_jiggle()` only accepts an idle,
nonfaulted feeder with released buttons. The second flag runs one lab
jiggle at first idle home. It never runs during ordinary feeding. Sequence:
normal direction -> pause -> reverse -> pause -> idle, with stop on cancel,
fault or low battery. Defaults: JIGGLE_FORWARD_MS=150,
JIGGLE_REVERSE_MS=150, JIGGLE_PWM=85, JIGGLE_PAUSE_MS=250.
Do not enable on food/person tests before empty-bowl reverse/coast testing.

Hardware UART remains prohibited: TX is the existing D1 mode switch.
ENABLE_UART_TELEMETRY=1 intentionally fails compilation. No Serial.begin,
UART pin reassignment or companion computer is added. `debug_snapshot()`
exposes state, fault, commanded q1/q2 and tip x/y, cycle profile, raw
servo-current/battery ADC, commanded plate PWM, retry count and offset in
the host harness. ENABLE_DEBUG_PIN_TRACE=1 optionally emits state+1 pulses
(50 ms on/off, 750 ms group pause) on the existing unused debug D4 output
for a laboratory oscilloscope. Normal firmware leaves D4 unused. Detailed
physical telemetry is unavailable with the current UART wiring.

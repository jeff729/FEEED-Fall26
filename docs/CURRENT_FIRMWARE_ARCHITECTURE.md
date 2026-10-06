# Inherited firmware architecture (before functionality changes)

Recorded from baseline `c9ea8ac` on 2026-10-06. All repository source,
both READMEs, Pinout.txt and the Doxygen configuration were reviewed.
This document describes the inherited behavior, including defects, rather
than describing the intended Fall 2026 behavior.

## Hardware and authoritative pin assignments

The board is an Arduino Uno. No joint position feedback exists: `q1/q2`
are last commanded angles, not measured positions. Both links are 100 mm;
L2 includes the spoon tip. The positive-elbow two-link model is
`x=100*cos(q1)+100*cos(q1+q2)`, similarly for y with sine.
Shoulder q1 is -pi..0; elbow q2 is 0..pi. Pulses are 544..2400 us,
with zero trim. Home is (-pi, 2.1817), approximately (-42.6424,-81.9152)
mm. Startup commands (-2.09,2.09) before enabling servo power: the servos
can snap there because their actual boot position is unknown.

| Pin | Function in the running source |
|---|---|
| D0 | Unused RX; do not enable UART with the current wiring |
| D1 | Mode switch INPUT_PULLUP: HIGH Advanced, LOW Simple |
| D2 | Main user button, active LOW INPUT_PULLUP |
| D3 | Motor-shield channel A PWM, used as servo power enable |
| D4 | Existing debug output, unused by normal operation |
| D5 / D6 | Shoulder / elbow servo signal |
| D7 | Joystick button, active LOW INPUT_PULLUP |
| D8 | Plate channel B brake, disabled |
| D9 | Channel A brake listed in documentation; not driven by sketch |
| D10 | Warning LED |
| D11 | Plate PWM |
| D12 | Channel A direction, held HIGH for servo supply polarity |
| D13 | Plate direction, normal operation LOW |
| A0 | Servo current ADC, approximately 337.6 counts/A per comment |
| A1 | Plate current listed in documentation, not sampled |
| A2 | **Joystick X in Joystick.h**, inverted |
| A3 | **Joystick Y in Joystick.h** |
| A4 | **Battery/servo supply voltage in sketch** |
| A5 | Four-profile selection potentiometer |

Pinout.txt incorrectly lists D1 as unused TX. Both READMEs and Pinout.txt
list voltage on A2 and joystick on A3/A4, disagreeing with the source.
The Fall 2026 code must preserve A2/A3/A4 as implemented; verify these
existing wires in the lab before energizing. Documentation is not evidence
that the functioning source should be rewired. Hardware UART TX conflicts
with D1; Serial.begin must remain disabled. The motor interface exposes a
brake but neither controller model nor safe reverse/brake testing is documented.

## State machine and all transitions

Function pointers `cur_mode`, pending `next_mode`, and `pre` implement
entry actions. A normal switch only sets an empty pending slot; a forced
switch overrides it. No interrupts actually request these transitions.

| State | Entry / active behavior | Exits and plate command |
|---|---|---|
| setup | Load EEPROM; command startup pose; power servos; busy wait 500 ms; sample joystick center | lift_step_fk; plate PWM not explicitly initialized |
| lift_step_fk | Synchronized joint stepping to (0,0) at 0.75x speed | feed_wait_step when done; return_step on >500 current; no battery check |
| feed_wait_step | Hold delivery pose | Advanced: input then busy wait release; Simple: same or 6500 ms timeout; return_step; no battery check |
| return_step | Joint motion toward (end_x,end_y+30) | home when done; any constraint resets **all** EEPROM profiles and requests lift; plate unchanged; no battery check |
| move_home_then_wait | Alternates Cartesian 0.25 mm targets with joint stepping at 4x speed | wait_mode at home; plate unchanged |
| wait_mode | Blocking button-duration classification; choose profile | Advanced quick press -> descend; 500 ms hold -> rotate; Simple release -> brief_rotate; joystick quick -> same cycle; 1..10 s release -> calibrate; >=10 s -> reset all |
| rotate_plate_step | Ramp max(85,255*elapsed/1000), while main button held | release: PWM 0, wait, blocking 250 ms |
| brief_rotate_step | Same ramp for 600 ms | PWM 0; blocking settle 400 ms; descend |
| descend_step | Joint motion toward calibrated entry at 1x speed | scoop on completion |
| scoop_step | 0.25 mm Cartesian increments through bottom, middle, front and adjusted end | completion -> lift; >500 current -> home; input -> cancel_up |
| cancel_scoop_up_step | Vertical Cartesian motion to adjusted end Y at current X | cancel_out |
| cancel_scoop_out_step | Cartesian motion to adjusted end X/Y | lift then feed_wait (cancel still delivers) |
| calibration_mode | Plate PWM 255; joystick moves targets; blocking button confirmation/nod | five points saved -> home, plate 0; >1 s joystick hold -> home, plate 0 |
| low_power_mode | Servo power LOW, plate PWM 0; blocking 500 ms LED halves | Latched until power cycle |

Most states call check_low_power, but lift, return and feed_wait do not.
The first ordinary transition wins, so low power can lose to an already
requested normal transition. Plate stop is not guaranteed on arbitrary exits.

## Scoop and recovery

The stored points are entry, bottom, middle, front, end. Entry is reached
in joint space; subsequent segments step in Cartesian space. `get_profile_step`
adds (+5,+20) mm to the end point. A raw A0 count >400 increments a Y
offset by 1 mm and decrements the segment index (minimum 1); >500 requests
home. There is no retry/offset bound. Applied Y is capped at stored end_y.
Joint speeds are reused from descend. Several IK results are ignored, so
failed calculations can continue toward old or nonfinite targets.

## Calibration and storage

Joystick center is sampled once after startup power settling. Deadzones
are +/-100 counts; directions are discrete -1/0/1. Calibration advances
0.05 mm per loop without a defined timebase and writes IK joints directly.
Five short joystick presses store the five points; confirmation moves
elbow +0.25 rad and back in blocking loops, without checking joint limits.
The selected EEPROM slot is captured when calibration begins. Plate rotates
continuously at full PWM during calibration. Main button is not a cancel.

EEPROM stores four raw 40-byte structs at offsets 0,40,80,120, containing
ten AVR floats each, with no version/checksum/header. Slots 0/1 default to
bowl; 2/3 default to plate. Loading always succeeds for an in-range index,
constrains points silently, and incorrectly passes end_y twice. Default
points: bowl (-75,-82.5),(-70,-175),(0,-175),(70,-175),(60,-90);
plate (-80,-155),(-76,-175),(0,-177.5),(76,-180),(80,-155).
The potentiometer mapping is `(ADC-403)/146`, clamped 0..3.

## Errors, blocking and tuning inventory

FK has no bool return. IK checks D>1 only, not D<-1, finite inputs or
invalid geometry. Constraints divide by zero at zero radius and the
shoulder exclusion-circle center; infinity is not rejected. Constraints
are outer reach 200 mm, inner radius 10 mm, exclusion circle centered
(-100,0) with radius 100, y<=-0.001, x>=-180. A constraint is treated as
profile corruption on return even when only clearance needed adjustment.

Blocking waits exist in setup, both button classifications, calibration,
feed release and nod. Delays exist in LED feedback, low power, motor settle
and calibration. `millis()%100==0` schedules profile checks unreliably.

Inherited tuning: links 100/100 mm; home constants above; startup -2.09/2.09;
pulses 544/2400 us, trims 0/0; Q_EPSILON 0.00001 rad; DIST_EPSILON
0.001 mm; MAX_JOINT_SPEED 0.0003 rad/**loop** (not rad/s);
home multiplier 4, lift 0.75, nod outbound 0.5; IK_STEP_SIZE 0.25 mm;
DC_MOTOR_SPEED 255, ramp 1000 ms and minimum PWM 85;
current thresholds 400/500 ADC; low-power cutoff 662 ADC;
long press 500 ms; auto rotation 600 ms; settle 400 ms; feed wait 6500 ms;
startup settle 500 ms; joystick hold 1000 ms, reset hold 10000 ms;
calibration speed 0.05 mm/loop, deadzones 100 counts, X_SIGN -1,Y_SIGN 1;
return clearance +30 mm; scoop exit +5/+20 mm; contact offset +1 mm;
profile polling nominally 100 ms. Actual old velocity depends on loop load
and cannot be recovered reliably from source alone.

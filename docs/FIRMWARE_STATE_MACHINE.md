# Fall 2026 state responsibilities

Firmware.cpp owns the state; Control.cpp owns the active command trajectory.
Every `enter()` first stops the plate and discards the previous trajectory.
No queued ordinary transition can override a safety transition. Every loop
debounces inputs, polls the profile knob, samples battery/current on a 10 ms
elapsed schedule, and updates the LED before normal state work. LOW_POWER
and FAULT bypass normal state work. Servo commands are transactional, finite,
within inherited joint/pulse limits, and derived from current valid targets.

| State | Entry / active responsibility | Exit | Plate |
|---|---|---|---|
| STARTUP | Inherited initial command with supply disabled, enable supply; nonblocking 500 ms settle; then joystick centre sampling | STARTUP_LIFT | Stopped |
| STARTUP_LIFT | Eased synchronized move to inherited straight pose (0,0) | RETURN automatically | Stopped |
| RETURN | Joint move to selected end_x, end_y+30; valid IK required | HOME | Stopped |
| HOME | Preflight Cartesian home; if invalid, try joint home with sampled clearance-floor check | WAIT, or FAULT if neither path valid | Stopped |
| WAIT | Accept only gestures begun here; joystick long/reset gestures; latch selected mode/profile for cycle | ROTATE, AUTO_ROTATE, DESCEND, CALIBRATE or FAULT | Stopped |
| ROTATE | Advanced hold: elapsed ramp; raw main release or mode LOW stops immediately | SETTLE | Powered only while appropriate input held |
| AUTO_ROTATE | Simple elapsed ramp for 600 ms | SETTLE; new press cancels | Powered within bounded duration |
| SETTLE | 400 ms nonblocking coast interval | Simple -> DESCEND; manual -> WAIT; cancel -> retract | Stopped |
| DESCEND | Eased joint approach to calibrated entry | SCOOP; new press -> retract | Stopped |
| SCOOP | Eased Cartesian bottom/middle/front/adjusted-end traversal; preserve calibrated corners | Next segment, LIFT, CONTACT_BACKOFF, cancellation or FAULT | Stopped |
| CONTACT_BACKOFF | Validate and move vertically +1 mm, never rewind a segment; count capped globally | CONTACT_SETTLE; overload/invalid -> FAULT | Stopped |
| CONTACT_SETTLE | 200 ms pause; retain current segment and accumulated offset | SCOOP retries same segment; new press cancels | Stopped |
| LIFT | Synchronized eased joint move to (0,0) after adjusted exit | FEED_WAIT; new press cancels | Stopped |
| FEED_WAIT | Hold valid delivery command; mode fixed at cycle start | RETURN on new press, or Simple 6500 ms elapsed | Stopped |
| CANCEL_UP | From actual commanded FK, rise toward calibrated clearance without exceeding shoulder stop; no scoop continuation | CANCEL_HOME, or FAULT if path invalid | Stopped |
| CANCEL_HOME | Same preflighted home logic; never deliver canceled scoop | WAIT or FAULT | Stopped |
| CALIBRATE | Lock slot/profile for movement and cancel; rate-limit joystick; five named point releases; LED counts, save readback | Cancel -> retract; valid END -> save/retract; invalid END remains END | Stopped by default; existing rotating calibration only behind lab setting |
| LOW_POWER | Stop PWM, disable existing servo supply, retain entry timestamp for blink | Power cycle only | Stopped |
| FAULT | Stop PWM/trajectory; hold last valid servo command; blink fault count | Inspect/power cycle; only profile/storage fault accepts explicit reset-all gesture | Stopped |
| JIGGLE | Disabled default; lab-only forward/pause/reverse/pause, changing direction at zero PWM | WAIT; new press immediately -> WAIT; safety -> latched state | Powered only in timed forward/reverse phases |

The profile index and Simple/Advanced choice are captured at cycle start;
pot/mode changes cannot redirect an active feed. Calibration separately
captures its slot and associated clearance profile. Input release that began
in another state is consumed, preventing accidental calibration reentry or
a second feed after holding cancel through home.

Global current >500 causes a hold fault in every powered state, including
return/calibration/manual rotation. Battery <662 causes latched LOW_POWER
in every state. Moderate contact >400 is handled only during SCOOP, with at
most three retries and 3 mm offset. No automatic overload retract is claimed
safe because there are no position sensors and current can indicate a jam.

The sampled/preflighted paths verify mathematical command geometry only.
They cannot verify actual bowl shape, clearance, mechanical lag or contact
force. A route can fail safely at the shoulder stop. Use the physical test
guide before energizing and before any person-delivery trial.

#pragma once
#include <stdint.h>

// Existing hardware assignments: authoritative source baseline, never rewire.
namespace Config {
constexpr uint8_t INPUT_PIN=2, MODE_SELECT_PIN=1, DEBUG_PIN=4;
constexpr uint8_t SERVO_POWER_PWM=3, SERVO_POWER_DIR=12, WARNING_LED_PIN=10;
constexpr uint8_t SHOULDER_PIN=5, ELBOW_PIN=6, JOYSTICK_BUTTON_PIN=7;
constexpr uint8_t PLATE_DIR_PIN=13, PLATE_PWM_PIN=11, PLATE_BRAKE_PIN=8;
constexpr uint8_t SERVO_CURRENT_PIN=0, PLATE_CURRENT_PIN=1;
constexpr uint8_t JOY_X_PIN=2, JOY_Y_PIN=3, SERVO_VOLTAGE_PIN=4, PROFILE_POT_PIN=5;

// ARM GEOMETRY and inherited workspace (mm, radians).
constexpr float ARM_L1_MM=100.0f, ARM_L2_MM=100.0f;
constexpr float WORKSPACE_MIN_RADIUS=10.0f, WORKSPACE_MIN_Y=-0.001f;
constexpr float WORKSPACE_LEFT_FRACTION=0.9f, IK_ROUNDOFF_TOLERANCE=0.00001f;
constexpr float PROFILE_ROUNDOFF_MM=0.05f;
constexpr float SCOOP_EXIT_X_MM=5.0f, SCOOP_EXIT_Y_MM=20.0f;
constexpr float RETURN_CLEARANCE_MM=30.0f;
constexpr float STARTUP_Q1=-2.09f, STARTUP_Q2=2.09f;
constexpr float HOME_Q1=-3.1415926f, HOME_Q2=2.1817f;
constexpr float HOME_X_MM=-42.6424f, HOME_Y_MM=-81.9152f;

// SERVO LIMITS (unchanged). q1 [-pi,0], q2 [0,pi].
constexpr int SERVO_MIN_PW=544, SERVO_MAX_PW=2400, SERVO1_TRIM=0, SERVO2_TRIM=0;

// MOTION SPEEDS (rad/s). Provisional conservative caps: old firmware used
// 0.0003 rad/loop, so a measured time conversion is impossible from source.
constexpr float HOME_SPEED=0.3f, DESCEND_SPEED=0.3f, SCOOP_SPEED=0.3f;
constexpr float LIFT_SPEED=0.225f, DELIVERY_SPEED=0.225f, RETURN_SPEED=0.3f;
constexpr float CALIBRATION_SPEED=0.3f, JOINT_ACCELERATION=0.6f;
constexpr float SCOOP_CARTESIAN_SPEED=10.0f, CALIBRATION_MM_PER_SECOND=10.0f;
constexpr uint32_t MOTION_TICK_MS=20, STARTUP_SETTLE_MS=500;

// PLATE MOTOR. Preserve PWM/ramp/duration; no measured rotation angle.
constexpr uint8_t PLATE_PWM=255, PLATE_START_PWM=85;
constexpr uint32_t PLATE_RAMP_TIME=1000, AUTO_ROTATE_DURATION=600, PLATE_SETTLE_TIME=400;
constexpr bool USE_PLATE_BRAKE=false;

// BUTTON TIMING.
constexpr uint32_t BUTTON_DEBOUNCE_MS=30, LONG_PRESS_MS=500;
constexpr uint32_t CALIBRATION_HOLD_MS=1000, PROFILE_RESET_HOLD_MS=10000;
constexpr uint32_t PROFILE_CHECK_INTERVAL=100;
constexpr int PROFILE_POT_FIRST_EDGE=403, PROFILE_POT_WIDTH=146, PROFILE_POT_HYSTERESIS=10;

// CURRENT THRESHOLDS: raw ADC counts, unchanged until measured in the lab.
constexpr int THRESHOLD_CURRENT=400, OVERLOAD_CURRENT=500;
constexpr uint32_t SAFETY_CHECK_INTERVAL=10, CONTACT_SETTLE_MS=200;
constexpr float CONTACT_OFFSET_MM=1.0f, MAX_CONTACT_OFFSET_MM=3.0f;
constexpr uint8_t MAX_CONTACT_RETRIES=3;

// BATTERY: raw ADC cutoff unchanged; low-power state is latched.
constexpr int LOW_POWER_VOLTAGE=662;

// CALIBRATION. Keep deadzones/signs; eliminate physical nod confirmation.
constexpr int JOYSTICK_DEADZONE=100, JOYSTICK_X_SIGN=-1, JOYSTICK_Y_SIGN=1;
constexpr uint32_t CALIBRATION_CONFIRM_MS=180;
constexpr bool ROTATE_DURING_CALIBRATION=false;

// FEED WAIT and LED feedback.
constexpr uint32_t FEED_WAIT_TIME=6500, LED_PULSE_MS=150, LED_GROUP_PAUSE_MS=750;
constexpr uint32_t LOW_POWER_LED_HALF_MS=500;

// EXPERIMENTAL FEATURES: never in ordinary feeding by default.
constexpr uint32_t JIGGLE_FORWARD_MS=150, JIGGLE_REVERSE_MS=150, JIGGLE_PAUSE_MS=250;
constexpr uint8_t JIGGLE_PWM=85;
}
#ifndef ENABLE_PLATE_JIGGLE
#define ENABLE_PLATE_JIGGLE 0
#endif
#ifndef ENABLE_UART_TELEMETRY
#define ENABLE_UART_TELEMETRY 0
#endif
#ifndef ENABLE_DEBUG_PIN_TRACE
#define ENABLE_DEBUG_PIN_TRACE 0
#endif
#ifndef RUN_PLATE_JIGGLE_ONCE_AT_HOME
#define RUN_PLATE_JIGGLE_ONCE_AT_HOME 0
#endif
#if RUN_PLATE_JIGGLE_ONCE_AT_HOME && !ENABLE_PLATE_JIGGLE
#error "The lab boot jiggle requires ENABLE_PLATE_JIGGLE=1."
#endif
// D1 is wired to mode selection. Compilation must prevent unsafe UART use.
#if ENABLE_UART_TELEMETRY
#error "UART TX conflicts with the existing D1 mode switch; keep UART disabled."
#endif

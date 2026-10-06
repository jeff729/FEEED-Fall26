#pragma once
#include <stdint.h>

enum class State : uint8_t {
  STARTUP, STARTUP_LIFT, HOME, WAIT, ROTATE, AUTO_ROTATE, SETTLE,
  DESCEND, SCOOP, LIFT, FEED_WAIT, RETURN, CANCEL_UP, CANCEL_HOME,
  CONTACT_BACKOFF, CONTACT_SETTLE, CALIBRATE, LOW_POWER, FAULT, JIGGLE,
  RETURN_CLEAR_START
};
enum class Fault : uint8_t { NONE, KINEMATICS, PROFILE_INVALID, OVERLOAD, CONTACT_LIMIT, STORAGE };
struct DebugSnapshot {
  State state;
  Fault fault;
  float q1,q2,target_x,target_y;
  uint8_t profile;
  int16_t servo_current,battery_adc;
  uint8_t plate_pwm,retries;
  float offset;
};
void feeder_setup();
void feeder_loop();
DebugSnapshot debug_snapshot();
// Lab-only entry point; disabled unless ENABLE_PLATE_JIGGLE=1. Never called
// by the ordinary button handlers. Current hardware needs no extra input.
bool start_plate_jiggle();

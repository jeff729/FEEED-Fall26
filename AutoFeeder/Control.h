#pragma once
#include <stdint.h>
#include "kinematics.h"

struct DebouncedButton {
  bool down=false, pressed=false, released=false;
  uint32_t held_ms=0;
  void begin(bool raw, uint32_t now);
  void update(bool raw, uint32_t now);
  bool long_press(uint32_t duration, uint32_t now);
private:
  bool raw_=false, armed_=true, long_sent_=false;
  uint32_t changed_=0, pressed_at_=0;
};

enum class MotionResult { Running, Done, Invalid };
// Both joints share one eased trajectory clock. No heap or physical feedback.
class Motion {
public:
  bool begin_joint(float a, float b, float target_a, float target_b, float speed, uint32_t now);
  bool begin_cart(float a, float b, float x, float y, float speed, uint32_t now);
  MotionResult step(uint32_t now, float &a, float &b);
  void stop() { active_=false; }
  bool active() const { return active_; }
private:
  bool active_=false, cart_=false;
  float a_=0,b_=0,da_=0,db_=0,x_=0,y_=0,dx_=0,dy_=0,speed_=0;
  uint32_t last_=0,elapsed_=0,duration_=0;
  bool duration(float velocity_distance, float acceleration_distance, uint32_t now);
};

struct ContactRecovery {
  uint8_t retries=0;
  float offset=0;
  void reset() { retries=0;offset=0; }
  bool retry();
};

bool checked_ik(float x, float y, float &a, float &b);

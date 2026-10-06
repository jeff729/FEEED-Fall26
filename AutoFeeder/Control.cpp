#include "Control.h"
#include "Config.h"
#include <math.h>

void DebouncedButton::begin(bool raw,uint32_t now) {
  raw_=down=raw; armed_=!raw; pressed=released=long_sent_=false;
  changed_=pressed_at_=now; held_ms=0;
}
void DebouncedButton::update(bool raw,uint32_t now) {
  pressed=released=false;
  if (raw != raw_) { raw_=raw;changed_=now; }
  if (raw_ != down && uint32_t(now-changed_) >= Config::BUTTON_DEBOUNCE_MS) {
    down=raw_;
    if (down) { pressed_at_=now;long_sent_=false;pressed=armed_; }
    else { held_ms=uint32_t(now-pressed_at_);released=armed_;armed_=true; }
  }
}
bool DebouncedButton::long_press(uint32_t duration,uint32_t now) {
  if (!armed_ || !down || long_sent_ || uint32_t(now-pressed_at_) < duration) return false;
  long_sent_=true;
  return true;
}

bool checked_ik(float x,float y,float &a,float &b) {
  float na,nb;
  if (!calc_ik(x,y,na,nb)) return false;
  const float pi=3.14159265358979323846f;
  if (na < -pi && na >= -pi-Config::IK_ROUNDOFF_TOLERANCE) na=-pi;
  if (na > 0 && na <= Config::IK_ROUNDOFF_TOLERANCE) na=0;
  if (!valid_joint_angles(na,nb)) return false;
  a=na;b=nb;return true;
}

bool Motion::duration(float velocity_distance,float acceleration_distance,uint32_t now) {
  if (!isfinite(speed_) || speed_ <= 0 || !isfinite(velocity_distance) || !isfinite(acceleration_distance))
    return false;
  float seconds=fmaxf(1.875f*velocity_distance/speed_,
                     sqrtf(acceleration_distance/Config::JOINT_ACCELERATION));
  if (!isfinite(seconds) || seconds > 600.0f) return false;
  duration_=uint32_t(ceilf(seconds*1000.0f));
  if (duration_ < Config::MOTION_TICK_MS) duration_=Config::MOTION_TICK_MS;
  last_=now;elapsed_=0;active_=true;
  return true;
}
bool Motion::begin_joint(float a,float b,float ta,float tb,float speed,uint32_t now) {
  stop();
  if (!valid_joint_angles(a,b) || !valid_joint_angles(ta,tb)) return false;
  cart_=false;a_=a;b_=b;da_=ta-a;db_=tb-b;speed_=speed;
  float distance=fmaxf(fabsf(da_),fabsf(db_));
  return duration(distance,5.774f*distance,now);
}
bool Motion::begin_cart(float a,float b,float x,float y,float speed,uint32_t now) {
  stop();
  float ta,tb;
  if (!valid_joint_angles(a,b) || !calc_fk(a,b,x_,y_) || !checked_ik(x,y,ta,tb)) return false;
  cart_=true;a_=a;b_=b;dx_=x-x_;dy_=y-y_;speed_=speed;
  // Preflight the actual straight path. Estimate derivatives to set a slow
  // common clock; live stepping still checks every candidate and joint rate.
  const uint8_t samples=24;
  float last_a=a,last_b=b,last_sa=0,last_sb=0,derivative=0,curvature=0;
  for (uint8_t i=1;i<=samples;++i) {
    float t=float(i)/samples;
    if (!checked_ik(x_+dx_*t,y_+dy_*t,ta,tb)) return false;
    float sa=(ta-last_a)*samples,sb=(tb-last_b)*samples;
    derivative=fmaxf(derivative,fmaxf(fabsf(sa),fabsf(sb)));
    if (i>1) curvature=fmaxf(curvature,fmaxf(fabsf(sa-last_sa),fabsf(sb-last_sb))*samples);
    last_a=ta;last_b=tb;last_sa=sa;last_sb=sb;
  }
  float cart_distance=hypotf(dx_,dy_)*speed/Config::SCOOP_CARTESIAN_SPEED;
  return duration(fmaxf(derivative,cart_distance),
                  5.774f*derivative+3.516f*curvature,now);
}
MotionResult Motion::step(uint32_t now,float &a,float &b) {
  if (!active_) return MotionResult::Done;
  uint32_t delta=uint32_t(now-last_);
  if (delta < Config::MOTION_TICK_MS) return MotionResult::Running;
  last_=now;
  // No catch-up bursts: a slow loop slows the command path instead of jumping.
  delta=Config::MOTION_TICK_MS;
  uint32_t advance=delta;
  for (uint8_t tries=0;tries<12;++tries) {
    uint32_t next=elapsed_+advance;
    if (next>duration_) next=duration_;
    float t=float(next)/duration_;
    // Symmetric evaluation avoids cancellation/overshoot near t=1 on AVR float.
    float u=(t <= 0.5f) ? t : 1.0f-t;
    float s=u*u*u*(10.0f+u*(-15.0f+6.0f*u));
    if (t > 0.5f) s=1.0f-s;
    float na,nb;
    if (cart_) {
      if (!checked_ik(x_+dx_*s,y_+dy_*s,na,nb)) { stop();return MotionResult::Invalid; }
    } else { na=a_+da_*s;nb=b_+db_*s; }
    if (!valid_joint_angles(na,nb)) { stop();return MotionResult::Invalid; }
    const float max_delta=speed_*float(delta)/1000.0f+0.000001f;
    if (fabsf(na-a) <= max_delta && fabsf(nb-b) <= max_delta) {
      a=na;b=nb;elapsed_=next;
      if (elapsed_ == duration_) { stop();return MotionResult::Done; }
      return MotionResult::Running;
    }
    advance/=2;
    if (!advance) break;
  }
  stop();return MotionResult::Invalid;
}

bool ContactRecovery::retry() {
  if (retries >= Config::MAX_CONTACT_RETRIES ||
      offset+Config::CONTACT_OFFSET_MM > Config::MAX_CONTACT_OFFSET_MM+0.0001f) return false;
  ++retries;offset+=Config::CONTACT_OFFSET_MM;
  return true;
}

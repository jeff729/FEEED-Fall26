#include <cstddef>
#include <cstdio>
#include "Config.h"
#include "Profile.h"
#include "kinematics.h"
using namespace Config;
// Literal values independently transcribed from imported main c9ea8ac.
// A1 is documentation only: this contract does not establish physical wiring.
static_assert(INPUT_PIN==2 && MODE_SELECT_PIN==1 && DEBUG_PIN==4,"digital inputs");
static_assert(SERVO_POWER_PWM==3 && SERVO_POWER_DIR==12 && WARNING_LED_PIN==10,"power/LED");
static_assert(SHOULDER_PIN==5 && ELBOW_PIN==6 && JOYSTICK_BUTTON_PIN==7,"servos/button");
static_assert(PLATE_DIR_PIN==13 && PLATE_PWM_PIN==11 && PLATE_BRAKE_PIN==8,"plate pins");
static_assert(SERVO_CURRENT_PIN==0 && PLATE_CURRENT_PIN==1,"sense channels");
static_assert(JOY_X_PIN==2 && JOY_Y_PIN==3 && SERVO_VOLTAGE_PIN==4 && PROFILE_POT_PIN==5,"analog source assignments");
static_assert(ARM_L1_MM==100 && ARM_L2_MM==100,"link lengths");
static_assert(SERVO_MIN_PW==544 && SERVO_MAX_PW==2400 && SERVO1_TRIM==0 && SERVO2_TRIM==0,"pulse limits/trims");
static_assert(HOME_Q1==-3.1415926f && HOME_Q2==2.1817f && HOME_X_MM==-42.6424f && HOME_Y_MM==-81.9152f,"home");
static_assert(STARTUP_Q1==-2.09f && STARTUP_Q2==2.09f,"startup");
static_assert(WORKSPACE_MIN_RADIUS==10 && WORKSPACE_MIN_Y==-0.001f && WORKSPACE_LEFT_FRACTION==0.9f,"workspace");
static_assert(SCOOP_EXIT_X_MM==5 && SCOOP_EXIT_Y_MM==20 && RETURN_CLEARANCE_MM==30,"offsets");
static_assert(THRESHOLD_CURRENT==400 && OVERLOAD_CURRENT==500 && LOW_POWER_VOLTAGE==662,"ADC thresholds");
static_assert(FEED_WAIT_TIME==6500,"inherited Simple timeout requires caregiver review");
static_assert(NUM_PROFILES==4 && sizeof(Profile)==40,"EEPROM slot size/count");
static_assert(offsetof(Profile,entry_x)==0 && offsetof(Profile,entry_y)==4 &&
              offsetof(Profile,bottom_x)==8 && offsetof(Profile,bottom_y)==12 &&
              offsetof(Profile,middle_x)==16 && offsetof(Profile,middle_y)==20 &&
              offsetof(Profile,front_x)==24 && offsetof(Profile,front_y)==28 &&
              offsetof(Profile,end_x)==32 && offsetof(Profile,end_y)==36,"EEPROM field order");
static_assert(!USE_PLATE_BRAKE && !ROTATE_DURING_CALIBRATION,"normal physical experiments off");
static_assert(!ENABLE_PLATE_JIGGLE && !ENABLE_UART_TELEMETRY &&
              !ENABLE_DEBUG_PIN_TRACE && !RUN_PLATE_JIGGLE_ONCE_AT_HOME,"normal build flags");
const float L1=100,L2=100;
int main() {
  const float pi=3.14159265358979323846f;
  bool ok=valid_joint_angles(-pi,pi) && valid_joint_angles(0,0) &&
    !valid_joint_angles(-pi-0.001f,0) && !valid_joint_angles(0.001f,0) &&
    !valid_joint_angles(0,-0.001f) && !valid_joint_angles(0,pi+0.001f);
  std::printf("hardware invariants: 18 compile-time groups, 6 joint-boundary predicates: %s\n",ok?"PASS":"FAIL");
  return ok?0:1;
}

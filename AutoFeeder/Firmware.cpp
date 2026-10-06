#include "Firmware.h"
#include "Config.h"
#include "Control.h"
#include "Profile.h"
#include "Joystick.h"
#include "DCMotor.h"
#include <Arduino.h>
#include <Servo.h>
#include <math.h>

const float L1=Config::ARM_L1_MM, L2=Config::ARM_L2_MM;
uint16_t X_CENTER=512,Y_CENTER=512;

namespace {
using namespace Config;
Servo shoulder_servo,elbow_servo;
Motion motion;
DebouncedButton main_button,joy_button,mode_switch;
ContactRecovery contact;
State state=State::STARTUP;
Fault fault=Fault::NONE;
uint32_t entered=0,last_safety=0,last_profile=0,last_calibration=0,confirm_until_start=0,led_started=0;
float q1=STARTUP_Q1,q2=STARTUP_Q2,target_x=0,target_y=0;
int16_t servo_current=0,battery_adc=0;
uint8_t selected=0,cycle_profile=0,scoop_segment=1,plate_pwm=0,led_pulses=0;
bool simple_cycle=false,manual_long=false,calibration_confirm=false;
bool wait_main_pending=false,wait_joy_pending=false;
bool loaded_valid[NUM_PROFILES]={};
CalibrationPoint calibration_point=CalibrationPoint::ENTRY;
Profile profile,calibration_profile;
uint8_t calibration_slot=0;
#if ENABLE_PLATE_JIGGLE
uint8_t jiggle_phase=0;
uint32_t jiggle_timestamp=0;
bool lab_jiggle_done=false;
#endif

void stop_plate() {
  DCMotor::stop(USE_PLATE_BRAKE);plate_pwm=0;
  DCMotor::set_direction(false);
}
void command_plate(uint8_t pwm) {
  if (state==State::FAULT || state==State::LOW_POWER) { stop_plate();return; }
  DCMotor::set_brake(false);
  DCMotor::set_speed(pwm);plate_pwm=pwm;
}
void enter(State next) {
  // Single transition owner: no state can inherit a powered plate or stale path.
  stop_plate();motion.stop();state=next;entered=uint32_t(millis());
  wait_main_pending=wait_joy_pending=false;
  if(next==State::WAIT) {main_button.require_release(entered);joy_button.require_release(entered);}
}
void set_fault(Fault code) {
  fault=code;enter(State::FAULT);
}
void clear_fault() { fault=Fault::NONE; }
void pulse_led(uint8_t pulses) {led_pulses=pulses;led_started=uint32_t(millis());}

bool write_servos(float a,float b) {
  if (!valid_joint_angles(a,b)) {set_fault(Fault::KINEMATICS);return false;}
  const float pi=3.14159265358979323846f;
  const float slope=float(SERVO_MAX_PW-SERVO_MIN_PW)/pi;
  const float center=SERVO_MIN_PW+slope*pi*0.5f;
  int p1=int(slope*(-a-pi*0.5f)+center)+SERVO1_TRIM;
  int p2=int(slope*(-b+pi*0.5f)+center)+SERVO2_TRIM;
  if (p1<SERVO_MIN_PW || p1>SERVO_MAX_PW || p2<SERVO_MIN_PW || p2>SERVO_MAX_PW) {
    set_fault(Fault::KINEMATICS);return false;
  }
  // Calculate and validate both pulses before either physical command.
  shoulder_servo.writeMicroseconds(p1);elbow_servo.writeMicroseconds(p2);q1=a;q2=b;
  if (!calc_fk(q1,q2,target_x,target_y)) {set_fault(Fault::KINEMATICS);return false;}
  return true;
}

bool begin_joint(float a,float b,float speed,State next) {
  enter(next);
  if (!motion.begin_joint(q1,q2,a,b,speed,entered)) {set_fault(Fault::KINEMATICS);return false;}
  return true;
}
bool begin_point(float x,float y,float speed,State next,bool cartesian) {
  float a,b;
  if (!checked_ik(x,y,a,b)) {set_fault(Fault::KINEMATICS);return false;}
  enter(next);
  bool ok=cartesian ? motion.begin_cart(q1,q2,x,y,speed,entered) : motion.begin_joint(q1,q2,a,b,speed,entered);
  if (!ok) {set_fault(Fault::KINEMATICS);return false;}
  return true;
}
bool move_tick(uint32_t now) {
  float a=q1,b=q2;
  MotionResult result=motion.step(now,a,b);
  if (result==MotionResult::Invalid) {set_fault(Fault::KINEMATICS);return false;}
  if ((a!=q1 || b!=q2) && !write_servos(a,b)) return false;
  return result==MotionResult::Done;
}
void start_home(State next=State::HOME) {
  enter(next);
  if(motion.begin_cart(q1,q2,HOME_X,HOME_Y,HOME_SPEED,entered))return;
  float x,y;
  if(!calc_fk(q1,q2,x,y)) {set_fault(Fault::KINEMATICS);return;}
  const float floor=fminf(y,HOME_Y)-Config::PROFILE_ROUNDOFF_MM;
  // A straight Cartesian chord can cross the shoulder exclusion circle.
  // Preflight a synchronized joint path and reject any downward dip below
  // the clearance already established. This is command-space validation.
  for(uint8_t i=1;i<=24;++i) {
    float t=float(i)/24;
    if(!calc_fk(q1+(Q1_HOME-q1)*t,q2+(Q2_HOME-q2)*t,x,y) || y<floor) {
      set_fault(Fault::KINEMATICS);return;
    }
  }
  begin_joint(Q1_HOME,Q2_HOME,HOME_SPEED,next);
}
bool joint_clearance_path(float a,float b,float floor) {
  float x,y;
  for(uint8_t i=1;i<=24;++i) {
    float t=float(i)/24;
    if(!calc_fk(q1+(a-q1)*t,q2+(b-q2)*t,x,y) || y<floor-Config::PROFILE_ROUNDOFF_MM)return false;
  }
  return true;
}
void start_return() {
  const float x=profile.end_x,y=profile.end_y+RETURN_CLEARANCE_MM;
  if(valid_cartesian_angles(q1,q2)) {
    begin_point(x,y,RETURN_SPEED,State::RETURN,true);return;
  }
  // Escape the straight-arm singularity along the first part of the inherited
  // joint return route. Verify clearance before movement; then use a straight
  // Cartesian route to the existing calibrated return waypoint.
  float a,b,cx,cy;
  if(!checked_ik(x,y,a,b) || !calc_fk(q1,q2,cx,cy)) {set_fault(Fault::KINEMATICS);return;}
  float fraction=RETURN_START_FRACTION;
  if(b>q2)fraction=fmaxf(fraction,(2*CARTESIAN_SINGULARITY_ANGLE-q2)/(b-q2));
  if(fraction>1)fraction=1;
  a=q1+(a-q1)*fraction;b=q2+(b-q2)*fraction;
  if(!valid_cartesian_angles(a,b) || !joint_clearance_path(a,b,fminf(cy,y))) {
    set_fault(Fault::KINEMATICS);return;
  }
  begin_joint(a,b,RETURN_SPEED,State::RETURN_CLEAR_START);
}
void safe_abort(bool retract) {
  stop_plate();motion.stop();
  if (!retract) return;
  float x,y;
  if (!calc_fk(q1,q2,x,y)) {set_fault(Fault::KINEMATICS);return;}
  // Use the existing calibrated end + inherited clearance; never move lower
  // to "clear" a bowl. At/above the rim preserve height during retreat.
  float clearance=profile.end_y+RETURN_CLEARANCE_MM;
  if (y>=clearance) {
    if(!valid_cartesian_angles(q1,q2))start_return();
    else begin_point(profile.end_x,y,RETURN_SPEED,State::RETURN,true);
    return;
  }
  // Respect the inherited shoulder -pi stop while retracting vertically.
  // This ceiling comes from arm geometry, not an invented bowl dimension.
  float shoulder_x=x+L1;
  if(x<0 && fabsf(shoulder_x)<L2) {
    float ceiling=-sqrtf(L2*L2-shoulder_x*shoulder_x);
    if(clearance>ceiling)clearance=ceiling;
  }
  if(clearance<y)clearance=y;
  begin_point(x,clearance,RETURN_SPEED,State::CANCEL_UP,true);
}

uint8_t pot_choice() {
  int index=(analogRead(PROFILE_POT_PIN)-PROFILE_POT_FIRST_EDGE)/PROFILE_POT_WIDTH;
  if (index<0)index=0;
  if (index>=NUM_PROFILES)index=NUM_PROFILES-1;
  return uint8_t(index);
}
void check_profile_choice(uint32_t now) {
  if (uint32_t(now-last_profile)<PROFILE_CHECK_INTERVAL)return;
  last_profile=now;
  int value=analogRead(PROFILE_POT_PIN);
  uint8_t candidate=pot_choice();
  if (candidate>selected && value<PROFILE_POT_FIRST_EDGE+PROFILE_POT_WIDTH*candidate+PROFILE_POT_HYSTERESIS)return;
  if (candidate<selected && value>PROFILE_POT_FIRST_EDGE+PROFILE_POT_WIDTH*selected-PROFILE_POT_HYSTERESIS)return;
  if (candidate!=selected) {selected=candidate;pulse_led(selected+1);}
}
bool start_cycle() {
  if (!loaded_valid[selected] || !validate_profile(profiles[selected])) {set_fault(Fault::PROFILE_INVALID);return false;}
  profile=profiles[selected];cycle_profile=selected;simple_cycle=mode_switch.down;
  contact.reset();scoop_segment=1;
  if (simple_cycle) enter(State::AUTO_ROTATE);
  else begin_point(profile.entry_x,profile.entry_y,DESCEND_SPEED,State::DESCEND,false);
  return true;
}
void start_scoop_segment() {
  float x,y;
  if (!get_profile_step(profile,scoop_segment,x,y)) {set_fault(Fault::PROFILE_INVALID);return;}
  // Preserve exit adjustment; bounded contact offset raises the current segment
  // without rewinding to an earlier profile point or jumping down again.
  y+=contact.offset;
  begin_point(x,y,SCOOP_SPEED,State::SCOOP,true);
}
void start_startup() {
  clear_fault();enter(State::STARTUP);
  digitalWrite(SERVO_POWER_PWM,LOW);
  if (write_servos(STARTUP_Q1,STARTUP_Q2))digitalWrite(SERVO_POWER_PWM,HIGH);
}
void reset_selected_profile() {
  // Preserve the inherited explicit ten-second reset-all gesture. This is
  // never called automatically for corruption or a constrained motion target.
  for(uint8_t i=0;i<NUM_PROFILES;++i) {
    profiles[i]=default_profile(i);
    if(!save_profile(profiles[i],i)) {set_fault(Fault::STORAGE);return;}
    loaded_valid[i]=true;
  }
  profile=profiles[selected];cycle_profile=selected;
  pulse_led(4);
  if (state==State::FAULT) start_startup();
}
void start_calibration() {
  calibration_slot=selected;calibration_profile=profiles[selected];
  // Cancellation must use this bowl's clearance, not the preceding feed's.
  profile=profiles[calibration_slot];cycle_profile=calibration_slot;
  calibration_point=CalibrationPoint::ENTRY;calibration_confirm=false;
  enter(State::CALIBRATE);last_calibration=entered;pulse_led(1);
}
void store_calibration_point(float x,float y) {
  switch(calibration_point) {
    case CalibrationPoint::ENTRY: calibration_profile.entry_x=x;calibration_profile.entry_y=y;break;
    case CalibrationPoint::BOTTOM: calibration_profile.bottom_x=x;calibration_profile.bottom_y=y;break;
    case CalibrationPoint::MIDDLE: calibration_profile.middle_x=x;calibration_profile.middle_y=y;break;
    case CalibrationPoint::FRONT: calibration_profile.front_x=x;calibration_profile.front_y=y;break;
    case CalibrationPoint::END: calibration_profile.end_x=x;calibration_profile.end_y=y;break;
  }
}
void calibration_tick(uint32_t now) {
  if (main_button.pressed || joy_button.long_press(CALIBRATION_HOLD_MS,now)) {safe_abort(true);return;}
  if (calibration_confirm) {
    if (uint32_t(now-confirm_until_start)<CALIBRATION_CONFIRM_MS)return;
    calibration_confirm=false;
  }
  if (ROTATE_DURING_CALIBRATION)command_plate(PLATE_PWM);
  if (uint32_t(now-last_calibration)>=MOTION_TICK_MS) {
    last_calibration=now;
    float x,y;
    if (!calc_fk(q1,q2,x,y)) {set_fault(Fault::KINEMATICS);return;}
    x+=CALIBRATION_MM_PER_SECOND*float(MOTION_TICK_MS)/1000*read_joystick_x();
    y+=CALIBRATION_MM_PER_SECOND*float(MOTION_TICK_MS)/1000*read_joystick_y();
    if (project_workspace(x,y)==WorkspaceResult::Invalid) {set_fault(Fault::KINEMATICS);return;}
    float a,b;
    if (!checked_ik(x,y,a,b)) {set_fault(Fault::KINEMATICS);return;}
    // Scale both deltas together so an IK singularity cannot jump a joint.
    float da=a-q1,db=b-q2;
    float biggest=fmaxf(fabsf(da),fabsf(db));
    float limit=CALIBRATION_SPEED*float(MOTION_TICK_MS)/1000;
    float fraction=biggest>limit ? limit/biggest : 1.0f;
    if (da!=0 || db!=0)if (!write_servos(q1+da*fraction,q2+db*fraction))return;
  }
  if (joy_button.released && joy_button.held_ms<CALIBRATION_HOLD_MS) {
    // Store achieved command position, not a target the servos have not received.
    float x,y;
    if (!calc_fk(q1,q2,x,y)) {set_fault(Fault::KINEMATICS);return;}
    store_calibration_point(x,y);
    if (calibration_point==CalibrationPoint::END) {
      if (!normalize_profile(calibration_profile)) {pulse_led(6);return;}
      if (!save_profile(calibration_profile,calibration_slot)) {set_fault(Fault::STORAGE);return;}
      profiles[calibration_slot]=calibration_profile;loaded_valid[calibration_slot]=true;
      profile=calibration_profile;cycle_profile=calibration_slot;pulse_led(5);
      safe_abort(true);return;
    }
    calibration_point=CalibrationPoint(uint8_t(calibration_point)+1);
    calibration_confirm=true;confirm_until_start=now;pulse_led(uint8_t(calibration_point)+1);
  }
}

bool safety_tick(uint32_t now,bool force=false) {
  if (!force && uint32_t(now-last_safety)<SAFETY_CHECK_INTERVAL)return state!=State::LOW_POWER && state!=State::FAULT;
  last_safety=now;
  battery_adc=analogRead(SERVO_VOLTAGE_PIN);servo_current=analogRead(SERVO_CURRENT_PIN);
  if (state==State::LOW_POWER)return false; // Preserve entry clock / latched warning.
  if (battery_adc<LOW_POWER_VOLTAGE) {
    enter(State::LOW_POWER);digitalWrite(SERVO_POWER_PWM,LOW);return false;
  }
  if (state!=State::FAULT && servo_current>OVERLOAD_CURRENT) {set_fault(Fault::OVERLOAD);return false;}
  return state!=State::FAULT;
}
void update_led(uint32_t now) {
  bool on=false;
  if (state==State::LOW_POWER)on=(uint32_t(now-entered)%(2*LOW_POWER_LED_HALF_MS))<LOW_POWER_LED_HALF_MS;
  else if(state==State::FAULT) {
    uint8_t count=uint8_t(fault);
    uint32_t group=count*2*LED_PULSE_MS+LED_GROUP_PAUSE_MS;
    uint32_t phase=uint32_t(now-entered)%group;
    on=phase<count*2*LED_PULSE_MS && (phase/LED_PULSE_MS)%2==0;
  } else if(led_pulses && uint32_t(now-led_started)<led_pulses*2*LED_PULSE_MS) {
    on=(uint32_t(now-led_started)/LED_PULSE_MS)%2==0;
  } else {
    led_pulses=0;
    if(state==State::CALIBRATE) {
      uint8_t count=uint8_t(calibration_point)+1;
      uint32_t group=count*2*LED_PULSE_MS+LED_GROUP_PAUSE_MS;
      uint32_t phase=uint32_t(now-entered)%group;
      on=phase<count*2*LED_PULSE_MS && (phase/LED_PULSE_MS)%2==0;
    }
  }
  digitalWrite(WARNING_LED_PIN,on?HIGH:LOW);
}
void rotate_tick(uint32_t now,bool automatic) {
  uint32_t elapsed=uint32_t(now-entered);
  // Immediate raw release stop, even while debounced input is catching up.
  if (!automatic && (digitalRead(INPUT_PIN)==HIGH || digitalRead(MODE_SELECT_PIN)==LOW)) {enter(State::SETTLE);return;}
  if (automatic && elapsed>=AUTO_ROTATE_DURATION) {enter(State::SETTLE);return;}
  uint32_t ramp=elapsed<PLATE_RAMP_TIME?elapsed:PLATE_RAMP_TIME;
  uint8_t pwm=PLATE_RAMP_TIME ? uint32_t(PLATE_PWM)*ramp/PLATE_RAMP_TIME : PLATE_PWM;
  if (pwm<PLATE_START_PWM)pwm=PLATE_START_PWM;
  command_plate(pwm);
}
#if ENABLE_PLATE_JIGGLE
void jiggle_tick(uint32_t now) {
  uint32_t elapsed=uint32_t(now-jiggle_timestamp);
  switch(jiggle_phase) {
    case 0:
      if(elapsed>=JIGGLE_FORWARD_MS) {stop_plate();jiggle_phase=1;jiggle_timestamp=now;}
      else command_plate(JIGGLE_PWM);
      break;
    case 1:
      if(elapsed>=JIGGLE_PAUSE_MS) {
        DCMotor::set_direction(true);jiggle_phase=2;jiggle_timestamp=now;command_plate(JIGGLE_PWM);
      }
      break;
    case 2:
      if(elapsed>=JIGGLE_REVERSE_MS) {stop_plate();jiggle_phase=3;jiggle_timestamp=now;}
      else command_plate(JIGGLE_PWM);
      break;
    default:
      if(elapsed>=JIGGLE_PAUSE_MS)enter(State::WAIT);
      break;
  }
}
#endif
} // namespace

void feeder_setup() {
  using namespace Config;
  digitalWrite(SERVO_POWER_PWM,LOW);pinMode(SERVO_POWER_PWM,OUTPUT);
  pinMode(SERVO_POWER_DIR,OUTPUT);digitalWrite(SERVO_POWER_DIR,HIGH);
  pinMode(DEBUG_PIN,OUTPUT);pinMode(WARNING_LED_PIN,OUTPUT);
  pinMode(INPUT_PIN,INPUT_PULLUP);pinMode(MODE_SELECT_PIN,INPUT_PULLUP);
  pinMode(Config::JOYSTICK_BUTTON_PIN,INPUT_PULLUP);
  shoulder_servo.attach(SHOULDER_PIN,SERVO_MIN_PW,SERVO_MAX_PW);
  elbow_servo.attach(ELBOW_PIN,SERVO_MIN_PW,SERVO_MAX_PW);
  DCMotor::attach();stop_plate();DCMotor::set_direction(false);
  uint32_t now=uint32_t(millis());
  main_button.begin(digitalRead(INPUT_PIN)==LOW,now);
  joy_button.begin(read_joystick_button(),now);
  mode_switch.begin(digitalRead(MODE_SELECT_PIN)==LOW,now);
  last_safety=last_profile=now;led_pulses=0;manual_long=false;contact.reset();
#if ENABLE_PLATE_JIGGLE
  lab_jiggle_done=false;
#endif
  selected=pot_choice();cycle_profile=selected;clear_fault();
  for(uint8_t i=0;i<NUM_PROFILES;++i) {
    loaded_valid[i]=load_profile(i,profiles[i]);
    if (!loaded_valid[i])profiles[i]=default_profile(i); // RAM only, never automatic EEPROM repair.
  }
  profile=profiles[selected];
  enter(State::STARTUP);
  if (!safety_tick(now,true))return;
  if (!loaded_valid[selected]) {set_fault(Fault::PROFILE_INVALID);return;}
  start_startup();
}

void feeder_loop() {
  using namespace Config;
  uint32_t now=uint32_t(millis());
  main_button.update(digitalRead(INPUT_PIN)==LOW,now);
  joy_button.update(read_joystick_button(),now);
  mode_switch.update(digitalRead(MODE_SELECT_PIN)==LOW,now);
  check_profile_choice(now);
#if ENABLE_DEBUG_PIN_TRACE
  // Existing oscilloscope debug output D4; no UART or added feeder wiring.
  // High pulse count per group is state enum + 1 (50 ms on / 50 ms off).
  uint32_t trace_count=uint8_t(state)+1;
  uint32_t trace_phase=uint32_t(now-entered)%(trace_count*100+750);
  digitalWrite(DEBUG_PIN,(trace_phase<trace_count*100 && (trace_phase/50)%2==0)?HIGH:LOW);
#endif
  bool safe=safety_tick(now);
  update_led(now);
  if (!safe) {
    stop_plate();
    // Faults are latched. Only an invalid-profile/storage fault can be repaired
    // by an explicit reset; overload and kinematics require inspection/restart.
    if (state==State::FAULT && (fault==Fault::PROFILE_INVALID || fault==Fault::STORAGE) &&
        joy_button.released && joy_button.held_ms>=PROFILE_RESET_HOLD_MS)reset_selected_profile();
    return;
  }
  bool cancel=main_button.pressed || joy_button.pressed;
  if(cancel && state==State::JIGGLE) {enter(State::WAIT);return;}
  if (cancel && (state==State::AUTO_ROTATE || state==State::SETTLE || state==State::DESCEND ||
                 state==State::SCOOP || state==State::CONTACT_BACKOFF || state==State::CONTACT_SETTLE || state==State::LIFT)) {
    safe_abort(true);return;
  }
  switch(state) {
    case State::STARTUP:
      if (uint32_t(now-entered)>=STARTUP_SETTLE_MS) {
        X_CENTER=analogRead(JOY_X_PIN);Y_CENTER=analogRead(JOY_Y_PIN);
        begin_joint(0,0,DELIVERY_SPEED,State::STARTUP_LIFT);
      }
      break;
    case State::STARTUP_LIFT:
      if (move_tick(now))start_return();
      break;
    case State::HOME:
    case State::CANCEL_HOME:
      if (move_tick(now))enter(State::WAIT);
      break;
    case State::WAIT:
#if RUN_PLATE_JIGGLE_ONCE_AT_HOME
      if(!lab_jiggle_done) {lab_jiggle_done=true;start_plate_jiggle();break;}
#endif
      if (main_button.pressed) {manual_long=false;wait_main_pending=true;}
      if (joy_button.pressed)wait_joy_pending=true;
      if (wait_main_pending && !mode_switch.down && main_button.long_press(LONG_PRESS_MS,now)) {
        manual_long=true;simple_cycle=false;enter(State::ROTATE);break;
      }
      if (main_button.released && wait_main_pending && !manual_long) {wait_main_pending=false;start_cycle();break;}
      if (joy_button.released && wait_joy_pending) {
        wait_joy_pending=false;
        if (joy_button.held_ms>=PROFILE_RESET_HOLD_MS)reset_selected_profile();
        else if(joy_button.held_ms>=CALIBRATION_HOLD_MS)start_calibration();
        else start_cycle();
      }
      break;
    case State::ROTATE: rotate_tick(now,false);break;
    case State::AUTO_ROTATE: rotate_tick(now,true);break;
    case State::SETTLE:
      if(uint32_t(now-entered)>=PLATE_SETTLE_TIME) {
        if(simple_cycle)begin_point(profile.entry_x,profile.entry_y,DESCEND_SPEED,State::DESCEND,false);
        else enter(State::WAIT);
      }
      break;
    case State::DESCEND:
      if(move_tick(now))start_scoop_segment();
      break;
    case State::SCOOP:
      if(servo_current>THRESHOLD_CURRENT) {
        if(!contact.retry()) {set_fault(Fault::CONTACT_LIMIT);break;}
        float x,y;
        if(!calc_fk(q1,q2,x,y)) {set_fault(Fault::KINEMATICS);break;}
        begin_point(x,y+CONTACT_OFFSET_MM,SCOOP_SPEED,State::CONTACT_BACKOFF,true);
      } else if(move_tick(now)) {
        if(++scoop_segment<=4)start_scoop_segment();
        else begin_joint(0,0,LIFT_SPEED,State::LIFT);
      }
      break;
    case State::CONTACT_BACKOFF:
      if(move_tick(now))enter(State::CONTACT_SETTLE);
      break;
    case State::CONTACT_SETTLE:
      if(uint32_t(now-entered)>=CONTACT_SETTLE_MS)start_scoop_segment();
      break;
    case State::LIFT:
      if(move_tick(now))enter(State::FEED_WAIT);
      break;
    case State::FEED_WAIT:
      if(cancel || (simple_cycle && uint32_t(now-entered)>=FEED_WAIT_TIME))start_return();
      break;
    case State::RETURN_CLEAR_START:
      if(move_tick(now))begin_point(profile.end_x,profile.end_y+RETURN_CLEARANCE_MM,RETURN_SPEED,State::RETURN,true);
      break;
    case State::RETURN:
      if(move_tick(now))start_home();
      break;
    case State::CANCEL_UP:
      if(move_tick(now))start_home(State::CANCEL_HOME);
      break;
    case State::CALIBRATE: calibration_tick(now);break;
    case State::LOW_POWER:
    case State::FAULT: stop_plate();break;
    case State::JIGGLE:
#if ENABLE_PLATE_JIGGLE
      jiggle_tick(now);
#else
      stop_plate();enter(State::WAIT);
#endif
      break;
  }
}

DebugSnapshot debug_snapshot() {
  return {state,fault,q1,q2,target_x,target_y,cycle_profile,servo_current,battery_adc,
          plate_pwm,contact.retries,contact.offset};
}
bool start_plate_jiggle() {
#if ENABLE_PLATE_JIGGLE
  if(state!=State::WAIT || fault!=Fault::NONE || main_button.down || joy_button.down)return false;
  if(!safety_tick(uint32_t(millis()),true))return false;
  enter(State::JIGGLE);jiggle_phase=0;jiggle_timestamp=entered;
  return true;
#else
  return false;
#endif
}

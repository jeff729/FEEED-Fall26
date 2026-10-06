#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "Firmware.h"
#include "Profile.h"
#include "Config.h"
#include "Arduino.h"
#include "EEPROM.h"
uint32_t test_now=0;
int test_digital[20],test_analog[6],test_pwm[20],test_output[20],test_servo_pulse[20],test_servo_writes=0;
FakeEEPROM EEPROM;
void delay(unsigned long) { throw std::runtime_error("Blocking delay in firmware"); }
static int checks=0,failures=0;
#define CHECK(c) do { ++checks;if(!(c)){ std::fprintf(stderr,"FAIL %d: %s (state=%d fault=%d)\n",__LINE__,#c,int(debug_snapshot().state),int(debug_snapshot().fault));++failures; } } while(0)
static void run(uint32_t ms) { for(uint32_t i=0;i<ms;i+=10) {test_now+=10;feeder_loop();} }
static void click(int pin,uint32_t hold=100) {test_digital[pin]=LOW;run(hold);test_digital[pin]=HIGH;run(60);}
static bool await(State state,uint32_t budget=120000) {
  for(uint32_t i=0;i<budget;i+=10) {if(debug_snapshot().state==state)return true;run(10);}
  return debug_snapshot().state==state;
}
static bool joystick_to(float x,float y) {
  for(uint32_t i=0;i<90000;i+=20) {
    DebugSnapshot d=debug_snapshot();
    float dx=x-d.target_x,dy=y-d.target_y;
    if(std::fabs(dx)<0.35f && std::fabs(dy)<0.35f) {
      test_analog[2]=test_analog[3]=512;run(40);return true;
    }
    test_analog[2]=std::fabs(dx)<0.3f?512:(dx>0?312:712);
    test_analog[3]=std::fabs(dy)<0.3f?512:(dy>0?712:312);
    run(20);
    if(debug_snapshot().state!=State::CALIBRATE)return false;
  }
  test_analog[2]=test_analog[3]=512;return false;
}
static void boot(bool simple=false,bool valid=true) {
  test_now=0;test_servo_writes=0;
  for(int &v:test_digital)v=HIGH;
  std::memset(test_pwm,0,sizeof(test_pwm));std::memset(test_output,0,sizeof(test_output));
  for(int &v:test_analog)v=512;
  test_analog[Config::SERVO_CURRENT_PIN]=100;
  test_analog[Config::SERVO_VOLTAGE_PIN]=800;
  test_analog[Config::PROFILE_POT_PIN]=476;
  test_digital[Config::MODE_SELECT_PIN]=simple?LOW:HIGH;
  std::memset(EEPROM.bytes,0xff,sizeof(EEPROM.bytes));EEPROM.writes=0;
  if(valid)for(uint8_t i=0;i<4;++i)save_profile(default_profile(i),i);
  feeder_setup();
}
int main() {
  try {
    boot(); CHECK(await(State::WAIT));CHECK(test_pwm[11]==0);
    click(2);CHECK(await(State::SCOOP));
    click(2);CHECK(await(State::WAIT)); // Cancel retracts/home; never delivers.
    CHECK(test_pwm[11]==0);CHECK(debug_snapshot().fault==Fault::NONE);

    boot();CHECK(await(State::WAIT));
    test_digital[2]=LOW;run(650);CHECK(debug_snapshot().state==State::ROTATE);
    CHECK(test_pwm[11]>0);
    test_digital[2]=HIGH;run(10);CHECK(test_pwm[11]==0); // Raw release stops before debounce.
    CHECK(await(State::WAIT));run(500);CHECK(debug_snapshot().state==State::WAIT);

    boot(true);CHECK(await(State::WAIT));
    test_digital[2]=LOW;run(2000);CHECK(debug_snapshot().state==State::WAIT); // Release starts cycle.
    test_digital[2]=HIGH;run(60);CHECK(debug_snapshot().state==State::AUTO_ROTATE);
    run(650);CHECK(test_pwm[11]==0);CHECK(debug_snapshot().state==State::SETTLE);
    CHECK(await(State::FEED_WAIT));
    test_digital[2]=LOW;run(100); // Early return, then keep holding through home.
    CHECK(await(State::WAIT));run(5000);CHECK(debug_snapshot().state==State::WAIT);
    CHECK(test_pwm[11]==0);

    boot();CHECK(await(State::WAIT));
    test_digital[2]=LOW;run(650);CHECK(test_pwm[11]>0);
    test_analog[4]=600;run(10);CHECK(debug_snapshot().state==State::LOW_POWER);
    CHECK(test_pwm[11]==0 && test_output[3]==LOW);
    test_analog[4]=800;run(2000);CHECK(debug_snapshot().state==State::LOW_POWER);

    boot();CHECK(await(State::WAIT));click(2);CHECK(await(State::SCOOP));
    int writes=test_servo_writes;
    test_analog[0]=501;run(10);CHECK(debug_snapshot().state==State::FAULT);
    CHECK(test_pwm[11]==0 && test_servo_writes==writes);
    CHECK(debug_snapshot().fault==Fault::OVERLOAD);

    boot(false,false);CHECK(debug_snapshot().state==State::FAULT);
    CHECK(debug_snapshot().fault==Fault::PROFILE_INVALID);
    CHECK(EEPROM.writes==0);CHECK(test_pwm[11]==0);
    // Explicit ten-second reset recovers invalid EEPROM without automatic writes.
    click(7,10100);CHECK(await(State::WAIT));CHECK(EEPROM.writes==4);

    boot();CHECK(await(State::WAIT));click(7,1100);
    CHECK(debug_snapshot().state==State::CALIBRATE);CHECK(test_pwm[11]==0);
    int writes_before=EEPROM.writes;
    test_analog[5]=950;run(300); // Pot changes cannot redirect calibration save.
    click(7,1100);CHECK(await(State::WAIT));CHECK(EEPROM.writes==writes_before);
    CHECK(test_pwm[11]==0);

    boot();CHECK(await(State::WAIT));click(2);CHECK(await(State::SCOOP));
    run(5000);test_analog[0]=450;run(30000);
    CHECK(debug_snapshot().state==State::FAULT);
    CHECK(debug_snapshot().fault==Fault::CONTACT_LIMIT);
    CHECK(debug_snapshot().retries==3 && debug_snapshot().offset<=3);
    CHECK(test_pwm[11]==0);

    boot();CHECK(await(State::WAIT));
    test_analog[5]=800;run(200);click(7,1100);
    CHECK(debug_snapshot().state==State::CALIBRATE);
    Profile previous_slot3;EEPROM.get(120,previous_slot3);
    writes_before=EEPROM.writes;
    test_analog[5]=1000;run(200); // Move knob, but save must remain in slot 2.
    CHECK(joystick_to(-80,-155));click(7);run(250);
    CHECK(joystick_to(-76,-175));click(7);run(250);
    CHECK(joystick_to(0,-177.5f));click(7);run(250);
    CHECK(joystick_to(76,-180));click(7);run(250);
    CHECK(joystick_to(80,-155));click(7);
    CHECK(await(State::WAIT));CHECK(EEPROM.writes==writes_before+1);
    Profile saved,unchanged;EEPROM.get(80,saved);EEPROM.get(120,unchanged);
    CHECK(validate_profile(saved));CHECK(std::fabs(saved.entry_x+80)<0.5f);
    CHECK(std::memcmp(&previous_slot3,&unchanged,sizeof(Profile))==0);

    boot();CHECK(await(State::WAIT));click(7,1100);writes_before=EEPROM.writes;
    for(int point=0;point<5;++point) {click(7);if(point<4)run(250);}
    CHECK(debug_snapshot().state==State::CALIBRATE); // Home is not a valid bowl profile.
    CHECK(EEPROM.writes==writes_before);
    int flashes=test_output[10]?1:0;bool lit=test_output[10];
    for(int i=0;i<170;++i) {run(10);bool next=test_output[10];if(next&&!lit)++flashes;lit=next;}
    CHECK(flashes==6); // Rejected profile has distinct six-flash feedback.

    boot();CHECK(await(State::WAIT));
    CHECK(!start_plate_jiggle()); // Disabled in the normal firmware build.
  } catch(const std::exception &e) {std::fprintf(stderr,"FAIL exception: %s\n",e.what());++failures;}
  std::printf("firmware integration: %d checks, %d failures\n",checks,failures);
  return failures?1:0;
}

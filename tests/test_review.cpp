#include <cstdio>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include "Firmware.h"
#include "Control.h"
#include "Profile.h"
#include "Arduino.h"
#include "EEPROM.h"

uint32_t test_now=0;
int test_digital[20],test_analog[6],test_pwm[20],test_output[20];
int test_servo_pulse[20],test_servo_writes=0;
FakeEEPROM EEPROM;
void delay(unsigned long) { throw std::runtime_error("blocking delay"); }
static int checks=0,failures=0,scenarios=0;
static const char *scenario="";
#define CASE(name) do { scenario=name; ++scenarios; } while(0)
#define CHECK(c) do { ++checks; if(!(c)) { ++failures; std::fprintf(stderr,"FAIL %s:%d: %s\n",scenario,__LINE__,#c); } } while(0)
static void run(uint32_t ms) { for(uint32_t n=0;n<ms;n+=10) {test_now+=10;feeder_loop();} }
static bool await(State wanted,uint32_t budget=120000) {
  for(uint32_t n=0;n<budget;n+=10) {
    if(debug_snapshot().state==wanted)return true;
    if(debug_snapshot().state==State::FAULT || debug_snapshot().state==State::LOW_POWER)return false;
    run(10);
  }
  return debug_snapshot().state==wanted;
}
static void click(int pin,uint32_t hold=100) {
  test_digital[pin]=LOW;run(hold);test_digital[pin]=HIGH;run(60);
}
static void boot(bool valid=true) {
  test_now=0;test_servo_writes=0;EEPROM.drop_writes=false;
  for(int &v:test_digital)v=HIGH;
  for(int &v:test_analog)v=512;
  std::memset(test_pwm,0,sizeof(test_pwm));std::memset(test_output,0,sizeof(test_output));
  std::memset(EEPROM.bytes,0xff,sizeof(EEPROM.bytes));EEPROM.writes=0;
  test_analog[0]=100;test_analog[4]=800;test_analog[5]=476;
  if(valid)for(uint8_t slot=0;slot<4;++slot)save_profile(default_profile(slot),slot);
  feeder_setup();
}
int main() {
  CASE("Cartesian chord cannot cross inherited 10 mm inner workspace");
  Motion m;float a,b;
  CHECK(checked_ik(10,-0.001f,a,b));
  CHECK(!m.begin_cart(a,b,0,-10,0.3f,0));
  CHECK(!m.active());

  CASE("profile reset cannot energize servos during persistent overload");
  boot(false);CHECK(debug_snapshot().fault==Fault::PROFILE_INVALID);
  test_analog[0]=501;
  click(7,10100);
  CHECK(debug_snapshot().fault==Fault::OVERLOAD);
  CHECK(test_servo_writes==0 && test_output[3]==LOW);
  CHECK(EEPROM.writes==0 && test_pwm[11]==0);

  for(int adc : {399,400,401,500,501}) {
    CASE("current threshold boundary in scoop");
    boot();CHECK(await(State::WAIT));click(2);CHECK(await(State::SCOOP));
    int writes=test_servo_writes;test_analog[0]=adc;run(10);
    if(adc<=400)CHECK(debug_snapshot().state==State::SCOOP && debug_snapshot().retries==0);
    else if(adc<=500)CHECK(debug_snapshot().state==State::CONTACT_BACKOFF && debug_snapshot().retries==1);
    else CHECK(debug_snapshot().fault==Fault::OVERLOAD && test_servo_writes==writes);
    CHECK(test_pwm[11]==0 && test_output[13]==LOW);
  }
  for(int adc : {661,662,663}) {
    CASE("voltage threshold boundary and simultaneous input priority");
    boot();CHECK(await(State::WAIT));
    test_digital[2]=LOW;run(650);CHECK(test_pwm[11]>0);
    test_analog[4]=adc;test_digital[7]=LOW;run(40);
    if(adc<662)CHECK(debug_snapshot().state==State::LOW_POWER && test_output[3]==LOW && test_pwm[11]==0);
    else CHECK(debug_snapshot().state==State::ROTATE && test_output[3]==HIGH);
  }
  CASE("simultaneous short buttons start only one cycle");
  boot();CHECK(await(State::WAIT));test_digital[2]=test_digital[7]=LOW;run(100);
  test_digital[2]=test_digital[7]=HIGH;run(60);CHECK(await(State::FEED_WAIT));
  click(2);CHECK(await(State::WAIT));run(2000);CHECK(debug_snapshot().state==State::WAIT);

  CASE("bounce at idle does not start motion");
  boot();CHECK(await(State::WAIT));
  for(int i=0;i<5;++i) {test_digital[2]=LOW;run(10);test_digital[2]=HIGH;run(20);}
  run(100);CHECK(debug_snapshot().state==State::WAIT);click(2);CHECK(await(State::SCOOP));

  CASE("long delayed loop cannot jump joint commands");
  boot();CHECK(await(State::WAIT));click(2);run(500);
  DebugSnapshot before=debug_snapshot();test_now+=5000;feeder_loop();
  DebugSnapshot after=debug_snapshot();
  CHECK(after.fault==Fault::NONE);
  CHECK(std::fabs(after.q1-before.q1)<=0.00601f && std::fabs(after.q2-before.q2)<=0.00601f);
  CHECK(await(State::FEED_WAIT));

  for(State phase : {State::DESCEND,State::SCOOP,State::LIFT}) {
    CASE("cancel from motion phase never proceeds to delivery");
    boot();CHECK(await(State::WAIT));click(2);CHECK(await(phase));click(7);
    bool delivered=false,finished=false;
    for(uint32_t ms=0;ms<120000;ms+=10) {
      State s=debug_snapshot().state;
      if(s==State::FEED_WAIT)delivered=true;
      if(s==State::WAIT || s==State::FAULT) {finished=true;break;}
      run(10);
    }
    CHECK(finished && !delivered);CHECK(test_pwm[11]==0 && test_output[13]==LOW);
  }

  CASE("failed explicit reset latches storage without startup command");
  boot(false);EEPROM.drop_writes=true;click(7,10100);
  CHECK(debug_snapshot().fault==Fault::STORAGE);
  CHECK(test_servo_writes==0 && test_pwm[11]==0 && test_output[3]==LOW);
  EEPROM.drop_writes=false;

  std::printf("review regressions: %d scenarios, %d assertion checks, %d failures\n",scenarios,checks,failures);
  return failures?1:0;
}

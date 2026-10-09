#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "Firmware.h"
#include "Profile.h"
#include "Config.h"
#include "Arduino.h"
#include "EEPROM.h"
uint32_t test_now=0;
int test_digital[20],test_analog[6],test_pwm[20],test_output[20];
int test_servo_pulse[20],test_servo_writes=0;
FakeEEPROM EEPROM;
void delay(unsigned long) {throw std::runtime_error("blocking delay");}
static uint32_t phase_ms[21];
static void tick() {
  phase_ms[int(debug_snapshot().state)]+=10;
  test_now+=10;feeder_loop();
}
static bool await(State wanted,uint32_t budget=180000) {
  for(uint32_t ms=0;ms<budget;ms+=10) {
    State s=debug_snapshot().state;
    if(s==wanted)return true;
    if(s==State::FAULT || s==State::LOW_POWER)return false;
    tick();
  }
  return debug_snapshot().state==wanted;
}
static double seconds(State s) {return phase_ms[int(s)]/1000.0;}
int main() {
  int failures=0,cycles=0;
  std::puts("SIMULATION ONLY: 10 ms loop, ideal commanded positions, no hardware/load model.");
  std::puts("slot,mode,startup_home_s,descend_s,scoop_s,lift_s,feed_wait_s,return_s,plate_rotate_settle_s,cycle_movement_s");
  for(int slot=0;slot<4;++slot)for(int simple=0;simple<2;++simple) {
    test_now=0;std::memset(phase_ms,0,sizeof(phase_ms));
    for(int &v:test_digital)v=HIGH;
    for(int &v:test_analog)v=512;
    test_analog[0]=100;test_analog[4]=800;test_analog[5]=476+146*slot;
    test_digital[1]=simple?LOW:HIGH;
    for(uint8_t i=0;i<4;++i)if(!save_profile(default_profile(i),i))++failures;
    feeder_setup();
    if(!await(State::WAIT)) {++failures;continue;}
    double startup=test_now/1000.0; // Includes inherited 0.5 s startup settle.
    std::memset(phase_ms,0,sizeof(phase_ms));
    test_digital[2]=LOW;for(int i=0;i<10;++i)tick();test_digital[2]=HIGH;
    if(!await(State::FEED_WAIT)) {++failures;continue;}
    if(!simple) {
      // Explicit synthetic user wait, excluded from commanded movement time.
      for(int i=0;i<200;++i)tick();
      test_digital[2]=LOW;for(int i=0;i<4;++i)tick();test_digital[2]=HIGH;
    }
    if(!await(State::WAIT) || test_pwm[11]!=0) {++failures;continue;}
    double ret=seconds(State::RETURN_CLEAR_START)+seconds(State::RETURN)+seconds(State::HOME);
    double movement=seconds(State::DESCEND)+seconds(State::SCOOP)+seconds(State::LIFT)+ret;
    std::printf("%d,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",slot,simple?"Simple":"Advanced",startup,
      seconds(State::DESCEND),seconds(State::SCOOP),seconds(State::LIFT),seconds(State::FEED_WAIT),ret,
      seconds(State::AUTO_ROTATE)+seconds(State::SETTLE),movement);
    ++cycles;
  }
  std::printf("phase timing: %d completed default-profile/mode scenarios, %d failures\n",cycles,failures);
  return failures || cycles!=8 ? 1:0;
}

#include <cstdio>
#include <cmath>
#include <limits>
#include "Control.h"
#include "Config.h"
const float L1=100.0f, L2=100.0f;
static int checks=0, failures=0;
#define CHECK(c) do { ++checks; if (!(c)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); ++failures; } } while(0)
int main() {
  DebouncedButton button;
  button.begin(false,0);
  button.update(true,1); button.update(false,10); button.update(true,15);
  button.update(true,44); CHECK(!button.pressed && !button.down);
  button.update(true,45); CHECK(button.pressed && button.down);
  button.update(true,545); CHECK(button.long_press(Config::LONG_PRESS_MS,545));
  CHECK(!button.long_press(Config::LONG_PRESS_MS,546));
  button.update(false,550); button.update(false,580);
  CHECK(button.released && button.held_ms == 535);
  button.begin(true,0); button.update(true,2000);
  CHECK(!button.long_press(500,2000)); // Held at boot must be released first.
  button.update(false,2001); button.update(false,2031); CHECK(!button.released);
  button.update(true,2032); button.update(true,2062); CHECK(button.pressed);
  button.begin(false,0xfffffff0u);
  button.update(true,0xfffffff1u); button.update(true,15);
  CHECK(button.pressed); CHECK(button.long_press(500,515));

  Motion motion;
  float q1=-2.0f,q2=2.0f;
  CHECK(motion.begin_joint(q1,q2,-1.0f,1.5f,0.3f,0));
  float previous1=q1,previous2=q2, previousVelocity=0;
  bool done=false;
  for (uint32_t now=20; now<20000; now+=20) {
    MotionResult result=motion.step(now,q1,q2);
    CHECK(result != MotionResult::Invalid);
    CHECK(q1 >= previous1 && q1 <= -1.0f && q2 <= previous2 && q2 >= 1.5f);
    CHECK(std::fabs(q1-previous1) <= 0.3f*0.02f+0.00001f);
    CHECK(std::fabs((q1+2.0f)-2.0f*(2.0f-q2)) < 0.00001f); // Synchronized fraction.
    float velocity=(q1-previous1)/0.02f;
    CHECK(std::fabs(velocity-previousVelocity)/0.02f <= Config::JOINT_ACCELERATION+0.002f);
    previousVelocity=velocity; previous1=q1;previous2=q2;
    if (result == MotionResult::Done) { done=true;break; }
  }
  CHECK(done && q1 == -1.0f && q2 == 1.5f);
  CHECK(!motion.begin_joint(q1,q2,std::numeric_limits<float>::quiet_NaN(),1,0.3f,0));
  CHECK(!motion.begin_joint(q1,q2,-1,4,0.3f,0));
  CHECK(motion.begin_joint(-2,2,-1,1.5f,0.3f,0));
  q1=-2; q2=2;
  motion.step(5000,q1,q2);
  CHECK(std::fabs(q1+2) < 0.006f); // A delayed loop never jumps five seconds ahead.
  CHECK(motion.begin_cart(-1.5707963f,1.5707963f,80,-120,0.3f,0));
  q1=-1.5707963f;q2=1.5707963f;done=false;
  for (uint32_t now=20;now<30000;now+=20) {
    previous1=q1;previous2=q2;
    MotionResult result=motion.step(now,q1,q2);
    CHECK(result != MotionResult::Invalid);
    CHECK(std::fabs(q1-previous1) <= 0.00601f && std::fabs(q2-previous2) <= 0.00601f);
    if (result == MotionResult::Done) {done=true;break;}
  }
  CHECK(done);
  float x,y; CHECK(calc_fk(q1,q2,x,y)); CHECK(std::fabs(x-80)<0.002f && std::fabs(y+120)<0.002f);
  CHECK(!motion.begin_cart(q1,q2,250,0,0.3f,0));

  ContactRecovery contact;
  CHECK(contact.retry()); CHECK(contact.offset == 1 && contact.retries == 1);
  CHECK(contact.retry()); CHECK(contact.retry()); CHECK(!contact.retry());
  CHECK(contact.offset == 3 && contact.retries == 3);
  contact.reset(); CHECK(contact.offset == 0 && contact.retries == 0);
  std::printf("control: %d checks, %d failures\n",checks,failures);
  return failures ? 1 : 0;
}

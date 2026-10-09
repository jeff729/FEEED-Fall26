#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <algorithm>
#include <stdexcept>
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define PI 3.14159265358979323846
using std::min;
using std::max;
extern uint32_t test_now;
extern int test_digital[20], test_analog[6], test_pwm[20], test_output[20];
inline unsigned long millis() { return test_now; }
inline void pinMode(int, int) {}
inline int digitalRead(int pin) { return test_digital[pin]; }
inline int analogRead(int pin) { return test_analog[pin]; }
inline void digitalWrite(int pin, int value) {
  if(pin==13 && value!=test_output[pin] && test_pwm[11]!=0)
    throw std::runtime_error("plate direction changed with nonzero PWM");
  test_output[pin] = value;
}
inline void analogWrite(int pin, int value) { test_pwm[pin] = value; }
// A blocking delay in firmware must fail the host integration test.
void delay(unsigned long);

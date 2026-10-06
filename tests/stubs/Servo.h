#pragma once
#include <stdexcept>
#include <stdint.h>
extern int test_servo_pulse[20], test_servo_writes;
class Servo {
  int pin_=-1;
public:
  void attach(int pin, int=544, int=2400) { pin_=pin; }
  void writeMicroseconds(int pulse) {
    if (pin_ < 0 || pulse < 544 || pulse > 2400) throw std::runtime_error("Unsafe servo pulse");
    test_servo_pulse[pin_]=pulse;
    ++test_servo_writes;
  }
};

#include "DCMotor.h"
#include <Arduino.h>
#include "Config.h"

namespace DCMotor {

void attach() {
  pinMode(Config::PLATE_DIR_PIN, OUTPUT);
  pinMode(Config::PLATE_PWM_PIN, OUTPUT);
  pinMode(Config::PLATE_BRAKE_PIN, OUTPUT);
  stop();
  set_direction(false);
}

void set_brake(bool brake) {
  digitalWrite(Config::PLATE_BRAKE_PIN, brake);
}

void set_speed(uint8_t speed) {
  analogWrite(Config::PLATE_PWM_PIN, speed);
}

void stop(bool brake) {
  set_speed(0);
  set_brake(brake);
}

void set_direction(bool dir) {
  // Never reverse a powered motor. Caller supplies the lab-tested coast pause.
  set_speed(0);
  digitalWrite(Config::PLATE_DIR_PIN, dir);
}

};

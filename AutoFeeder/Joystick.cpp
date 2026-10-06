#include "Joystick.h"
#include <Arduino.h>
int read_joystick_x() {
  int val=Config::JOYSTICK_X_SIGN*(analogRead(Config::JOY_X_PIN)-int(X_CENTER));
  if(val < -Config::JOYSTICK_DEADZONE)return -1;
  if(val > Config::JOYSTICK_DEADZONE)return 1;
  return 0;
}
int read_joystick_y() {
  int val=Config::JOYSTICK_Y_SIGN*(analogRead(Config::JOY_Y_PIN)-int(Y_CENTER));
  if(val < -Config::JOYSTICK_DEADZONE)return -1;
  if(val > Config::JOYSTICK_DEADZONE)return 1;
  return 0;
}
int read_joystick_button() {return digitalRead(Config::JOYSTICK_BUTTON_PIN)==LOW;}

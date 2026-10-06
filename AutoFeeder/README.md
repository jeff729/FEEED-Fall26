# AutoFeeder Uno sketch

Open AutoFeeder.ino in the Arduino IDE and select Arduino Uno. All .cpp/.h
files in this directory are required. Do not upload automatically from scripts.

The complete Fall 2026 operating, build and safety instructions are in the
[root README](../README.md). Tuning is centralized in Config.h. The corrected
source-authoritative pin reference is [Pinout.txt](Pinout.txt); the joystick
uses A2/A3 and voltage uses A4, preserving the inherited source assignments.
D1 is the existing mode switch, so hardware Serial must remain disabled.

See [tuning](../docs/TUNING_GUIDE.md),
[first physical tests](../docs/PHYSICAL_TUNING_GUIDE.md), and
[movement checklist](../docs/MOVEMENT_TEST_CHECKLIST.md) before running a feeder.
Main is the untouched file-import baseline. Development uses fall26-development.

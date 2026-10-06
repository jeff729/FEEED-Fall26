# FEEED

The FEEED device is a robot arm with a spoon that lets those who would need human assistance to eat a meal feed themselves more independently The robot scoops food out of plates or bowls and its scooping path can be reprogrammed to work with almost any circular plate or bowl It can store multiple bowl/plate profiles

**Function of each Arduino pin for this project:**

&emsp; 0: Serial RX (unused)\
&emsp; 1: Mode select switch (HIGH = original, LOW = auto rotate-then-scoop)\
&emsp; 2: User input switch\
&emsp; 3: DC motor A power pwm (high = full power)\
&emsp; 4: debug pin (unused in final design)\
&emsp; 5: linkage 1 servo signal\
&emsp; 6: linkage 2 servo signal\
&emsp; 7: Joystick button input\
&emsp; 8: DC motor B brake (high = brake)\
&emsp; 9: DC motor A brake (high = brake)\
&emsp; 10: Warning LED output\
&emsp; 11: DC motor B power pwm (high = full power)\
&emsp; 12: Servo power "direction" pin (high = forward)\
&emsp; 13: DC motor B direction pin (high = forward)

&emsp; A0: Servo current sensing\
&emsp; A1: DC Motor current sensing\
&emsp; A2: Servo motor voltage sensing\
&emsp; A3: Joystick x axis\
&emsp; A4: Joystick y axis\
&emsp; A5: Potentiometer signal

---

## Operation Modes

The feeder supports two operation modes selected by a physical toggle switch on **Pin 1** (`MODE_SELECT_PIN`)

### Mode 0 - Original (switch flipped to open / HIGH)

- **Quick press**: Arm descends into bowl scoops across profile points lifts spoon to user waits for button press to return
- **Long press** (hold > 500ms): Plate rotates while button is held Stops on release Does not scoop - returns to waiting

### Mode 1 - Auto Rotate-then-Scoop (switch flipped to GND / LOW)

- **Press button**: Plate automatically rotates ~35 degrees (with gradual ramp-up) pauses briefly then the arm scoops lifts to user waits 6.5 seconds for eating and returns home automatically
- Single press does the full cycle No second press needed

### Calibration (both modes)

- **Hold joystick button > 1 second**: Enter calibration mode Use the joystick to position the arm click joystick to set each point The arm nods to confirm each point 5 points total: entry bottom edge middle bottom front end
- **Hold joystick button > 1 second during calibration**: Cancel calibration
- **Hold joystick button > 10 seconds**: Reset all saved profiles

---

## Tunable Values

These `#define` constants at the top of `AutoFeeder.ino` can be adjusted:

| Constant | Default | Description |
|----------|---------|-------------|
| `BRIEF_ROTATE_TIME` | 600 ms | How long the motor runs for the ~35 degree rotation in Mode 1 Increase for more rotation decrease for less |
| `ROTATE_TO_SCOOP_DELAY` | 400 ms | Pause between plate stopping and scoop starting in Mode 1 |
| `FEED_WAIT_TIME` | 6500 ms | Time the spoon stays up for eating before auto-returning in Mode 1 |
| `ROTATE_PLATE_TIME` | 500 ms | How long button must be held to trigger plate rotation in Mode 0 |
| `DC_MOTOR_SPEED` | 255 | Motor speed (0-255) Affects both modes |
| `MAX_JOINT_SPEED` | 0.0003 | Servo speed in radians per step |
| `THRESHOLD_CURRENT` | 400 | Current threshold where scoop restarts with vertical offset |
| `OVERLOAD_CURRENT` | 500 | Current threshold where scoop cancels entirely |

---

## File Structure

All files must be in the same folder and the folder name must match the `.ino` filename

```
AutoFeeder/
  AutoFeeder.ino    - Main sketch (modes state machine servo control)
  DCMotor.cpp       - DC motor control (speed direction brake)
  DCMotor.h
  Joystick.cpp      - Joystick input reading
  Joystick.h
  Profile.cpp       - Profile storage and EEPROM read/write
  Profile.h
  kinematics.cpp    - Inverse/forward kinematics calculations
  kinematics.h
  Pinout.txt        - Pin reference
```

---

## How It Works

The code runs as a state machine The main `loop()` executes whichever mode function is currently active Each mode function can call `switch_mode()` to transition to the next step

### Mode 0 scoop cycle:
`wait_mode` → `descend_step` → `scoop_step` → `lift_step_fk` → `feed_wait_step` → `return_step` → `move_home_then_wait` → `wait_mode`

### Mode 1 scoop cycle:
`wait_mode` → `brief_rotate_step` → `descend_step` → `scoop_step` → `lift_step_fk` → `feed_wait_step` (auto 6.5s) → `return_step` → `move_home_then_wait` → `wait_mode`

### Mode 0 rotate only:
`wait_mode` → `rotate_plate_step` (while held) → `wait_mode`

All inputs (button joystick mode switch) use `INPUT_PULLUP` - the pin sits at 5V by default and reads LOW (0V) when connected to GND through a button press or switch flip

---

## Board

Arduino Uno Select **Tools → Board → Arduino Uno** and the correct **Tools → Port** before uploading

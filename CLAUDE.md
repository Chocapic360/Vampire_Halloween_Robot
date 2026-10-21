# Vampire Halloween Robot

Arduino sketch (`Vampire_Halloween_Robot.ino`) for a Halloween prop: when someone
comes close to an HC-SR04 ultrasonic sensor it prints "Boo!", turns two servos
90 degrees and plays a scream on a soundboard.

GitHub: https://github.com/Chocapic360/Vampire_Halloween_Robot (public, branch `main`,
pushed over SSH). Commit and push only when asked.

## Hardware (actual, as wired)

- **Board: classic Arduino Nano clone** (ATmega328P, 5 V, CH340 USB -> `/dev/ttyUSB0`).
  The user called it a "Nano 33" but it is NOT a Nano 33 (those are 3.3 V, native USB,
  `/dev/ttyACM*`). The sketch also builds for the Uno R4 WiFi it was first developed on.
- Board powered from a 9 V supply on VIN.
- HC-SR04 (plain Trig/Echo, not I2C): TRIG -> D3, ECHO -> D2, VCC -> 5V.
- Soundboard trigger -> D4. Active low: the pin is idle as INPUT (released) and is
  "pressed" by switching to OUTPUT LOW. Each press plays the NEXT track. Module is an
  XLW 16MB push-button recorder (single button, `config.txt` with `IO=01`/`PLAY=XX`;
  PLAY=01 = press interrupts playback). Only the scream is recorded on it now.
- PCA9685 servo driver at I2C 0x40: SDA -> A4, SCL -> A5, **VCC -> Nano 5V**, common GND.
  Servos (2x MG996R, channels 0 and 1) powered from a separate supply on V+.

## Build / upload / monitor

```
arduino-cli compile --upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano Vampire_Halloween_Robot
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=9600
```
(run from `/home/val/snap/arduino/85/Arduino`). The monitor holds the port: close it
before uploading. Library: Adafruit PWM Servo Driver Library (+ Adafruit BusIO).

## How the code works

- `loop()` every ~100 ms: 10 us TRIG pulse, `pulseIn(echo, HIGH, 30000)` (0 = no echo),
  distance = `0.017 * duration_us`, then calibrated with `CAL_GAIN`/`CAL_OFFSET`
  (copied from the user's `ultrasonic-Display` project). Prints every reading.
  No echo prints as about `-1.87 cm` and never counts as "close".
- Boo logic: `closeCount` counts consecutive readings below `THRESHOLD_CM` (10 cm).
  At `REQUIRED_READINGS` (5) it fires once (`booPrinted`), only if the 10 s cooldown
  (`BOO_COOLDOWN_MS`, tracked with `hasBooed`/`lastBooMs`) has passed. Firing does:
  print "Boo!" -> `booMotion()` (blocking, ~4.5-8 s): snap to `SERVO90` + scream press,
  hold `HOLD_MS` (2 s), then `sweepServos()` slowly back to `SERVOMIN` over `RETURN_MS`.
  `DOUBLE_TAKE_CHANCE` (25 %) of boos first sweep a third of the way down (`DOUBLE_TAKE_LOW`), pause, jump
  back to 90 and hold before the real return (`random()` seeded from unconnected A0).
  `lastBooMs` is set after the motion, so the cooldown counts from back-at-rest.
  Any far/invalid reading resets the count and re-arms the boo; it no longer moves the
  servos (that reset was what made them twitch back right after a boo).
- Servos: `setServoPulse()` converts microseconds to PCA9685 ticks at 50 Hz.
  `SERVOMIN` 900 us (rest), `SERVOMAX` 2300 us, `SERVO90` = `SERVOMAX` (the boo up position).
  The old midpoint (1600 us, 700 us of travel) only turned ~30 degrees on the real servos.
- Soundboard: only one recording (the scream), so a single 50 ms press always plays it.
  Multi-track selection by counting presses was dropped: the module can't report its
  position and missed/extra presses kept desyncing the count (wrong sound played).
- `setup()`: `Wire.setWireTimeout(3000, true)` so a stuck I2C bus can't freeze the
  sketch, then probes 0x40 and prints "PCA9685 found" / "PCA9685 not found ...".

## Lessons learned (don't repeat)

- **Do not lengthen `PRESS_MS`/`TAP_GAP_MS`.** Tried 100/150 ms to fix occasional wrong
  sounds: it made the wrong sound play every time. User asked to revert to 50/100.
  Wrong tracks were caused by press counting; solved by recording only one track.
- Keep the order `moveServos(SERVO90)` then the scream press (the reverse was part of
  the reverted attempt).
- "Ultrasonic stops working when the servo board is on A4/A5" was the PCA9685 VCC not
  connected: the unpowered chip held SDA/SCL low and Wire hung forever. Fix was wiring
  VCC to 5V; the Wire timeout now keeps the sketch alive if it happens again.
  It was not a power-supply issue (the user corrected that guess).
- The user edits the sketch directly (e.g. threshold, track numbers); treat on-disk
  changes as intentional.

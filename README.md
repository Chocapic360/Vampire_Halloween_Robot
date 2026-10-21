# Vampire Halloween Robot

Arduino sketch for a Halloween robot. When someone stays closer than 10 cm to
the HC-SR04 ultrasonic sensor for 5 readings in a row, it prints "Boo!",
turns two MG996R servos 90 degrees and plays the scream track on a soundboard.
The servos hold for 2 seconds, then slowly go back to rest. About 1 in 4 boos
adds a "double take": the vampire starts going down, then jumps back up before
the real return. There is a 10 second cooldown after the servos are back at rest.

## Hardware

- Arduino Nano (ATmega328P, 5 V; CH340 USB clone). Also builds for the Uno R4 WiFi.
- HC-SR04 ultrasonic sensor: TRIG -> pin 3, ECHO -> pin 2
- Soundboard trigger -> pin 4 (pulled to GND to "press"; only the scream is recorded, so each press plays it)
- PCA9685 servo driver (I2C, 0x40: SDA -> A4, SCL -> A5) with MG996R servos on channels 0 and 1,
  servos powered from an external supply on V+

## Libraries

- Adafruit PWM Servo Driver Library (installs Adafruit BusIO)

## Build and upload

```
arduino-cli compile --upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano Vampire_Halloween_Robot
```

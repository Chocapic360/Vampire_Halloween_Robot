# Vampire Halloween Robot

Arduino sketch for a Halloween robot. When someone stays closer than 10 cm to
the HC-SR04 ultrasonic sensor for 5 readings in a row, it prints "Boo!",
turns two MG996R servos 90 degrees and plays the scream track on a soundboard.
The servos go back to rest when the person leaves, and there is a 10 second
cooldown between boos.

## Hardware

- Arduino Uno R4 WiFi
- HC-SR04 ultrasonic sensor: TRIG -> pin 3, ECHO -> pin 2
- Soundboard trigger -> pin 4 (pulled to GND to "press"; each press plays the next track)
- PCA9685 servo driver (I2C, 0x40) with MG996R servos on channels 0 and 1,
  servos powered from an external supply on V+

## Libraries

- Adafruit PWM Servo Driver Library (installs Adafruit BusIO)

## Build and upload

```
arduino-cli compile --upload -p /dev/ttyACM0 --fqbn arduino:renesas_uno:unor4wifi Vampire_Halloween_Robot
```

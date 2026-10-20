/*
 * Vampire Halloween Robot - HC-SR04 "Boo!" detector
 *
 * Based on the ultrasonic and ultrasonic-Display sketches (ArduinoGetStarted's
 * HC-SR04 tutorial). Prints "Boo!" and plays the scream track on the
 * soundboard when something stays closer than the threshold distance for at
 * least 5 readings in a row, and turns the servos 90 degrees until it leaves.
 *
 * Wiring: Ultrasonic Sensor -> Arduino:
 * - VCC  -> 5VDC
 * - TRIG -> Pin 3
 * - ECHO -> Pin 2
 * - GND  -> GND
 *
 * Wiring: Soundboard -> Arduino:
 * - Trigger -> Pin 4 (pulled to GND to "press" it)
 * - GND     -> GND
 *
 * Wiring: PCA9685 servo driver (0x40) -> Arduino:
 * - SDA -> SDA, SCL -> SCL, VCC -> 5V, GND -> GND
 * - MG996R servos on channels 0 and 1, powered from V+ (external supply)
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Ultrasonic sensor pins (same as the other ultrasonic sketches)
int trigPin = 3;    // TRIG pin
int echoPin = 2;    // ECHO pin

// Calibarion Coefficients (from ultrasonic-Display)
const float CAL_GAIN   = 0.965;
const float CAL_OFFSET = 1.800;

// Boo settings
const float THRESHOLD_CM      = 10.0;  // trigger distance
const int   REQUIRED_READINGS = 5;     // consecutive readings below threshold
const unsigned long BOO_COOLDOWN_MS = 10000;  // wait after a boo before the next one

// Soundboard: each press plays the next track, cycling 1..TOTAL_TRACKS
const int SOUND_PIN = 4;
const int TOTAL_TRACKS = 5;     // number of recordings on the module
const int PRESS_MS = 50;        // how long each "press" lasts
const int TAP_GAP_MS = 100;     // pause between skip taps
int nextTrack = 1;              // track the module will play on the next press

int Bat = 5;
int Organ = 2;
int Thunder = 3;
int Laugh = 4;
int Scream = 1;

// Servos (PCA9685)
Adafruit_PWMServoDriver pwm(0x40);

const int SERVO1 = 0;  // MG996R on channel 0
const int SERVO2 = 1;  // MG996R on channel 1

// Initial pulse limits in microseconds (assumed to span 0-180 degrees)
const int SERVOMIN = 900;
const int SERVOMAX = 2300;
const int SERVO90  = SERVOMIN + (SERVOMAX - SERVOMIN) / 2;  // 90 degrees from rest

float duration_us, distance_cm, distance_raw_cm;
int closeCount = 0;
bool booPrinted = false;
bool hasBooed = false;
unsigned long lastBooMs = 0;

// pull the soundboard trigger to ground briefly, then release it
void pressButton() {
  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, LOW);
  delay(PRESS_MS);
  pinMode(SOUND_PIN, INPUT);
}

void setServoPulse(int channel, int pulseUs) {
  // PCA9685 period at 50 Hz = 20,000 microseconds
  int ticks = (int)((long)pulseUs * 4096 / 20000);
  pwm.setPWM(channel, 0, ticks);
}

void moveServos(int pulseUs) {
  setServoPulse(SERVO1, pulseUs);
  setServoPulse(SERVO2, pulseUs);
}

// press as many times as needed so the last press plays the target track
void playTrack(int target) {
  int presses = (target - nextTrack + TOTAL_TRACKS) % TOTAL_TRACKS + 1;

  for (int i = 0; i < presses; i++) {
    pressButton();
    if (i < presses - 1) delay(TAP_GAP_MS);
  }

  nextTrack = target % TOTAL_TRACKS + 1;
}

void setup() {
  // begin serial port
  Serial.begin(9600);

  // configure the trigger pin to output mode
  pinMode(trigPin, OUTPUT);
  // configure the echo pin to input mode
  pinMode(echoPin, INPUT);

  // start both servos at rest (minimum)
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);
  moveServos(SERVOMIN);

  // leave the soundboard trigger released until needed
  pinMode(SOUND_PIN, INPUT);
  delay(1000);          // let the module settle
  // The module should be on track 1 right after power-up
}

void loop() {
  // generate 10-microsecond pulse to TRIG pin
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // measure duration of pulse from ECHO pin (30 ms timeout ~ 5 m, returns 0 if no echo)
  duration_us = pulseIn(echoPin, HIGH, 30000);

  // calculate the distance
  distance_raw_cm = 0.017 * duration_us;
  distance_cm = (1/CAL_GAIN) * distance_raw_cm - (CAL_OFFSET/CAL_GAIN);

  // print the value to Serial Monitor
  Serial.print("distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");

  // count consecutive readings below the threshold (ignore "no echo" readings)
  if (duration_us > 0 && distance_cm < THRESHOLD_CM) {
    if (closeCount < REQUIRED_READINGS) {
      closeCount++;
    }
    bool cooledDown = !hasBooed || millis() - lastBooMs >= BOO_COOLDOWN_MS;
    if (closeCount >= REQUIRED_READINGS && !booPrinted && cooledDown) {
      Serial.println("Boo!");
      moveServos(SERVO90);
      playTrack(Scream);
      booPrinted = true;  // only once per approach
      hasBooed = true;
      lastBooMs = millis();
    }
  } else {
    // reset once they move away
    if (booPrinted) {
      moveServos(SERVOMIN);  // back to rest
    }
    closeCount = 0;
    booPrinted = false;
  }

  delay(100);
}

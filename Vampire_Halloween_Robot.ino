/*
 * Vampire Halloween Robot - HC-SR04 "Boo!" detector
 *
 * Based on the ultrasonic and ultrasonic-Display sketches (ArduinoGetStarted's
 * HC-SR04 tutorial). Prints "Boo!" and plays the scream track on the
 * soundboard when something stays closer than the threshold distance for at
 * least 5 readings in a row. The servos snap 90 degrees, hold, then slowly go
 * back to rest, sometimes with a "double take" (fake going down, jump back up).
 *
 * Wiring: Ultrasonic Sensor -> Arduino:
 * - VCC  -> 5VDC
 * - TRIG -> Pin 3
 * - ECHO -> Pin 2
 * - GND  -> GND
 *
 * Wiring: Soundboard -> Arduino:
 * - Trigger -> Pin 4 (pulled to GND to "press" it)
 * - Only the scream is recorded on the module, so every press plays it
 * - GND     -> GND
 *
 * Wiring: Red LEDs -> Pin 5 (HIGH = on, lit during the boo)
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

// Soundboard: holds only the scream, so each press plays it
const int SOUND_PIN = 4;
const int PRESS_MS = 50;        // how long each "press" lasts

// Red LEDs, on while the vampire is up
const int LED_PIN = 5;

// Servos (PCA9685)
Adafruit_PWMServoDriver pwm(0x40);

const int SERVO1 = 0;  // MG996R on channel 0
const int SERVO2 = 1;  // MG996R on channel 1

// Initial pulse limits in microseconds (assumed to span 0-180 degrees)
const int SERVOMIN = 900;
const int SERVOMAX = 2300;
// Up position for the boo. The midpoint (1600 us) only turned ~30 degrees,
// so use the full range; raise SERVOMAX if it still doesn't reach 90.
const int SERVO90  = SERVOMAX;

// Boo motion
const unsigned long HOLD_MS     = 2000;  // stay up at 90 degrees
const unsigned long RETURN_MS   = 2500;  // slow sweep back to rest
const int DOUBLE_TAKE_CHANCE    = 25;    // percent of boos that do a double take
const int DOUBLE_TAKE_LOW       = SERVO90 - (SERVO90 - SERVOMIN) / 3;  // fakes going a third of the way down
const unsigned long DOUBLE_TAKE_PAUSE_MS = 400;   // pause low before jumping back up
const unsigned long DOUBLE_TAKE_HOLD_MS  = 1000;  // stay up after the jump

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

// move both servos gradually from one pulse to another (one step per 20 ms servo frame)
void sweepServos(int fromUs, int toUs, unsigned long durationMs) {
  int steps = durationMs / 20;
  for (int i = 1; i <= steps; i++) {
    moveServos(fromUs + (long)(toUs - fromUs) * i / steps);
    delay(20);
  }
}

// snap up, hold, then slowly go back down (sometimes with a double take)
void booMotion() {
  digitalWrite(LED_PIN, HIGH);
  moveServos(SERVO90);
  pressButton();  // play the scream
  delay(HOLD_MS);

  if (random(100) < DOUBLE_TAKE_CHANCE) {
    Serial.println("Double take!");
    sweepServos(SERVO90, DOUBLE_TAKE_LOW, RETURN_MS / 3);  // same speed as the real return
    delay(DOUBLE_TAKE_PAUSE_MS);
    moveServos(SERVO90);  // jump back up
    delay(DOUBLE_TAKE_HOLD_MS);
  }

  sweepServos(SERVO90, SERVOMIN, RETURN_MS);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  // begin serial port
  Serial.begin(9600);
  randomSeed(analogRead(A0));  // A0 is unconnected, its noise seeds the double take

  // configure the trigger pin to output mode
  pinMode(trigPin, OUTPUT);
  // configure the echo pin to input mode
  pinMode(echoPin, INPUT);

  // don't let a stuck I2C bus freeze the sketch (and the ultrasonic sensor)
  Wire.begin();
#ifdef WIRE_HAS_TIMEOUT
  Wire.setWireTimeout(3000, true);  // 3 ms, reset the bus on timeout
#endif

  // check the servo board answers before using it
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) {
    Serial.println("PCA9685 found");
  } else {
    Serial.println("PCA9685 not found - check VCC, GND, SDA (A4), SCL (A5)");
  }

  // start both servos at rest (minimum)
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);
  moveServos(SERVOMIN);

  // LEDs off until the boo
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // leave the soundboard trigger released until needed
  pinMode(SOUND_PIN, INPUT);
  delay(1000);          // let the module settle
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
      booMotion();
      booPrinted = true;  // only once per approach
      hasBooed = true;
      lastBooMs = millis();  // cooldown starts once the servos are back at rest
    }
  } else {
    // reset once they move away
    closeCount = 0;
    booPrinted = false;
  }

  delay(100);
}

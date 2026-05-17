#include <CapacitiveSensor.h>

// ---------------- TOUCH ----------------
CapacitiveSensor cs = CapacitiveSensor(2, 4);

// ---------------- PINS ----------------
int ledPin = 9;
int batteryPin = A0;

// ---------------- LAMP ----------------
bool lampOn = false;
int brightness = 120;
bool increasing = true;

// Fade engine
int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastFade = 0;
int fadeDelay = 4;

// ---------------- TOUCH ----------------
long threshold = 800;
unsigned long touchStart = 0;

// Tap system
unsigned long lastTapTime = 0;
int tapCount = 0;

// Timer
bool timerActive = false;
unsigned long timerStart = 0;
const unsigned long TIMER_DURATION = 1800000;

// Battery
float batteryVoltage = 0;

// Idle
unsigned long lastInteraction = 0;
bool idleMode = false;

// ---------------- BATTERY ----------------
float readBattery() {
  long sum = 0;
  for (int i = 0; i < 5; i++) sum += analogRead(batteryPin);
  float v = (sum / 5.0) * (5.0 / 1023.0);
  return v * 2;
}

// ---------------- FADE ----------------
void updateFade() {
  if (millis() - lastFade > fadeDelay) {
    lastFade = millis();

    if (currentBrightness < targetBrightness) currentBrightness++;
    else if (currentBrightness > targetBrightness) currentBrightness--;

    analogWrite(ledPin, currentBrightness);
  }
}

// ---------------- SOFT PULSE ----------------
void softPulse(int times) {
  for (int t = 0; t < times; t++) {
    for (int i = 0; i <= 50; i++) {
      analogWrite(ledPin, i);
      delay(5);
    }
    for (int i = 50; i >= 0; i--) {
      analogWrite(ledPin, i);
      delay(5);
    }
    delay(80);
  }

  analogWrite(ledPin, lampOn ? brightness : 0);
}

// ---------------- BATTERY PULSE ----------------
void showBatteryLevel() {
  float v = batteryVoltage;

  if (v >= 4.0) softPulse(4);
  else if (v >= 3.8) softPulse(3);
  else if (v >= 3.6) softPulse(2);
  else if (v >= 3.4) softPulse(1);
  else softPulse(6);
}

// ---------------- SETUP ----------------
void setup() {
  pinMode(ledPin, OUTPUT);
  cs.set_CS_AutocaL_Millis(0);
  lastInteraction = millis();
}

// ---------------- LOOP ----------------
void loop() {

  updateFade();

  // Battery
  batteryVoltage = readBattery();

  if (batteryVoltage < 3.0) {
    lampOn = false;
    targetBrightness = 0;
  }

  if (batteryVoltage < 3.3 && brightness > 120) {
    brightness = 120;
  }

  // Timer
  if (timerActive && millis() - timerStart > TIMER_DURATION) {
    timerActive = false;
    lampOn = false;
    targetBrightness = 0;
  }

  // Idle
  if (!lampOn && millis() - lastInteraction > 120000) {
    idleMode = true;
  }

  int samples = idleMode ? 10 : 30;
  long touch = cs.capacitiveSensor(samples);

  static bool wasTouched = false;
  bool isTouched = (touch > threshold);

  // TOUCH START
  if (isTouched && !wasTouched) {
    touchStart = millis();
    lastInteraction = millis();
    idleMode = false;

    if (timerActive) {
      timerActive = false;
      softPulse(2); // cancel feedback
    }
  }

  // HOLD DIM
  if (isTouched) {
    unsigned long d = millis() - touchStart;

    if (d > 700) {
      if (increasing) brightness++;
      else brightness--;

      if (brightness >= 255) {
        brightness = 255;
        increasing = false;
      }
      if (brightness <= 10) {
        brightness = 10;
        increasing = true;
      }

      if (lampOn) targetBrightness = brightness;
    }
  }

  // RELEASE
  if (!isTouched && wasTouched) {
    unsigned long d = millis() - touchStart;

    if (d < 250) {
      tapCount++;
      lastTapTime = millis();
    }
  }

  // TAP PROCESSOR (non-blocking)
  if (tapCount > 0 && millis() - lastTapTime > 250) {

    if (tapCount == 1) {
      lampOn = !lampOn;
      softPulse(1);
      targetBrightness = lampOn ? brightness : 0;
    }

    else if (tapCount == 2) {
      timerActive = true;
      timerStart = millis();
      softPulse(2);
    }

    else if (tapCount == 3) {
      lampOn = true;
      brightness = 10;
      softPulse(3);
      targetBrightness = brightness;
    }

    else if (tapCount >= 4) {
      showBatteryLevel();
    }

    tapCount = 0;
  }

  wasTouched = isTouched;

  delay(idleMode ? 70 : 25);
}

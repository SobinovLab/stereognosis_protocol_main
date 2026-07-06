// Break-beam IR receiver on D4, internal pull-up.
// Minimal diagnostic sketch: samples the beam state at a fixed rate and prints
// it over serial. No serial commands, no break-event memory -- just polling.
//
// Wiring / logic match sketch_may21a_IR_breakbeam:
//   Most break-beam receiver modules output LOW when the beam is broken.
//   If your module is inverted, set BROKEN_LEVEL to HIGH instead.

#include <Arduino.h>

static const uint8_t  BEAM_PIN = 4;
static const uint8_t  BROKEN_LEVEL = LOW;

// Fixed sampling frequency: 4 Hz -> one reading every 250 ms.
static const uint16_t SAMPLE_HZ = 4;
static const uint16_t SAMPLE_INTERVAL_MS = 1000 / SAMPLE_HZ;

static inline bool beamBroken() {
  return (digitalRead(BEAM_PIN) == BROKEN_LEVEL);
}

void setup() {
  pinMode(BEAM_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);  // on-board LED (pin 13) indicates beam state
  Serial.begin(115200);
  Serial.println("READY BREAKBEAM_PERIODIC");
}

void loop() {
  static uint32_t lastSampleMs = 0;
  uint32_t now = millis();

  // Fire once per fixed interval (millis() handles rollover correctly).
  if ((uint32_t)(now - lastSampleMs) >= SAMPLE_INTERVAL_MS) {
    lastSampleMs += SAMPLE_INTERVAL_MS;
    // Guard against drift if the loop was ever delayed by more than one period.
    if ((uint32_t)(now - lastSampleMs) >= SAMPLE_INTERVAL_MS) lastSampleMs = now;

    bool broken = beamBroken();
    // LED on when the beam is clear, off when it is covered.
    digitalWrite(LED_BUILTIN, broken ? LOW : HIGH);
    Serial.println(broken ? "COVERED" : "CLEAR");
  }
}

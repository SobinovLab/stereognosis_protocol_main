// E18-D80NK on Arduino Nano
// Wiring:
// Brown -> 5V
// Blue  -> GND
// Black -> D4
// Uses internal pull-up: HIGH = clear, LOW = detected

const byte SENSOR_PIN = 4;
const byte LED_PIN = LED_BUILTIN;   // Nano onboard LED (usually D13)

const unsigned long DEBOUNCE_MS = 8;    // helps suppress chatter/noise
const unsigned long MIN_PULSE_MS = 5;   // ignore very short glitches

int lastRawState = HIGH;
int stableState  = HIGH;

unsigned long lastRawChangeMs = 0;
unsigned long detectStartMs   = 0;
unsigned long eventCount      = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);  // internal pull-up enabled
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("E18-D80NK monitor started");
  Serial.println("State meaning: LOW=DETECTED, HIGH=CLEAR");
}

void loop() {
  unsigned long now = millis();
  int raw = digitalRead(SENSOR_PIN);

  // Track raw changes
  if (raw != lastRawState) {
    lastRawState = raw;
    lastRawChangeMs = now;
  }

  // Debounce: accept a new state only if stable for DEBOUNCE_MS
  if ((now - lastRawChangeMs) >= DEBOUNCE_MS && raw != stableState) {
    int previousStable = stableState;
    stableState = raw;

    if (stableState == LOW) {
      // Object just detected
      detectStartMs = now;
      eventCount++;
      digitalWrite(LED_PIN, HIGH);
      Serial.print("DETECTED, count=");
      Serial.print(eventCount);
      Serial.print(", t_ms=");
      Serial.println(now);
    } else {
      // Object just cleared
      unsigned long pulseMs = now - detectStartMs;
      digitalWrite(LED_PIN, LOW);

      // Ignore unrealistically short pulses (noise)
      if (pulseMs >= MIN_PULSE_MS) {
        Serial.print("CLEARED, blockage_ms=");
        Serial.print(pulseMs);
        Serial.print(", t_ms=");
        Serial.println(now);
      }
    }
  }
}
// IR Break-Beam sensor (B0FP9HP6LG-style pair)
// RX OUT -> D4
// Use internal pull-up: HIGH = beam OK, LOW = beam broken (typical)  :contentReference[oaicite:5]{index=5}

const byte RX_PIN = 4;

const unsigned long DEBOUNCE_MS = 5;
const unsigned long STATUS_PERIOD_MS = 100;

int lastRaw = HIGH;
int stable = HIGH;
unsigned long lastChangeMs = 0;
unsigned long lastStatusMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(RX_PIN, INPUT_PULLUP);  // provides pull-up for open-collector style outputs :contentReference[oaicite:6]{index=6}
  Serial.println("Break-beam ready. HIGH=beam OK, LOW=beam broken (typical).");
}

bool isBeamBroken() {
  return (stable == LOW);
}

void loop() {
  unsigned long now = millis();
  int raw = digitalRead(RX_PIN);

  // debounce
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChangeMs = now;
  }
  if ((now - lastChangeMs) >= DEBOUNCE_MS && raw != stable) {
    stable = raw;

    if (isBeamBroken()) Serial.println("EVENT: BEAM BROKEN (object detected)");
    else                Serial.println("EVENT: BEAM RESTORED (clear)");
  }

  // periodic status
  if (now - lastStatusMs >= STATUS_PERIOD_MS) {
    lastStatusMs = now;
    Serial.print("STATE: ");
    Serial.println(isBeamBroken() ? "DETECTED" : "CLEAR");
  }
}
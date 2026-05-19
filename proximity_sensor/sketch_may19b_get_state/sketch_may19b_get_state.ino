// E18-D80NK on Arduino Nano
// Wiring:
//   Brown -> 5V
//   Blue  -> GND
//   Black -> D4
// Logic:
//   With INPUT_PULLUP: HIGH = clear, LOW = detected

const byte SENSOR_PIN = 4;
const unsigned long DEBOUNCE_MS = 8;

int lastRawState = HIGH;
int stableState  = HIGH;
unsigned long lastRawChangeMs = 0;

String cmd;

void updateDebouncedState() {
  unsigned long now = millis();
  int raw = digitalRead(SENSOR_PIN);

  if (raw != lastRawState) {
    lastRawState = raw;
    lastRawChangeMs = now;
  }
  if ((now - lastRawChangeMs) >= DEBOUNCE_MS) {
    stableState = raw;
  }
}

void handleCommand(const String& s) {
  // Trim CR/LF/spaces
  String c = s;
  c.trim();
  c.toUpperCase();

  if (c == "PING") {
    Serial.println("OK E18D80NK");
  } else if (c == "GET" || c == "STATE") {
    // Report 1 when detected, 0 when clear
    Serial.println((stableState == LOW) ? "1" : "0");
  } else if (c == "INFO") {
    Serial.println("E18D80NK D4 INPUT_PULLUP");
  } else {
    Serial.println("ERR");
  }
}

void setup() {
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  // Optional: announce readiness (useful for debugging)
  Serial.println("READY E18D80NK");
}

void loop() {
  updateDebouncedState();

  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    if (ch == '\n') {
      handleCommand(cmd);
      cmd = "";
    } else if (ch != '\r') {
      // Avoid unbounded growth if the host sends garbage
      if (cmd.length() < 64) cmd += ch;
    }
  }
}
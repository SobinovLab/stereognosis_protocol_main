// Break-beam IR receiver on D4, internal pull-up.
// Maintains a "memory" flag: was beam broken in last 500 ms.
// Answers serial requests: PING / GET / RAW / RESET

#include <Arduino.h>

static const uint8_t BEAM_PIN = 4;

// Most break-beam receiver modules output LOW when beam is broken.
// If your module is inverted, set this to HIGH instead.
static const uint8_t BROKEN_LEVEL = LOW;

static const uint16_t WINDOW_MS = 500;

// Debounce/glitch filtering (optional but cheap):
// require the same level N consecutive reads before accepting a change.
static const uint8_t STABLE_COUNT = 3;

volatile bool beamBrokenNow = false;
volatile uint32_t lastBrokenMs = 0;
volatile bool everBroken = false;

// Fast read for ATmega328P when BEAM_PIN == 4 (D4 is PD4)
static inline bool readBeamBrokenFast() {
#if defined(__AVR_ATmega328P__)
  if (BEAM_PIN == 4) {
    uint8_t level = (PIND & _BV(4)) ? HIGH : LOW;
    return (level == BROKEN_LEVEL);
  }
#endif
  return (digitalRead(BEAM_PIN) == BROKEN_LEVEL);
}

// Command parser (no String allocations)
static char cmdBuf[32];
static uint8_t cmdLen = 0;

static inline void serialWrite01(bool v) {
  Serial.println(v ? '1' : '0');
}

static inline bool brokenRecently500ms() {
  if (!everBroken) return false;
  uint32_t now = millis();
  uint32_t last;
  noInterrupts();
  last = lastBrokenMs;
  interrupts();
  return (uint32_t)(now - last) <= WINDOW_MS;
}

static void handleCommand(const char* s) {
  // Uppercase compare without allocating
  if (!strcmp(s, "PING")) {
    Serial.println("OK BREAKBEAM");
  } else if (!strcmp(s, "GET") || !strcmp(s, "STATE")) {
    serialWrite01(brokenRecently500ms());
  } else if (!strcmp(s, "RAW")) {
    bool raw;
    noInterrupts();
    raw = beamBrokenNow;
    interrupts();
    serialWrite01(raw);
  } else if (!strcmp(s, "RESET")) {
    // Clear break-event memory so the next trial starts from a clean state.
    noInterrupts();
    everBroken = false;
    lastBrokenMs = 0;
    interrupts();
    Serial.println("OK");
  } else if (!strcmp(s, "INFO")) {
    Serial.println("BREAKBEAM D4 INPUT_PULLUP WINDOW_MS=500");
  } else {
    Serial.println("ERR");
  }
}

void setup() {
  pinMode(BEAM_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("READY BREAKBEAM");
}

void loop() {
  // === FAST BEAM MONITOR (no delay, minimal overhead) ===
  // Tiny stability filter: accept a state after STABLE_COUNT consecutive reads.
  static bool lastSample = false;
  static uint8_t sameCount = 0;
  static bool stable = false;

  bool sample = readBeamBrokenFast();

  if (sample == lastSample) {
    if (sameCount < 255) sameCount++;
  } else {
    lastSample = sample;
    sameCount = 1;
  }

  if (sameCount >= STABLE_COUNT && sample != stable) {
    stable = sample;
    noInterrupts();
    beamBrokenNow = stable;
    interrupts();
  } else {
    // keep beamBrokenNow updated even without transitions (cheap)
    noInterrupts();
    beamBrokenNow = stable;
    interrupts();
  }

  if (stable) {
    // record "last broken" time
    uint32_t now = millis();
    noInterrupts();
    lastBrokenMs = now;
    everBroken = true;
    interrupts();
  }

  // === NON-BLOCKING SERIAL COMMAND HANDLING ===
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      cmdBuf[cmdLen] = 0;

      // uppercase in-place for easy compare
      for (uint8_t i = 0; i < cmdLen; i++) {
        if (cmdBuf[i] >= 'a' && cmdBuf[i] <= 'z') cmdBuf[i] -= 32;
      }

      if (cmdLen > 0) handleCommand(cmdBuf);
      cmdLen = 0;
    } else {
      if (cmdLen < sizeof(cmdBuf) - 1) cmdBuf[cmdLen++] = ch;
    }
  }
}

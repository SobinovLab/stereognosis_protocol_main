#define OUTPUT_PIN 3
#define TOUCH_PIN 9

int state = 0;
int previousState = 0;
int triggered = 0;
int lastTriggeredState = 0;
unsigned long priorTime; 
int const delayTime = 5;

void setup() {
  Serial.begin(9600);
  pinMode(TOUCH_PIN, INPUT);
  digitalWrite(TOUCH_PIN, LOW);
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
}

void loop() {
  state = digitalRead(TOUCH_PIN);
  if (state != previousState)
  {
    priorTime = millis();
    previousState = state;
    triggered = 1;
  }
  
  if (triggered && (lastTriggeredState != state) && millis() - priorTime > delayTime)
  {
    lastTriggeredState = state;
    triggered = 0;
    if (state == LOW)
    {
      //Touched the thing
      Serial.write("TOUCHED\n");
      digitalWrite(OUTPUT_PIN, HIGH);
    }
    if (state == HIGH)
    {
      //Let go of the thing
      Serial.write("RELEASED\n");
      digitalWrite(OUTPUT_PIN, LOW);
    }
  }
}

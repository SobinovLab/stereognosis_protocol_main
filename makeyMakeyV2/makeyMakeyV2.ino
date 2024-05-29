#define OUTPUT_PIN1 12
#define OUTPUT_PIN2 13

#define INPUT_PIN1 2
#define INPUT_PIN2 3

int state1 = 0;
int previousState1 = 0;
int triggered1 = 0;
int lastTriggeredState1 = 0;
unsigned long priorTime1 = 0;

int state2 = 0;
int previousState2 = 0;
int triggered2 = 0;
int lastTriggeredState2 = 0;
unsigned long priorTime2 = 0;

int const delayTime = 5;

void setup() {
  Serial.begin(9600);
  pinMode(INPUT_PIN1, INPUT);
  digitalWrite(INPUT_PIN1, LOW);
  pinMode(OUTPUT_PIN1, OUTPUT);
  digitalWrite(OUTPUT_PIN1, LOW);

  pinMode(INPUT_PIN2, INPUT);
  digitalWrite(INPUT_PIN2, LOW);
  pinMode(OUTPUT_PIN2, OUTPUT);
  digitalWrite(OUTPUT_PIN2, LOW);
}

void loop() {
  state1 = digitalRead(INPUT_PIN1);
  state2 = digitalRead(INPUT_PIN2);
  
  if(state1 != previousState1)
  {
    priorTime1 = millis();
    previousState1 = state1;
    triggered1 = 1;
  }
  
  if(state2 != previousState2)
  {
    priorTime2 = millis();
    previousState2 = state2;
    triggered2 = 1;
  }

  if (triggered1 && (lastTriggeredState1 != state1) && millis() - priorTime1 > delayTime)
  {
    lastTriggeredState1 = state1;
    triggered1 = 0;
    if (state1 == LOW)
    {
      //Touched the thing
      Serial.write("TOUCHED1\n");
      digitalWrite(OUTPUT_PIN1, LOW);
    }
    if (state1 == HIGH)
    {
      //Let go of the thing
      Serial.write("RELEASED1\n");
      digitalWrite(OUTPUT_PIN1, HIGH);
    }
  }

  if (triggered2 && (lastTriggeredState2 != state2) && millis() - priorTime2 > delayTime)
  {
    lastTriggeredState2 = state2;
    triggered2 = 0;
    if (state2 == LOW)
    {
      //Touched the thing
      Serial.write("TOUCHED2\n");
      digitalWrite(OUTPUT_PIN2, LOW);
    }
    if (state2 == HIGH)
    {
      //Let go of the thing
      Serial.write("RELEASED2\n");
      digitalWrite(OUTPUT_PIN2, HIGH);
    }
  }
}

#include <FastLED.h>

#define INPUT_SIZE 19
#define LED_OFF 0
#define LED_LEFT 1
#define LED_RIGHT 2
#define LED_BOTH 3
#define MAX_LEDS 40

#define LEFT_PIN 6
#define RIGHT_PIN 9

char input[INPUT_SIZE + 1];
int side;
int bot_leds;
int top_leds;
int red;
int green;
int blue;
CRGB left_leds[MAX_LEDS];
CRGB right_leds[MAX_LEDS];


void setup() {
  // put your setup code here, to run once:
  //whatever pins we need, do pinMode(i, OUTPUT)
  Serial.begin(9600);
  pinMode(LEFT_PIN, OUTPUT);
  pinMode(RIGHT_PIN, OUTPUT);

  FastLED.addLeds<WS2812, LEFT_PIN>(left_leds, MAX_LEDS);
  FastLED.addLeds<WS2812, RIGHT_PIN>(right_leds, MAX_LEDS);
  
  side = LED_OFF;
  bot_leds = 0;
  top_leds = 0;
  red = 0;
  green = 0;
  blue = 0;
}

void loop() {
  if(Serial.available() > 0)
  {
    byte size = Serial.readBytes(input, INPUT_SIZE);
    if(size == INPUT_SIZE)
    {
      //Split the message with null terminators for string reading, predefined message format
      //so we can hardcode this with basic point arithmetic
      input[1] = '\0';
      input[4] = '\0';
      input[7] = '\0';
      input[11] = '\0';
      input[15] = '\0';
      input[19] = '\0';

      side = atoi(input);
      bot_leds = atoi(input + 2);
      top_leds = atoi(input + 5);
      red = atoi(input + 8);
      green = atoi(input + 12);
      blue = atoi(input + 16);
    }
  
    switch(side)
    {
      //Hilariously the strip currently being used is wired wrong so its GRB not RGB, so
      //thats why I do the stupid green = red, red = green thing.
      case(LED_OFF):
        for (int i = 0; i < MAX_LEDS; i++)
        {
          left_leds[i] = CRGB::Black; //shorthand for [0,0,0]
          right_leds[i] = CRGB::Black;
        }
        break;
      case(LED_LEFT):
        for (int i = 0; i < bot_leds; i++)
        {
          left_leds[i] = CRGB::Black;
        }
        for (int i = bot_leds; i < top_leds; i++)
        {
          left_leds[i].g = red;
          left_leds[i].r = green;
          left_leds[i].b = blue;
        }
        for (int i = top_leds; i < MAX_LEDS; i++)
        {
          left_leds[i] = CRGB::Black;
        }
        break;
      case(LED_RIGHT):
        for (int i = 0; i < bot_leds; i++)
        {
          right_leds[i] = CRGB::Black;
        }
        for (int i = bot_leds; i < top_leds; i++)
        {
          right_leds[i].g = red;
          right_leds[i].r = green;
          right_leds[i].b = blue;
        }
        for (int i = top_leds; i < MAX_LEDS; i++)
        {
          right_leds[i] = CRGB::Black;
        }
        break;
      case(LED_BOTH):
        for (int i = 0; i < bot_leds; i++)
        {
          left_leds[i] = CRGB::Black;
          right_leds[i] = CRGB::Black;
        }
        for (int i = 0; i < top_leds; i++)
        {
          left_leds[i].g = red;
          left_leds[i].r = green;
          left_leds[i].b = blue;
          right_leds[i].g = red;
          right_leds[i].r = green;
          right_leds[i].b = blue;
        }
        for (int i = top_leds; i < MAX_LEDS; i++)
        {
          left_leds[i] = CRGB::Black;
          right_leds[i] = CRGB::Black;
        }
        break;
    }
  }
  FastLED.show();
}

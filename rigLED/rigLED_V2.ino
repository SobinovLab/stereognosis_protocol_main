#include <FastLED.h>

#define INPUT_SIZE 20
#define LED_OFF 0
#define LED_TOP 1
#define LED_BOTTOM 2
#define LED_BOTH 3
#define TEST_MSG 4
#define MAX_LEDS 60

#define TOP_PIN 6
#define BOTTOM_PIN 9

char input[INPUT_SIZE];
int side;
int start_led;
int end_led;
int red;
int green;
int blue;
CRGB top_leds[MAX_LEDS];
CRGB bot_leds[MAX_LEDS];


void setup() {
    // put your setup code here, to run once:
    // whatever pins we need, do pinMode(i, OUTPUT)
    Serial.begin(9600);

    pinMode(TOP_PIN, OUTPUT);
    pinMode(BOTTOM_PIN, OUTPUT);

    FastLED.addLeds<WS2812, TOP_PIN>(top_leds, MAX_LEDS);
    FastLED.addLeds<WS2812, BOTTOM_PIN>(bot_leds, MAX_LEDS);

    side = LED_OFF;
    start_led = 0;
    end_led = 0;
    red = 0;
    green = 0;
    blue = 0;
}

void loop() {
    if (!Serial.available())
        return;

    // if a new message, refresh the values
    byte size = Serial.readBytes(input, INPUT_SIZE);
    if (size == INPUT_SIZE)
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
        start_led = atoi(input + 2);
        end_led = atoi(input + 5);
        red = atoi(input + 8);
        green = atoi(input + 12);
        blue = atoi(input + 16);

        // to prevent out of bounds error
        if (start_led > MAX_LEDS)
            start_led = MAX_LEDS;
        if (end_led > MAX_LEDS)
            end_led = MAX_LEDS;
    }

    //RGB was fixed on the second rig
    switch(side)
    {
        case(LED_OFF):  // OFF
            for (int i = 0; i < MAX_LEDS; i++)
            {
                top_leds[i] = CRGB::Black; //shorthand for [0,0,0]
                bot_leds[i] = CRGB::Black;
            }
            break;

        case(LED_TOP): // TOP
            for (int i = 0; i < start_led; i++)
            {
                top_leds[i] = CRGB::Black;
            }
            for (int i = start_led; i < end_led; i++)
            {
                top_leds[i].g = green;
                top_leds[i].r = red;
                top_leds[i].b = blue;
            }
            for (int i = end_led; i < MAX_LEDS; i++)
            {
                top_leds[i] = CRGB::Black;
            }
            break;

        case(LED_BOTTOM): // BOTTOM
            for (int i = 0; i < start_led; i++)
            {
                bot_leds[i] = CRGB::Black;
            }
            for (int i = start_led; i < end_led; i++)
            {
                bot_leds[i].g = green;
                bot_leds[i].r = red;
                bot_leds[i].b = blue;
            }
            for (int i = end_led; i < MAX_LEDS; i++)
            {
                bot_leds[i] = CRGB::Black;
            }
            break;

        case(LED_BOTH): // BOTH
            for (int i = 0; i < start_led; i++)
            {
                top_leds[i] = CRGB::Black;
                bot_leds[i] = CRGB::Black;
            }
            for (int i = 0; i < end_led; i++)
            {
                top_leds[i].g = green;
                top_leds[i].r = red;
                top_leds[i].b = blue;
                bot_leds[i].g = green;
                bot_leds[i].r = red;
                bot_leds[i].b = blue;
            }
            for (int i = end_led; i < MAX_LEDS; i++)
            {
                top_leds[i] = CRGB::Black;
                bot_leds[i] = CRGB::Black;
            }
            break;

        case(TEST_MSG): // TEST_MSG
            Serial.println("OK");
            break;
    }

    FastLED.show();
}

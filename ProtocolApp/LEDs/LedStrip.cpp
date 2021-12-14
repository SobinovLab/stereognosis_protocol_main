#include "stdafx.h"
#include "LedStrip.h"


using namespace std;

#define enforce_range(v, a, b) min(max(a, v), b)


int LedStrip::set_top_stripe_lights(const double end_portion)
{
    return set_top_stripe_lights(0, end_portion);
}

int LedStrip::set_top_stripe_lights(const double start_portion, const double end_portion)
{
    int st = portionToNumLeds(start_portion);
    int ed = portionToNumLeds(end_portion);
    if (top_reverse_order) {
        int buf = st;
        st = num_leds_per_strip - ed;
        ed = num_leds_per_strip - buf;
    }

    return sendLedData(LED_STRIP_SIDE::LED_TOP, st, ed, 
        (int) round((double)top_stripe_red * top_brightness),
        (int) round((double)top_stripe_green * top_brightness),
        (int) round((double)top_stripe_blue * top_brightness));
}

int LedStrip::set_bottom_stripe_lights(const double end_portion)
{
    return set_bottom_stripe_lights(0, end_portion);
}

int LedStrip::set_bottom_stripe_lights(const double start_portion, const double end_portion)
{
    int st = portionToNumLeds(start_portion);
    int ed = portionToNumLeds(end_portion);
    if (bottom_reverse_order) {
        int buf = st;
        st = num_leds_per_strip - ed;
        ed = num_leds_per_strip - buf;
    }
    return sendLedData(LED_STRIP_SIDE::LED_BOTTOM, st, ed,
        (int)round((double)bottom_stripe_red * bottom_brightness),
        (int)round((double)bottom_stripe_green * bottom_brightness),
        (int)round((double)bottom_stripe_blue * bottom_brightness));
}

void LedStrip::test(int delay)
{
    for (int i = 1; i <= num_leds_per_strip; i++)
    {
        set_top_stripe_lights((double) i / num_leds_per_strip);
        Sleep(delay);
    }
    for (int i = num_leds_per_strip; i >=0 ; i--)
    {
        set_top_stripe_lights((double) i / num_leds_per_strip);
        Sleep(delay);
    }

    for (int i = 1; i <= num_leds_per_strip; i++)
    {
        set_bottom_stripe_lights((double) i / num_leds_per_strip);
        Sleep(delay);
    }
    for (int i = num_leds_per_strip; i >= 0; i--)
    {
        set_bottom_stripe_lights((double) i / num_leds_per_strip);
        Sleep(delay);
    }
}

bool LedStrip::wasInitializedCorrectly()
{
    return initializedCorrectly;
}

int LedStrip::portionToNumLeds(const double portion)
{
    return enforce_range((int) round(portion*num_leds_per_strip), 0, num_leds_per_strip);
}

int LedStrip::sendLedData(LED_STRIP_SIDE side)
{
    return sendLedData(side,  0, num_leds_per_strip, 0, 0, 0);
}

int LedStrip::sendLedData(LED_STRIP_SIDE side, int start_led, int end_led, int red, int green, int blue)
{
    if (!wasInitializedCorrectly())
        return -1;

    // Make sure we are connected to the COM port, otherwise Write fails badly
    if (!SP->IsConnected()) 
        return -2;

    // Enforce the stip length bounds
    start_led = enforce_range(start_led, 0, num_leds_per_strip);
    end_led = enforce_range(end_led, 0, num_leds_per_strip);

    // enforce the color bounds
    red = enforce_range(red, 0, 255);
    green = enforce_range(green, 0, 255);
    blue = enforce_range(blue, 0, 255);

    //Time for some manual buffer writing, not too bad because it is
    //a strict predetermined format but you know it is what it is
    //Uses super basic ascii math, if you don't understand this gitgud
    //Ascii 0 is 48
    // side:
    message_buffer[0] = 48 + static_cast<int>(side);
    // starting light:
    message_buffer[2] = 48 + (start_led / 10);
    message_buffer[3] = 48 + (start_led % 10);
    // ending light
    message_buffer[5] = 48 + (end_led / 10);
    message_buffer[6] = 48 + (end_led % 10);
    // red
    message_buffer[8] = 48 + (red / 100);
    message_buffer[9] = 48 + ((red % 100) / 10);
    message_buffer[10] = 48 + (red % 10);
    // green
    message_buffer[12] = 48 + (green / 100);
    message_buffer[13] = 48 + ((green % 100) / 10);
    message_buffer[14] = 48 + (green % 10);
    // blue
    message_buffer[16] = 48 + (blue / 100);
    message_buffer[17] = 48 + ((blue % 100) / 10);
    message_buffer[18] = 48 + (blue % 10);

    //Actually do the writing to serial port
    if (SP->WriteData(message_buffer, message_size))
        return 0;
    else
        return -4;
}

LedStrip::LedStrip(std::string led_port, std::string comPortFriendlyName)
{
    SP = new Serial(led_port, comPortFriendlyName);

    // it will be set as Connected only if the initialization went fine.
    if (SP->IsConnected()) {
        initializedCorrectly = true;
        logInfo("LEDs connected.");
    }
    else {
        logError("LEDs not connected.");
    }

    message_buffer[0] = '0';
    message_buffer[1] = separator;
    message_buffer[2] = '0';
    message_buffer[3] = '0';
    message_buffer[4] = separator;
    message_buffer[5] = '0';
    message_buffer[6] = '0';
    message_buffer[7] = separator;
    message_buffer[8] = '0';
    message_buffer[9] = '0';
    message_buffer[10] = '0';
    message_buffer[11] = separator;
    message_buffer[12] = '0';
    message_buffer[13] = '0';
    message_buffer[14] = '0';
    message_buffer[15] = separator;
    message_buffer[16] = '0';
    message_buffer[17] = '0';
    message_buffer[18] = '0';
    message_buffer[19] = '\0';
}

LedStrip::~LedStrip()
{
}

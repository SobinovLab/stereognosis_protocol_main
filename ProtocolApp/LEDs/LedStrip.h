#pragma once

#include <atomic>
#include <thread>
#include "Times.h"
#include "Serial.h"

enum class LED_STRIP_SIDE
{
	LED_OFF = 0,
	LED_TOP = 1,
	LED_BOTTOM = 2,
	LED_BOTH = 3,
	TEST = 4,
};

class LedStrip
{
private:
	LedStrip() = delete;
	LedStrip(LedStrip&) = delete;

	// hardcoded message type
	unsigned int message_size = 20;
	char* message_buffer = new char[message_size];

	Serial* SP;

	bool initializedCorrectly = false;  // set by constructor
public:
	LedStrip(std::string led_port, std::string comPortFriendlyName);
	~LedStrip();

	// parameters of the messages
	unsigned int num_leds_per_strip = 40;
	char separator = ':';

	// default coloring
	int top_stripe_red = 244;
	int top_stripe_green = 67;
	int top_stripe_blue = 54;
	double top_brightness = 0.5;
	bool top_reverse_order = true;
	int bottom_stripe_red = 76;
	int bottom_stripe_green = 175;
	int bottom_stripe_blue = 80;
	double bottom_brightness = 0.5;
	bool bottom_reverse_order = true;

	// easy control functions
	int set_top_stripe_lights(const double end_portion);
	int set_top_stripe_lights(const double start_portion, const double end_portion);
	int turn_off_top_stripe_ligths();
	int set_bottom_stripe_lights(const double end_portion);
	int set_bottom_stripe_lights(const double start_portion, const double end_portion);
	int turn_off_bottom_stripe_ligths();
	int turn_off_both_stripe_lights();

	// test
	void test(int delay=10);

	// If false, delete the object and try again
	bool wasInitializedCorrectly();

	// transform from 0-1 into number of leds
	int portionToNumLeds(const double portion);

	//-------- sending to LED functions
	// turns off the leds on that side. if LED_OFF, turns all of them off
	int sendLedData(LED_STRIP_SIDE side);
	// leds with numbers within [start_led, end_led) will be lit, subject to [0, num_leds_per_strip) bounds
	// red, green, blue are forced within [0, 255]
	int sendLedData(LED_STRIP_SIDE side, int start_led, int end_led, int red, int green, int blue);
};


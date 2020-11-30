#pragma once
class Sounds
{
public:
	static void playStartTaskTone();
	static void playErrorTone();
	static void playWarningTone();

	static void playTone(const unsigned long frequency, const unsigned long duration);
};


#include "stdafx.h"
#include "Sounds.h"

constexpr auto FREQUENCY_START_TASK_TONE = 500;
constexpr auto FREQUENCY_WARNING_TONE = 400;
constexpr auto FREQUENCY_ERROR_TONE = 250;
constexpr auto DURATION_TONE = 1000; //msecs


void Sounds::playStartTaskTone()
{
	playTone(FREQUENCY_START_TASK_TONE, DURATION_TONE);
}

void Sounds::playErrorTone()
{
	playTone(FREQUENCY_ERROR_TONE, DURATION_TONE);
}

void Sounds::playWarningTone()
{
	playTone(FREQUENCY_WARNING_TONE, DURATION_TONE);
}

void Sounds::playTone(const unsigned long frequency, const unsigned long duration)
{
	Beep(frequency, duration);
}

void Sounds::playTone(const unsigned long frequency)
{
	playTone(frequency, DURATION_TONE);
}

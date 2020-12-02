#pragma once
#include <chrono>

using namespace std;
using namespace chrono;

class Times
{
public:
	static steady_clock::time_point getCurrentTime();

	static bool isTimeout(const steady_clock::time_point& startTime, const long& periodSecs);

	// between
	static long getElapsedMicroSecsBetween(const steady_clock::time_point& startTime, const steady_clock::time_point& endTime);
	static long getElapsedMilliSecsBetween(const steady_clock::time_point& startTime, const steady_clock::time_point& endTime);

	// since
	static long getElapsedMicroSecsSince(const steady_clock::time_point& startTime);
	static long getElapsedMilliSecsSince(const steady_clock::time_point& startTime);

	// transformations
	static long microToMillisecs(const long& microsecs);
	static long milliToMicrosecs(const long& millisecs);
	static long secToMicrosecs(const double& secs);

};


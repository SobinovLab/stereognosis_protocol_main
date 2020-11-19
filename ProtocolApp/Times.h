#pragma once
#include <chrono>

using namespace std;
using namespace chrono;

class Times
{
public:
	static steady_clock::time_point getCurrentTime();

	// between
	static long getElapsedMicroSecsBetween(steady_clock::time_point& startTime, steady_clock::time_point& endTime);
	static long getElapsedMilliSecsBetween(steady_clock::time_point& startTime, steady_clock::time_point& endTime);

	// since
	static long getElapsedMicroSecsSince(steady_clock::time_point& startTime);
	static long getElapsedMilliSecsSince(steady_clock::time_point& startTime);

	// transformations
	static long microToMillisecs(const long& microsecs);
	static long milliToMicrosecs(const long& millisecs);
	static long secToMicrosecs(const double& secs);

};


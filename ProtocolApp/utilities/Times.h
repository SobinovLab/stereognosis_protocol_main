#pragma once
#include <chrono>
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <string>  // string

#define DATE_TIME_FORMAT "%Y_%m_%d_%H_%M_%S"
#define DATE_FORMAT "%Y_%m_%d"

class Times
{
public:
	static std::chrono::steady_clock::time_point getCurrentTime();
	static long long getCurrentTimeInMilliSecs();

	static std::string getFormattedDateTime();
	static std::string getFormattedDate();
	static std::string getFormattedDateTime(std::chrono::system_clock::time_point timePoint, std::string format);

	static bool isTimeout(const std::chrono::steady_clock::time_point& startTime, const long& periodSecs);

	// between
	static long getElapsedMicroSecsBetween(const std::chrono::steady_clock::time_point& startTime, const std::chrono::steady_clock::time_point& endTime);
	static long getElapsedMilliSecsBetween(const std::chrono::steady_clock::time_point& startTime, const std::chrono::steady_clock::time_point& endTime);

	// since
	static long getElapsedMicroSecsSince(const std::chrono::steady_clock::time_point& startTime);
	static long getElapsedMilliSecsSince(const std::chrono::steady_clock::time_point& startTime);

	// transformations
	static long microToMillisecs(const long& microsecs);
	static long milliToMicrosecs(const long& millisecs);
	static long secToMicrosecs(const double& secs);

};


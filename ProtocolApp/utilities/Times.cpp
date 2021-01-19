#include "stdafx.h"
#include "Times.h"

using namespace std;
using namespace chrono;

steady_clock::time_point Times::getCurrentTime()
{
    return steady_clock::now();
}

long long Times::getCurrentTimeInMilliSecs()
{
    auto d = getCurrentTime().time_since_epoch();
    auto t = duration_cast<milliseconds>(d).count();
    return t;
}

std::string Times::getFormattedDateTime()
{
    return getFormattedDateTime(system_clock::now(), DATE_TIME_FORMAT);
}

std::string Times::getFormattedDateTime(std::chrono::system_clock::time_point timePoint, std::string format)
{
    time_t in_time_t = std::chrono::system_clock::to_time_t(timePoint);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), format.c_str());
    return ss.str();
}

std::string Times::getFormattedDate()
{
    return getFormattedDateTime(system_clock::now(), DATE_FORMAT);
}

bool Times::isTimeout(const steady_clock::time_point& startTime, const long& periodSecs)
{
    long timeElapsed = getElapsedMicroSecsSince(startTime);
    if (timeElapsed > secToMicrosecs(periodSecs))
        return true;
    return false;
}

long Times::getElapsedMicroSecsBetween(const steady_clock::time_point& startTime, const steady_clock::time_point& endTime)
{
    auto elapsed = duration_cast<microseconds>(endTime - startTime);
    return (long)elapsed.count();
}

long Times::getElapsedMilliSecsBetween(const steady_clock::time_point& startTime, const steady_clock::time_point& endTime)
{
    auto elapsed = duration_cast<milliseconds>(endTime - startTime);
    return (long)elapsed.count();
}

long Times::getElapsedMicroSecsSince(const steady_clock::time_point& startTime)
{
    return getElapsedMicroSecsBetween(startTime, steady_clock::now());
}

long Times::getElapsedMilliSecsSince(const steady_clock::time_point& startTime)
{
    return getElapsedMilliSecsBetween(startTime, steady_clock::now());
}

long Times::microToMillisecs(const long& microsecs)
{
    return (long)round(microsecs / 1e3);
}

long Times::milliToMicrosecs(const long& millisecs)
{
    return millisecs * 1000;
}

long Times::secToMicrosecs(const double& secs)
{
    return (long)(secs * 1000000);
}

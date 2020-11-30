#include "stdafx.h"
#include "Times.h"

steady_clock::time_point Times::getCurrentTime()
{
    return chrono::steady_clock::now();
}

long Times::getElapsedMicroSecsBetween(steady_clock::time_point& startTime, steady_clock::time_point& endTime)
{
    auto elapsed = duration_cast<chrono::microseconds>(endTime - startTime);
    return (long)elapsed.count();
}

long Times::getElapsedMilliSecsBetween(steady_clock::time_point& startTime, steady_clock::time_point& endTime)
{
    auto elapsed = duration_cast<chrono::milliseconds>(endTime - startTime);
    return (long)elapsed.count();
}

long Times::getElapsedMicroSecsSince(steady_clock::time_point& startTime)
{
    return getElapsedMicroSecsBetween(startTime, chrono::steady_clock::now());
}

long Times::getElapsedMilliSecsSince(steady_clock::time_point& startTime)
{
    return getElapsedMilliSecsBetween(startTime, chrono::steady_clock::now());
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

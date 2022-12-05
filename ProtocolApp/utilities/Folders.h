#pragma once

#include <regex> 
#include <ctime>
#include <chrono>
#include <unordered_set>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> 

#include "Logger.h"


class Folders
{
public:
    static bool path_exists(const std::string& pathpath);

    // returns true if found a file
    static bool find_latest_csv(const std::string& dirpath, std::string& latestfile);

private:
    static bool extract_date(const std::string str, std::chrono::system_clock::time_point& dt);
};


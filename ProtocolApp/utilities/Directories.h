#pragma once

#include <regex> 
#include <ctime>
#include <chrono>
#include <unordered_set>
#include <string>
#include <vector>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> 

#include "Logger.h"

class Directories
{
public:
	// transforms a vector of strings with names of files and directories into a string
	static std::string nestedDirnamesToString(const std::vector<std::string> nested_dirnames);

	// common function that reports on exceptions if wanted
	static int createDirectory(const std::string dirname, const bool reportExistsCreation = true);
	// create a series of nested directories, all directories are created
	static int createDirectory(const std::vector<std::string> nested_dirnames, const bool reportExistsCreation = true);

	// returns a filename that has incremental markers (N) until no such file exists
	// difference from the directory version - deals with extension
	static std::string ensureFileDoesNotExist(const std::string filename);

	// returns a dirname that has incremental markers (N) until no such dir exists
	static std::string ensureDirectoryDoesNotExist(const std::string dirname);

	// returns absolute name of the directory that contains the filename
	static std::string dirname(const std::string& filename);

	// returns true if the associated path exists
	static bool path_exists(const std::string& path);

	// returns true if found a csv in the directory, and provides filename with the latest date in the title
	static bool find_latest_csv(const std::string& dirpath, std::string& latestfile);

	// finds one file that matches the grep
	static bool grep_one(const std::string& dirpath, const std::regex& r, std::string& rel_filename);

private:
	static bool extract_date(const std::string str, std::chrono::system_clock::time_point& dt);
};


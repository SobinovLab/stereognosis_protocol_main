#include "stdafx.h"
#include "Folders.h"

namespace fs = std::experimental::filesystem;
using namespace std;
using namespace std::chrono;


// find date information in a string and fill up the DateTime object, return true upon success
bool Folders::extract_date(const string str, std::chrono::system_clock::time_point& dt) {
    regex date("[0-9]{4}.[0-9]{2}.[0-9]{2}[_][0-9]{2}.[0-9]{2}.[0-9]{2}");
    smatch match;
    int matchcount = 0;
    regex_search(str, match, date);

    tm datetime;

    // did not 
    if (match.empty()) 
        return false;

    for (string x : match) {
        if (matchcount > 0) {
            string buf = "Folders:: more than one date matched in " + str + " string.";
            logWarning(buf.c_str());
            break;
        }

        datetime = { 
            /* .tm_sec  = */ stoi(x.substr(17, 2)),
            /* .tm_min  = */ stoi(x.substr(14, 2)),
            /* .tm_hour = */ stoi(x.substr(11, 2)),
            /* .tm_mday = */ stoi(x.substr(8, 2)),
            /* .tm_mon  = */ stoi(x.substr(5, 2)) - 1,
            /* .tm_year = */ stoi(x.substr(0, 4)) - 1900,
        };

        dt = system_clock::from_time_t(mktime(&datetime));

        ++matchcount;
    }

    return true;
}

bool Folders::path_exists(const std::string& path)
{
    return fs::exists(path);
}

bool Folders::find_latest_csv(const string& dirpath, string& latestfile) {
    system_clock::time_point dt_latest, dt_compare;
    bool foundAFile = false;

    for (const auto& entry : fs::directory_iterator(dirpath)) {
        // check if CSV
        if (entry.path().extension() != ".csv")
            continue;

        // find the first filename that contain valid date
        if (!foundAFile) {
            if (extract_date(entry.path().filename().generic_string(), dt_latest)) {
                latestfile = fs::canonical(entry.path()).generic_string();
                foundAFile = true;
            }
        }
        else { // get the next one to compare 
            if (extract_date(entry.path().filename().generic_string(), dt_compare)) {
                if (dt_compare > dt_latest) {
                    dt_latest = dt_compare;
                    latestfile = fs::canonical(entry.path()).generic_string();
                }
            }
        }
    }

    if (!foundAFile) {
        string buf = "Found no CSV files with datetimes in " + dirpath + ".";
        logWarning(buf.c_str());
    }

    return foundAFile;
}

std::string Folders::dirname(const std::string& filename)
{
    return fs::absolute(fs::path(filename)).parent_path().string();
}

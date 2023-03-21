#include "Directories.h"

using namespace std;
using namespace std::chrono;
namespace fs = std::experimental::filesystem;


std::string Directories::nestedDirnamesToString(const std::vector<std::string> nested_dirnames)
{
	string answ = "";
	for (auto dirname : nested_dirnames)
		answ += dirname + "/";
	return answ;
}

int Directories::createDirectory(const std::string dirname, const bool reportExistsCreation)
{
	string buf;
	int answ = 0;

	// if not a drive
	int suc = 1;
	if (dirname.size() > 3 || dirname.size() == 1 || (dirname.size() > 1 && dirname[1] != ':'))
		suc = CreateDirectory(dirname.c_str(), NULL);

	if (suc) {
		if (reportExistsCreation) {
			buf = "Made directory " + dirname + ".";
			logInfo(buf.c_str());
		}
	}
	else {
		DWORD gle = GetLastError();
		switch (gle)
		{
		case ERROR_ALREADY_EXISTS:
			if (reportExistsCreation) {
				buf = "Directory " + dirname + " already exists.";
				logWarning(buf.c_str());
			}
			answ = 1;
			break;
		case ERROR_PATH_NOT_FOUND:
			buf = "One or more intermediate directories in " + dirname + " do not exist; this function will only create the final directory in the path.";
			logError(buf.c_str());
			answ = -1;
			break;
		default:
			buf = "Unknown error with code %d during creation of " + dirname + ".";
			logError(buf.c_str());
			answ = -2;
			break;
		}
	}
	return answ;
}

int Directories::createDirectory(const std::vector<std::string> nested_dirnames, const bool reportExistsCreation)
{
	int answ = 0;
	string buf = "";
	for (string dirname : nested_dirnames) {
		buf += dirname + "/";
		answ = createDirectory(buf, reportExistsCreation);
		if (answ < 0)  // positive are warnings e.g. already existing
			break;
	}
	return answ;
}

std::string Directories::ensureFileDoesNotExist(const std::string _filename)
{
	string filename = _filename;

	// check if file exists
	while (experimental::filesystem::exists(filename)) {
		// adjust it to have another name: filename(NUMBER).csv
		experimental::filesystem::path filepath(filename);

		string ext = filepath.extension().string();
		string basename = filepath.filename().string();
		// replace_extension does not remove the '.'
		if (!ext.empty())
			basename = basename.substr(0, basename.size() - ext.size());

		// check if the name already has a number in the name
		regex filenumber_regex("\\([0-9]+\\)$");
		smatch filenumber_smatch;
		int filenumber = 0;
		if (regex_search(basename, filenumber_smatch, filenumber_regex)) {
			// if exists, replace it w
			basename = regex_replace(basename, filenumber_regex, "");
			string filenumber_s = filenumber_smatch[0].str();
			filenumber_s = filenumber_s.substr(1, filenumber_s.size() - 2); // remove parentheses
			try
			{
				filenumber = stoi(filenumber_s);
			}
			catch (const std::exception&)
			{
				string buf = "Problems extracting file number from " + filename + ".";
				logWarning(buf.c_str());
			}
		}
		filenumber++;

		basename = basename + "(" + to_string(filenumber) + ")" + ext;
		filepath.replace_filename(basename);
		filename = filepath.string();
	}

	return filename;
}

std::string Directories::ensureDirectoryDoesNotExist(const std::string _dirname)
{
	string dirname = _dirname;

	// check if file exists
	while (experimental::filesystem::exists(dirname)) {
		// adjust it to have another name: dirname(NUMBER)

		// check if the name already has a number in the name
		regex filenumber_regex("\\([0-9]+\\)$");
		smatch filenumber_smatch;
		int filenumber = 0;
		if (regex_search(dirname, filenumber_smatch, filenumber_regex)) {
			// if exists, replace it w
			dirname = regex_replace(dirname, filenumber_regex, "");
			string filenumber_s = filenumber_smatch[0].str();
			filenumber_s = filenumber_s.substr(1, filenumber_s.size() - 2); // remove parentheses
			try
			{
				filenumber = stoi(filenumber_s);
			}
			catch (const std::exception&)
			{
				string buf = "Problems extracting dir number from " + dirname + ".";
				logWarning(buf.c_str());
			}
		}
		filenumber++;

		dirname = dirname + "(" + to_string(filenumber) + ")";
	}

	return dirname;
}

std::string Directories::dirname(const std::string& filename)
{
	return fs::absolute(fs::path(filename)).parent_path().string();
}

bool Directories::path_exists(const std::string& path)
{
	return fs::exists(path);
}


// find date information in a string and fill up the DateTime object, return true upon success
bool Directories::extract_date(const string str, std::chrono::system_clock::time_point& dt) {
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

bool Directories::find_latest_csv(const string& dirpath, string& latestfile) {
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

bool Directories::grep_one(const std::string& dirpath, const std::regex& r, std::string& rel_filename)
{
	string rf;
	smatch match;
	int matchcount = 0;
	for (auto const& dir_entry : fs::directory_iterator{ dirpath })
	{
		rf = dir_entry.path().filename().string();
		regex_search(rf, match, r);
		if (!match.empty()) {
			rel_filename = rf;
			return true;
		}
	}
	return false;
}

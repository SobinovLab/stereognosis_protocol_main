#include "stdafx.h"
#include "CsvParser.h"

using namespace std;


void CsvParser::line2vec_str(string line, string delim, vector<string>& vec) {
    size_t pos = 0;
    string token;
    while ((pos = line.find(delim)) != string::npos) {
        token = line.substr(0, pos);
        vec.push_back(token);
        line.erase(0, pos + delim.length());
    }
    // put the last one into vector
    vec.push_back(line);
}

int CsvParser::line2vec_double(string line, string delim, vector<double>& vec) {
    size_t pos = 0;
    double token;
    while ((pos = line.find(delim)) != string::npos) {
        try {
            token = stod(line.substr(0, pos));
        }
        catch (const invalid_argument) {
            string buf = "Argument is invalid, fail to convert " + line.substr(0, pos) + " to double";
            logError(buf.c_str());
            return 1;
        }
        vec.push_back(token);
        line.erase(0, pos + delim.length());
    }

    try {
        token = stod(line);
    }
    catch (const invalid_argument) {
        logError("Argument is invalid, fail to convert to double\n");
        return 1;
    }
    vec.push_back(token);

    return 0;
}

// return 0 upon success
int CsvParser::parseCSV(const string& filename, vector<string>& line1, vector<string>& line2, vector<vector<double>>& vec2d) {

    ifstream fp(filename);
    if (!fp.is_open()) {
        string buf = "Failed to open " + filename;
        logError(buf.c_str());
        return -1;
    }

    int linecount = 0;
    string line;

    try
    {
        while (getline(fp, line)) {
            ++linecount;
            if (linecount == 1) {
                line2vec_str(line, ",", line1);
            }
            else if (linecount == 2) {
                line2vec_str(line, ",", line2);
                if (line1.size() != line2.size()) {
                    logError("The lengths of line 1 and line 2 do not match.");
                    return -2;
                }
            }
            else {
                vector<double> vec;
                int err = line2vec_double(line, ",", vec);
                if (err) {
                    string buf = "Could not import line " + to_string(linecount) + ".";
                    logError(buf.c_str());
                    return -3;
                }
                if (line1.size() != vec.size()) {
                    string buf = "Lengths of line 1 and line " + to_string(linecount) + " do not match.";
                    logError(buf.c_str());
                    return -4;
                }
                vec2d.push_back(vec);
            }
        }
    }
    catch (const std::exception&)
    {
        string buf = "Encountered unknown error while parsing  " + filename + ".";
        logError(buf.c_str());
        return -5;
    }

    return 0;
}
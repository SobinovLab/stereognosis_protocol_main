#pragma once
#include <vector>
#include <string>
#include <fstream>

#include "Logger.h"

class CsvParser
{
public:
	static int parseCSV(const std::string& filename, 
        std::vector<std::string>& line1,
        std::vector<std::string>& line2,
        std::vector<std::vector<double>>& vec2d);

private:
    static void line2vec_str(std::string line, std::string delim, std::vector<std::string>& vec);
    static int line2vec_double(std::string line, std::string delim, std::vector<double>& vec);
};


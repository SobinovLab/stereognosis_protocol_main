#pragma once

#include <vector>

#include "Logger.h"

class Convertor {
public:
    Convertor();
    Convertor(
        const double _in_offset, const double _in_to_out_factor,
        const bool _bind_to_int = false);
    ~Convertor();

    double convert(const double val);
    double operator() (const double val);  // calls 'convert'

    // from output - counts into input units
    double convertBack(const double val);

    // forces the value to be bound to the range
    double forceInRange(const double val);
    std::vector<double> getInputRange();
    void setInputRange(std::vector<double> v);  // automatically calculates the outrange
    void setOutputRange(std::vector<double> v);  // should not be usually used
    void removeRange();
private:
    // -------- Order of operations:
    // added to the input
    double in_offset = 0;
    // multiply, can change the sign
    double in_to_out_factor = 1;
    // bind to integer value (needed for motor control - integer counts)
    bool bind_to_int = false;
    // bound output range (in counts)
    double min_out = 0;
    double max_out = 0;

    // ---------- other variables
    // was the range set?
    bool range_was_defined = false;
};
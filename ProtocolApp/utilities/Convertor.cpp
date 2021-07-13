#include "pch.h"
#include "Convertor.h"

using namespace std;


Convertor::Convertor()
{
}

Convertor::Convertor(
    const double _in_offset, const double _in_to_out_factor,
    const bool _bind_to_int)
{
    in_offset = _in_offset;
    in_to_out_factor = _in_to_out_factor;
    bind_to_int = _bind_to_int;

    if (in_to_out_factor == 0) {
        logError("Convertor in_to_out_factor is 0.");
        throw invalid_argument("Convertor in_to_out_factor is 0.");
    }
}

Convertor::~Convertor()
{
}


double Convertor::convert(const double val)
{
    double answ = (val + in_offset) * in_to_out_factor;

    if (bind_to_int)
        answ = round(answ);

    if (range_was_defined) {
        if (answ < min_out)
            answ = min_out;
        else if (answ > max_out)
            answ = max_out;
    }

    return answ;
}

double Convertor::operator()(const double val)
{
    return this->convert(val);
}

double Convertor::convertBack(const double val)
{
    double answ = val;

    if (range_was_defined) {
        if (answ < min_out)
            answ = min_out;
        else if (answ > max_out)
            answ = max_out;
    }

    if (bind_to_int)
        answ = round(answ);

    answ = answ / in_to_out_factor - in_offset;

    return answ;
}

double Convertor::forceInRange(const double val)
{
    if (range_was_defined) {
        double answ_counts = convert(val);
        if (answ_counts < min_out)
            return convertBack(min_out);
        else if (answ_counts > max_out)
            return convertBack(max_out);
    }
    return val;
}

std::vector<double> Convertor::getInputRange()
{
    double buf0 = convertBack(min_out);
    double buf1 = convertBack(max_out);
    return std::vector<double>({ min(buf0, buf1), max(buf0, buf1) });
}

void Convertor::setInputRange(std::vector<double> v)
{
    double min_in = min(v[0], v[1]);
    double max_in = max(v[0], v[1]);

    // auto runs a correct unbound convert and rounds to int if needed
    // additionally prevents recalculating of convert
    double buf0 = convert(min_in);
    double buf1 = convert(max_in);

    setOutputRange(vector<double>{buf0, buf1});
}

void Convertor::setOutputRange(std::vector<double> v)
{
    min_out = min(v[0], v[1]);
    max_out = max(v[0], v[1]);

    if (bind_to_int) {
        min_out = round(min_out);
        max_out = round(max_out);
    }

    range_was_defined = true;
}

void Convertor::removeRange()
{
    range_was_defined = false;
}
